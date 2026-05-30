# 05-integration · Open issues

> **Purpose**: Bob 待定项 · 决策前 **不得** 写入契约或 ingest 硬编码  
> **Index**: ADR-012 · [`decisions/README.md`](../../decisions/README.md)

---

## OI-001 · IQTrailer `state` / `reporting_mode`（D-6 遗留）

**状态**: ⬜ 待讨论  
**背景**: `energy.telemetry.v1` 的 `state`（007 边缘状态机：NORMAL / LOW_POWER / SURVIVAL / FAULT）与 `reporting_mode` 为 IQWatch ESP32 语义。Cerbo/RUT 路径无等价 FSM。

**选项（未决）**:

| 选项 | Trailer 行为 |
|------|--------------|
| A | 固定 `state=NORMAL`, `reporting_mode=NORMAL` |
| B | 由 Cerbo Modbus **844** 充电状态推导 `state` |
| C | IQTrailer Schema 豁免 `state`（major 版本变更） |
| D | RUT 自有降频策略映射 `reporting_mode` |

**阻塞范围**: EDGE-T2 RUT JSON 模板最终字段 · 前端状态展示 · 告警规则  
**不阻塞**: M4 ingest（当前 Schema 仍 required；临时可填占位值 **仅台架**，**非生产定稿**）

**决策后动作**: 更新 ADR-012 · `telemetry.v1.json` · Field Reference · RUT Data to Server 模板

---

## OI-002 · Yield 寄存器 system 级等价性（D-10 · 台架验证）

**状态**: ⬜ 待台架实测  
**背景**: Victron 3.73 中 **784/786**（Yield today/yesterday）在 `com.victronenergy.solarcharger` 服务（per-device Unit ID）。Bob 定稿：**仅当 Unit 100（system）存在等价寄存器时才读**；否则 MVP 不上 yield 或延后 EDGE-T1+。

**台架步骤**:

1. Modbus 读 Unit **100** 全表或 Venus 文档对照 — 确认 system 级 yield 地址  
2. 若无等价 — 标记 yield 为 Phase 2（solarcharger Unit ID 方案 **不在** D-10 批准范围）  
3. 若有等价 — 更新 [`modbus-register-map.md`](../cerbo/modbus-register-map.md) 与 RUT Request 表

**关联**: ADR-012 D-9（MVP 含 yield **条件性**）

---

*Agent 008 · 2026-05-30*
