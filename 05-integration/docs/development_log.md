# 05-integration 开发日志

> **格式**: 倒序（最新在上）

---

## 2026-05-30 — VRM API 预检 PASS（007 可开工 Cerbo 集成）

- T1–T6 全过 · 站点 **964243 ST-03** · Gateway **`c0619ab6be37`**
- 报告：[`deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md`](deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md)
- 脚本：[`scripts/vrm_cerbo_preflight.py`](../scripts/vrm_cerbo_preflight.py)

---

## 2026-05-30 — VRM API 凭据同步（003_IQTrailer → 本地 .env）

- Bob 提供研究仓 token · 已写入 **`05-integration/.env`**（gitignored）  
- 说明：[`docs/vrm-api-local-credentials.md`](vrm-api-local-credentials.md) · 模板 `.env.example`  
- **用途**：Modbus 台架交叉校验 · **非** G2 生产 ingest

---

## 2026-05-30 — Agent 分工：007 主责 `05-integration/`

- Bob 授权：007 主责 Cerbo · Modbus · RUT/RutOS · 台架；可读写本目录全树  
- 008：云端 Topic / Policy / M4/M5 对齐 · 不另改 Schema  
- 同步：[`AGENTS.md`](../../AGENTS.md)

---

## 2026-05-30 — ADR-012 IQTrailer Cerbo 定稿（Bob D-1～D-10）

- **ADR-012** · [`decisions/README.md`](../../decisions/README.md) · [`open_issues.md`](open_issues.md)（D-6 · D-10）
- M4：`ingest-energy` **豁免** IQTrailer/Cerbo 固件门禁
- 契约：`component_role=cerbo` · 样例 [`09-contract/examples/iqtrailer/`](../../09-contract/examples/iqtrailer/)
- **下一步**: EDGE-T2 RutOS Data to Server 模板 · OI-002 台架 yield 验证

---

## 2026-05-30 — Modbus 寄存器 + RUT Client 摘要（PDF + Excel 3.73）

- 来源：`IQCloud_RUT_Modbus_Client_Configuration_Guide_V1.0.pdf` · `Victron-CCGX-Modbus-TCP-register-list-3.73.xlsx`  
- 产出：[`G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](../cerbo/docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md) · 更新 [`modbus-register-map.md`](../cerbo/modbus-register-map.md)  
- 要点：Unit ID **100** · RUT 地址 **+1** · 黄金四件套 840–850 → G2 `measures`  
- **下一步**: RUT `Data to Server` / 脚本 → G2 MQTT（`../../rut/`）

---

## 2026-05-30 — G2 Cerbo Modbus 连接摘要

- 自 `003_IQTrailer/05-modbus/` 提炼（研究仓尚无寄存器表，仅目录 + 台架 IP）  
- 产出：[`cerbo/docs/G2_Cerbo_Modbus_Connectivity.md`](../cerbo/docs/G2_Cerbo_Modbus_Connectivity.md) · [`cerbo/modbus-register-map.md`](../cerbo/modbus-register-map.md) 模板  
- **下一步**: EDGE-T1 首条 Modbus 实测填入寄存器表

---

## 2026-05-30 — G2 Cerbo 字段参考摘要

- 自 `003_IQTrailer/02-study/` 提炼 Victron 物理量 → `energy.telemetry.v1` 映射  
- 产出：[`cerbo/docs/G2_Cerbo_Energy_Field_Reference.md`](../cerbo/docs/G2_Cerbo_Energy_Field_Reference.md)  
- **排除**：VRM API、前端图表等非 G2 ingest 内容  
- **下一步**: EDGE-T1 Modbus 寄存器地址表

---

## 2026-05-30 — 目录脚手架

- 创建 `05-integration/` · `cerbo/` · `rut/` · `iqtrailer/`
- **ADR-011** · 更新 `AGENTS.md` · 根 `README.md` · System Model §3.4 数据流（RUT 发 MQTT）
- **下一步**: EDGE-T1 Modbus 寄存器图 · EDGE-T2 RUT→G2 JSON

---
