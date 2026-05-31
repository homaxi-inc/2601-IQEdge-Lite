# 02 · IoT Rule — energy telemetry

**Rule**: `iqedge_g2_dev_rule_energy`  
**Region**: `us-east-1`

```sql
SELECT * FROM 'iqedge/g2/dev/energy/telemetry'
```

**Action**: `arn:aws:lambda:us-east-1:661631955220:function:iqedge-g2-dev-fn-ingest-energy`

**008 verify**: 2026-05-31 · Cerbo packets routed to M4 ingest (see `03-lambda-ingest/`).
