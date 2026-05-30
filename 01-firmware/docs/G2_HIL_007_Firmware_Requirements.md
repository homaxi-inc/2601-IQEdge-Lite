# G2 HIL 固件修改要求 — 007 执行说明（联调台架 IQ-26-00001）

> **下达**: Agent 008 · Bob 授权联调  
> **日期**: 2026-05-29  
> **状态**: **008 环境部分就绪** — 见 §2；G2 Timestream 验证门禁依赖 008 完成 M2+M4  
> **读者**: Agent 007（实施）· Bob（审批 OTA）· 008（云端管道）

---

## 1. 联调资产身份（请全文使用下表）

| 项 | 值 | 备注 |
|----|-----|------|
| **站点称呼（历史）** | IQW-9041 | 仅文档/报告用；**不是** G2 `sys_id` |
| **G2 sys_id（权威）** | **`IQ-26-00001`** | ADR-004；NVS/产测烧录；MQTT payload 必填 |
| **MPPT SER# / component_id** | **`HQ2513A69PJ`** | VE.Direct；Legacy `deviceId` |
| **system_type（Registry）** | `iqwatch` | 008 写入 Registry；固件 **不烧录** |
| **IoT Thing 名称** | `IQEdge_1C:69:20:B8:D7:F4` | OTA Job target |
| **MQTT Client ID（惯例）** | `IQEdge_1C:69:20:B8:D7:F4` | 与 Thing 一致 |
| **G2 环境** | **`dev`** | Topic 前缀 `iqedge/g2/dev/…` |
| **当前固件（Legacy DDB）** | **`v2.2.3.25`** | **&lt; G2 起线**；须 OTA 至 **≥ `v2.3.0`** |
| **G2 起线版本（Bob 定稿）** | **`v2.3.0`** | ADR-008；见 §3.0 |
| **Registry `track`** | **`g2`** | 008 已种子；**Timestream 仍须 fw ≥ 2.3** |
| **固件目录** | `01-firmware/` | PlatformIO |

```text
IQ-26-00001  (sys_id，终身不变)
    └── component HQ2513A69PJ (mppt)
            └── Thing IQEdge_1C:69:20:B8:D7:F4
```

---

## 2. 008 环境就绪确认（2026-05-29 实测）

### 2.1 已就绪 ✅

| 项 | 状态 |
|----|------|
| AWS 账户 `661631955220` · `us-east-1` | ✅ CLI 可访问 |
| G2 IoT Policy | ✅ `iqedge-g2-dev-iot-policy-g2-device` 已创建（M1） |
| IoT Thing | ✅ `IQEdge_1C:69:20:B8:D7:F4` 存在 |
| **Legacy 写入管道** | ✅ `device/status` → `DeviceLatestStatus` + `IQWatchDB.DeviceStatus` |
| **007 只读验证工具** | ✅ `tools/aws-verify/verify_telemetry.py`（DDB + Legacy Timestream） |
| **OTA 先例** | ✅ `IQW-OTA-HQ2513A69PJ-20260529` → **SUCCEEDED**（v2.2.3.25） |
| **Energy 契约** | ✅ `09-contract/schemas/energy/telemetry.v1.json` |

### 2.2 008 交付前未完成 ⬜（007 G2 云端验证门禁）

| 项 | 负责人 | 说明 |
|----|--------|------|
| Thing 挂载 **G2 IoT Policy** 到设备证书 | **008** | 当前 `list-thing-principals` 为空，需核对实际连接证书并 `attach-policy` |
| Timestream **`iqedge_g2_dev_database` / `table_energy`** | **008 M2** | 未部署前无法在 G2 表查 `sys_id` |
| IoT Rule + **ingest-energy Lambda** | **008 M4** | 未部署前 G2 Topic 无持久化 |
| Registry **`IQ-26-00001` 记录** | **008 M3** | ✅ 已种子 `track=g2` |
| Timestream + Shadow + S3 | **008 M2** | ✅ dev 已部署 |
| G2 专用验证脚本 | **008** | `verify_g2_telemetry.py`（见 §6.2） |

