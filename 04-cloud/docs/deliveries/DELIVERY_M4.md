# 交付清单 — M4 energy Ingest Stack（dev）

> **提交人**: Agent 008  
> **日期**: 2026-05-30  
> **Stack**: `iqedge-g2-dev-ingest`

---

## 1. 子任务状态

| ID | 子任务 | 状态 |
|----|--------|------|
| M4.1 | IoT Rule `iqedge_g2_dev_rule_energy` | ✅ |
| M4.2 | Lambda `iqedge-g2-dev-fn-ingest-energy` + IAM | ✅ |
| M4.3 | Schema 校验 → Timestream WriteRecords | ✅ |
| M4.4 | Shadow DDB 快照更新 | ✅ |
| M4.5 | 结构化日志 | ✅ |
| M4.6 | CloudWatch 指标 `IngestSuccess` / `IngestValidationError` | ✅ |
| M4.7 | dev HIL 端到端（Thing MQTT → Timestream） | ⬜ 待 007 G2 Topic 发包 |

---

## 2. AWS 产出（dev）

| 资源 | 名称 |
|------|------|
| IoT Rule | `iqedge_g2_dev_rule_energy` |
| MQTT Topic | `iqedge/g2/dev/energy/telemetry` |
| Lambda | `iqedge-g2-dev-fn-ingest-energy` |
| SSM | `/iqedge/g2/dev/ingest/*` |
| HIL Policy | `iqedge-g2-dev-iot-policy-g2-device` → cert `…eb590933…` on Thing `IQEdge_1C:69:20:B8:D7:F4` |

---

## 3. 代码

| 路径 | 说明 |
|------|------|
| `04-cloud/cdk/lib/stacks/g2-ingest-stack.ts` | M4 Stack |
| `04-cloud/cdk/lambda/ingest-energy/handler.py` | Ingest 逻辑（ADR-008 门禁） |
| `04-cloud/cdk/bin/app.ts` | 注册 `G2IngestStack` |
| `04-cloud/scripts/attach_g2_iot_policy.py` | G2 Policy attach 运维脚本 |

---

## 4. 验收命令

```powershell
# Stack 存在
aws cloudformation describe-stacks --stack-name iqedge-g2-dev-ingest --region us-east-1

# Rule + Lambda
aws iot get-topic-rule --rule-name iqedge_g2_dev_rule_energy --region us-east-1
aws lambda get-function --function-name iqedge-g2-dev-fn-ingest-energy --region us-east-1

# Policy on HIL cert
aws iot list-targets-for-policy --policy-name iqedge-g2-dev-iot-policy-g2-device --region us-east-1

# Timestream（007 发包后）
cd 01-firmware
python tools\aws-verify\verify_g2_telemetry.py --sys-id IQ-26-00001 --minutes 30
```

---

## 5. 下游

- **007** — G2 MQTT §5.2 → `verify_g2_telemetry.py` §5.3（见 [`NOTICE_008_to_007`](../../../01-firmware/report/NOTICE_008_to_007_IQ-26-00001_2026-05-30.md)）  
- **M5** — network 域 ingest（复用 M4 模式）
