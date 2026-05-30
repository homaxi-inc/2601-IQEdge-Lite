# G2 五大业务领域模型（Domain Map）

> **性质**: Bob 定稿 · G2 顶层命名宪法  
> **状态**: 活文档 — 后续逐步补充  
> **适用**: 新轨 G2  only（老架构 `device/status`、`IQWatchDB` 等 **禁止改动**）  
> **关联**: [`008_Strategic_Guide.md`](008_Strategic_Guide.md) · [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md) · [`02-backend/docs/G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)

---

## 建模原则

物理世界所有外设归入 **五个顶级领域**。Topic、API、Timestream/DynamoDB 表名 **必须是这五词的排列组合**，禁止引入混乱旧命名。

| 定名 | 弃用别名 | 选定 |
|------|----------|------|
| 视觉域 | media | **vision** |
| 环境域 | telemetry（作域名） | **environment** |
| 控制域 | actuator | **control** |

---

## 五域定义

| 域 | 本质 | 物理对象 | 数据特征 |
|----|------|----------|----------|
| **energy** | 能量流 | Solar、Battery、MPPT、逆变器、Load | V/I/P、Wh/kWh；发电/储电/用电 |
| **network** | 信息流 | RUT241/956、ESP32 RSSI、SIM 流量、GPS | 在线、连通、运维健康 |
| **vision** | 视频/图像/媒体信令 + **VQA 健康 telemetry** | Camera、流媒体、SD 录像、抓拍、WebRTC 信令、**VQA 质量指标** | 视觉事件 + 流协商 + **focus_blur / video_blind / scene_change** 时序 |
| **environment** | 环境状态流 | 温湿度、风速风向、Lux、气压（Modbus 传感器） | 只读生态气象指标 |
| **control** | 反向控制流 | **Relay**、DO/DI、**IP Speaker（预置音频播放）**、**Strobe Light（爆闪灯）**、警铃、补光灯 | **异步命令执行** — 开/关/播放/触发 |

```text
energy        ──► 发电 / 储电 / 用电
network       ──► 在线 / 不失联 / 流量可控
vision        ──► 录像 / 抓拍 / AI 事件 / WebRTC 信令 / VQA 健康 telemetry
environment   ──► 大自然只读指标
control       ──► 异步执行：Relay / 预置语音 / 爆闪灯 …
```

---

## 命名宪法看板（dev 示例）

> `{env}` = `dev` | `prod` — 全局替换即可。

| 域 | IoT Topic | API 路由 (v2 · Fleet) | Timestream 表 |
|----|-----------|----------------------|---------------|
| **energy** | `iqedge/g2/dev/energy/telemetry` | `GET .../systems/{sys_id}/energy` | `iqedge_g2_dev_table_energy` |
| **network** | `iqedge/g2/dev/network/telemetry` | `GET .../systems/{sys_id}/network` | `iqedge_g2_dev_table_network` |
| **vision** | `iqedge/g2/dev/vision/event` · `.../vision/telemetry` | `GET .../vision/events` · `GET .../vision/health` · `POST .../vision/webrtc/offer` | `iqedge_g2_dev_table_vision` |
| **environment** | `iqedge/g2/dev/environment/telemetry` | `GET .../systems/{sys_id}/environment` | `iqedge_g2_dev_table_environment` |
| **control** | `iqedge/g2/dev/control/command` | `POST .../systems/{sys_id}/control/command` | `iqedge_g2_dev_table_control_logs` |

### 共享基础设施命名

| 组件 | dev 命名 |
|------|----------|
| Timestream Database | `iqedge_g2_dev_database` |
| DynamoDB 状态影子表 | `iqedge-g2-dev-table-shadow` |
| IoT Rule（示例） | `iqedge-g2-dev-rule-{domain}` |
| IAM Role | `iqedge-g2-dev-role-iot-ingest` |

### Topic 末段约定

| 末段 | 方向 | 用于 |
|------|------|------|
| `telemetry` | 上行 | energy / network / environment 时序数据 |
| `telemetry` | 上行 | vision **VQA 健康率** 周期指标（与 AI `event` 分离） |
| `event` | 上行 | vision AI/告警事件（含边缘自治联动结果摘要） |
| `command` | 下行 | control 异步指令（含 `play_audio` 等） |

---

## IP Speaker · 双场景域划分（Bob 定稿）

同一物理 **IP Speaker** 在 Registry 中为 **`component_role: ip_speaker`**（或 `audio_horn`），能力拆分到两域：

| 场景 | 域 | 机制 | API |
|------|-----|------|-----|
| **预置语录**（自动/手动报警语音） | **control** | 异步 JSON 命令 → MQTT `control/command` → X1 局域网 HTTP/ONVIF 触发播放 | `POST .../control/command` |
| **实时 Talk-down** | **vision** | **WebRTC** 信令协商；音频 **不走 MQTT** | `POST .../vision/webrtc/offer` |

**control 命令示例**:

```json
{
  "action": "play_audio",
  "component_id": "SPEAKER-01",
  "track_id": "warning_01",
  "volume": 90
}
```

**vision WebRTC**: 前端 SDP Offer → 边缘 X1 Answer → P2P/打洞音频流至 IP Speaker。

**边缘自治（G2 铁律）**: Camera AI 触发预置语音 **必须在 X1 本地行为树完成**（局域网直控 Speaker）；云端仅 **异步上报** vision event + control 执行结果日志。**禁止** Camera→AWS→Lambda→Speaker 长链路作为入侵实时响应。

---

## Vision · VQA 健康率 telemetry（Bob 定稿 · 车队级运维）

**现存盲区**: 仅看 **心跳 Ping** 会误判健康（绿灯）— 镜头黑屏、遮挡、失焦、视场偏移时设备仍“在线”，导致 **Truck Roll**（派人工现场）运维成本飙升。

**G2 要求**: **VQA（Video Quality Assessment）** 升级为 vision 域 **核心上行时序指标**，与 `network` 连通性 **解耦**。

### Camera 物理孪生 · VQA 度量（Timestream / MQTT 必填）

| 字段 | 类型 | 含义 | 典型触发 |
|------|------|------|----------|
| **`focus_blur`** | float 0–1 或 score | 对焦模糊度 | 振动、失焦、固件 AF 漂移 |
| **`video_blind`** | bool 或 severity 0–1 | 镜头遮挡 / 喷漆 / 全黑 | 蜘蛛网、鸟粪、泥沙、恶意遮挡 |
| **`scene_change`** | bool 或 delta score | 视场角偏移 / 场景剧变 | 大风吹歪桅杆、人为掰动镜头 |

**派生（云端或边缘）**:

| 字段 | 说明 |
|------|------|
| `vqa_health` | `healthy` \| `degraded` \| `failed` — 综合三指标 |
| `ping_ok` | network 域；**不得** 替代 `vqa_health` |

### 上行路径

```text
X1 / NVR 边缘 VQA 引擎（周期，如 5–15 min）
    → MQTT iqedge/g2/{env}/vision/telemetry
        { sys_id, component_id, focus_blur, video_blind, scene_change, ... }
    → Timestream table_vision + DDB Shadow
    → 车队看板：按 sys_id 聚合「视觉健康率」≠「在线率」
```

### 车队级 API（Fleet Health）

```text
GET /api/v2/fleet/systems/{sys_id}/vision/health          # 单系统最新 VQA
GET /api/v2/fleet/systems/{sys_id}/vision/health/history  # 时序
GET /api/v2/fleet/vision/health-summary                     # 车队：healthy / degraded / failed 计数
```

**告警策略（Roadmap）**: `vqa_health=failed` 且 `deployment_state=deployed` → 工单 / 减少 Truck Roll 盲派。

**断网记忆（Roadmap）**: 见 [`G2_Smart_Backfill_Architecture.md`](G2_Smart_Backfill_Architecture.md)。

**GPS 三议题（方案待决 ⏸）**: ① 双源融合/主备 ② 提频/省流动态切换 ③ HDOP 虚警过滤 — [`G2_GPS_Fusion_And_Tracking_Open_Issues.md`](G2_GPS_Fusion_And_Tracking_Open_Issues.md)。

---

## 008 / 007 分工

| 层 | 负责人 | 要求 |
|----|--------|------|
| IoT Topic（固件 publish） | **007** | 按域发布至上表 Topic |
| IoT Rule + Lambda + DB | **008** | CDK 按域建 Rule/表，一域一表 |
| API 路由 | **008** | `02-backend` 前缀 `/api/v2/{domain}/…` |
| Legacy 合流 | **008** | 老设备仍走 Legacy API；G2 设备走 v2 域路由 |

---

## 与 Legacy 对照（勿混用）

| G2 域 | 老架构近似物 | 关系 |
|-------|-------------|------|
| energy | `device/status` → `DeviceLatestStatus` / `IQWatchDB` | 老轨锁定；G2 MPPT 进 `table_energy` |
| network | `iot/rut241/status` → RUT DDB/Timestream | 老轨锁定；G2 进 `table_network` |
| vision | `IQCamera*` / `mm8hc3ud9c` API | 待 G2 迁入 `table_vision` |
| environment | — | 新域 |
| control | `device/command` / IoT Jobs OTA | 下行：`control/command`；**OTA 仍走 Jobs**（与 `play_audio` 分离） |

---

## 待 Bob 后续补充

- [x] ~~vision `event` vs `stream`~~ → event 上行 + WebRTC offer 信令；实时流不走 MQTT
- [ ] control 域 OTA / Jobs 与 `play_audio` / `strobe` 命令白名单
- [ ] WebRTC TURN/STUN 部署（Talk-down Phase 2）
- [ ] VQA 阈值标定（`focus_blur` / `scene_change` 告警线 · 按 Camera 型号）

---

*2026-05-29 · Bob 定稿 · Agent 008 归档*
