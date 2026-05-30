# Energy Telemetry v1 — 007 固件对齐要求 (G2)

> **契约**: [`telemetry.v1.json`](telemetry.v1.json)  
> **MQTT Topic**: `iqedge/g2/{env}/energy/telemetry`（**非** Legacy `device/status`）  
> **sys_id 规则**: **ADR-004** — [`decisions/README.md`](../../../decisions/README.md)  
> **track / 版本门禁**: **ADR-008** — [`04-cloud/docs/G2_Registry_Track_Assignment_SOP.md`](../../../04-cloud/docs/G2_Registry_Track_Assignment_SOP.md)  
> **HIL 联调（007 必读）**: [`01-firmware/docs/G2_HIL_007_Firmware_Requirements.md`](../../../01-firmware/docs/G2_HIL_007_Firmware_Requirements.md)

---

## 1. 目标

G2 按 Fleet 最佳实践 **新建管道**。007 在 G2 固件分支上须：

1. 发布到 **五域 Topic**（本域 `energy/telemetry`）
2. 携带 **`sys_id` + `component_id`**（Fleet 主键）
3. 优先填充 **`measures.*`**；可同时保留 **legacy flat** 直至 adapter 下线
4. 从 **`v2.3.0`** 起作为 **G2 正式固件线**（见 §2）

---

## 2. G2 固件版本线（Bob 定稿 · 007 立即执行）

### 2.1 版本号格式

**沿用现网**，与 `Config.h` 的 `FIRMWARE_VERSION` 一致：

```text
v{MAJOR}.{MINOR}.{PATCH}[.{BUILD}]

现网样例（HQ2513A69PJ）：v2.2.3.25
G2 起线（首个联调/量产目标）：v2.3.0
```

| 项 | 要求 |
|----|------|
| 字段名 | 顶层 **`firmware_version`**（Legacy / G2 共用） |
| 写入位置 | `CommManager` 构建 JSON 时赋值（与 `device/status` 相同源） |
| G2 payload | **必须** 携带（`energy.telemetry.v1`） |
| OTA | 量产通道 **不得** 发布 **&lt; v2.3.0** 的 BIN |

### 2.2 云端行为（007 需知晓，非固件实现）

| 条件 | 云端 |
|------|------|
| Registry `track=g2` + 上报 **≥ v2.3.0** + G2 Topic + Schema OK | 写入 **Timestream**（M4） |
| `track=g2` + 上报 **&lt; v2.3.0**（如当前 v2.2.3.25） | **不写** Timestream；告警 |
| `track` 从 legacy → g2 | **仅人工**改 Registry；**Ingest 不会自动晋升** |
| `track=g2` 一旦设定 | **永不** 改回 legacy |

**HIL 台架**：`IQ-26-00001` 已为 `track=g2`，但现场仍为 **v2.2.3.25** 时，007 **必须 OTA 到 ≥ v2.3.0** 才能通过 §5.3 Timestream 门禁。

### 2.3 007 下一版发布检查清单

- [ ] `Config.h`：`FIRMWARE_VERSION` ≥ **`"v2.3.0"`**
- [ ] G2 Topic `iqedge/g2/dev/energy/telemetry` 发包
- [ ] Payload 含 `firmware_version` + `sys_id`=`IQ-26-00001` + `component_id`=`HQ2513A69PJ`
- [ ] `npm run validate:energy`（仓库根 `09-contract/`）
- [ ] OTA Job 成功 + `verify_g2_telemetry.py`（008 M4 部署后）

---

## 3. 必填字段（007 必须实现）

| 字段 | 说明 |
|------|------|
| `schema_version` | 固定 `"energy.telemetry.v1"` |
| `sys_id` | 产测烧录 / NVS；格式 **`IQ-{YY}-{NNNNN}`**（例 `IQ-26-00001`）— **不含** Watch/Box/Trailer |
| `component_id` | MPPT SER# 或 Cerbo ID（与 Registry 一致） |
| `component_role` | `mppt`（Watch/Box）或 `cerbo`（Trailer） |
| `domain` | 固定 `"energy"` |
| `timestamp` | ISO 8601 UTC |
| **`firmware_version`** | **≥ `v2.3.0` 用于 G2 Timestream**；格式见 §2.1 |
| `status` | `running` \| `stopped` \| `fault` \| `unknown` |
| `data_stale` | bool |
| `reporting_mode` | `NORMAL` \| `REDUCED` \| `SURVIVAL` \| `RECONCILIATION` |

**度量**: 推荐 `measures.battery.soc_pct` + `measures.battery.voltage_v`；或 legacy `soc` + `battery_voltage`。

**禁止**: 在固件内根据产品形态写死 `IQW-` / `IQB-` / `IQT-` 前缀；`system_type` **不烧录进 NVS**（Registry 权威）。

---

## 4. IQTrailer 特别说明

- **SSOT**: Cerbo GX；`component_role` = `cerbo`
- 一条 telemetry = 系统级 Cerbo 快照

---

## 5. 与 Legacy 并存

| 轨道 | Topic | 固件版本 |
|------|-------|----------|
| Legacy | `device/status` | 任意（含 v2.2.x） |
| G2 | `iqedge/g2/{env}/energy/telemetry` | **≥ v2.3.0** 方计入 G2 时序库 |

已发放的 **`IQW-*` 等旧 sys_id** 设备可继续 Legacy；**新批次** 仅用 `IQ-26-*` 格式。

---

## 6. 007 待实现优化（008 请求项）

| # | 项 |
|---|-----|
| F1 | `state` / `SURVIVAL` 与低电量状态机对齐 |
| F2 | `reporting_mode=REDUCED` 降频省流量 |
| F3 | NVS 仅存中央发放的 `sys_id` |
| F4 | 量产后期可选仅发 `measures` 块 |

---

## 7. 验收

- 样例通过 `09-contract` → `npm run validate:energy`
- dev Thing 发布至 G2 Topic → M4 ingest 验收（**fw ≥ v2.3.0**）

---

*Agent 008 → Agent 007 · ADR-004 · ADR-008*
