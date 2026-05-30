# G2 系统身份模型（System Identity · sys_id）

> **性质**: Bob 定稿 · G2 资源主键宪法  
> **状态**: 活文档 — 2026-05-29 确立 · **ADR-004** `IQ-{YY}-{NNNNN}` 见 [`decisions/README.md`](../../decisions/README.md)  
> **适用**: API、Registry、MQTT Payload、Timestream 分区、Legacy 合流  
> **关联**: [`G2_API_Architecture_Draft.md`](G2_API_Architecture_Draft.md) · [`G2_Domain_Map.md`](../../04-cloud/docs/G2_Domain_Map.md)

---

## 1. 核心结论

**Fleet API 的主键不是 MPPT SER#，而是 `sys_id`（System ID）。**

| 概念 | 说明 |
|------|------|
| **sys_id** | 一台 **IQ 系统** 的全局唯一标识（对标 Tesla VIN） |
| **component_id** | 系统内单个物理组件 ID（MPPT SER#、Camera SN、RUT SN 等） |
| **system_type** | 产品形态枚举，决定默认组件清单与能力 |

Legacy 云端以 MPPT SER# 作 `deviceId` → 通过 Registry **`aliases`** 映射到 `sys_id`，**不改造老表**。

---

## 2. 四种 IQ 系统形态

| system_type | 产品 | 部署形态 | 说明 |
|-------------|------|----------|------|
| `iqwatch` | **IQWatch** | 自带 **9ft / 12ft 柱**，self-contained | 一体化杆式系统 |
| `solar_iqbox` | **Solar IQBox** | 安装于 **独立柱**（新建或既有灯杆等） | 安装商现场立柱 |
| `ac_iqbox` | **AC IQBox** | 同上，独立柱 | 市电场景 IQBox |
| `iqtrailer` | **IQTrailer** | **大型拖车** 平台 | 我方提供核心部件集成 |

**商业模式预留**：**IQWatch**、**IQTrailer** 可能采用 **设备租赁（lease/rental）**；`sys_id` 终身绑定物理资产，租约变更 **不换号**（见 §11）。

**SaaS 租户预留**：**Admin → Dealer → Client** 三级商业结构（Client = 最终付费方）；隐私隔离、RMR、布撤防自助、审计 — 见 [`G2_Client_Tenant_Model.md`](../../04-cloud/docs/G2_Client_Tenant_Model.md)（**方案待决**）。

## 3. 组件清单矩阵（按 system_type）

图例：**●** 标配 · **○** 可选 · **—** 不适用

### 3.1 总表

| 组件 / 域 | IQWatch | Solar IQBox | AC IQBox | IQTrailer |
|-----------|---------|-------------|----------|-----------|
| **vision** Camera ×2–3 | ● | ● | ● ×3–4 | ● ×3–4 |
| **control** IP Speaker (`ip_speaker`) | ○ | ○ | ● | ● |
| **control** Strobe Light (`strobe_light`) | ○ | ○ | ○ | ○ |
| **energy** MPPT | ● | ● | — | ● |
| **energy** Solar Panel | ● | ● | — | ● |
| **energy** Battery | ● | ● | — | ● |
| **energy** Cerbo GX | — | — | — | ● |
| **energy** Shunt | — | — | — | ● |
| **energy** Battery Protect | — | — | — | ● |
| **energy** Inverter | — | — | — | ● |
| **network** 4G Router (RUT241/956) | ○ | ○ | ○ | ● 4G/5G |
| **network** GPS（Router 内置） | ○ Roadmap | — | — | ○ Roadmap |
| **network** GPS Asset Tracker（独立备电） | ○ Roadmap | — | — | ○ Roadmap |
| **network** WiFi Bridge | ○ | ○ | ○ | ○ |
| **network** Starlink | — | — | — | ○ |
| **network** PoE Switch | ○ | ○ | ● | ● |
| **control** Relay | ○ | ○ | ○ | ● |
| **environment** 温湿度 / 风速 | ○ | ○ | ○ | ○ |
| **edge** IQEdge (ESP32) | ● | ● | ● | ● (多节点可能) |

> AC IQBox **无 MPPT/太阳能/电池** — energy 域数据来源 **遗留**（见 §10）。

### 3.2 五域与组件映射

