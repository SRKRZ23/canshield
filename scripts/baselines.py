#!/usr/bin/env python3
"""Compare the hardened logic-gate network with classic classifiers on the SAME features and the SAME
protocol (threshold chosen on train at benign FPR <= 0.1%, alarm rule K=4/N=8 + bus rule, test once).
Writes build/bench/baselines.md. The logic-gate model files are restored afterwards."""
import json, os, shutil, subprocess, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from train_lgn import unique_split
from data import ROOT
from sklearn.linear_model import LogisticRegression
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestClassifier, HistGradientBoostingClassifier

B = os.path.join(ROOT, 'build')
X, y, c, feat = unique_split('train')
_, yb, cb, fb = unique_split('train', balanced=True)
cbal = dict(zip(((fb << np.uint64(1)) | yb.astype(np.uint64)).tolist(), cb.tolist()))
wb = np.array([cbal[k] for k in ((feat << np.uint64(1)) | y.astype(np.uint64)).tolist()])
w = np.where(y == 1, wb / wb[y == 1].sum(), wb / wb[y == 0].sum())
Xt, yt, ct, ft = unique_split('test')
allf = np.unique(np.concatenate([feat, ft])); Xall = ((allf[:, None] >> np.arange(X.shape[1], dtype=np.uint64)) & 1).astype(np.uint8)

models = {
    'logistic regression': LogisticRegression(max_iter=2000),
    'decision tree (depth 8)': DecisionTreeClassifier(max_depth=8, random_state=0),
    'random forest (100 trees)': RandomForestClassifier(n_estimators=100, random_state=0, n_jobs=4),
    'gradient boosting (HistGB)': HistGradientBoostingClassifier(random_state=0),
}
for f in ('parity.npz', 'model_meta.json'):
    shutil.copy(os.path.join(B, f), os.path.join(B, f + '.lgn_backup'))
rows = []
try:
    for name, m in models.items():
        m.fit(X, y, sample_weight=w)
        s_tr = m.predict_proba(X)[:, 1]; s_all = m.predict_proba(Xall)[:, 1]
        ben = y == 0; thr = None
        for t in np.unique(np.round(s_tr, 4))[::-1][::-1]:
            if c[ben & (s_tr >= t)].sum() / c[ben].sum() <= 0.001: thr = t; break
        votes = np.round(s_all * 10000).astype(int); ithr = int(round(thr * 10000))
        np.savez(os.path.join(B, 'parity.npz'), feat=allf, votes=votes)
        meta = json.load(open(os.path.join(B, 'model_meta.json.lgn_backup'))); meta.update(threshold=ithr, gates=0, groups=10000)
        json.dump(meta, open(os.path.join(B, 'model_meta.json'), 'w'))
        out = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'evaluate.py'), '--split', 'test', '--k', '4', '--n', '8'],
                             capture_output=True, text=True).stdout
        get = lambda key: [l for l in out.splitlines() if key in l][0]
        missed = sum(1 for l in out.splitlines() if 'MISSED' in l)
        caps = sum(1 for l in out.splitlines() if 'alarm frames outside attack' in l)
        rows.append((name, get('frame-level').split('|')[0].split('recall')[1].strip(), get('benign FPR').split('ambient')[1].split('(')[0].strip(),
                     get('balanced').split(':')[1].strip(), f"{caps - missed}/{caps}", get('false alarm events').split('=')[1].strip()))
        print(rows[-1])
finally:
    for f in ('parity.npz', 'model_meta.json'):
        shutil.move(os.path.join(B, f + '.lgn_backup'), os.path.join(B, f))
out = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'evaluate.py'), '--split', 'test', '--k', '4', '--n', '8'], capture_output=True, text=True).stdout
get = lambda key: [l for l in out.splitlines() if key in l][0]
missed = sum(1 for l in out.splitlines() if 'MISSED' in l); caps = sum(1 for l in out.splitlines() if 'alarm frames outside attack' in l)
rows.insert(0, ('**logic-gate network (CANSHIELD)**', get('frame-level').split('|')[0].split('recall')[1].strip(), get('benign FPR').split('ambient')[1].split('(')[0].strip(),
                get('balanced').split(':')[1].strip(), f"{caps - missed}/{caps}", get('false alarm events').split('=')[1].strip()))
md = ["| model | frame recall | benign FPR | balanced recall | captures detected | false alarms |", "|---|---|---|---|---|---|"]
md += [f"| {' | '.join(r)} |" for r in rows]
open(os.path.join(B, 'bench', 'baselines.md'), 'w').write("\n".join(md) + "\n")
print("\n".join(md))
