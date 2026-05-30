# Lambda ingest — ✅ 已通过（008 代验 · Bob 只验结果）

- **验收策略**: Bob 以 Timestream / MQTT 结果为准；008 补充 Lambda 侧证据
- **函数**: `iqedge-g2-dev-fn-ingest-energy`
- **Log group**: `/aws/lambda/iqedge-g2-dev-fn-ingest-energy`

## 008 确认

| 检查项 | 结果 |
|--------|------|
| Invocations 随 G2 上报增加 | ✅ |
| Errors ≈ 0 | ✅ |
| 日志含 `ingest_ok sys_id=IQ-26-00001` | ✅ |
| `days_running` 去重 handler redeploy | ✅ 2026-05-30 20:47 UTC |

## 推断

G2 MQTT → Rule → Lambda → Timestream 全链路正常；与 Bob Timestream 控制台所见一致。
