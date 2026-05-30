# registry — 设备注册表 Schema (M3.2 ✅)

| 文件 | 说明 |
|------|------|
| [`device-record.v1.json`](device-record.v1.json) | DynamoDB `iqedge-g2-{env}-table-registry` 项契约 |
| [`../../examples/registry/hil-iq-26-00001.v1.json`](../../examples/registry/hil-iq-26-00001.v1.json) | HIL 台架种子 |

**验收**: `npm run validate:registry`

**写入**: `04-cloud/scripts/seed_registry_item.py`

字段详解 → [`02-backend/docs/G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §6。