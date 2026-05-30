# G2 云端部署现状审查与开发计划

| 项 | 值 |
|----|-----|
| **审查日期** | 2026-05-29 |
| **审查方** | Agent 008（云原生与后端） |
| **AWS 账号** | `661631955220` |
| **IAM 身份** | `arn:aws:iam::661631955220:user/iqwatch` |
| **区域** | `us-east-1` |
| **IoT Endpoint** | `a3vcfgcj3um9l2-ats.iot.us-east-1.amazonaws.com` |
| **关联** | [`01-firmware/docs/AWS_Cloud_Pipeline.md`](../../01-firmware/docs/AWS_Cloud_Pipeline.md) · [`01-firmware/report/PreProduction_Audit_Report_2026-05-29.md`](../../01-firmware/report/PreProduction_Audit_Report_2026-05-29.md) |

---

## 1. 执行摘要

当前 AWS 云端为 **S90 遗留手工部署**，尚未迁移至 G2 IaC 架构：

| 维度 | 结论 |
|------|------|
| **IaC / CDK** | ⬜ 无 CloudFormation Stack；`04-cloud/` 此前为空 |
| **dev / prod 隔离** | ⚠️ 单账号混跑，未在 IaC 层体现 |
| **Energy 域（MPPT）** | ✅ 主路径健康，与 Agent 007 审计一致 |
| **Network 域（RUT）** | ⚠️ 部分运行，Timestream 存在已知缺口 |
| **Vision 域** | 🟡 独立管线，未纳入 G2 五域命名 |
| **Environment / Control** | ⬜ 未建 |
| **关键风险** | ~~🔴 `Route_Energy_To_Lambda` catch-all~~ → **已禁用**（2026-05-29，临时调试 Rule） |

**裁决**：Energy 写入/读出路径已达生产可用水位（Conditional GO）；G2 统一架构需从 Phase 0 脚手架开始系统性建设。

---

## 2. AWS 登录与验证状态

| 项 | 值 |
|----|-----|
| 账号 | `661631955220` |
| 身份 | `arn:aws:iam::661631955220:user/iqwatch` |
| 区域 | `us-east-1` |
| CLI 凭据 | ✅ `aws sts get-caller-identity` 通过 |
| 007 验证脚本 | ⚠️ 需 `01-firmware/.env`（根目录 `.env` 不自动被脚本读取） |

---

## 3. 五大领域映射（现状 vs G2 目标）

### 3.1 架构总览

```mermaid
flowchart TB
    subgraph energy["energy ✅ 运行中"]
        ESP[ESP32 MQTT device/status]
        R1[DeviceStatusToLambda<br/>SQL 2015-10-08 ✅]
        L1[SaveDeviceStatus]
        DDB1[(DeviceLatestStatus<br/>70 devices)]
        TS1[(IQWatchDB.DeviceStatus<br/>37 active / 7d)]
        API1[API GW 1y9689tax0<br/>/devices/.../status|history]
        ESP --> R1 --> L1 --> DDB1 & TS1
        DDB1 --> API1
        TS1 --> API1
    end

    subgraph network["network ⚠️ 部分运行"]
        RUT[RUT241 iot/rut241/status]
        R2[RUT_Master_Production_Rule]
        R3[RUT_Timestream_Ingestor_Rule]
        L2[RUTDevices*Processor]
        DDB2[(RUTDevicesLatestStatus<br/>RUTDevices_Attributes)]
        TS2[(RUTDevices_Metrics_DB)]
        RUT --> R2 & R3 --> L2 --> DDB2 & TS2
    end

    subgraph vision["vision 🟡 独立管线"]
        CAM[IQCameraController/Importer]
        API2[API GW mm8hc3ud9c]
        S3C[S3 iqcloud-camera-*]
        CAM --> API2 & S3C
    end

    subgraph g2router["G2 调试路由 — 已禁用"]
        ALL["Route_Energy_To_Lambda<br/>topic: # catch-all ⬜"]
        L3[IQTrailerDataRouter]
        TS3[(IQTrailerFleet.EnergyMetrics)]
        ALL --> L3 --> TS3
    end

    subgraph environment["environment ⬜ 未建"]
        ENV_PLACE[传感器/环境域 — 无独立表/API]
    end

    subgraph control["control ⬜ 未建"]
        CTRL_PLACE[继电器/POE XOR 推导 — 无云端实现]
    end
```

### 3.2 Energy 域（MPPT / IQEdge）— 健康 ✅