```text
sys_id (IQ System)
 ├── energy      ← MPPT SER#, Cerbo, Shunt, Inverter …
 ├── network     ← RUT SN, Starlink, WiFi bridge, GPS 多源
 ├── vision      ← Camera IDs[]；WebRTC Talk-down **信令**（非 MQTT 音频）
 ├── environment ← Modbus 传感器 IDs[]
 └── control     ← Relay, **IP Speaker（预置播放）**, Strobe Light, DO/DI
```

**IP Speaker 纳管**: Registry `components.control[]` 中 `component_role: ip_speaker`（别名 `audio_horn`）；同一物理喇叭 **不** 重复挂在 vision 下，Talk-down 走 vision **API 信令**，播放走 control **命令**。

### 3.3 Energy 域数据来源（按 system_type）

| system_type | 能源网关 | 采集方式 | G2 现状 |
|-------------|----------|----------|---------|
| `iqwatch` | MPPT（Victron） | ESP32 **VE.Direct** UART | 007 已验证 |
| `solar_iqbox` | MPPT | VE.Direct | 同 IQWatch |
| `ac_iqbox` | **待定** | **遗留** | — |
| `iqtrailer` | **Cerbo GX**（唯一，Gateway） | **Modbus TCP** 读 Cerbo | Bob 定稿见 §3.4 |

API / Registry 的 energy **主读组件**（`energy_primary_component`）因产品线而异：Watch/Box 为 MPPT SER#；Trailer 为 **Cerbo**。

### 3.4 IQTrailer · Cerbo 能源架构（Bob 定稿）

**当前范围**：

- 每台 IQTrailer **仅 1× Cerbo GX**（不考虑多 Cerbo）。
- 是否挂 **多 MPPT** 由 **Cerbo 侧汇聚**；云端 **不逐 MPPT 聚合**，**以 Cerbo 为 energy 单一真相源（SSOT）**。
- 现场采集：**Modbus TCP → Cerbo**（非 VE.Direct 直读各 MPPT）。

```text
IQTrailer (sys_id)
  MPPT ×N ──► Cerbo GX (Modbus TCP Server / Gateway)
                    ▲
                    │ Modbus TCP 轮询（RUT 或集成主机）
              RUT241/956 ──► MQTT → AWS IoT
                    │
                    ├── Legacy: iot/rut241/status …
                    └── G2: iqedge/g2/{env}/energy/telemetry
                           component_id = Cerbo Venus serial · component_role = cerbo
```

实现与文档：**[`05-integration/`](../../05-integration/)**（`cerbo/` · `rut/`）。

**G2 影响**（ADR-012 · Bob 2026-05-30）：

| 层 | 策略 |
|----|------|
| Registry | Cerbo `role=cerbo` 主项 + MPPT `role=mppt` 子项（资产清单）；`component_id` = Venus uniqueId |
| MQTT Payload | `component_role=cerbo`；`firmware_version` **可选**（豁免 ADR-008 ESP32 门禁） |
| IoT | **RUT** 独立 Thing（SN）持 G2 Policy 发布 energy |
| EDGE-T2 | RutOS **Data to Server** + 自定义 JSON → G2 Topic |
| API `GET .../energy` | Cerbo 汇聚快照；**不做** 多 MPPT 云端聚合（MVP） |
| 007/边缘 | **IQWatch**: ESP32 VE.Direct。**IQTrailer**: Modbus + RUT（`05-integration/`） |

待定：`state` / `reporting_mode` → [`05-integration/docs/open_issues.md`](../../05-integration/docs/open_issues.md) OI-001。

> 将来若需 per-MPPT 明细，优先 **Cerbo 寄存器 / Venus OS 数据**，仍不必在云端拆 MPPT 聚合。

### 3.5 GPS 定位 · 硬件 Roadmap 预留（Bob · network 域）

**适用产品**：**IQWatch**、**IQTrailer**（租赁场景下资产找回尤为重要）。Solar / AC IQBox 暂不强制，Registry 可留空。

**两种形态（可并存，Registry 声明实际配置）**：

| component_role | 形态 | 典型来源 | 说明 |
|----------------|------|----------|------|
| **`gps_router`** | Router 内置 GPS | RUT241/956 等 4G Router | 与 router 同 lifecycle；随 `network/telemetry` 上报 |
| **`gps_asset_tracker`** | 独立备电 Asset Tracker | 第三方或自研 GPS 标签 | 可贴 Trailer 隐蔽位；弱依赖主系统供电 |

