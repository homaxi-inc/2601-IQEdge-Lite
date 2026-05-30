# G2 系统身份模型（System Identity · sys_id）

> **性质**: Bob 定稿 · G2 资源主键宪法  
> **状态**: 活文档 — 2026-05-29 确立  
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

## 3. 组件清单矩阵（按 system_type）

图例：**●** 标配 · **○** 可选 · **—** 不适用

### 3.1 总表

| 组件 / 域 | IQWatch | Solar IQBox | AC IQBox | IQTrailer |
|-----------|---------|-------------|----------|-----------|
| **vision** Camera ×2–3 | ● | ● | ● ×3–4 | ● ×3–4 |
| **vision** IP Speaker | — | — | ● | ● |
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
 ├── energy      ← MPPT SER#, Cerbo, Shunt, Inverter, Battery Protect …
 ├── network     ← RUT SN, Starlink, WiFi bridge, **GPS 多源**（Router / Asset Tracker）
 ├── vision      ← Camera IDs[], IP Speaker
 ├── environment ← Modbus 传感器 IDs[]
 └── control     ← Relay / DO / DI
```

**一个 sys_id 下可有多个 component_id**；Timestream / MQTT ingest 写入时 **必须携带两者**。

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
  MPPT ×N ──► Cerbo GX (Modbus 汇聚 / Gateway)
                    │
                    ├── Modbus TCP 读数（当前做法）
                    └──► MQTT iqedge/g2/.../energy/telemetry
                           component_id = Cerbo
                           component_role = cerbo_gx
```

**G2 影响**：

| 层 | 策略 |
|----|------|
| Registry | `components.energy` 主项为 Cerbo；MPPT 可为子组件引用或省略 |
| MQTT Payload | `component_id` = Cerbo ID；多 MPPT 明细在 payload 嵌套或省略 |
| API `GET .../energy` | 返回 Cerbo 汇聚后的系统级快照；**不做** 多 MPPT API 聚合逻辑（MVP） |
| 007/边缘 | 维持 Modbus TCP 读 Cerbo；与 Watch 的 VE.Direct 路径 **分轨实现** |

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

**多 GPS 源策略（遗留）**：Router GPS vs Asset Tracker 冲突时 **以 Registry `location_primary_role` 为准**；融合算法 Phase 2+。

---

## 4. sys_id 发号规则（Bob 定稿 · 方案 A）

### 4.1 格式

```text
{prefix}-{serial}
```

| system_type | prefix | 起始序号 | 示例 |
|-------------|--------|----------|------|
| `iqwatch` | **IQW** | **9001** | `IQW-9041` |
| `solar_iqbox` | **IQB** | **1001** | `IQB-1203` |
| `ac_iqbox` | **IQB** | **1001**（与 Solar 共用序号池） | `IQB-1456` |
| `iqtrailer` | **IQT** | **6001** | `IQT-6077` |

> Solar / AC IQBox **共用 `IQB-` 前缀**，靠 Registry 字段 `system_type` 区分形态。  
> `serial` 为 **4 位数字**，左补零可选（存储建议统一为整数比较，展示可 `IQW-9041`）。

### 4.2 序号池与溢出策略（Bob 原则）

**每产品线按「千位块」向下借用**，而非跳到 5 位或改 UUID：

| 产品线 | 第 1 块 | 第 2 块 | 第 3 块 | … |
|--------|---------|---------|---------|---|
| IQWatch | 9001–9999 | 8001–8999 | 7001–7999 | 直至 1001–1999 |
| IQBox | 1001–1999 | 2001–2999* | … | *仅当 1001–1999 用尽后启用下一千位块 |
| IQTrailer | 6001–6999 | 5001–5999 | … | 同上 |

**IQWatch 到 9999 怎么办？** → 继续 **8001**，再 **7001** …… Bob 的判断成立：

1. **短期足够**：9001–9999 单块 ≈ **999 台** IQWatch；加上 8001–8999 等，单线 **近万级** 才需架构升级。
2. **真到占满时**：出货量级已使 **Registry / API / 多租户** 必然演进 — 届时做 **sys_id v2**（如 5 位序号或带年份）是自然升级，而非现在过度设计。
3. **008 认同**：这与 Tesla 早期 VIN 够用、后期扩展的策略一致 — **先简单可读，瓶颈到了再版本化**。

### 4.3 容量粗算（心里有数）

| 块 | 每块约可发号 |
|----|-------------|
| x001–x999（千位块） | ~999 |
| IQWatch 可用块 9000→1000 | 9 块 × 999 ≈ **8,991** |
| IQTrailer 6000→5000→… | 同理，数千级 |

对特种 IoT 硬件公司，**8,000+ 同产品线** 已是极大成功；在此之前无需 UUID。

### 4.4 Registry 发号约束

| 规则 | 说明 |
|------|------|
| **中央发号** | Provisioning 工具原子递增；禁止现场手工跳号 |
| **全局唯一 · 共用序号池** | **`sys_id` 全公司唯一**；dev 云环境与 prod **共用发号**，不采用 `IQW-DEV-*` 独立前缀 |
| **状态区分身份** | 仓库零件 / 工厂待组装 / 现场部署等 → Registry 字段 **`deployment_state`**（见 §5），**不用** dev/prod 序号池区分 |
| **Legacy 导入** | 已有现场单元保留现有编号写入 Registry，不重新发号 |
| **Reserved** | 9000 / 6000 / 1000 等「整千」不入库，避免与块边界混淆 |

> **与 AWS `dev`/`prod` 云环境的区别**：云环境指 MQTT Topic `iqedge/g2/dev|prod/`、Timestream 库等 **基础设施隔离**；`sys_id` 是 **物理系统身份**，两者正交。同一 `IQW-9041` 在产测阶段可连 dev 云，部署后切 prod 云，**编号不变**。

### 4.5 架构升级触发条件（将来时）

当 **任一 prefix 全部千位块用尽**，或 **年化出货量 > 2,000** 时，启动 `sys_id v2` 评估（5 位序号、`IQW26-09041` 带年份等）。**当前不实现。**

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
  "sys_id": "IQW-9041",
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
  "sys_id": "IQW-9041",
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
      { "component_id": "RELAY-1", "role": "relay" }
    ],
    "environment": []
  },
  "aliases": {
    "legacy_device_id": "HQ2513NH99U",
    "mppt_serial": "HQ2513NH99U"
  },
  "created_at": "2026-05-29T00:00:00Z"
}
```

### Legacy 合流规则

```text
API 请求 sys_id=IQW-9041
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
  "sys_id": "IQW-9041",
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
| Provisioning 工作流（扫码顺序 sys_id → components） | **遗留** | 待产线 SOP |
| `deployment_state` 转换权限 | **遗留** | 待运维角色 / 工具链定义 |
| 多 GPS 源融合 / 冲突策略 | **Roadmap** | 先用 `location_primary_role` |
| `commercial` PATCH 权限与 ERP 对接 | **Roadmap** | 租赁模式上线前 |

### Roadmap 预留（已写入模型，MVP 不实现）

| 项 | 章节 |
|----|------|
| IQWatch / IQTrailer **租赁** `commercial.*` | §11 |
| **GPS Router** + **GPS Asset Tracker** 双形态 | §3.5 |
| `GET .../network/location` | §3.5 · API 初稿 |

---

*2026-05-29 · Bob 定稿 · Agent 008 归档*