| 组件 | 状态 | 备注 |
|------|------|------|
| IoT Rule `DeviceStatusToLambda` | ✅ 启用 | SQL `2015-10-08` 已确认 |
| Lambda `SaveDeviceStatus` | ✅ | Python 3.12，128 MB，3 s timeout |
| DynamoDB `DeviceLatestStatus` | ✅ | 70 条记录 |
| Timestream `IQWatchDB.DeviceStatus` | ✅ | 近 7 天 37 台活跃 |
| API `1y9689tax0` | ✅ | GET `/devices`, `/{id}/status`, `/{id}/history` |
| OTA S3 `iqwatch-firmware-ota` | ✅ | Jobs OTA 已验证 v2.2.3.25 |
| HIL `HQ2513NH99U` | ✅ | TS 最新 `2026-05-29 01:46:12 UTC`，fw `v2.2.3.24`，SoC 85% |

**写入管线**（与 007 文档一致）：

```text
ESP32 MQTT Publish (topic: device/status)
    ↓
AWS IoT Rule: DeviceStatusToLambda  (SQL MUST be 2015-10-08)
    ↓
Lambda: SaveDeviceStatus
    ├─► DynamoDB  DeviceLatestStatus   (最新快照)
    └─► Timestream IQWatchDB.DeviceStatus   (时序分析)
```

**Egress 读路径**：

| 项 | 值 |
|----|-----|
| API ID | `1y9689tax0` |
| Base URL | `https://1y9689tax0.execute-api.us-east-1.amazonaws.com/v1` |
| 典型接口 | `GET /devices/{serial}/status`, `GET /devices/{serial}/history` |
| 认证 | Header `x-api-key`（仅主机工具，固件不调用） |

### 3.3 Network 域（RUT 4G）— 部分运行 ⚠️

| 组件 | 状态 |
|------|------|
| Rules `RUT_Master_Production_Rule`, `RUT_Timestream_Ingestor_Rule`, `RUTDevices_LWT_Rule` | ✅ 启用 |
| DDB `RUTDevicesLatestStatus`, `RUTDevices_Attributes` | ✅ |
| Timestream `RUTDevices_Metrics_DB.RUTDevices_Telemetry_Data` | ✅ |
| Lambda `RUTDevicesTimestreamProcessor`, `RUTDevicesDynamoDBProcessor`, `RUTGetDeviceListProcessor` | ✅ |
| 数据缺口 | ⚠️ **2026-03-07 ~ 2026-04-01**（SOP 红线，查询须标注） |
| POE 交换机 XOR 死机推导 | ⬜ **云端未实现** |

### 3.4 Vision 域 — 独立存在 🟡

| 组件 | 状态 |
|------|------|
| Lambda `IQCameraController`, `IQCameraImporter` | ✅ |
| API Gateway `mm8hc3ud9c` (IQCloudCameraControl) | ✅ |
| S3 `iqcloud-camera-imports`, `iqcloud-camera-snapshots` | ✅ |
| G2 五域统一 | ⬜ 未与 energy/network API 命名对齐 |

### 3.5 Environment / Control — 未建 ⬜

- **environment**：无独立 Timestream 表、IoT Rule 或 API。
- **control**：固件侧已有 `device/command` MQTT 订阅与 IoT Jobs OTA；云端无统一 control 域状态存储与 POE XOR 推导服务。

---

## 4. AWS 资源清单（2026-05-29 快照）

### 4.1 IoT Topic Rules

| Rule | Topic Pattern | 状态 | 目标 |
|------|---------------|------|------|
| `DeviceStatusToLambda` | `device/status` | ✅ 启用 | `SaveDeviceStatus` |
| `Route_Energy_To_Lambda` | `#` | ⬜ **已禁用**（2026-05-29） | `IQTrailerDataRouter` |
| `RUT_Master_Production_Rule` | `iot/rut241/status` | ✅ 启用 | RUT processors |
| `RUT_Timestream_Ingestor_Rule` | `iot/rut241/status` | ✅ 启用 | Timestream |
| `RUTDevices_LWT_Rule` | `iot/rut241/lwt` | ✅ 启用 | LWT 处理 |
| `SaveBatteryVoltage` | `device/status` | ⬜ 禁用 | 遗留 |
| `RUT_CatchAll_DB_Rule` | `#` | ⬜ 禁用 | 遗留 |
| `RUT_Global_Catch` | `#` | ⬜ 禁用 | 遗留 |
| `RegisterRouter` | `iot/rut241/status` | ⬜ 禁用 | 遗留 |

### 4.2 Lambda（IQ 相关）

