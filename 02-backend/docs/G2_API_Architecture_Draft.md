# G2 API 架构初稿（API Architecture Draft）

> **作者**: Agent 008 · 参考 Tesla Fleet API 惯例 + G2 五域模型  
> **版本**: v0.2 · 2026-05-29（sys_id 系统模型）  
> **状态**: 初稿 — 待 Bob 评审  
> **代码归属**: [`02-backend/`](../../02-backend/)（应用层，与 `04-cloud/` IaC 分离）  
> **关联**: [`G2_System_Model.md`](G2_System_Model.md) · [`008_Strategic_Guide.md`](008_Strategic_Guide.md) · [`G2_Domain_Map.md`](G2_Domain_Map.md) · [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md)

---

## 1. 为什么 API 放在 `02-backend/` 而非 `04-cloud/`

| 原则 | 说明 |
|------|------|
| **关注点分离** | `04-cloud/` = IaC（IoT Rule、Timestream、Lambda ingest）；`02-backend/` = HTTP 应用（FastAPI、合流、鉴权） |
| **独立生命周期** | 基础设施周级变更；API 日级迭代。分开部署、分开回滚 |
| **Tesla 对标** | Fleet API 是 **产品接口**，不是 CloudFormation；G2 同理 |
| **双轨合流唯一入口** | Strategic Guide 规定合流在 backend；代码与文档同仓 |

```text
2601-IQEdge-Lite/
  02-backend/          ← API 实现 + API 架构文档（本文副本索引）
    docs/G2_API_Architecture_Draft.md   # 主文档（canonical）
    app/               ← (Phase 1) FastAPI
  04-cloud/            ← CDK only；README 链接至 02-backend API 文档
  09-contract/         ← OpenAPI / JSON Schema 契约（待建）
```

---

## 2. 设计目标（对标 Tesla Fleet API）

| Tesla 做法 | G2 采纳 |
|------------|---------|
| 官方唯一 Fleet API，Legacy Owner API 逐步下线 | **v2 Fleet API** 为 G2 官方；**v1** 只读兼容 Legacy，标记 deprecated |
| URL 版本 `/api/1/` | **`/api/v2/`** |
| 资源以 **Vehicle(VIN)** 为中心 | 资源以 **`sys_id`（IQ System）** 为中心 — 见 [`G2_System_Model.md`](G2_System_Model.md) |
| OAuth Bearer + Scope 限权 | MVP: API Key；prod: **Cognito JWT + Scope** |
| `GET /status` 健康检查 | **`GET /status`** → `"ok"` |
| 读（device_data）与写（vehicle_cmds）分离 | **GET 五域 telemetry** vs **POST commands** |
| 结构化错误码 | **`error` + `error_description` + HTTP 状态** |
| Fleet Telemetry 流式上报 | 设备走 **MQTT ingest**；API 只负责 **读/命令** |
| Command 需审计与协议分层 | **control** 域 + `table_control_logs` + IoT Jobs（OTA 边界另文档） |

---

## 3. API 总览

### 3.1 基址与版本

| 环境 | Base URL（示例） |
|------|------------------|
| dev | `https://api-dev.iqedge.example.com` |
| prod | `https://api.iqedge.example.com` |

| 版本 | 路径前缀 | 状态 |
|------|----------|------|
| **v2** | `/api/v2/` | G2 官方 Fleet API |
| v1 | `/v1/` | Legacy 兼容（代理 `1y9689tax0` 语义） |

### 3.2 资源层次（Tesla Fleet 风格 · sys_id 中心）

```text
Fleet
 └── systems/{sys_id}              ← 一台 IQ 系统（IQWatch / IQBox / IQTrailer）
      ├── (metadata + components)   ← system_type, MPPT/Camera/Router 清单
      ├── energy                    ← 域：读快照 / 历史（可含多 component）
      ├── network
      ├── vision
      ├── environment
      └── commands                  ← 域：下行控制
```

**主键决策（Bob 2026-05-29）**: **`sys_id` ≠ MPPT SER#**。MPPT、Camera、RUT 等是系统内 **`component_id`**，详见 [`G2_System_Model.md`](G2_System_Model.md)。

