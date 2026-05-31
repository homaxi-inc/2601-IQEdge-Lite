# G2 Cerbo HIL — IQ-26-60001 · EDGE-T3

> **台架**: RUT VPN `10.24.121.199` · SN `6004727310` · Cerbo `c0619ab6be37`  
> **007**: EDGE-T2 ✅ · **008**: Registry + M4 联调  
> **Topic**: `iqedge/g2/dev/energy/telemetry`

---

## 验收清单

| # | 检查 | 负责 | 状态 |
|---|------|------|------|
| 1 | Registry `IQ-26-60001` · `track=g2` · `iqtrailer` | 008 | ⬜ |
| 2 | MQTT G2 消息 ~60 s · `component_role=cerbo` | 007/008 | ⬜ |
| 3 | Lambda `ingest_ok` · `iqtrailer=True` | 008 | ⬜ |
| 4 | Timestream 有 `sys_id=IQ-26-60001` 行 | 007/008 | ⬜ |
| 5 | Shadow 快照更新 | 008 | ⬜ |
| 6 | Legacy `iot/rut241/status` 仍可用 | 007 | ⬜ |
| 7 | VRM/Modbus 数值同量级（可选） | 007 | ⬜ |

---

## 子目录（证据占位）

| 目录 | 内容 |
|------|------|
| `01-iot-mqtt-g2-telemetry/` | MQTT 样本 JSON |
| `02-iot-rule/` | Rule SQL 截图/notes |
| `03-lambda-ingest/` | CloudWatch ingest_ok 日志 |
| `04-timestream-query/` | SQL + 结果 |
| `05-dynamodb-registry/` | Registry get-item |
| `06-dynamodb-shadow/` | Shadow snapshot |

---

## 命令

```powershell
python 01-firmware/tools/aws-verify/verify_g2_telemetry.py --sys-id IQ-26-60001 --minutes 30
```

联调分工：[`G2_CERBO_008_007_Handoff.md`](../../G2_CERBO_008_007_Handoff.md)
