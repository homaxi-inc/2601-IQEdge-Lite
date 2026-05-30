# 交付清单 — M1 Foundation Stack（dev）

> **提交人**: Agent 008  
> **日期**: 2026-05-29  
> **账户**: `661631955220` · `us-east-1`  
> **Stack**: `iqedge-g2-dev-foundation`

---

## 1. 子任务状态

| ID | 子任务 | 状态 |
|----|--------|------|
| M1.1 | `G2FoundationStack` dev 部署 | ✅ |
| M1.2 | KMS CMK + alias | ✅ |
| M1.3 | SSM 参数层级 | ✅ |
| M1.4 | G2 IoT Thing Policy（五域精确 Topic，禁止跨 env） | ✅ |
| M1.5 | `iqedge-g2-dev-role-lambda-base` | ✅ |

---

## 2. AWS 产出（dev）

| 资源 | 名称 / ARN |
|------|------------|
| CloudFormation | `iqedge-g2-dev-foundation` |
| KMS | `alias/iqedge-g2-dev-cmk` |
| IAM Role | `iqedge-g2-dev-role-lambda-base` |
| IoT Policy | `iqedge-g2-dev-iot-policy-g2-device` |
| SSM 前缀 | `/iqedge/g2/dev/foundation/*` · `/iqedge/g2/dev/config/g2-env` |

**Outputs（部署日志）**:

- `KmsKeyArn`: `arn:aws:kms:us-east-1:661631955220:key/b6efa1ea-9281-498f-9edc-4458e78644a0`
- `LambdaBaseRoleArn`: `arn:aws:iam::661631955220:role/iqedge-g2-dev-role-lambda-base`

---

## 3. 代码产出

| 路径 | 说明 |
|------|------|
| `04-cloud/cdk/lib/stacks/g2-foundation-stack.ts` | M1 主 Stack |
| `04-cloud/cdk/lib/constructs/g2-iot-thing-policy-document.ts` | IoT 策略文档生成 |
| `04-cloud/cdk/lib/config/domains.ts` | 五域 MQTT 后缀 |
| `04-cloud/cdk/lib/config/ssm-paths.ts` | SSM 路径助手 |
| `04-cloud/cdk/bin/app.ts` | 入口改为 Foundation（移除 Scaffold 部署） |

---

## 4. 验收命令

```powershell
cd e:\2601-IQEdge-Lite\04-cloud\cdk
npx cdk synth -c env=dev
aws ssm get-parameter --name /iqedge/g2/dev/foundation/kms-key-arn --region us-east-1
aws iot get-policy --policy-name iqedge-g2-dev-iot-policy-g2-device --region us-east-1
```

---

## 5. 说明

- 首次部署前已执行 `cdk bootstrap aws://661631955220/us-east-1`。
- **未改动 Legacy** IoT Rule / DDB / Lambda。
- **下一步建议**: **M2**（Timestream `table_energy` + Shadow DDB）→ **M3** Registry。

---

*Agent 008 · M1 · dev deployed*
