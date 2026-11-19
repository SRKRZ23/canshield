#!/usr/bin/env python3
"""Build the CANSHIELD dataset from ROAD (CC BY 4.0, Verma et al. 2020, arXiv:2012.14600).
Honest split:
  * profile ("normal behaviour") is learned ONLY from PROFILE_AMBIENT captures;
  * classifier negatives/positives come from TRAIN captures (never used for the profile);
  * false-positive rate is measured on TEST_AMBIENT captures nobody trained on;
  * attacks: *_1 instances (+fuzzing_1) train, *_2/*_3 + coolant + fuzzing_2/3 test.
accelerator_* captures are excluded: they are firmware modifications with no injected frames.
"""
import json, os, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# ROAD root (the folder that contains ambient/ and attacks/). Download: Zenodo record 10462796.
ROAD = os.path.expanduser(os.environ.get('ROAD_DIR', os.path.join(ROOT, 'data', 'road')))
OUT = os.path.join(ROOT, 'build', 'data')
os.makedirs(OUT, exist_ok=True)

PROFILE_AMBIENT = ['ambient_dyno_drive_basic_long', 'ambient_dyno_exercise_all_bits',
                   'ambient_dyno_drive_extended_long', 'ambient_dyno_idle_radio_infotainment',
                   'ambient_highway_street_driving_long']
TRAIN_AMBIENT = ['ambient_dyno_drive_extended_short']
TEST_AMBIENT = ['ambient_dyno_drive_basic_short', 'ambient_dyno_drive_radio_infotainment',
                'ambient_dyno_drive_winter', 'ambient_dyno_reverse', 'ambient_dyno_drive_benign_anomaly',
                'ambient_highway_street_driving_diagnostics']

def sh(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode: sys.exit(f'FAILED {cmd}\n{r.stderr}')
    return r.stderr.strip()

def main():
    b = os.path.join(ROOT, 'build'); core = os.path.join(ROOT, 'core', 'canshield.c')
    for t in ('profile_build', 'featurize'):
        sh(['cc', '-O2', '-Wall', '-Wextra', '-o', os.path.join(b, t), os.path.join(ROOT, 'tools', t + '.c'), core])
    prof = os.path.join(b, 'profile.bin')
    meta = json.load(open(os.path.join(ROAD, 'attacks', 'capture_metadata.json')))
    # benign parts (outside the injection interval) of TRAIN attack captures also describe normal traffic
    extra_dir = os.path.join(b, 'benign_extra'); os.makedirs(extra_dir, exist_ok=True); extra = []
    for n, m in sorted(meta.items()):
        if n.startswith('accelerator') or not n.replace('_masquerade', '').endswith('_1'):
            continue
        t0, t1 = m['injection_interval']; first = None
        dst = os.path.join(extra_dir, n + '.log')
        with open(os.path.join(ROAD, 'attacks', n + '.log')) as f, open(dst, 'w') as o:
            for line in f:
                ts = float(line[1:line.index(')')])
                first = ts if first is None else first
                if not (t0 - 1.0 <= ts - first <= t1 + 1.0):
                    o.write(line)
        extra.append(dst)
    print(sh([os.path.join(b, 'profile_build'), prof] + [os.path.join(ROAD, 'ambient', n + '.log') for n in PROFILE_AMBIENT] + extra))
    manifest = []
    for n in TRAIN_AMBIENT + TEST_AMBIENT:
        o = os.path.join(OUT, n + '.bin')
        print(sh([os.path.join(b, 'featurize'), prof, os.path.join(ROAD, 'ambient', n + '.log'), o]))
        manifest.append(dict(name=n, split='train' if n in TRAIN_AMBIENT else 'test', kind='ambient', path=o))
    for n, m in sorted(meta.items()):
        if n.startswith('accelerator'):
            continue
        iv, iid, pat = m['injection_interval'], m['injection_id'], m['injection_data_str']
        lid = 'any' if iid in (None, 'XXX') else iid.replace('0x', '')
        o = os.path.join(OUT, n + '.bin')
        print(sh([os.path.join(b, 'featurize'), prof, os.path.join(ROAD, 'attacks', n + '.log'), o,
                  str(iv[0]), str(iv[1]), lid, pat]))
        base = n.replace('_masquerade', '')
        split = 'train' if base.endswith('_1') else 'test'
        manifest.append(dict(name=n, split=split, kind='attack', masquerade=m['modified'], path=o))
    json.dump(manifest, open(os.path.join(OUT, 'manifest.json'), 'w'), indent=1)
    print('manifest:', len(manifest), 'files')

if __name__ == '__main__':
    main()
