"""Shared loader for CANSHIELD feature records (16-byte records written by tools/featurize.c)."""
import json, os
import numpy as np
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REC = np.dtype([('feat', '<u8'), ('id', '<u2'), ('label', 'u1'), ('dlc', 'u1'), ('t_ms', '<u4')])
NFEAT = 36
NAMES = ["unknown_id", "dlc_mismatch", "first_seen", "dt<0.25*p01", "dt<0.5*p01", "dt<0.8*p01", "dt>3*p99"] + \
        [f"oor{i}" for i in range(8)] + [f"dlt{i}" for i in range(8)] + [f"untog{i}" for i in range(8)] + \
        ["all_ff", "repeat_payload", "any_oor", "any_delta", "any_untoggled"]

def manifest():
    return json.load(open(os.path.join(ROOT, 'build', 'data', 'manifest.json')))

def load(entry):
    r = np.fromfile(entry['path'], dtype=REC)
    bits = ((r['feat'][:, None] >> np.arange(NFEAT, dtype=np.uint64)) & 1).astype(np.uint8)
    return bits, r['label'].astype(np.uint8), r

def load_split(split, kinds=('ambient', 'attack')):
    X, Y, G = [], [], []
    for e in manifest():
        if e['split'] != split or e['kind'] not in kinds:
            continue
        x, y, _ = load(e); X.append(x); Y.append(y); G += [e['name']] * len(y)
    return np.concatenate(X), np.concatenate(Y), np.array(G)
