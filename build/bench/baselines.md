| model | frame recall | benign FPR | balanced recall | captures detected | false alarms |
|---|---|---|---|---|---|
| **logic-gate network (CANSHIELD)** | 75.7% | 0.021% | 73.8% | 15/20 | 34.8/hour |
| logistic regression | 27.0% | 0.023% | 25.7% | 5/20 | 15.5/hour |
| decision tree (depth 8) | 77.2% | 0.087% | 80.0% | 16/20 | 29.0/hour |
| random forest (100 trees) | 77.0% | 0.027% | 72.5% | 14/20 | 17.4/hour |
| gradient boosting (HistGB) | 68.9% | 1.349% | 55.3% | 11/20 | 278.6/hour |
