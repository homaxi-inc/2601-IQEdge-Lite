# schemas/ — JSON Schema 仓库

## 放置规则

1. 每个域一个子目录（**仅五域 + registry**）。
2. 每个 **message-type** 一个 Schema 文件；多版本并存时用 `v1`、`v2` 后缀。
3. 必须包含 `$schema`、`$id`、`title`、`type`、`properties`；域内对象默认 `additionalProperties: false`，信封层允许扩展（见 `common/g2-envelope.v1.json`）。
4. 公共字段 → [`common/g2-envelope.v1.json`](common/g2-envelope.v1.json)（**M0.3 ✅**）

## 占位状态 (M0.2)

| 目录 | 状态 | 下一任务 |
|------|------|----------|
| `energy/` | ✅ `telemetry.v1.json` | M4 ingest |
| `network/` | 占位 | P2 |
| `vision/` | 占位 | P4 |
| `environment/` | 占位 | P5 |
| `control/` | 占位 | P3 |
| `registry/` | 占位 | M3.2 |

## 样例目录（可选）

`examples/{domain}/*.json` 可与 Schema 同 PR 提交，供 ajv 与单元测试使用。
