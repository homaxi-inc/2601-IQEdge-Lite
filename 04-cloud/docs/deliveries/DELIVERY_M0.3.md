# 交付清单 — M0.3 Energy Telemetry Schema（待 Bob 审阅）

> **提交人**: Agent 008  
> **日期**: 2026-05-29  
> **前置**: M0.1 + M0.2 **已批准**  
> **任务**: [`G2_Implementation_Task_Breakdown.md`](../G2_Implementation_Task_Breakdown.md) · M0.3

---

## 1. 交付摘要

| 任务 | 产出 | 验收 |
|------|------|------|
| **M0.3** | `energy.telemetry.v1` JSON Schema + 样例 + 007 对齐文档 | `npm run validate:energy` ✅ |

---

## 2. 产出物

| # | 路径 | 说明 |
|---|------|------|
| 1 | `09-contract/schemas/common/g2-envelope.v1.json` | 跨域 telemetry 信封 |
| 2 | `09-contract/schemas/energy/telemetry.v1.json` | Energy 域契约（G2 + Legacy flat 过渡） |
| 3 | `09-contract/examples/energy/telemetry-iqwatch-live.v1.json` | IQWatch + MPPT 样例 |
| 4 | `09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json` | IQTrailer + Cerbo 样例 |
| 5 | `09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md` | **007 固件对齐要求** |
| 6 | `09-contract/package.json` | `npm run validate:energy` |
| 7 | `09-contract/package-lock.json` | 依赖锁定 |

---

## 3. 设计要点（G2 最佳实践）

| 原则 | 实现 |
|------|------|
| Fleet 主键 | 必填 `sys_id` + `component_id` + `component_role` |
| 域契约 | `schema_version` = `energy.telemetry.v1`，`domain` = `energy` |
| 正确单位 | 推荐 `measures.*`（`_v` `_a` `_w` `_kwh` `_pct`）；ingest **不二次 ÷1000** |
| Legacy 过渡 | 保留 `soc` / `battery_voltage` / nested `battery` 等（`payload.md` v2） |
| IQTrailer | `component_role=cerbo`，系统级汇聚，不逐 MPPT |
| Smart Backfill | 信封支持 `ingest_mode` / `event_time`（与 vision 共用） |

---

## 4. 验收命令

```powershell
cd e:\2601-IQEdge-Lite\09-contract
npm install
npm run validate:energy
```

**008 结果**: 两个 example **valid**（2026-05-29）

---

## 5. 007 协作项（已写入 FIRMWARE_ALIGNMENT_007.md）

- 新 Topic：`iqedge/g2/{env}/energy/telemetry`
- NVS/产测烧录 `sys_id`
- 优先 `measures` 块；`state` / `reporting_mode` 与求生状态机对齐

---

## 6. 批准后建议下一任务

1. **M1.1** — `G2FoundationStack` dev deploy  
2. **M2.1** — Timestream DB + `table_energy`  
3. **M4.1** — IoT Rule + ingest Lambda（Schema 门禁引用本文件）

---

## 7. 批准栏

| 角色 | 决定 | 日期 |
|------|------|------|
| Bob | ☐ 批准 / ☐ 驳回 | |

---

*Agent 008 · M0.3 Delivery*