**008 承诺**：在 007 进入 **§5.3 门禁 G2-Timestream** 之前，完成 **M2（energy）+ M3（Registry 种子）+ M4（energy ingest）** 的 **dev deploy**，并通知 007。

### 2.3 007 可立即开始的工作（不阻塞）

- 固件分支开发：`sys_id`、G2 Topic、payload 对齐 Schema  
- 本地串口 / MQTT 抓包（含是否发到新 Topic）  
- 编译固件 + **OTA 升级**（§4）  
- Legacy 路径验证：`device/status` 仍可用 `verify_telemetry.py --mppt-serial HQ2513A69PJ`

---

## 3. 固件功能要求（必须实现）

### 3.0 固件版本线（Bob 定稿 · ADR-008 — 007 优先阅读）

| 项 | 要求 |
|----|------|
| **G2 正式起线** | **`FIRMWARE_VERSION` = `v2.3.0`**（或更高，如 `v2.3.0.1`） |
| **格式** | 沿用现网：`v{MAJOR}.{MINOR}.{PATCH}[.{BUILD}]`（与 `Config.h` 一致） |
| **payload 字段** | 顶层 **`firmware_version`**（G2 与 Legacy 同源） |
| **当前台架** | **v2.2.3.25** → **不满足** G2 Timestream；**下一版 OTA 必须 ≥ v2.3.0** |
| **OTA 门禁** | 量产/联调发布 **禁止** &lt; `v2.3.0` 的 BIN |
| **Registry `track`** | `IQ-26-00001` 已为 **`g2`**；**晋升其它设备仅人工**，Ingest **不会**自动改 track |
| **008 文档** | [`FIRMWARE_ALIGNMENT_007.md`](../../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) §2 · [`G2_Registry_Track_Assignment_SOP.md`](../../04-cloud/docs/G2_Registry_Track_Assignment_SOP.md) |

```text
v2.2.3.25  →  Legacy Timestream/DDB 路径可验；G2 Timestream 门禁不通过
v2.3.0+    →  G2 Topic + Schema + (Registry track=g2) → Timestream 可写
```

### 3.1 MQTT — G2 新轨（主验收路径）

| 项 | 要求 |
|----|------|
| **Topic** | `iqedge/g2/dev/energy/telemetry`（**禁止**使用 `#`；**禁止**仅依赖 `device/status` 作为 G2 验收） |
| **QoS** | 1（建议，与 Legacy 一致即可） |
| **频率** | 与现网 `device/status` 相同或略低（`reporting_mode` 见 Schema） |

### 3.2 Payload — `energy.telemetry.v1`

权威 Schema：

- [`09-contract/schemas/energy/telemetry.v1.json`](../../09-contract/schemas/energy/telemetry.v1.json)
- 样例：[`09-contract/examples/energy/telemetry-iqwatch-live.v1.json`](../../09-contract/examples/energy/telemetry-iqwatch-live.v1.json)

**本台最低必填**（007 自检用）：

```json
{
  "schema_version": "energy.telemetry.v1",
  "sys_id": "IQ-26-00001",
  "component_id": "HQ2513A69PJ",
  "component_role": "mppt",
  "domain": "energy",
  "timestamp": "2026-05-29T12:00:00Z",
  "ingest_mode": "live",
  "status": "running",
  "state": "NORMAL",
  "data_stale": false,
  "reporting_mode": "NORMAL",
  "firmware_version": "v2.3.0",
  "measures": {
    "battery": { "soc_pct": 59.5, "voltage_v": 13.13 },
    "solar": { "power_w": 0 },
    "load": { "power_w": 0, "status": "ON" },
    "yield": { "total_kwh": 6.25, "today_kwh": 0.03, "days_running": 218 }
  }
}
```

可同时保留 Legacy **flat** 字段（`soc`、`battery_voltage`、`deviceId` 等）便于过渡期对照。

**禁止**：

- 在固件写死 `IQW-` / `IQB-` / `IQT-` 作为 `sys_id`
- 把 `HQ2513A69PJ` 当作 Fleet 主键（应作为 `component_id`）

