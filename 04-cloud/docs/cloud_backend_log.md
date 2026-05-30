# G2 云端与后端开发日志

> **维护责任**: Agent 008  
> **目录**: `04-cloud/docs/`  
> **格式**: 倒序（最新条目在上）

---

## 2026-05-30 — Bob 控制台验收签收 · G2 HIL 关闭

**验收**: [`verification/G2-HIL-IQ-26-00001-2026-05-30/SIGNOFF-2026-05-30.md`](verification/G2-HIL-IQ-26-00001-2026-05-30/SIGNOFF-2026-05-30.md) — Rule / Policy / Timestream / DDB / CFN / OTA **全部 OK**  
**007+008**: v2.3.003 · G2 双发 · M4.7 端到端 · Legacy 并存

---

## 2026-05-30 — M4.7 HIL 签收 · Lambda `days_running` 去重上线

**007 完成**: [`NOTICE_007_to_008_IQ-26-00001_G2_DONE_2026-05-30.md`](../../01-firmware/report/NOTICE_007_to_008_IQ-26-00001_G2_DONE_2026-05-30.md) · 固件 **v2.3.003** · `verify_g2_telemetry.py` **PASS**（~60 s 间隔 · 6 rows/30m）  
**008 动作**: `cdk deploy iqedge-g2-dev-ingest` — Lambda CodeSha256 更新（`days_running` flat 回填仅当 `measures.yield` 无值）  
**M4.7**: ✅ dev HIL 端到端关闭  
**生产提示**: 现场量产 OTA 用 `pio run -e esp32prod`（NORMAL **300 s**）

---

## 2026-05-30 — M4 energy Ingest + HIL Policy attach · 007 可跑 §5.3

**Stack**: `iqedge-g2-dev-ingest`  
**Rule**: `iqedge_g2_dev_rule_energy` · Topic `iqedge/g2/dev/energy/telemetry`  
**Lambda**: `iqedge-g2-dev-fn-ingest-energy`（Schema 校验 · Timestream + Shadow · ADR-008 `track=g2` + fw ≥ v2.3.0）  
**运维**: Thing `IQEdge_1C:69:20:B8:D7:F4` ← cert `…eb590933…` · Policies `AllowAllPolicy` + `iqedge-g2-dev-iot-policy-g2-device`  
**交付**: [`deliveries/DELIVERY_M4.md`](deliveries/DELIVERY_M4.md)  
**007 通知**: [`01-firmware/report/NOTICE_008_to_007_IQ-26-00001_2026-05-30.md`](../../01-firmware/report/NOTICE_008_to_007_IQ-26-00001_2026-05-30.md) — **P0 关闭，可进入 §5.2 / §5.3**  
**待 007**: G2 Topic 发包 → `verify_g2_telemetry.py`（M4.7 端到端）

---

## 2026-05-29 — M3.4 `track` / 固件门禁定稿（Bob · ADR-008）

**规则**: Timestream 仅 `track=g2` **且** `firmware_version` **≥ v2.3.0**；新生产默认 `track=g2`；Legacy 导入初始 `legacy`；**晋升 g2 仅人工**；`track=g2` 永不回退。  
**HQ2513A69PJ 现网**: `v2.2.3.25`（尚不满足 G2 时序写入）。  
**007**: [`FIRMWARE_ALIGNMENT_007.md`](../../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) §2 · HIL §3.0  
**SOP**: [`G2_Registry_Track_Assignment_SOP.md`](G2_Registry_Track_Assignment_SOP.md)

---

## 2026-05-29 — M3 Registry Stack（dev 已部署 + HIL 种子）

**Stack**: `iqedge-g2-dev-registry`  
**表**: `iqedge-g2-dev-table-registry` · GSI `gsi-alias-mppt` / `gsi-alias-legacy-device`  
**种子**: `IQ-26-00001` · `track=g2` · `HQ2513A69PJ`  
**契约**: `09-contract/schemas/registry/device-record.v1.json`  
**交付**: [`deliveries/DELIVERY_M3.md`](deliveries/DELIVERY_M3.md)  
**下一步**: M4 energy Ingest

---

## 2026-05-29 — M2 Storage Stack（dev 已部署）

