# common/ — G2 跨域公共 Schema

| 文件 | 用途 |
|------|------|
| [`g2-envelope.v1.json`](g2-envelope.v1.json) | 所有 `*/telemetry` 上行消息的公共信封（`sys_id`、`ingest_mode` 等） |

各域 Schema 通过 JSON Schema `allOf` + `$ref` 引用本信封。
