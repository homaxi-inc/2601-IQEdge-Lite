# 交付清单 — M2 Storage Stack（dev）

> **提交人**: Agent 008  
> **日期**: 2026-05-29  
> **账户**: `661631955220` · `us-east-1`  
> **Stack**: `iqedge-g2-dev-storage`（依赖 `iqedge-g2-dev-foundation`）

---

## 1. 子任务状态

| ID | 子任务 | 状态 |
|----|--------|------|
| M2.1 | Timestream DB `iqedge_g2_dev_database` | ✅ |
| M2.2 | 五张域表 + 保留策略（memory 24h / magnetic 365d） | ✅ |
| M2.3 | DDB Shadow `iqedge-g2-dev-table-shadow` | ✅ |
| M2.4 | S3 `iqedge-g2-dev-vision-assets`（KMS + lifecycle IA 90d） | ✅ |
| M2.5 | DDB Control 审计 `iqedge-g2-dev-table-control-logs` | ✅ |

---

## 2. AWS 产出（dev）

| 资源 | 名称 |
|------|------|
| CloudFormation | `iqedge-g2-dev-storage` |
| Timestream DB | `iqedge_g2_dev_database` |
| Timestream 表 | `iqedge_g2_dev_table_energy` · `…_network` · `…_vision` · `…_environment` · `…_table_control_logs` |
| DDB Shadow | `iqedge-g2-dev-table-shadow`（PK `pk`, SK `sk`） |
| DDB Control logs | `iqedge-g2-dev-table-control-logs` |
| S3 | `iqedge-g2-dev-vision-assets` |

**Timestream 分区**: 复合分区键仅 **`sys_id`**（AWS 限制单键）；`component_id` 作为 record dimension 写入（M4 Ingest 负责）。

**SSM**（`/iqedge/g2/dev/storage/`）:

- `timestream-database`
- `timestream-table-energy`
- `dynamodb-shadow-table`
- `dynamodb-control-logs-table`
- `s3-vision-bucket`

---

## 3. 代码产出

| 路径 | 说明 |
|------|------|
| `04-cloud/cdk/lib/stacks/g2-storage-stack.ts` | M2 主 Stack |
| `04-cloud/cdk/lib/constructs/g2-timestream-domain-table.ts` | 单域 Timestream 表 |
| `04-cloud/cdk/lib/config/timestream-tables.ts` | 表名后缀映射 |
| `04-cloud/cdk/lib/config/ssm-paths.ts` | 新增 `ssmStoragePath()` |
| `04-cloud/cdk/bin/app.ts` | 注册 Storage，依赖 Foundation |

---

## 4. 验收命令

```powershell
cd e:\2601-IQEdge-Lite\04-cloud\cdk
aws timestream-write describe-database --database-name iqedge_g2_dev_database --region us-east-1
aws timestream-write describe-table --database-name iqedge_g2_dev_database --table-name iqedge_g2_dev_table_energy --region us-east-1
aws dynamodb describe-table --table-name iqedge-g2-dev-table-shadow --region us-east-1
aws s3api head-bucket --bucket iqedge-g2-dev-vision-assets --region us-east-1
aws ssm get-parameter --name /iqedge/g2/dev/storage/timestream-database --region us-east-1
```

---

## 5. 下游

| 里程碑 | 说明 |
|--------|------|
| **M3** | Registry DDB + 种子 `IQ-26-00001` |
| **M4** | energy IoT Rule + Ingest → 本 Stack Timestream/Shadow |
| **007 HIL** | `verify_g2_telemetry.py` 在 M4 后有数据后可查 |

---

## 6. 部署备注

- 首次失败原因：Timestream 仅允许 **1** 个 composite partition key；CFN 保留期字段需 PascalCase。
- 回滚后表名有短暂 **AlreadyExists** 窗口；表按序创建 + 清理孤儿资源后重试成功。