```text
sys_id
  network/components
    ├─ router_4g          (可选内置 GPS → gps_router)
    └─ gps_asset_tracker  (可选独立设备)
           │
           ▼
  network/telemetry 或 独立 ingest（Roadmap）
    lat, lon, alt, speed, fix_quality, gps_source
           │
           ▼
  API: GET .../network/location  （主读；多源策略见 Registry）
```

**G2 预留（MVP 不实现采集，Schema/API 先占位）**：

| 层 | 预留 |
|----|------|
| Registry | `components.network[]` 支持 `gps_router` / `gps_asset_tracker`；`location_primary_role` 指定主 GPS 源 |
| MQTT | `gps_source`: `router` \| `asset_tracker` \| `esp32`（Legacy）；与 `component_id` 成对 |
| Timestream `table_network` | measures: `lat`, `lon`, `alt_m`, `speed_kmh`, `fix_quality`, `gps_source` |
| API | `GET /api/v2/fleet/systems/{sys_id}/network/location` · `.../location/history` |
| 租赁 | 逾期/收回工单可订阅 **Asset Tracker 最后已知位置**（`lease_status` 联动，§11） |

**多 GPS 源策略（遗留 → 专文）**：Router vs Asset Tracker 融合、主备判决、提频 FSM、HDOP 虚警过滤 — **均为空白，方案待决**。详见 [`G2_GPS_Fusion_And_Tracking_Open_Issues.md`](../../04-cloud/docs/G2_GPS_Fusion_And_Tracking_Open_Issues.md)。

### 3.6 IP Speaker · 双场景与边缘自治（Bob 定稿）

#### 场景 1 · 预置语录（control 域）

| 项 | 说明 |
|----|------|
| 性质 | 纯 **异步命令**（Actuation） |
| 触发 | 接警员手动 **或** 边缘自治规则（见下） |
| API | `POST /api/v2/fleet/systems/{sys_id}/control/command` |
| Payload | `{"action":"play_audio","component_id":"SPEAKER-01","track_id":"warning_01","volume":90}` |
| 下行 | MQTT `iqedge/g2/{env}/control/command` 或 Device Shadow |
| 边缘 | **IQEdge X1** 收令 → 局域网 HTTP API / ONVIF → IP Speaker 播放 |

#### 场景 2 · 实时 Talk-down（vision 域 · 信令）

| 项 | 说明 |
|----|------|
| 性质 | **全双工实时音频** — 不可经 MQTT |
| API | `POST /api/v2/fleet/systems/{sys_id}/vision/webrtc/offer`（SDP Offer → Answer） |
| 媒体 | **WebRTC** P2P/打洞：前端麦克风 ↔ 现场 IP Speaker |
| 云端 | 仅信令协商 + 会话审计；**不传音频 payload** |

#### 边缘自治 · 防止「哑巴喇叭」（G2 铁律）

```text
❌ 错误（云端联动）:
   Camera AI → MQTT AWS → Lambda → MQTT → Speaker
   （链路过长；4G/AWS 断则入侵无响应）

✅ G2（边缘联动）:
   Camera AI (ONVIF/Webhook)
        → X1 本地行为树 / 微服务（局域网 <10ms）
        → IP Speaker play "You are trespassing!"
        → 异步打包 vision/event + control 结果 → MQTT 上报（事后大屏）
```

| 层 | 职责 |
|----|------|
| **X1 边缘** | 入侵 **实时** 响应；预置 `track_id` 本地映射 |
| **AWS** | 日志、审计、人工 Talk-down 信令、运营看板 |
| **007 / 边缘团队** | X1 容器内行为树；与 ESP32 固件路径分离 |

Registry 示例：

```json
"components": {
  "control": [
    { "component_id": "SPEAKER-01", "role": "ip_speaker", "lan_api": "http://192.168.1.50/onvif" },
    { "component_id": "STROBE-01", "role": "strobe_light" }
  ],
  "vision": [
    { "component_id": "CAM-001", "role": "camera", "channel": 1 }
  ]
}
```

---

### 3.7 Camera 物理孪生 · VQA 健康 telemetry（Bob 定稿）

**问题**: 野外塔台 **Truck Roll** 成本高；摄像头 Ping 正常但画面已黑/糊/偏，中台误报绿灯。

**原则**: **network 在线 ≠ vision 健康**；VQA 为 vision 域 **核心上行时序**，非可选附件。