**Legacy 兼容**: v1 API 仍接受 MPPT SER# 作为 `device_id`；Registry `aliases.mppt_serial` 反查 `sys_id`。

---

## 4. 端点清单（v2 · MVP 加粗）

### 4.1 平台

| Method | Path | 说明 |
|--------|------|------|
| GET | **`/status`** | 健康检查，返回 `"ok"`（无需鉴权） |
| GET | `/api/v2/meta` | API 版本、环境、文档链接 |

### 4.2 Fleet · 系统目录

| Method | Path | 说明 |
|--------|------|------|
| GET | **`/api/v2/fleet/systems`** | 系统列表；可按 `system_type`、`deployment_state`、`track` 过滤 |
| GET | **`/api/v2/fleet/systems/{sys_id}`** | 系统元数据 + `system_type` + `components[]` + `track` |

**Query 参数（列表）**: `page`, `page_size`, `system_type`, `deployment_state`, `ownership_model`, `lease_status`, `track`, `online_since`

**system_type 枚举**: `iqwatch` · `solar_iqbox` · `ac_iqbox` · `iqtrailer`

### 4.3 Energy 域

| Method | Path | 说明 |
|--------|------|------|
| GET | **`/api/v2/fleet/systems/{sys_id}/energy`** | 域级 energy 聚合快照 |
| GET | **`/api/v2/fleet/systems/{sys_id}/energy/history`** | 时序历史 |
| GET | `/api/v2/fleet/systems/{sys_id}/energy/components/{component_id}` | 单 MPPT / Cerbo 组件读（可选） |

**Query（history）**: `start_time`, `end_time`, `measures`（逗号分隔）, `interval`

**响应要点**: `yield_kwh` 等必须使用 **G2 正确比例**（÷1000.0）；Legacy 设备由 adapter 做老算法转换，对外 schema 统一。

### 4.4 Network 域

| Method | Path | 说明 |
|--------|------|------|
| GET | `/api/v2/fleet/systems/{sys_id}/network` | 最新 RSSI / GPS / RUT 在线状态 |
| GET | `/api/v2/fleet/systems/{sys_id}/network/history` | 时序历史 |
| GET | `/api/v2/fleet/systems/{sys_id}/network/location` | **Roadmap** — 最新 GPS（Router / Asset Tracker，见 System Model §3.5） |
| GET | `/api/v2/fleet/systems/{sys_id}/network/location/history` | **Roadmap** — GPS 轨迹 |
| GET | `/api/v2/fleet/systems/{sys_id}/network/diagnostics` | POE XOR 推导（sys_id 粒度，Phase 2） |

### 4.5 Vision 域

| Method | Path | 说明 |
|--------|------|------|
| GET | `/api/v2/fleet/systems/{sys_id}/vision/events` | 事件列表（分页） |
| GET | `/api/v2/fleet/systems/{sys_id}/vision/stream/{event_id}` | 预签名 S3 URL |

### 4.6 Environment 域

| Method | Path | 说明 |
|--------|------|------|
| GET | `/api/v2/fleet/systems/{sys_id}/environment` | 最新传感器读数 |
| GET | `/api/v2/fleet/systems/{sys_id}/environment/history` | 时序历史 |

### 4.7 Control 域（Tesla `vehicle_cmds` 对标）

| Method | Path | 说明 |
|--------|------|------|
| POST | **`/api/v2/fleet/systems/{sys_id}/commands/{command}`** | 执行命令（见 §4.8） |
| GET | `/api/v2/fleet/systems/{sys_id}/commands/{command_id}` | 命令状态 / 审计 |

**command 示例**: `relay_on`, `relay_off`, `flash_led`, `request_telemetry`  
**非 command**: OTA 固件升级 → **IoT Jobs** 专用端点（Phase 3），不混入通用 command。

### 4.8 Legacy 兼容（v1 · deprecated）

| Method | Path | 说明 |
|--------|------|------|
| GET | `/v1/devices` | 代理 Legacy 设备列表 |
| GET | `/v1/devices/{device_id}/status` | 代理 Legacy；`device_id` = MPPT SER# |
| GET | `/v1/devices/{device_id}/history` | 代理 Legacy 历史 |

