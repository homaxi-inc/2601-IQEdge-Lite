# 04-cloud — G2 云端基础设施

Agent 008 主战场：AWS CDK (IaC)、Timestream、IoT Rules、Lambda、多环境 (dev/prod) 部署。

## 目录结构

```text
04-cloud/
  README.md           ← 本文件
  docs/               ← 审查报告、开发日志
  cdk/                ← (Phase 0) AWS CDK 项目
```

## 文档

| 文件 | 说明 |
|------|------|
| [`docs/G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) | **系统身份** — sys_id · 四种 IQ 形态 · 组件矩阵 |
| [`docs/G2_System_Model.md`](docs/G2_System_Model.md) | **系统身份** — 索引 → `02-backend/docs/` |
| [`docs/G2_API_Architecture_Draft.md`](docs/G2_API_Architecture_Draft.md) | **API 架构** — 索引 → `02-backend/docs/` |
| [`docs/G2_Cloud_Architecture_Design.md`](docs/G2_Cloud_Architecture_Design.md) | **云架构设计** — CDK 分层、数据流、存储、安全 |
| [`docs/G2_Domain_Map.md`](docs/G2_Domain_Map.md) | **五域命名宪法** — Topic / API / 表名矩阵 |
| [`docs/008_Strategic_Guide.md`](docs/008_Strategic_Guide.md) | **008 首要参考** — 双轨并行战略（活文档） |
| [`docs/G2_Cloud_Deployment_Audit_2026-05-29.md`](docs/G2_Cloud_Deployment_Audit_2026-05-29.md) | 云端部署现状审查 + G2 开发计划 |
| [`docs/cloud_backend_log.md`](docs/cloud_backend_log.md) | 技术决策与变更日志 |

## 关联

- 固件写入管线：[`01-firmware/docs/AWS_Cloud_Pipeline.md`](../01-firmware/docs/AWS_Cloud_Pipeline.md)
- 后端 API：[`02-backend/`](../02-backend/) · [`G2 API 架构初稿`](../02-backend/docs/G2_API_Architecture_Draft.md)
- 契约 Schema：[`09-contract/`](../09-contract/)（待建）
