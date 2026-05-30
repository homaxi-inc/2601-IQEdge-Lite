# Rule 核对记录 — ✅ 已通过

- **验收人**: Bob · 2026-05-30
- **Rule**: `iqedge_g2_dev_rule_energy`
- **SQL**: `SELECT * FROM 'iqedge/g2/dev/energy/telemetry'`
- **Action**: Lambda `iqedge-g2-dev-fn-ingest-energy`
- **结论**: Rule 正常将 G2 MQTT 消息转发至 ingest Lambda（M4.1 OK）
