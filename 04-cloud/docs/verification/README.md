# G2 云端验收证据 (verification/)

> **维护**: Agent 008 / Bob  
> **用途**: AWS 控制台逐步验证的截图说明、查询结果、MQTT 抓包等**原始证据**  
> **与 `deliveries/` 区别**: `deliveries/` = 交付清单；本目录 = 可复现的验收素材

---

## 目录结构

```text
verification/
  README.md                                    ← 本文件
  G2-HIL-{sys_id}-{YYYY-MM-DD}/                ← 单次联调/验收会话
    README.md                                  ← 会话索引 + 勾选清单
    01-iot-mqtt-g2-telemetry/                  ← IoT Test Client · G2 Topic
    02-iot-mqtt-legacy/                        ← Legacy device/status（可选）
    03-iot-rule/                               ← Rule iqedge_g2_*_rule_energy
    04-iot-thing-policy/                       ← Thing + Policy attach
    05-timestream-query/                       ← Query editor 结果
    06-lambda-ingest-logs/                     ← ingest-energy CloudWatch
    07-dynamodb-shadow/                        ← Shadow 表项
    08-dynamodb-registry-legacy/               ← Registry + Legacy DDB
    09-cloudformation-stacks/                  ← 四栈状态
    10-iot-jobs-ota/                           ← OTA Job 验收
```

---

## 命名约定

| 类型 | 文件名示例 |
|------|------------|
| JSON 抓包 | `sample-001-2026-05-30T205927Z.json` |
| 查询结果 | `query-energy-30m-2026-05-30.txt` 或 `.sql` + `.txt` |
| 截图说明 | `notes.md`（描述控制台路径 + 粘贴关键字段） |
| 截图文件 | `screenshot-01.png`（自行添加） |

---

## 索引

| 日期 | 会话 | 摘要 |
|------|------|------|
| 2026-05-30 | [G2-HIL-IQ-26-00001-2026-05-30](G2-HIL-IQ-26-00001-2026-05-30/README.md) | **✅ 签收** · M4 HIL · v2.3.003 · [SIGNOFF](G2-HIL-IQ-26-00001-2026-05-30/SIGNOFF-2026-05-30.md) |

---

## 关联

- 控制台验证步骤：见 Bob 会话「AWS 控制台验证」或 [`DELIVERY_M4.md`](../deliveries/DELIVERY_M4.md) §4  
- 007 完成通知：[`01-firmware/report/NOTICE_007_to_008_IQ-26-00001_G2_DONE_2026-05-30.md`](../../../01-firmware/report/NOTICE_007_to_008_IQ-26-00001_G2_DONE_2026-05-30.md)