#### 孪生字段（每 `component_id` = Camera）

| 字段 | 说明 |
|------|------|
| `focus_blur` | 对焦模糊度（分数或 0–1） |
| `video_blind` | 镜头遮挡 / 喷漆 / 全黑 |
| `scene_change` | 视场角偏移（风、人为掰动） |
| `vqa_health` | 综合：`healthy` \| `degraded` \| `failed` |
| `vqa_sample_at` | 最近 VQA 采样时间 |

#### MQTT Payload（`vision/telemetry`）

```json
{
  "sys_id": "IQ-26-06077",
  "component_id": "CAM-001",
  "component_role": "camera",
  "domain": "vision",
  "timestamp": "2026-05-29T12:00:00Z",
  "focus_blur": 0.12,
  "video_blind": false,
  "scene_change": false,
  "vqa_health": "healthy",
  "ping_ok": true
}
```

> `ping_ok` 可冗余携带便于关联分析；**权威在线状态仍在 network 域**。

#### 边缘 vs 云端

| 层 | 职责 |
|----|------|
| **X1 / NVR** | 周期跑 VQA 算法；上报 `vision/telemetry`；**断网 WAL + Smart Backfill**（见 [`G2_Smart_Backfill_Architecture.md`](../../04-cloud/docs/G2_Smart_Backfill_Architecture.md)） |
| **AWS** | 存储、车队健康率聚合、告警、运维看板 |
| **API** | `GET .../vision/health` · `GET .../fleet/vision/health-summary` |

Registry `components.vision[]` 扩展：

```json
{
  "component_id": "CAM-001",
  "role": "camera",
  "channel": 1,
  "vqa_enabled": true,
  "vqa_interval_min": 15
}
```

---

## 4. sys_id 发号规则（Bob 定稿 · ADR-004 / 方案 Y1）

### 4.1 格式（中性 · 不含产品形态）

```text
IQ-{YY}-{NNNNN}
```

| 段 | 规则 | 示例 |
|----|------|------|
| `IQ` | 固定前缀（IQEdge Fleet） | `IQ` |
| `YY` | **发号公历年**后两位；分配后 **终身不变** | `26` = 2026 |
| `NNNNN` | 该年内 **5 位顺序号**，左补零，从 `00001` 递增 | `00001`、`06001` |

**Canonical 示例**: `IQ-26-00001` · `IQ-26-10482` · `IQ-27-00001`（跨年重置序号）

**JSON Schema（新设备）**: `^IQ-[0-9]{2}-[0-9]{5}$`  
**归一化**: 输入 `iq-26-6001` → 存储 `IQ-26-06001`

> **`system_type`（iqwatch / solar_iqbox / ac_iqbox / iqtrailer）不在 sys_id 内** — 产测或部署时写入 Registry，改型可 PATCH，**不换号**。  
> 讨论过程 → [`G2_sys_id_Design_Proposal.md`](G2_sys_id_Design_Proposal.md)（已批准）。

### 4.2 与旧方案（IQW/IQB/IQT 前缀）的关系

| 项 | 规则 |
|----|------|
| **2026 起新设备** | 仅发 `IQ-YY-NNNNN` |
| **已存在 `IQW-*` / `IQB-*` / `IQT-*`** | **Grandfather** — 保留为合法 sys_id，不强制迁移 |
| **Registry** | 可选 `id_format: "iq_y1" \| "legacy_prefixed"`；`aliases.legacy_sys_id` 便于搜索 |
| **现场改型** | 例：原 Watch 改装为 Box → **同一 `IQ-26-01234`**，仅更新 `system_type` + `components[]` |

### 4.3 发号与容量

| 规则 | 说明 |
|------|------|
| **发号时点** | 产测首次写入 Registry；禁止现场手工跳号 |
| **中央原子递增** | 计数器 `sys_id_counter_{YY}`（如 DynamoDB） |
| **每年上限** | 单年 `NNNNN` ≈ **99,999** 台 |
| **跨年** | `IQ-27-00001` 新开池，不占用前一年未用号 |
| **Reserved** | `IQ-26-00000`、测试全 `9` 等不入生产库 |
| **dev/prod 云** | **共用** sys_id 池；云环境隔离靠 Topic/Timestream，不靠改号 |

