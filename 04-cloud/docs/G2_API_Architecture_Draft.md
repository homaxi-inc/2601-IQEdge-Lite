# G2 API 架构初稿 — 索引

> **Canonical 文档**: [`02-backend/docs/G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)  
> API 实现代码归属 **`02-backend/`**；本文件仅为 `04-cloud/docs` 交叉引用。

## 要点

- **主键**: **`sys_id`**（IQ System），非 MPPT SER# — 见 [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md)
- **风格**: Tesla Fleet API — `/api/v2/fleet/systems/{sys_id}/{domain}`
- **合流**: Registry `sys_id` + `aliases.mppt_serial` → Legacy / G2 Adapter
- **Legacy**: `/v1/devices/{mppt_serial}/...` deprecated shim
- **MVP**: `/status` + energy 读路径 + v1 兼容

详见 canonical 文档。
