# G2 HIL 验收证据 — IQ-26-00001 · 2026-05-30

| 字段 | 值 |
|------|-----|
| **sys_id** | `IQ-26-00001` |
| **MPPT / component_id** | `HQ2513A69PJ` |
| **Thing** | `IQEdge_1C:69:20:B8:D7:F4` |
| **G2 env** | `dev` |
| **固件** | `v2.3.003` · `IQEdge_G2` |
| **AWS 区域** | `us-east-1` |
| **签收** | ✅ [SIGNOFF-2026-05-30.md](SIGNOFF-2026-05-30.md) · Bob · 2026-05-30 |

---

## 验收清单

| # | 证据目录 | 控制台路径 | 状态 |
|---|----------|------------|------|
| 1 | [01-iot-mqtt-g2-telemetry](01-iot-mqtt-g2-telemetry/) | IoT Core → MQTT test client → G2 Topic | ✅ |
| 2 | [03-iot-rule](03-iot-rule/) | Rules → `iqedge_g2_dev_rule_energy` | ✅ Bob |
| 3 | [04-iot-thing-policy](04-iot-thing-policy/) | Thing + G2 Policy | ✅ Bob |
| 4 | [05-timestream-query](05-timestream-query/) | Timestream Query editor | ✅ Bob |
| 5 | [06-lambda-ingest-logs](06-lambda-ingest-logs/) | Lambda ingest logs | ✅ 008 代验 |
| 6 | [07-dynamodb-shadow](07-dynamodb-shadow/) + [08-dynamodb-registry-legacy](08-dynamodb-registry-legacy/) | Shadow + Registry + Legacy | ✅ Bob |
| 7 | [09-cloudformation-stacks](09-cloudformation-stacks/) | CloudFormation 四栈 | ✅ Bob |
| 8 | [10-iot-jobs-ota](10-iot-jobs-ota/) | IoT Jobs OTA v2.3.003 | ✅ Bob |
| — | [02-iot-mqtt-legacy](02-iot-mqtt-legacy/) | Legacy `device/status`（可选） | ⬜ 未单独录入 |

---

## 成果索引

| 样本 | 文件 | 说明 |
|------|------|------|
| 成果 1 | [sample-001-2026-05-30T205927Z.json](01-iot-mqtt-g2-telemetry/sample-001-2026-05-30T205927Z.json) | G2 MQTT · 20:59:27 UTC · soc 71% |

---

## 备注

**G2 dev 全链路验收通过。** 007 / 008 联调配合优秀 — 见 [SIGNOFF](SIGNOFF-2026-05-30.md)。

**下一步**: 量产 `esp32prod` OTA（300 s）；M5 network ingest 排期。
