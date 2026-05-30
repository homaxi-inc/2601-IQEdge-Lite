# 04-cloud — G2 云端基础设施

Agent 008 主战场：AWS CDK (IaC)、Timestream、IoT Rules、Lambda、多环境 (dev/prod) 部署。

## 目录结构

```text
04-cloud/
  README.md           ← 本文件
  cdk/                ← M0.1 ✅ AWS CDK (TypeScript)
  docs/               ← 审查报告、开发日志
```

## 契约与 CDK

| 路径 | 说明 |
|------|------|
| [`cdk/README.md`](cdk/README.md) | **CDK 使用说明** — `npm run synth:dev` |
| [`../09-contract/README.md`](../09-contract/README.md) | **M0.2 ✅** JSON Schema / OpenAPI 约定 |

## 文档

| 文件 | 说明 |
|------|------|
| [`docs/G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) | **系统身份** — sys_id · 四种 IQ 形态 · 组件矩阵 |
| [`docs/G2_System_Model.md`](docs/G2_System_Model.md) | **系统身份** — 索引 → `02-backend/docs/` |
| [`docs/G2_API_Architecture_Draft.md`](docs/G2_API_Architecture_Draft.md) | **API 架构** — 索引 → `02-backend/docs/` |
| [`01-firmware/docs/G2_HIL_007_Firmware_Requirements.md`](../01-firmware/docs/G2_HIL_007_Firmware_Requirements.md) | **007 HIL** — IQ-26-00001 固件/OTA/验证 |
| [`docs/G2_HIL_008_007_Handoff.md`](docs/G2_HIL_008_007_Handoff.md) | 008↔007 分工 · M2+M4 门禁 |
| [`docs/deliveries/DELIVERY_M0.3.md`](docs/deliveries/DELIVERY_M0.3.md) | M0.3 Energy Schema |
| [`docs/G2_Implementation_Task_Breakdown.md`](docs/G2_Implementation_Task_Breakdown.md) | **执行蓝图** — 模块 M0–M18 · 子任务 · MVP 关键路径 |
| [`docs/G2_Cloud_Architecture_Design.md`](docs/G2_Cloud_Architecture_Design.md) | **云架构设计** — CDK 分层、数据流、存储、安全 |
| [`docs/G2_Domain_Map.md`](docs/G2_Domain_Map.md) | **五域命名宪法** — Topic / API / 表名矩阵 |
| [`docs/G2_Customer_User_Frontend_Blueprint.md`](docs/G2_Customer_User_Frontend_Blueprint.md) | **开放议题** — Customer vs User 前端蓝图 |
| [`docs/G2_Client_Tenant_Model.md`](docs/G2_Client_Tenant_Model.md) | **开放议题** — Admin/Dealer/Client 租户 |
| [`docs/G2_GPS_Fusion_And_Tracking_Open_Issues.md`](docs/G2_GPS_Fusion_And_Tracking_Open_Issues.md) | **开放议题** — GPS 融合 / 提频 / HDOP |
| [`docs/G2_Smart_Backfill_Architecture.md`](docs/G2_Smart_Backfill_Architecture.md) | **架构预留** — 断网 Smart Backfill |
| [`docs/G2_SIM_Provisioning_Deadlock.md`](docs/G2_SIM_Provisioning_Deadlock.md) | **开放议题** — SIM 激活死锁 · 解法待决 |
| [`docs/008_Strategic_Guide.md`](docs/008_Strategic_Guide.md) | **008 首要参考** — 双轨并行战略（活文档） |
| [`docs/G2_Cloud_Deployment_Audit_2026-05-29.md`](docs/G2_Cloud_Deployment_Audit_2026-05-29.md) | 云端部署现状审查 + G2 开发计划 |
| [`docs/cloud_backend_log.md`](docs/cloud_backend_log.md) | 技术决策与变更日志 |

## 关联

- 固件写入管线：[`01-firmware/docs/AWS_Cloud_Pipeline.md`](../01-firmware/docs/AWS_Cloud_Pipeline.md)
- 后端 API：[`02-backend/`](../02-backend/) · [`G2 API 架构初稿`](../02-backend/docs/G2_API_Architecture_Draft.md)
- 契约 Schema：[`09-contract/`](../09-contract/)（M0.2 ✅）