**Stack**: `iqedge-g2-dev-storage`  
**产出**: Timestream `iqedge_g2_dev_database` + 5 域表 · DDB `iqedge-g2-dev-table-shadow` · `iqedge-g2-dev-table-control-logs` · S3 `iqedge-g2-dev-vision-assets`  
**SSM**: `/iqedge/g2/dev/storage/*`  
**交付**: [`deliveries/DELIVERY_M2.md`](deliveries/DELIVERY_M2.md)  
**技术注记**: Timestream 复合分区键仅 `sys_id`；`component_id` 为写入 dimension（与 Cloud Design §5.1 文档「第二 dimension」在实现层等效）。

**下一步**: M3 Registry + M4 energy Ingest（007 HIL Timestream 门禁）。

---

## 2026-05-29 — G2 HIL 联调启动（IQ-26-00001 / HQ2513A69PJ）

**007 要求文档**: [`01-firmware/docs/G2_HIL_007_Firmware_Requirements.md`](../../01-firmware/docs/G2_HIL_007_Firmware_Requirements.md)  
**008 分工**: [`G2_HIL_008_007_Handoff.md`](G2_HIL_008_007_Handoff.md) — M2+M3+M4 为 007 G2 Timestream 门禁前置  
**sys_id**: `IQ-26-00001`（站点称呼 IQW-9041 仅历史别名）

---

**Stack**: `iqedge-g2-dev-foundation` · KMS · SSM · `iqedge-g2-dev-role-lambda-base` · `iqedge-g2-dev-iot-policy-g2-device`  
**前置**: `cdk bootstrap aws://661631955220/us-east-1`  
**交付**: [`deliveries/DELIVERY_M1.md`](deliveries/DELIVERY_M1.md)

---

Renamed from `重要决策/` → [`decisions/README.md`](../../decisions/README.md). All repo links updated.

---

**决策**: `IQ-{YY}-{NNNNN}`（例 `IQ-26-00001`）；中性 ID；`system_type` 部署 assign；Legacy `IQW-*` grandfather。  
**索引**: [`decisions/README.md`](../../decisions/README.md) ADR-003–007  
**宪法**: `G2_System_Model.md` §4 重写 · Schema/examples/007 对齐

---

## 2026-05-29 — sys_id 中性身份方案（讨论 → 已并入 ADR-004）

Bob：**sys_id 不应绑定 IQW/IQB/IQT 产品线前缀**；`system_type` 在产测/部署 assign；改型可 re-assign。  
**评审稿**: [`02-backend/docs/G2_sys_id_Design_Proposal.md`](../../02-backend/docs/G2_sys_id_Design_Proposal.md)  
**动作**: 定稿前 **不** 全局改 Schema；M0.3 契约暂保留旧 pattern，批准后可快速批量替换。

---

**产出**: `09-contract/schemas/energy/telemetry.v1.json` + `common/g2-envelope.v1.json` + examples + `FIRMWARE_ALIGNMENT_007.md`  
**验收**: `09-contract` → `npm run validate:energy` pass  
**交付**: [`deliveries/DELIVERY_M0.3.md`](deliveries/DELIVERY_M0.3.md)

---

## 2026-05-29 — M0.1 + M0.2 工程基座（Bob 批准）

**M0.1**: `04-cloud/cdk/` — TypeScript CDK v2，`bin/app.ts`，`-c env=dev|prod`，`G2ScaffoldStack`，`lib/naming.ts`  
**M0.2**: `09-contract/` — 五域 + registry Schema 目录约定与 README  
**验收**: `npm run synth:dev` / `synth:prod`（未 deploy）  
**交付清单**: [`deliveries/DELIVERY_M0.1_M0.2.md`](deliveries/DELIVERY_M0.1_M0.2.md)

---

## 2026-05-29 — G2 云端与后端任务分解（执行蓝图）

**产出**: [`G2_Implementation_Task_Breakdown.md`](G2_Implementation_Task_Breakdown.md) — 模块 M0–M18、子任务 ID、P0–P5 映射、MVP 关键路径

---

## 2026-05-29 — IQCloud 商业战略白皮书（00-strategy）