响应 Header: `Deprecation: true`, `Sunset: <date>`（Tesla Legacy 下线策略对标）

---

## 5. 双轨合流（Backend 核心）

```text
Request: GET /api/v2/fleet/systems/{sys_id}/energy
    │
    ▼
┌─────────────────┐
│ Auth Middleware │
└────────┬────────┘
         ▼
┌─────────────────┐
│ Registry Lookup │  by sys_id → components + aliases + track
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
 track=     track=
 legacy     g2
    │         │
    ▼         ▼
 Legacy     G2
 Adapter    Adapter
 (aliases.  (Shadow +
 mppt_serial Timestream
 → DDB)     by sys_id)
    │         │
    └────┬────┘
         ▼
   Unified EnergyResponse (09-contract schema)
```

| Adapter | 数据源 | 特殊处理 |
|---------|--------|----------|
| `LegacyEnergyAdapter` | `DeviceLatestStatus` GetItem | 老 Wh/kWh 算法 |
| `G2EnergyAdapter` | Shadow + Timestream | `/1000.0` 正确比例 |
| `LegacyNetworkAdapter` | RUT DDB / 老 Timestream | 标注 2026-03-07~04-01 缺口 |
| `G2NetworkAdapter` | G2 network 表 | — |

**客户端无感**：同一 URL、同一 JSON Schema；仅 `meta.track` 字段供运维/debug。

---

## 6. 鉴权与 Scope（Tesla 对标）

### 6.1 MVP（dev / 内部工具）

```http
GET /api/v2/fleet/systems/IQW-9041/energy
x-api-key: <IQWATCH_API_KEY>
Content-Type: application/json
```

兼容现有 `verify_telemetry.py` 密钥模式。

### 6.2 prod（目标态）

```http
Authorization: Bearer <jwt>
Content-Type: application/json
```

| Scope | 权限 |
|-------|------|
| `fleet:read` | 设备列表、元数据 |
| `energy:read` | energy GET |
| `network:read` | network GET |
| `vision:read` | vision GET + stream URL |
| `environment:read` | environment GET |
| `control:write` | POST commands |
| `offline_access` | refresh token（Tesla 同名惯例） |

---

## 7. 请求 / 响应约定（Tesla Conventions）

### 7.1 通用 Header

| Header | 要求 |
|--------|------|
| `Content-Type` | `application/json`（POST/PUT） |
| `Authorization` 或 `x-api-key` | 除 `/status` 外必填 |
| `X-Request-Id` | 客户端可选；服务端回显，便于追踪 |

### 7.2 分页

```json
{
  "data": [ ... ],
  "pagination": {
    "page": 1,
    "page_size": 50,
    "total": 70,
    "next": "/api/v2/fleet/systems?page=2"
  }
}
```

### 7.3 错误体（Tesla 风格 machine-readable）

```json
{
  "error": "system_not_found",
  "error_description": "sys_id IQW-9999 not in registry",
  "request_id": "abc-123"
}
```

| HTTP | error 示例 |
|------|------------|
| 400 | `invalid_parameter`, `invalid_time_range` |
| 401 | `unauthorized`, `invalid_api_key` |
| 403 | `insufficient_scope` |
| 404 | `system_not_found`, `component_not_found` |
| 429 | `rate_limit_exceeded` |
| 503 | `upstream_legacy_unavailable` |

### 7.4 Energy 快照响应（示例）

```json
{
  "sys_id": "IQW-9041",
  "system_type": "iqwatch",
  "domain": "energy",
  "meta": {
    "track": "g2",
    "last_reported": "2026-05-29T01:46:12Z",
    "data_stale": false
  },
  "components": [
    {
      "component_id": "HQ2513NH99U",
      "role": "mppt",
      "firmware_version": "v2.2.3.25",
      "battery": { "voltage_v": 13.35, "soc_pct": 85.0, "charge_state": "Float" },
      "solar": { "power_w": 0 },
      "load": { "power_w": 0 },
      "yield": { "total_kwh": 6.245, "today_kwh": 0.027 }
    }
  ]
}
```