### 3.3 NVS / 配置

| 项 | 要求 |
|----|------|
| `sys_id` | 产测或联调烧录 **`IQ-26-00001`**（大写 `IQ`） |
| `system_type` | **不要** 烧录；由云端 Registry 管理 |
| 联调构建 | 建议 `platformio.ini` 联调 env 或 `-D G2_SYS_ID="IQ-26-00001"`（007 自选，须在报告中说明） |

### 3.4 Legacy 轨道（过渡期）

| 项 | 建议 |
|----|------|
| `device/status` | **可保留** 双发，直至 Bob 明确下线；G2 验收以新 Topic 为准 |
| 扁平字段 | 保持 Lambda `SaveDeviceStatus` 兼容（见 `payload.md` v2） |

---

## 4. OTA 要求（强制）

007 **每次** 固件变更达到可测状态，**必须** 走远程 OTA，不得仅 USB 烧录了事（USB 仅用于救砖）。

### 4.1 目标设备

| 项 | 值 |
|----|-----|
| Thing | `IQEdge_1C:69:20:B8:D7:F4` |
| MPPT | `HQ2513A69PJ` |
| 参考 Job | `IQW-OTA-HQ2513A69PJ-20260529`（已成功） |

### 4.2 流程（与现有 SOP 一致）

```powershell
cd 01-firmware
# 1. 编译 release 固件；FIRMWARE_VERSION 必须 >= v2.3.0（Config.h）
# 2. 上传 S3 + 预签名 URL
# 3. create-job → monitor_ota.py
python tools\ota\monitor_ota.py --job-id <JOB_ID> --mac "1C:69:20:B8:D7:F4" --mppt-id HQ2513A69PJ --expected-fw <版本>
```

详见：[`tools/ota/README.md`](../tools/ota/README.md)、[`.cursor/skills/iqedge-remote-ota/SKILL.md`](../.cursor/skills/iqedge-remote-ota/SKILL.md)

### 4.3 OTA 成功标准

| # | 标准 |
|---|------|
| 1 | IoT Job 状态 **`SUCCEEDED`** |
| 2 | 升级后 **300s 内** 有新 MQTT 上报（Legacy 或 G2 Topic 任一） |
| 3 | 上报中 `firmware_version` 与预期一致 |
| 4 | 设备 **无** 连续 WDT 重启（串口无 `[WDT]` 循环） |
| 5 | 在 `01-firmware/report/` 写入简短 OTA 报告（Job ID、版本、时间 UTC） |

失败 → 修复固件 → **重新 OTA**，直至满足上表。

---

## 5. 验证闭环（007 自证 + 008 管道）

### 5.1 阶段 A — Legacy（008 已就绪，007 先做）

```powershell
cd 01-firmware
python tools\aws-verify\verify_telemetry.py --mppt-id HQ2513A69PJ --mppt-serial HQ2513A69PJ --skip-rut
```

**通过**：DDB `DeviceLatestStatus` 有新 `last_reported`；字段合理。

### 5.2 阶段 B — G2 MQTT 到达（证书 + Policy）

007 可选：

- AWS Console → IoT Core → Test → 订阅 `iqedge/g2/dev/energy/telemetry` 看是否入站  
- 或串口确认 `[COM] Publishing` 的 Topic 字符串正确  

**008 动作**：将 `iqedge-g2-dev-iot-policy-g2-device` **attach** 到该 Thing 实际使用的证书 Principal（Bob/008 在 Console 核对）。

若 **连接被拒绝 / publish 失败** → 先找 008 修 Policy，**不要** 改 Topic 违规。

### 5.3 阶段 C — G2 Timestream（008 M2+M4 完成后）

**门禁**：008 通知「M2+M4 dev 已部署」后执行。

```powershell
cd 01-firmware
python tools\aws-verify\verify_g2_telemetry.py --sys-id IQ-26-00001 --minutes 30
```

（脚本由 008 提供；查询 `iqedge_g2_dev_database` / `iqedge_g2_dev_table_energy`，按 `sys_id` 过滤。）

**通过**：

