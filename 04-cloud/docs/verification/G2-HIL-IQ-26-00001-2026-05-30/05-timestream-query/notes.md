# Timestream 查询 — ✅ 已通过

- **验收人**: Bob · 2026-05-30（控制台 Query editor）
- **Database / Table**: `iqedge_g2_dev_database` · `iqedge_g2_dev_table_energy`
- **过滤**: `sys_id = IQ-26-00001`
- **结论**: 多条 `g2_energy` 记录，~60 s 间隔，字段合理
- **脚本交叉**: `verify_g2_telemetry.py` PASS（见 `query-result.txt`）