```text
Provisioning:
  yy = current_year % 100
  n  = atomic_increment(f"sys_id_counter_{yy}")
  sys_id = f"IQ-{yy:02d}-{n:05d}"
```

### 4.4 Registry 约束（摘要）

| 规则 | 说明 |
|------|------|
| **全局唯一** | 全公司唯一；dev/prod **不**使用 `IQ-DEV-*` 独立池 |
| **Legacy 导入** | 已有现场单元保留原编号写入 Registry |
| **与云环境正交** | 同一 `IQ-26-00001` 可在 `qa_bench` 连 dev 云，部署后连 prod 云，**sys_id 不变** |

### 4.5 `system_type` 与 re-assign（ADR-006 / ADR-007）

| 阶段 | `system_type` |
|------|----------------|
| 零件 / 未组装 | `null` 或 `unassigned` |
| 工厂工单 | 默认值（可改） |
| **`deployed`** | **必须**已 assign（部署扫码确认） |
| 改型 / 重组 | Admin + 审计 + 组件清单校验后 PATCH |

```http
PATCH /api/v2/fleet/systems/{sys_id}/profile
{ "system_type": "solar_iqbox", "reason": "field_retrofit", "components": { ... } }
```

### 4.6 架构升级触发（将来时）

单年序号用尽（`99999`）或年化出货量持续 > 2,000 时，评估 **sys_id v2**（如 6 位序号）。**当前 Y1 足够。**

---

## 5. 部署生命周期（deployment_state）

Bob 定稿：**共用 `sys_id` 发号**，因为每台 IQWatch / IQBox / IQTrailer 必须有 **部署状态**，从零件到现场全链路可追踪。

### 5.1 状态枚举（初稿）

| deployment_state | 含义 | 典型 cloud 环境 | API / 遥测 |
|------------------|------|-----------------|------------|
| **`inventory_parts`** | 仓库内，零件/部件，未组装 | — | 无 MQTT |
| **`at_factory`** | 已发工厂，待组装 | — | 无 / 预留 |
| **`assembling`** | 工厂组装中 | 可选 dev | 局部产测 |
| **`qa_bench`** | HIL / 产测台架（如 007 沙箱） | **dev** | dev Topic 允许 |
| **`in_transit`** | 运输至安装商 / 现场 | — | 通常无 |
| **`deployed`** | **现场已部署、运行中** | **prod** | prod Topic |
| **`maintenance`** | 现场维护、停机检修 | prod | 可能限流 |
| **`decommissioned`** | 已退役 | — | 禁止 ingest |

```text
inventory_parts → at_factory → assembling → qa_bench → in_transit → deployed
                                                      ↘ maintenance ↗
                                                      → decommissioned
```

### 5.2 业务规则（008 建议）

| 规则 | 说明 |
|------|------|
| **发号时机** | 建议 **`inventory_parts` 或 `at_factory` 阶段** 即分配 `sys_id`（标签贴件），便于全生命周期追溯 |
| **cloud 切换** | `qa_bench` → `deployed` 时：更新 `deployment_state` + 切换 IoT Policy（dev Topic → prod Topic） |
| **API 过滤** | `GET /api/v2/fleet/systems?deployment_state=deployed` 供运营看板 |
| **ingest 门禁** | Ingest Lambda 可拒绝 `deployment_state=inventory_parts` 的 MQTT（防误测数据进 prod） |
| **Legacy 导入** | 已现场运行的老设备直接标 **`deployed`** |

### 5.3 Registry 扩展字段

```json
{
  "sys_id": "IQ-26-09041",
  "deployment_state": "qa_bench",
  "deployment_state_updated_at": "2026-05-29T12:00:00Z",
  "cloud_target": "dev",
  "site": null
}
```

现场部署后示例：

```json
{
  "deployment_state": "deployed",
  "cloud_target": "prod",
  "site": {
    "address_ref": "customer-site-uuid",
    "geo": { "lat": 33.45, "lon": -112.07 },
    "installed_at": "2026-06-01T08:00:00Z"
  }
}
```

---

## 6. Registry 数据模型

**表**: `iqedge-g2-{env}-table-registry`