| Function | Runtime | Last Modified |
|----------|---------|---------------|
| `SaveDeviceStatus` | python3.12 | 2026-05-18 |
| `IQTrailerDataRouter` | python3.10 | 2026-05-27 |
| `getDeviceStatusData` | python3.13 | 2025-05-24 |
| `getDeviceStatusHistory` | python3.9 | 2025-08-26 |
| `getDeviceStatusFromDDB` | python3.9 | 2025-07-18 |
| `getDeviceHistory` | python3.13 | 2026-03-05 |
| `getDeviceList` | python3.12 | 2026-03-07 |
| `RegisterDeviceInfo` | python3.13 | 2025-06-20 |
| `RUTDevicesTimestreamProcessor` | python3.9 | 2025-11-29 |
| `RUTDevicesDynamoDBProcessor` | python3.9 | 2026-03-07 |
| `RUTGetDeviceListProcessor` | python3.12 | 2026-03-07 |
| `IQCameraController` | python3.14 | 2026-03-07 |
| `IQCameraImporter` | python3.14 | 2025-12-24 |
| `IQWatch_Production_Verify` | python3.9 | 2026-03-08 |

### 4.3 DynamoDB

| Table | 用途 |
|-------|------|
| `DeviceLatestStatus` | MPPT 最新快照（70 条） |
| `DeviceStatus` | 遗留/辅助 |
| `RUTDevicesLatestStatus` | RUT 最新状态 |
| `RUTDevices_Attributes` | RUT 属性 |
| `IQDeviceRegistry` | 设备注册 |
| `IQDevice_Info` | 设备信息 |
| `IQWatch_Info` | 遗留命名 |
| `IQTrailerRealtimeStatus` | Trailer 实时 |
| `IQGuard_V3_Operational_Store` | 运维存储 |
| `IQ_Production_Hottrace` | 生产追踪 |

### 4.4 Timestream

| Database | Table | 状态 |
|----------|-------|------|
| `IQWatchDB` | `DeviceStatus` | ✅ ACTIVE，365d 保留 |
| `RUTDevices_Metrics_DB` | `RUTDevices_Telemetry_Data` | ✅ ACTIVE |
| `IQTrailerFleet` | `EnergyMetrics` | ✅ ACTIVE（G2 新表） |
| `mppt_data` | — | ⚠️ 0 表，疑似遗留空库 |

### 4.5 S3（IQ 相关）

| Bucket | 用途 |
|--------|------|
| `iqwatch-firmware-ota` | OTA 固件 |
| `iqwatch-mppt-data` | MPPT 数据 |
| `iqwatch-rut-data-661631955220` | RUT 数据 |
| `iqwatch-timestream-rejected-data-*` | Timestream 拒收 |
| `iqcloud-camera-imports` | 相机导入 |
| `iqcloud-camera-snapshots` | 相机快照 |
| `iqcloud-prod-use1-data-device` | 设备数据 |
| `iqedge-pwa-mobile-test-661631955220` | PWA 测试 |

### 4.6 IoT Things（抽样）

| Thing Name | 类型 |
|------------|------|
| `IQEdge_EC:E3:34:1A:F9:8C` | HIL ESP32 |
| `IQEdge_1C:69:20:B8:D7:F4` | 现场 OTA 验证 |
| `IQEdge_38:18:2B:83:E4:9C` | 生产单元 |
| `IQEdge_44:1D:64:E8:92:BC` | 生产单元 |
| `RUT241_71DC` | RUT 路由器 |

---

## 5. 关键风险项

### 🔴 P0 — 立即关注

| ID | 发现 | 影响 | 建议 |
|----|------|------|------|
| R0-01 | ~~`Route_Energy_To_Lambda` catch-all 已启用~~ | ~~双倍 Lambda 触发~~ | ✅ **已禁用**（2026-05-29，Bob：临时调试 Rule） |
| R0-02 | 无 IaC，无 dev/prod Stack | 变更不可审计、不可重复部署 | 启动 `04-cloud/cdk/` Phase 0 |

### 🟡 P1 — G2 迁移前须闭合

| ID | 发现 | 建议 |
|----|------|------|
| R1-01 | 命名混乱：`IQWatch_*` / `IQEdge_*` / `RUT_*` / `IQTrailer*` | 统一至五域前缀 |
| R1-02 | `mppt_data` Timestream DB 空库 | 标记 deprecated 或删除 |
| R1-03 | `AGENTS.md` 写 `03-cloud/`，仓库为 `04-cloud/` | 对齐文档 |
| R1-04 | POE XOR 死机推导无云端实现 | Phase 2 Network 域交付 |

### 🟢 P2 — 技术债

