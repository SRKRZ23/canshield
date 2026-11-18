#!/usr/bin/env python3
"""Honest evaluation of the hardened CANSHIELD model on held-out captures.
Frame level: recall on injected frames, FPR on benign frames.
Event level (what a vehicle would act on): per-id alarm when >= K of the last N frames of that id are
flagged; an attack capture counts as detected if an alarm on the injected id fires inside the injection
interval (time-to-detect reported); false alarms are counted on held-out ambient captures per hour.
usage: python3 scripts/evaluate.py [--k 3 --n 8] [--split test]"""
import argparse, json, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from data import manifest, REC, ROOT

def votes_lookup():
    p = np.load(os.path.join(ROOT, 'build', 'parity.npz'))
    return dict(zip(p['feat'].tolist(), p['votes'].tolist()))

def alarms(r, flag, K, N, KB=2, NB=64):
    """(1) per-id: >= K flagged among the last N frames of that id;
    (2) bus-level: >= KB flagged UNKNOWN-id frames among the last NB frames on the bus (catches fuzzing,
        whose random ids never repeat). Both are fixed-size shift registers in the C runtime."""
    out = np.zeros(len(r), bool)
    unk = flag & ((r['feat'] & np.uint64(1)) == 1)
    cu = np.concatenate([[0], np.cumsum(unk.astype(np.int32))])
    idx_all = np.arange(1, len(r) + 1)
    out |= (cu[1:] - cu[np.maximum(0, idx_all - NB)]) >= KB
    for i in np.unique(r['id']):
        idx = np.nonzero(r['id'] == i)[0]
        f = flag[idx].astype(np.int32)
        cs = np.concatenate([[0], np.cumsum(f)])
        win = cs[1:] - cs[np.maximum(0, np.arange(1, len(f) + 1) - N)]
        out[idx] |= win >= K
    return out

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--k', type=int, default=3); ap.add_argument('--n', type=int, default=8)
    ap.add_argument('--split', default='test'); ap.add_argument('--thr', type=int, default=None); A = ap.parse_args()
    meta = json.load(open(os.path.join(ROOT, 'build', 'model_meta.json'))); thr = A.thr if A.thr is not None else meta['threshold']
    V = votes_lookup()
    road_meta = json.load(open(os.path.expanduser('~/Alish/qnx-hackathon-2026/data/road/road/attacks/capture_metadata.json')))
    rows, ben_frames, ben_fp, ben_hours, ben_alarms = [], 0, 0, 0.0, 0
    tot_pos = tot_tp = 0; per_cap = []
    for e in manifest():
        if e['split'] != A.split: continue
        r = np.fromfile(e['path'], dtype=REC)
        v = np.array([V[f] for f in r['feat'].tolist()]); flag = v >= thr
        al = alarms(r, flag, A.k, A.n)
        if e['kind'] == 'ambient':
            ben_frames += len(r); ben_fp += int(flag.sum())
            hrs = (r['t_ms'][-1] - r['t_ms'][0]) / 3.6e6; ben_hours += hrs
            # count alarm EVENTS (rising edges per id)
            ev = 0
            for i in np.unique(r['id']):
                a = al[r['id'] == i].astype(np.int8); ev += int(((a[1:] - a[:-1]) == 1).sum() + a[:1].sum())
            ben_alarms += ev
            rows.append((e['name'], 'ambient', len(r), '', f"{flag.mean()*100:.3f}%", f"{ev} alarm events / {hrs*60:.1f} min"))
            continue
        y = r['label'] == 1; tot_pos += int(y.sum()); tot_tp += int((flag & y).sum()); per_cap.append((flag & y).sum() / max(1, y.sum()))
        m = road_meta[e['name']]; t0, t1 = m['injection_interval']
        tsec = r['t_ms'] / 1000.0
        inwin = (tsec >= t0) & (tsec <= t1)
        hit = al & inwin & (y | (r['id'] == (int(m['injection_id'], 16) if m['injection_id'] not in (None, 'XXX') else -1)) | (m['injection_id'] == 'XXX'))
        ttd = f"{(tsec[hit][0]-t0)*1000:.0f} ms" if hit.any() else "MISSED"
        fp_out = int((al & ~inwin).sum())
        rows.append((e['name'], 'masq' if e.get('masquerade') else 'attack', int(y.sum()), f"{(flag&y).sum()/max(1,y.sum())*100:.1f}%",
                     f"{(flag & ~y).sum()/max(1,(~y).sum())*100:.3f}%", f"detect {ttd}; alarm frames outside attack {fp_out}"))
    print(f"model: {meta['gates']} gates, threshold {thr}/{meta['groups']}; alarm rule K={A.k} of last N={A.n} frames per id; split={A.split}\n")
    print(f"{'capture':46s} {'type':7s} {'inj':>6s} {'recall':>7s} {'FPR':>8s}  event-level")
    for r_ in rows: print(f"{r_[0]:46s} {r_[1]:7s} {str(r_[2]):>6s} {r_[3]:>7s} {r_[4]:>8s}  {r_[5]}")
    print(f"\nframe-level: attack recall {tot_tp/max(1,tot_pos)*100:.1f}%  |  benign FPR on held-out ambient {ben_fp/max(1,ben_frames)*100:.3f}% ({ben_fp} of {ben_frames})")
    print(f"balanced recall (mean over attack captures): {np.mean(per_cap)*100:.1f}%")
    print(f"event-level: {ben_alarms} false alarm events in {ben_hours*60:.1f} min of held-out benign driving = {ben_alarms/max(1e-9,ben_hours):.1f}/hour")

if __name__ == '__main__':
    main()
