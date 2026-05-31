# 03 · Lambda ingest — IQ-26-60001

**Function**: `iqedge-g2-dev-fn-ingest-energy`  
**Expected**: `ingest_ok` · `iqtrailer=True` · `fw=(exempt)` · no `registry_not_found`

| 项 | 结果 |
|----|------|
| ADR-012 Cerbo fw 豁免 | ✅ `fw=(exempt)` |
| Registry lookup | ✅ `track=g2` |
| Timestream write | ✅ `g2_energy` measure |
| Shadow update | ✅ see `06-dynamodb-shadow/` |

详见 [`log-snippet.txt`](log-snippet.txt)。