- 至少 **1 条** 记录，`sys_id = IQ-26-00001`  
- `component_id = HQ2513A69PJ`  
- 时间与 OTA 后上报窗口一致  
- 电量字段与 DDB Legacy 快照 **量级一致**（不必逐字段相等）

未通过 → 007 改固件 → OTA → 重复 §5.1–5.3，直至通过。

### 5.4 迭代原则

```text
改代码 → 编译 → OTA → Legacy 验证 → (008 就绪后) G2 验证 → 报告
```

**不要** 等 API 三位一体；Fleet HTTP API 联调排在 **G2 管道通过后**（008 与 Bob 另排期）。

---

## 6. 008 为 007 提供的只读能力

### 6.1 已有（Legacy）

| 数据源 | 007 工具 |
|--------|----------|
| DynamoDB `DeviceLatestStatus` | `verify_telemetry.py` |
| Timestream `IQWatchDB.DeviceStatus` | `verify_telemetry.py` / `analyze_device_window.py` |
| IQWatch API | `IQWATCH_API_KEY` + egress（可选） |

### 6.2 008 将补充（M2+M4 后）

| 交付物 | 说明 |
|--------|------|
| `tools/aws-verify/verify_g2_telemetry.py` | 按 `sys_id` 查 G2 Timestream |
| Registry 种子项 | `IQ-26-00001` + `aliases.mppt_serial=HQ2513A69PJ` |
| IoT Test 订阅说明 | 可选，见 [`04-cloud/docs/G2_HIL_008_007_Handoff.md`](../../04-cloud/docs/G2_HIL_008_007_Handoff.md) |

**IAM**：007 现有 `Agent-007` 用户需具备 `timestream:Select` on G2 库（008 在 M2 部署时核对；若无权限 Bob 授权）。

---

## 7. 明确不在本次范围

| 项 | 负责 | 何时 |
|----|------|------|
| FastAPI `GET /api/v2/fleet/systems/.../energy` | 008 M9–M11 | 管道通过后 |
| 双轨 API 合流 / v1 shim | 008 M10–M12 | 同上 |
| `system_type` 部署扫码 UI | 前端 / 008 | 后续 |
| prod 环境 deploy | 008 | MVP 后 |

---

## 8. 007 交付物清单

| # | 交付物 | 路径 |
|---|--------|------|
| 1 | G2 MQTT + payload 实现 PR/提交 | `01-firmware/src/…` |
| 2 | OTA 成功报告 | `01-firmware/report/OTA_IQ26-00001_<date>.md` |
| 3 | Legacy 验证截图或命令输出 | 同上或贴 report |
| 4 | G2 Timestream 验证输出（008 开门禁后） | 同上 |
| 5 | `development_log.md` 条目 | `01-firmware/docs/development_log.md` |

---

## 9. 参考文档

| 文档 | 路径 |
|------|------|
| Energy Schema | [`09-contract/schemas/energy/telemetry.v1.json`](../../09-contract/schemas/energy/telemetry.v1.json) |
| 原 007 对齐摘要 | [`09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md`](../../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) |
| ADR sys_id | [`decisions/README.md`](../../decisions/README.md) ADR-004 |
| Legacy 管道 | [`AWS_Cloud_Pipeline.md`](AWS_Cloud_Pipeline.md) |
| 008 联调分工 | [`04-cloud/docs/G2_HIL_008_007_Handoff.md`](../../04-cloud/docs/G2_HIL_008_007_Handoff.md) |

---

## 10. 008 确认声明

- [x] 已核对 AWS dev 账户与 M1 资源  
- [x] 已确认联调身份 **`IQ-26-00001`** / **`HQ2513A69PJ`** / Thing 名称  
- [x] **M2+M3+M4 dev deploy** — 2026-05-30 完成（见 [`NOTICE_008_to_007`](../report/NOTICE_008_to_007_IQ-26-00001_2026-05-30.md)）  
- [x] Thing **G2 Policy attach** — cert `…eb590933…` · 2026-05-30  

---

*Agent 008 · G2 HIL · IQ-26-00001 · HQ2513A69PJ*