```json
{
  "sys_id": "IQ-26-09041",
  "id_format": "iq_y1",
  "system_type": "iqwatch",
  "track": "g2",
  "deployment_state": "deployed",
  "cloud_target": "prod",
  "batch_id": "2026-W01",
  "site": {
    "pole_height_ft": 12,
    "install_type": "self_contained"
  },
  "domains_enabled": ["energy", "network", "vision", "control"],
  "commercial": {
    "ownership_model": "leased",
    "lease_status": "active",
    "lessee_ref": "customer-uuid",
    "lease_start": "2026-06-01",
    "lease_end": "2027-05-31"
  },
  "location_primary_role": "gps_asset_tracker",
  "components": {
    "energy": [
      { "component_id": "HQ2513NH99U", "role": "mppt", "thing_name": "IQEdge_EC:E3:34:1A:F9:8C" }
    ],
    "network": [
      { "component_id": "RUT241_71DC", "role": "router_4g", "capabilities": ["gps_router"] },
      { "component_id": "GPS-ASSET-0091", "role": "gps_asset_tracker", "capabilities": ["battery_backup"] }
    ],
    "vision": [
      { "component_id": "CAM-001", "role": "camera", "channel": 1 }
    ],
    "control": [
      { "component_id": "RELAY-1", "role": "relay" },
      { "component_id": "SPEAKER-01", "role": "ip_speaker" }
    ],
    "environment": []
  },
  "aliases": {
    "legacy_device_id": "HQ2513NH99U",
    "mppt_serial": "HQ2513NH99U",
    "legacy_sys_id": "IQW-9041"
  },
  "created_at": "2026-05-29T00:00:00Z"
}
```

### Legacy 合流规则

```text
API 请求 sys_id=IQ-26-09041
  → Registry.components + aliases
  → track=legacy 且 aliases.mppt_serial 存在
      → LegacyAdapter 用 mppt_serial 查 DeviceLatestStatus

API 请求（v1 兼容）device_id=HQ2513NH99U
  → GSI aliases 反查 sys_id
  → 同上
```

---

## 7. API 路径演进（sys_id 中心）

**v0.1 草案** 使用 `/fleet/devices/{device_id}` — **自本文起修正为**:

```text
/api/v2/fleet/systems                          # 系统列表
/api/v2/fleet/systems/{sys_id}                 # 系统元数据 + components
/api/v2/fleet/systems/{sys_id}/energy          # 域聚合读（多 MPPT 时合并策略待定义）
/api/v2/fleet/systems/{sys_id}/energy/components/{component_id}  # 单组件（可选）
```

| Tesla | G2 |
|-------|-----|
| `/vehicles/{vin}` | `/systems/{sys_id}` |
| VIN | sys_id |
| 单车多 ECU | 单系统多 component |

**v1 Legacy** 仍接受 MPPT SER#，内部反查 `sys_id` 或直接 Legacy adapter。

---

## 8. MQTT / Timestream 写入约定（G2 新轨）

Payload **必填字段**:

```json
{
  "sys_id": "IQ-26-09041",
  "component_id": "HQ2513NH99U",
  "component_role": "mppt",
  "domain": "energy",
  "timestamp": "2026-05-29T01:46:12Z",
  "...": "domain-specific measures"
}
```

| 存储 | 分区 / 键建议 |
|------|---------------|
| Timestream | `sys_id` 为 dimension；`component_id` 为第二 dimension |
| DDB Shadow | `PK=SYS#{sys_id}`, `SK=DOMAIN#{domain}#COMP#{component_id}` |

---

## 9. 对 POE XOR 推导的意义

Network 域 XOR 推导（008 Phase 2）在 **sys_id 粒度** 运算：

```text
router_online (network) XOR poe_switch_alive XOR mppt_load_anomaly
  → 归属 sys_id，而非单个 MPPT SER#
```

Trailer 场景：energy 异常信号来自 **Cerbo 汇聚读数**；XOR 输入中的 MPPT/load 语义以 Cerbo 输出为准，而非单个 VE.Direct MPPT。

---

---

## 11. 商业模式预留 · 设备租赁（Lease / Rental）

Bob：**IQWatch**、**IQTrailer** 可能采用 **租赁模式**。G2 设计原则：

| 原则 | 说明 |
|------|------|
| **sys_id 不变** | 物理资产身份终身唯一；退租、翻新、再租 **不换号** |
| **租约可变** | `commercial.*` 字段随合同更新；与 `deployment_state` 正交 |
| **与 deployment_state 协作** | 例：退租入库 → `deployment_state=inventory_parts` + `lease_status=returned` |

### 11.1 commercial 字段（Registry 预留）

