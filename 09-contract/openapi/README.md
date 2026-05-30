# openapi/ — OpenAPI 导出约定 (M0.4)

| 路径 | 说明 |
|------|------|
| `v2/openapi.yaml` | **生成物** — 从 `02-backend` FastAPI 导出，勿手改 |
| `v2/.gitkeep` | 占位至 FastAPI 就绪 |

导出命令（M0.4 实现后）:

```powershell
# 示例 — 待 02-backend/app 就绪
# curl http://localhost:8000/openapi.json > 09-contract/openapi/v2/openapi.json
```

Canonical API 文档: [`02-backend/docs/G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)
