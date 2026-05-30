# G2 云端与后端开发日志

> **维护责任**: Agent 008  
> **目录**: `04-cloud/docs/`  
> **格式**: 倒序（最新条目在上）

---

## 2026-05-29 — 租赁模式 + GPS 双形态 Roadmap 预留

**预留**: IQWatch/IQTrailer `commercial.*` 租赁字段；GPS `gps_router` / `gps_asset_tracker`；`network/location` API。

**产出**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.5 · §11

---

**已定**: IQTrailer 单 Cerbo；Modbus TCP；多 MPPT 由 Cerbo 汇聚；云端/API 以 Cerbo 为 energy 主读组件。

**遗留**: AC IQBox energy 来源 · Provisioning 扫码 SOP · deployment_state 权限。

**产出**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.3 · §3.4 · §10

---

**决策**: dev/prod **共用** IQW/IQB/IQT 发号；不用 `IQW-DEV-*`。仓库零件 / 工厂待组装 / 现场部署等由 Registry **`deployment_state`** 表达；AWS 云环境 dev/prod 与 sys_id 正交（`cloud_target` 字段）。

**产出**: 更新 [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §4.4 · §5

---

**规则**: 方案 A — `IQW-9001+` · `IQB-1001+` · `IQT-6001+`；9999 用尽后向下借块（8001、7001…）。Bob 判断：占满时公司已需架构升级，不必现在过度设计。008 认同。

**产出**: 更新 [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §4

---

**执行方**: Agent 008（归档）  
**产出**: [`02-backend/docs/G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md)；API 架构升级 v0.2

**核心**: Fleet 主键 = **`sys_id`**（非 MPPT SER#）；四种形态 `iqwatch` / `solar_iqbox` / `ac_iqbox` / `iqtrailer`；组件 `component_id` 映射五域；Legacy 经 `aliases.mppt_serial` 合流。

---

**执行方**: Agent 008  
**产出**: [`02-backend/docs/G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)（canonical）；`04-cloud/docs/` 索引副本

**核心**: Tesla Fleet 风格 `/api/v2/fleet/devices/{device_id}/{domain}`；API 代码归 `02-backend/`；Legacy v1 shim；Registry 双轨 adapter。

---

**执行方**: Agent 008（Bob 授权）  
**动作**: `aws iot disable-topic-rule --rule-name Route_Energy_To_Lambda`  
**原因**: 临时调试用途的 `#` catch-all Rule；与 `DeviceStatusToLambda` 重复触发。G2 新 Rule 禁止复制此模式。  
**状态**: ✅ `ruleDisabled: true`（Lambda `IQTrailerDataRouter` 保留，未删除）

---

## 2026-05-29 — G2 云架构设计 v0.1

**执行方**: Agent 008  
**产出**: [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md)

**核心**: 6 层逻辑架构；7 个 CDK Stack；五域精确 Topic Rule + 轻量 Ingest Lambda；Timestream 五表 + DDB Shadow 单表 + Registry 合流；MVP = P0+P1 energy 全链路。

---

**执行方**: Agent 008（归档）  
**产出**: [`G2_Domain_Map.md`](G2_Domain_Map.md)；同步更新 [`008_Strategic_Guide.md`](008_Strategic_Guide.md) 命名节

**核心**: energy / network / vision / environment / control 五域；Topic `iqedge/g2/{env}/{domain}/…`；API `/api/v2/{domain}/…`；Timestream `iqedge_g2_{env}_table_{domain}`。

---

**执行方**: Agent 008（归档）  
**产出**: 更新 [`008_Strategic_Guide.md`](008_Strategic_Guide.md) § G2 命名体系

**核心**: 新轨统一前缀 `iqedge-g2-{env}-*` / `iqedge_g2_{env}_*`；MQTT Topic `iqedge/g2/{env}/telemetry`；老名称（`device/status`、`DeviceLatestStatus` 等）**禁止改动**。云端 → 008；固件 Topic → 007 对齐。

---

**执行方**: Agent 008（归档）  
**产出**: [`008_Strategic_Guide.md`](008_Strategic_Guide.md)

**核心**: 老设备锁老架构（维护态）；新批次 G2 IaC 全新隔离；唯一合流点在 `02-backend` API（按 deviceId 路由 Legacy DDB vs G2 Timestream）。

---

## 2026-05-29 — 首次云端部署审查

**执行方**: Agent 008  
**动作**: AWS 登录验证 + 全量资源盘点 + G2 开发计划制定  
**产出**: [`G2_Cloud_Deployment_Audit_2026-05-29.md`](G2_Cloud_Deployment_Audit_2026-05-29.md)

**关键结论**:

- Energy 域（`device/status` → `SaveDeviceStatus` → DDB/Timestream）健康；IoT Rule SQL `2015-10-08` 已确认。
- 无 CDK/IaC；`04-cloud/` 自本次起建立 `docs/` 文档目录。
- ~~`Route_Energy_To_Lambda`（topic `#`）~~ **已禁用**（2026-05-29，Bob 确认仅为临时调试 Rule）。
- G2 五域中 environment / control 尚未建设；POE XOR 推导待 Phase 2。

**下一步（待 Bob 确认）**: Phase 0 — 初始化 `04-cloud/cdk/`、`09-contract/`、`02-backend/`。