**核心**: 产品身份（离网资产自治 OS + 变现引擎）· B2B2C 双受众 · 四大商业价值（Truck Roll / 流量 OPEX / 主动威慑 / Dealer SaaS 溢价）。

**产出**: [`00-strategy/docs/IQCLOUD_COMMERCIAL_STRATEGY.md`](../../00-strategy/docs/IQCLOUD_COMMERCIAL_STRATEGY.md)（已脱敏，不含具体定价与运营商名）

---

## 2026-05-29 — Customer vs User 前端架构蓝图（方案待决）

**核心**: Customer=商业租户(Tenant) · User=自然人+Role；前端 **双模块解耦**；Client≈Customer。

**产出**: [`G2_Customer_User_Frontend_Blueprint.md`](G2_Customer_User_Frontend_Blueprint.md)

---

## 2026-05-29 — Client 租户层 SaaS 商业架构（方案待决）

**核心**: Admin 造平台 · Dealer 管资产 · Client 用安防；隐私隔离 / RMR / 布撤防自助 / control 审计。

**产出**: [`G2_Client_Tenant_Model.md`](G2_Client_Tenant_Model.md)

---

**议题**: ① 双 GPS 主备融合 ② High-Frequency Tracking FSM ③ HDOP 虚警过滤。

**状态**: ⏸ 仅记录盲区；`location_primary_role` 逻辑未实现。

**产出**: [`G2_GPS_Fusion_And_Tracking_Open_Issues.md`](G2_GPS_Fusion_And_Tracking_Open_Issues.md)

---

**问题**: 4G 中断 → 云端 Timestream 空洞 → 大屏假「平安」。

**方案**: X1 本地 WAL（SD/HDD）→ 重连对齐 `last_acked_event_time` → 批量追溯写入；`ingest_mode=backfill` + 幂等。

**产出**: [`G2_Smart_Backfill_Architecture.md`](G2_Smart_Backfill_Architecture.md)

---

**已定**: VQA 为 vision 核心上行；Camera 孪生字段 `focus_blur` / `video_blind` / `scene_change`；Topic `vision/telemetry`；**network Ping ≠ vision 健康**。

**产出**: Domain Map · System Model §3.7 · API `.../vision/health` · Cloud Architecture Timestream

---

**已定**: 预置语音 → **control** (`play_audio`)；实时 Talk-down → **vision** WebRTC 信令；Camera→Speaker **X1 本地行为树**，禁止云端长链路实时联动。

**产出**: [`G2_Domain_Map.md`](G2_Domain_Map.md) · [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.6 · API 初稿

---

**问题**: SIM 需连网激活，未激活则无法连网 — 现场「变砖」风险。

**候选**: ① Granite Test Ready 微流量 + First Ping Lambda（Zero-Touch）② OOB 扫码 + `POST .../activate`（Dealer Portal）。

**状态**: ⏸ 待 Granite 能力确认后选型。

**产出**: [`G2_SIM_Provisioning_Deadlock.md`](G2_SIM_Provisioning_Deadlock.md)

---

**预留**: IQWatch/IQTrailer `commercial.*` 租赁字段；GPS `gps_router` / `gps_asset_tracker`；`network/location` API。

**产出**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.5 · §11

---

**已定**: IQTrailer 单 Cerbo；Modbus TCP；多 MPPT 由 Cerbo 汇聚；云端/API 以 Cerbo 为 energy 主读组件。

**遗留**: AC IQBox energy 来源 · Provisioning 扫码 SOP · deployment_state 权限。

**产出**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.3 · §3.4 · §10

---

**决策**: dev/prod **共用** `IQ-YY-NNNNN` 发号（ADR-004）；Legacy `IQW/IQB/IQT` grandfather。`deployment_state` 表达生命周期。

**（已废止 2026-05-29）** 原 IQW/IQB/IQT 分产品线前缀发号 — 见 `cloud_backend_log` 同日 ADR-004。

**产出**: 更新 [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §4.4 · §5

---

**规则（已废止）**: 原方案 A — `IQW-9001+` / `IQB-1001+` / `IQT-6001+` — **由 ADR-004 取代**。

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
