# 交付清单 — M3 Registry Stack（dev）

> **提交人**: Agent 008  
> **日期**: 2026-05-29  
> **Stack**: `iqedge-g2-dev-registry`

---

## 1. 子任务状态

| ID | 子任务 | 状态 |
|----|--------|------|
| M3.1 | DDB `iqedge-g2-dev-table-registry` + GSI | ✅ |
| M3.2 | `device-record.v1.json` + HIL 示例 | ✅ |
| M3.3 | `import_legacy_registry.py` 脚手架 | ✅ |
| M3.4 | `track=g2` 判定 SOP（ADR-008） | ✅ |
| M3.5 | Provisioning Hook Lambda | ⏸ M17 |

---

## 2. AWS 产出（dev）

| 资源 | 名称 |
|------|------|
| DDB Registry | `iqedge-g2-dev-table-registry`（PK `sys_id`） |
| GSI | `gsi-alias-mppt` · `gsi-alias-legacy-device` |
| SSM | `/iqedge/g2/dev/registry/dynamodb-registry-table` |
| HIL 种子 | `sys_id=IQ-26-00001` · `track=g2` · `alias_mppt_serial=HQ2513A69PJ` |

**Lambda base role**: 已授予 Registry **只读**（供 M4 Ingest 查 track/aliases）。

---

## 3. 代码与契约

| 路径 | 说明 |
|------|------|
| `04-cloud/cdk/lib/stacks/g2-registry-stack.ts` | M3 Stack |
| `09-contract/schemas/registry/device-record.v1.json` | Registry JSON Schema |
| `09-contract/examples/registry/hil-iq-26-00001.v1.json` | HIL 种子 |
| `04-cloud/scripts/seed_registry_item.py` | 单条 upsert |
| `04-cloud/scripts/import_legacy_registry.py` | Legacy 批量导入脚手架 |

---

## 4. 验收命令

```powershell
cd e:\2601-IQEdge-Lite\09-contract
npm run validate:registry

cd e:\2601-IQEdge-Lite\04-cloud\scripts
python seed_registry_item.py --env dev --file ..\..\09-contract\examples\registry\hil-iq-26-00001.v1.json

aws dynamodb get-item --table-name iqedge-g2-dev-table-registry `
  --key '{"sys_id":{"S":"IQ-26-00001"}}' --region us-east-1

aws dynamodb query --table-name iqedge-g2-dev-table-registry `
  --index-name gsi-alias-mppt `
  --key-condition-expression "alias_mppt_serial = :s" `
  --expression-attribute-values '{":s":{"S":"HQ2513A69PJ"}}' --region us-east-1
```

---

## 5. 下游

**M4** — energy IoT Rule + Ingest（Registry 查 `track` + Schema 校验）→ 007 §5.3 Timestream 门禁。
