# 02-backend — G2 Fleet API

G2 官方 HTTP API（FastAPI），负责 **Legacy + G2 双轨合流**、鉴权与对外 REST 接口。

> Tesla Fleet API 风格：**Fleet → Device → Domain** 资源模型，版本 `/api/v2/`。

## 文档

| 文件 | 说明 |
|------|------|
| [`docs/G2_System_Model.md`](docs/G2_System_Model.md) | **sys_id 系统模型** — 四种 IQ 形态 · 组件矩阵 |
| [`docs/G2_API_Architecture_Draft.md`](docs/G2_API_Architecture_Draft.md) | **API 架构初稿 v0.2**（canonical） |

## 关联

| 目录 | 职责 |
|------|------|
| [`04-cloud/`](../04-cloud/) | IaC：IoT ingest、Timestream、Registry 表 |
| [`09-contract/`](../09-contract/) | OpenAPI / JSON Schema（待建） |
| [`04-cloud/docs/008_Strategic_Guide.md`](../04-cloud/docs/008_Strategic_Guide.md) | 双轨并行战略 |

## 状态

`app/` 代码待 Phase 1 实现。MVP 端点见 API 架构文档 §11。