Schema 权威来源 → `09-contract/schemas/energy/`（与 ingest Lambda 共用）。

---

## 8. `02-backend` 代码结构（Phase 1 目标）

```text
02-backend/
  README.md
  docs/
    G2_API_Architecture_Draft.md      # 本文（canonical）
  app/
    main.py                             # FastAPI app factory
    config.py                           # env, AWS region
    middleware/
      auth.py                           # API Key / JWT
      request_id.py
    router/
      status.py                         # GET /status
      v2_fleet.py                       # /api/v2/fleet/systems
      v2_energy.py                      # .../energy
      v2_network.py
      v2_vision.py
      v2_environment.py
      v2_control.py
      v1_legacy.py                      # deprecated shim
    adapters/
      registry.py                       # track routing
      legacy_energy.py
      g2_energy.py
      ...
    models/                             # Pydantic response models
    services/
      timestream.py
      dynamodb.py
      iot_control.py                    # publish command / Jobs
  tests/
  pyproject.toml
  Dockerfile                            # App Runner / ECS
```

**不在 02-backend 内**: CDK、IoT Rule 定义 → 留在 `04-cloud/`。

---

## 9. 部署与网关

| 阶段 | 方案 | 说明 |
|------|------|------|
| MVP | **AWS App Runner** 或 Lambda + HTTP API | 快启 FastAPI；CDK 在 `04-cloud` 只建域名 + 证书 |
| 规模期 | ECS Fargate + ALB | 长连接、vision stream 代理 |
| 文档 | OpenAPI 3.1 | 生成自 FastAPI，`09-contract/openapi/v2.yaml` 导出 |

`04-cloud/G2ApiStack`（可选）仅负责：API Gateway 自定义域名 → App Runner / ALB；**不含业务逻辑**。

---

## 10. 限流与计费（Tesla Billing 对标 · 路线图）

| 项 | MVP | 后续 |
|----|-----|------|
| Rate limit | 100 req/min per API Key | per-client tier |
| Timestream 扫描成本 | history 强制 `max 7d` 默认窗 | 按 scope 配额 |
| Command 配额 | 10 cmd/min/sys_id | 运维 tier |

---

## 11. MVP 交付范围

```text
✅ P0  GET /status
✅ P0  GET /api/v2/fleet/systems/{sys_id}
✅ P0  GET /api/v2/fleet/systems/{sys_id}/energy
✅ P0  GET /api/v2/fleet/systems/{sys_id}/energy/history
✅ P0  Registry 双轨 adapter（sys_id + aliases → legacy mppt）
✅ P0  v1 shim: /v1/devices/{mppt_serial}/status
⬜ P1  network / environment
⬜ P2  vision events + presigned URL
⬜ P3  POST commands + audit
```

---

## 12. 待 Bob 决策

- [ ] 公开 Base URL 域名（dev/prod）
- [ ] MVP 鉴权：仅 API Key vs 同步上 Cognito
- [x] ~~sys_id 发号~~ → IQW 9001+ / IQB 1001+ / IQT 6001+，溢出借 8001、7001…（见 System Model §4）
- [ ] v1 Sunset 目标日期
- [ ] Command 白名单首批（relay / OTA 边界）
- [ ] IQTrailer 多 MPPT 的 **API 明细展开**（若 Cerbo payload 不足时再议；MVP 不做云端聚合）

---

## 13. 文档索引

| 文档 | 路径 |
|------|------|
| 系统身份模型 | `02-backend/docs/G2_System_Model.md` |
| 本文（canonical） | `02-backend/docs/G2_API_Architecture_Draft.md` |
| 云端索引副本 | `04-cloud/docs/G2_API_Architecture_Draft.md` |
| 五域 Topic/表 | `04-cloud/docs/G2_Domain_Map.md` |
| 双轨战略 | `04-cloud/docs/008_Strategic_Guide.md` |
| OpenAPI 契约（待建） | `09-contract/openapi/v2.yaml` |

---

*Agent 008 · G2 Fleet API Baseline · v0.2*
