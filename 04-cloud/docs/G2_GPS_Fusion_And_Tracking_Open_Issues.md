# G2 GPS 融合、提频与定位质量 — 开放架构议题

> **性质**: Agent 008 记录 · Bob 野外实战痛点  
> **状态**: ⏸ **方案待决** — 今日仅归档盲区与约束  
> **日期**: 2026-05-29  
> **关联**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.5 · [`G2_Domain_Map.md`](G2_Domain_Map.md)

---

## 0. 背景

IQWatch / IQTrailer 可能配置 **双 GPS 源**（Roadmap 已预留）：

| component_role | 形态 | 供电 / 链路 |
|----------------|------|-------------|
| **`gps_router`** | RUT241/956 内置 GPS | 依赖 **拖车主电池 + 太阳能**；剪缆 → **瞬间断电断网** |
| **`gps_asset_tracker`** | 独立 Asset Tracker（底盘焊死） | **纽扣电池**，Cat-M1 / LoRa 弱网；偷车场景 **最后救命** |

当前文档仅有 Registry `location_primary_role` 占位，**下列三块融合/策略逻辑均为空白** — 本文记录，**方案后续再定**。

---

## 1. 议题 A · 双重 GPS 融合与主备判决（Fusion Logic）

### 1.1 野外痛点

专业偷车：**先剪太阳能 + 电池主线** → RUT 断电 → `gps_router` **变砖**。  
此时唯一希望：**`gps_asset_tracker`** 弱网续报。

### 1.2 架构缺失

未定义：

- 两源 **同时上报** 不同坐标时，Shadow / API **以谁为准**
- 一源 **死亡**、一源 **存活** 时的 **自动 failover**
- 两源 **皆无** 更新时的 `location_stale` 语义

### 1.3 待设计 · 融合矩阵（草案占位，未决）

| gps_router | gps_asset_tracker | 待决策略方向 |
|------------|-------------------|--------------|
| ✅ 新鲜 | ✅ 新鲜 | 融合 / 加权 / 选 HDOP 优者？ |
| ✅ 新鲜 | ❌ 超时 | **router 主** |
| ❌ 超时 | ✅ 新鲜 | **asset 主**（剪缆场景） |
| ❌ 超时 | ❌ 超时 | `location_unknown`；触发租赁 `overdue` + 工单？ |

**Registry 预留字段**（已部分存在，逻辑未实现）:

```json
{
  "location_primary_role": "gps_router",
  "location_failover_role": "gps_asset_tracker",
  "location_fusion_policy": "TBD"
}
```

**008 备注**: Shadow `PK=SYS#…` / network 域需 **`location_authoritative_source`** 字段；API `GET .../network/location` 应返回 **判决结果 + 各源原始读数**（便于审计）。

---

## 2. 议题 B · 高频追踪 vs 静默省流（Tracking Mode FSM）

### 2.1 野外痛点

| 模式 | 问题 |
|------|------|
| **固定 10s 上报** | 工地停放 3 个月 → **白烧 4G 套餐** |
| **固定 1h 上报** | 深夜被拖走 → **1h 延迟 = 进仓库拆解后才知** |

需要 **动态「点火提频」（High-Frequency Tracking State）** — 静止时低频，疑似移动时高频。

### 2.2 架构缺失

未定义：

- **Tracking Mode** 状态机（谁触发切换：边缘 / 云端 / 两者）
- 提频 **触发条件**（加速度？围栏？`scene_change`？人工？）
- 降频 **退出条件** 与 **最长提频时长**（防流量爆炸）
- Cat-M1 Asset Tracker 与 RUT **不同步提频** 时的策略

### 2.3 待设计 · 状态机占位（未决）

```text
STATIONARY_LOW     ── 默认：1h~6h / 次（可配置）
    │
    │ 触发：位移>阈值 / 围栏突破 / 振动 / lease_overdue
    ▼
HIGH_FREQ_TRACK    ── 10s~30s / 次，限时 T_max
    │
    │ 触发：静止确认 N 分钟 / 人工解除
    ▼
STATIONARY_LOW
```

**可能归属**:

| 层 | 职责 |
|----|------|
| **边缘 RUT / X1** | 检测移动，本地提频（减云端往返） |
| **云端** | 下发 `tracking_mode` 命令（control/network 边界 **待决**） |
| **Registry** | `tracking_mode_default` +  per-lease 覆盖 |

---

## 3. 议题 C · 定位质量信任度（HDOP / Fix Quality）

### 3.1 野外痛点

云层、高压线、楼群 → GPS **几十米漂移** → 若 **只看坐标变化就报警** → 接警中心 **虚警轰炸**。

### 3.2 架构缺失

模型未强制：

- **`hdop`** / **`fix_type`** / **`satellites`** 纳入 ingest 与告警 **卡口**
- 位移告警 = **位移 Δ + 质量门控**（低质量时 **抑制** 偷车告警）
- 大屏打点 **质量分级**（灰点 = 低信任）

### 3.3 待设计 · 字段与规则占位（未决）

**MQTT `network/telemetry` 扩展**:

```json
{
  "sys_id": "IQ-26-06077",
  "component_id": "RUT241_71DC",
  "component_role": "gps_router",
  "lat": 33.45,
  "lon": -112.07,
  "hdop": 1.2,
  "fix_type": "3D",
  "satellites": 9,
  "gps_quality": "TBD_enum"
}
```

| 规则方向（未决） | 说明 |
|------------------|------|
| `hdop > H_max` | 丢弃或降权，不触发位移告警 |
| `fix_type == none` | 不更新 Shadow 坐标 |
| 连续 N 次高质量位移 | 才触发 `theft_suspected` |

**与议题 B 联动**: 低质量漂移 **不得** 误触发 HIGH_FREQ_TRACK。

---

## 4. 三议题关系

```text
         ┌─────────────────────────────────────┐
         │  C · HDOP / Fix 质量门控           │
         │  （虚警过滤 — 所有坐标入口）         │
         └─────────────────┬───────────────────┘
                           ▼
         ┌─────────────────────────────────────┐
         │  A · 双源融合 / 主备判决             │
         │  （Shadow 权威坐标 + failover）       │
         └─────────────────┬───────────────────┘
                           ▼
         ┌─────────────────────────────────────┐
         │  B · Tracking Mode FSM              │
         │  （省流 ↔ 提频追车）                  │
         └─────────────────────────────────────┘
```

---

## 5. G2 预留（不改 MVP 实现）

| 层 | 预留 |
|----|------|
| Registry | `location_fusion_policy`, `tracking_mode_*`, `gps_quality_thresholds` |
| Timestream `table_network` | `hdop`, `fix_type`, `satellites`, `tracking_mode`, `gps_source` |
| API | `GET .../network/location` 返回 `authoritative_source` + `sources[]` |
| 告警 | `theft_suspected` 需 **质量 + 融合 + 模式** 三关（Roadmap） |

---

## 6. 待 Bob / 008 后续决策

- [ ] **A** 融合策略：`router优先` vs `HDOP优先` vs `asset failover only`
- [ ] **B** 提频触发：边缘自治 vs 云端下发；与 **租赁 overdue** 联动
- [ ] **C** HDOP 阈值：按产品 / 地区标定
- [ ] Asset Tracker **Cat-M1** ingest 路径：独立 Topic vs 经 X1 汇聚
- [ ] 与 **Smart Backfill** 断网补回：GPS 轨迹是否补写（见 Smart Backfill 专文）

---

*Agent 008 · GPS Open Issues · 方案后续再定*