| ID | 发现 | 建议 |
|----|------|------|
| R2-01 | 多个 disabled catch-all Rules 残留 | CDK 迁移时清理 |
| R2-02 | API `1y9689tax0` 无 `/energy/` 前缀 | FastAPI 新 API 采用 G2 路径，旧 API 过渡期保留 |

---

## 6. G2 云端开发计划

### Phase 0 — 仓库脚手架（Week 1）

**目标**：将空目录变为可开发的 G2 骨架。

| 任务 | 产出 |
|------|------|
| 对齐目录命名 | 更新 `AGENTS.md`：`03-cloud` → `04-cloud` |
| 初始化 CDK | `04-cloud/cdk/` — `dev` / `prod` 双 Stack |
| 契约层 | `09-contract/schemas/{energy,network,vision,environment,control}/` |
| 后端骨架 | `02-backend/` FastAPI + 五域 router 前缀 |
| 运维日志 | `04-cloud/docs/cloud_backend_log.md` |

```text
04-cloud/
  cdk/
    lib/stacks/
      energy-stack.ts
      network-stack.ts
      shared-stack.ts
    bin/app.ts
  docs/
    G2_Cloud_Deployment_Audit_2026-05-29.md   ← 本文
    cloud_backend_log.md
```

### Phase 1 — Energy 域硬化（Week 2–3）

1. CDK 导入/重建 `DeviceStatusToLambda` + `SaveDeviceStatus` + DDB/Timestream
2. `09-contract` 锁定 Payload v2 flat 字段（对齐 `01-firmware/src/payload.md`）
3. FastAPI 等价读路径：`GET /energy/devices/{serial}/status|history`
4. dev 沙箱：独立 Timestream 表 + 测试 Thing
5. ~~下线/禁用 `Route_Energy_To_Lambda` catch-all~~ ✅ 已完成（2026-05-29）

### Phase 2 — Network 域（RUT + POE XOR）（Week 4–5）

1. CDK 化 RUT Rules + `RUTDevices*Processor`
2. FastAPI：`GET /network/routers/{sn}/status`
3. POE XOR 死机推导服务（RUT 在线 ⊕ POE 端口 ⊕ MPPT load 异常）
4. Timestream 查询层标注 2026-03-07~04-01 缺口

### Phase 3 — 统一 G2 数据路由器（Week 6）

```text
MQTT topic 规范（G2）:
  energy/{deviceId}/status      → SaveDeviceStatus
  network/{routerSn}/status     → RUTProcessor
  vision/{cameraId}/event       → CameraImporter
  environment/{sensorId}/read   → (Phase 4)
  control/{nodeId}/command      → (Phase 4)

Legacy 兼容（过渡期）:
  device/status                 → energy 域
  iot/rut241/status             → network 域
```

### Phase 4 — Vision / Environment / Control（Week 7+）

| 域 | 任务 |
|----|------|
| **vision** | `IQCamera*` 迁入 CDK + `/vision/` API |
| **environment** | 新建 Timestream + 传感器 MQTT 规则 |
| **control** | IoT Jobs 命令通道 + 继电器状态回写 |

### Phase 5 — 前端 + CI/CD（并行）

- `03-frontend/` PWA（[`README`](../../03-frontend/README.md) · 复用 S3 `iqedge-pwa-mobile-test-*`）
- GitHub Actions：`cdk deploy --context env=dev` → 测试 → prod
- API Key / IAM 按 dev/prod 分离

---

## 7. 近期行动项

| 优先级 | 动作 | 负责 |
|--------|------|------|
| ~~🔴 P0~~ ✅ | ~~禁用 `Route_Energy_To_Lambda`~~ | **已完成** 2026-05-29 |
| 🔴 P0 | 确认 Phase 0 脚手架（CDK + contract + FastAPI） | Bob |
| 🟡 P1 | 创建 `01-firmware/.env` 供 007 三重验证 | Bob |
| 🟡 P1 | 清理/标记 `mppt_data` 空库 | 008 |
| 🟢 P2 | API 逐步迁移至 `/energy/` 前缀 | 008 |

---

## 8. 审查命令参考（008 复现）

```powershell
aws sts get-caller-identity
aws iot get-topic-rule --rule-name DeviceStatusToLambda --region us-east-1
aws lambda list-functions --region us-east-1
aws dynamodb list-tables --region us-east-1
aws timestream-write list-databases --region us-east-1
aws apigateway get-rest-apis --region us-east-1

# Timestream 活跃设备（7 天）
aws timestream-query query --query-string "SELECT count(DISTINCT deviceId) FROM IQWatchDB.DeviceStatus WHERE time > ago(7d)" --region us-east-1
```

---

*文档生成：Agent 008 · IQEdge-G2 Cloud Baseline · 2026-05-29*