| 字段 | 类型 | 说明 |
|------|------|------|
| `ownership_model` | `sold` \| `leased` \| `rental` | 售出 vs 租赁（`rental` 可与 `leased` 合并枚举，待法务） |
| `lease_status` | 见下表 | 租约生命周期 |
| `lessee_ref` | string | 承租方 ID（客户 / 项目 UUID） |
| `lease_start` / `lease_end` | ISO date | 合同期 |
| `lease_contract_ref` | string | 合同编号（可选） |

**lease_status 枚举（初稿）**：

| 值 | 含义 |
|----|------|
| `active` | 租约生效中 |
| `overdue` | 逾期未还 — 可触发 GPS 追踪加强 |
| `return_pending` | 已通知收回，待物理回收 |
| `returned` | 已入库，待检 |
| `available` | 可再次出租 |
| `written_off` | 报损 / 核销 |

### 11.2 API 预留

```text
GET  /api/v2/fleet/systems?ownership_model=leased
GET  /api/v2/fleet/systems?lease_status=overdue
PATCH /api/v2/fleet/systems/{sys_id}/commercial   # 运维/ERP 同步（权限遗留）
```

### 11.3 与 GPS Roadmap 联动

租赁 **逾期 / 收回** 场景：`lease_status=overdue` 时优先展示 **`gps_asset_tracker`** 最后位置（独立备电，主系统断电仍可能上报）。实现 Phase 2+。

---

## 12. 开放项与决策记录

### 已决

- [x] sys_id 发号：方案 A；IQW 9001+、IQB 1001+、IQT 6001+；溢出向下借千位块
- [x] dev/prod **共用序号池**；`deployment_state` + `cloud_target` 区分阶段与云环境
- [x] **IQTrailer energy**：单 Cerbo Gateway；Modbus TCP；多 MPPT 由 Cerbo 汇聚；云端以 Cerbo 为 SSOT（§3.4）

### 遗留（Deferred）

| 项 | 状态 | 备注 |
|----|------|------|
| AC IQBox 无 MPPT 时 energy 域数据来源 | **遗留** | 待 BOM / 硬件方案冻结 |
| Provisioning 工作流（扫码顺序 sys_id → components） | **遗留** | 与 SIM 激活死锁相关 → [`G2_SIM_Provisioning_Deadlock.md`](../../04-cloud/docs/G2_SIM_Provisioning_Deadlock.md) |
| `deployment_state` 转换权限 | **遗留** | 待运维角色 / 工具链定义 |
| 多 GPS 源融合 / 提频 / HDOP | **方案待决** | [`G2_GPS_Fusion_And_Tracking_Open_Issues.md`](../../04-cloud/docs/G2_GPS_Fusion_And_Tracking_Open_Issues.md) |
| `commercial` PATCH 权限与 ERP 对接 | **Roadmap** | 租赁模式上线前 |

### Roadmap 预留（已写入模型，MVP 不实现）

| 项 | 章节 |
|----|------|
| IQWatch / IQTrailer **租赁** `commercial.*` | §11 |
| **GPS Router** + **GPS Asset Tracker** 双形态 | §3.5 |
| **IP Speaker** control / vision WebRTC 双场景 + 边缘自治 | §3.6 · Domain Map |
| **Camera VQA** 健康 telemetry（focus_blur / video_blind / scene_change） | §3.7 · Domain Map |
| **Smart Backfill** 断网记忆补回 | [`G2_Smart_Backfill_Architecture.md`](../../04-cloud/docs/G2_Smart_Backfill_Architecture.md) |
| **GPS 融合 / 提频 / HDOP**（方案待决） | [`G2_GPS_Fusion_And_Tracking_Open_Issues.md`](../../04-cloud/docs/G2_GPS_Fusion_And_Tracking_Open_Issues.md) |
| **Client 租户层** Admin/Dealer/Client（方案待决） | [`G2_Client_Tenant_Model.md`](../../04-cloud/docs/G2_Client_Tenant_Model.md) |
| **Customer vs User** 前端双模块蓝图（方案待决） | [`G2_Customer_User_Frontend_Blueprint.md`](../../04-cloud/docs/G2_Customer_User_Frontend_Blueprint.md) |
| `GET .../network/location` | §3.5 · API 初稿 |

---

*2026-05-29 · Bob 定稿 · Agent 008 归档*
