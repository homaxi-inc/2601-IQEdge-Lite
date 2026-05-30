# G2 智能记忆补回（Smart Backfill Architecture）

> **性质**: Agent 008 记录 · 断网生存 / 历史连续性  
> **状态**: 架构预留 — MVP 不实现；X1 边缘 + 云端 ingest 协同  
> **日期**: 2026-05-29  
> **关联**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) · [`G2_Domain_Map.md`](G2_Domain_Map.md) · [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md)

---

## 1. 问题陈述（现存盲区）

野外 **4G 随时可能中断数小时**。断网期间：

| 本地 | 云端 |
|------|------|
| NVR / X1 **SD 卡或硬盘** 仍录像、AI 仍抓拍 | MQTT 不可达 |
| 结构化 `vision.event` 存于 **边缘队列** | Timestream / DDB **无写入** |

网络恢复后若 **不做补回**：

- 运营大屏断网时段显示 **空白 = 误读为「平安无事」**
- 历史报表、租赁审计、告警统计 **失真**
- 与 **VQA telemetry**（§System Model 3.7）同理：**在线 ≠ 历史完整**

```text
❌ 仅依赖实时 MQTT 上行 → 断网 = 云端记忆空洞
✅ Smart Backfill → 恢复后批量追溯写入 → 长期记忆连续
```

---

## 2. 设计目标

| # | 目标 |
|---|------|
| B1 | 断网期间 **零丢失** 结构化事件（AI 元数据、报警日志；二进制仍本地/S3 可选后传） |
| B2 | 重连后 **自主补回**，无需人工 Truck Roll |
| B3 | Timestream 历史 **按事件原始时间戳** 连续；大屏/报表可区分「实时」与「补回」 |
| B4 | **幂等** — 重复补回不 duplicate 污染 |
| B5 | 与 **双轨 Legacy** 无关；仅 **G2 新轨** + X1 容器 |

---

## 3. 架构总览

```text
┌─────────────────────────────────────────────────────────────────┐
│  IQEdge X1（断网生存）                                            │
│    AI / NVR → 本地环形缓冲（SD / HDD）                              │
│    每条记录：event_time + payload + optional media_ref            │
└───────────────────────────┬─────────────────────────────────────┘
                            │ 4G 恢复
                            ▼
              ① 与云端对齐 high-water mark（最后已确认 event_time）
              ② 启动 Backfill Pipeline（批量、限速、可续传）
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│  AWS G2 Ingest                                                   │
│    MQTT iqedge/g2/{env}/vision/event  （单条或 batch 包装）        │
│    或 HTTPS POST .../vision/backfill  （大批量可选）              │
│    → fn-ingest-vision → Timestream（Version=event_time）          │
│    → DDB Shadow 更新（最新态；历史以 Timestream 为准）              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. 边缘（X1）职责

| 阶段 | 行为 |
|------|------|
| **断网检测** | `network` 域本地判定（RUT 无默认路由 / MQTT disconnect）→ 进入 `offline_buffering` |
| **缓冲写入** | 所有 `vision.event`、关键 `vision.telemetry`（VQA 采样）、control 执行结果 **写本地 WAL** |
| **存储介质** | SD 卡或硬盘；结构化 JSON **优先**（体积小于原始视频） |
| **重连** | MQTT session restore 或新连接 |
| **对齐** | 向云端查询或 Shadow 读取 `last_acked_event_time` per `sys_id` + `component_id` |
| **补回** | 从 WAL 中 `event_time > last_acked` 的记录 **按时间序批量上传** |
| **限速** | 避免重连瞬间打满 4G（如 50 records/s 上限，可配置） |
| **完成** | 收到云端 ACK / high-water 推进后 **修剪** 本地已确认 WAL |

**本地行为树 / 入侵响应** 不受断网影响（§System Model 3.6 边缘自治）；补回仅影响 **云端长期记忆**。

---

## 5. 云端（008）职责

### 5.1 Payload 扩展字段

```json
{
  "sys_id": "IQ-26-06077",
  "component_id": "CAM-001",
  "domain": "vision",
  "event_time": "2026-05-29T08:15:00Z",
  "ingest_mode": "backfill",
  "backfill_batch_id": "bf-20260529-001",
  "sequence": 42,
  "...": "event body"
}
```

| 字段 | 说明 |
|------|------|
| `event_time` | **业务时间**（Timestream record time） |
| `ingest_mode` | `live` \| `backfill` |
| `backfill_batch_id` | 同一补回作业 ID |
| `sequence` | 批内序号，便于续传 |

### 5.2 Ingest Lambda 规则

1. **Timestream WriteRecords** 使用 `event_time` 作为 record timestamp（非 `now()`）
2. **幂等键**: `hash(sys_id + component_id + event_time + event_type)` → DDB 去重表或 conditional write
3. **Metrics**: `BackfillRecordsReceived`, `BackfillLagSeconds` = now - event_time
4. **大屏**: 可选 UI 标记补回时段（虚线 / badge），**数据仍计入历史**

### 5.3 high-water mark 存储

**Registry 或 Shadow 扩展**:

```json
{
  "vision_sync": {
    "CAM-001": {
      "last_acked_event_time": "2026-05-29T14:00:00Z",
      "last_backfill_at": "2026-05-29T14:05:00Z"
    }
  }
}
```

X1 重连时 **GET** 或通过 MQTT retained topic 读取对齐点。

### 5.4 可选：HTTPS 批量端点（大量积压时）

```http
POST /api/v2/fleet/systems/{sys_id}/vision/backfill
Authorization: Bearer <device-cert-or-token>
Content-Type: application/json

{ "batch_id": "...", "records": [ ... ] }
```

MQTT 适合 steady-state；**小时级积压** 可走 S3 presigned PUT + Lambda 异步 ingest（Phase 2+）。

---

## 6. 域范围

| 域 | 补回优先级 | 说明 |
|----|------------|------|
| **vision** | **P0** | AI event、VQA telemetry、报警日志 — 本文重点 |
| **control** | P1 | 本地已执行的 `play_audio` 等审计日志 |
| **energy** | P2 | MPPT/Cerbo 时序 — VE.Direct 本地若有缓冲可补 |
| **network** | — | 连通性本身；断网期无上行预期 |
| **environment** | P2 | 传感器低频，可选 |

---

## 7. 与 VQA / 租赁 / 报表的关系

- **VQA 空洞** + 断网 → 恢复后补回 `vision/telemetry`，避免「健康率虚高」
- **租赁审计** → `event_time` 连续，纠纷可追溯
- **车队 health-summary** → 查询 Timestream 时应 **包含** `ingest_mode=backfill` 记录（同一 ANALYTICS 时间轴）

---

## 8. 非目标 / 风险

| 项 | 说明 |
|----|------|
| 断网期 **实时视频上云** | 默认不补传全量录像；仅结构化事件 + 可选 thumbnail |
| Legacy `device/status` | 老轨 **不改造**；G2 X1 新轨启用 Backfill |
| 100% 完美 | 本地 SD 物理损坏则仍丢失 — 需边缘 WAL 冗余策略（遗留） |
| MVP | **仅文档预留**；实现依赖 X1 容器 + fn-ingest-vision 改造 |

---

## 9. 待决（后续）

- [ ] WAL 格式与 SD 满盘策略（环形覆盖 vs 停录）
- [ ] 补回与 **Smart Backfill** 的 Stripe / 计费窗口是否相关
- [ ] S3 媒体文件异步补传 vs 仅元数据
- [ ] `last_acked_event_time` 权威源：Shadow vs 独立 DDB 表

---

*Agent 008 · Smart Backfill · 架构预留*
