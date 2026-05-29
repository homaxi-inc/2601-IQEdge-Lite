# IQEdge MPPT 云端数据管线（Agent 007 基准）

> **来源**: Bob 提供的 S90 企业架构说明 + `agent_007_aws_verification_sop.md`  
> **HIL 设备**: MPPT SN `HQ2513NH99U` · ESP32 MAC `EC:E3:34:1A:F9:8C`

---

## 1. IQEdge MPPT API（Egress 读路径）

| 项 | 值 |
|----|-----|
| API ID | `1y9689tax0` |
| Base URL | `https://1y9689tax0.execute-api.us-east-1.amazonaws.com/v1` |
| 典型读接口 | `GET /devices/{serial}/status` |
| 认证 | Header `x-api-key: {{IQWATCH_API_KEY}}`（仅主机 `.env`，**固件不调用**） |

固件**仅 MQTT 写入**；API 供 `tools/aws-verify/verify_telemetry.py` 与运维读路径使用。

---

## 2. IQEdge (ESP32 — MPPT / Energy Core) 写入管线

```text
ESP32 MQTT Publish
    topic: device/status
        ↓
AWS IoT Rule: DeviceStatusToLambda
    ⚠️ CRITICAL: SQL 版本 MUST be `2015-10-08`
        ↓
Lambda: SaveDeviceStatus
        ├─► DynamoDB  DeviceLatestStatus   (最新快照 / GetItem)
        └─► Timestream IQWatchDB.DeviceStatus   (时序分析 / Select)
```

| 层级 | 名称 | 007 验证方式 |
|------|------|----------------|
| MQTT Topic | `device/status` | 串口 `[COM] Publishing payload ... device/status` |
| IoT Rule | `DeviceStatusToLambda` | AWS Console 核对 SQL 版本（非 007 本地可改） |
| Handler | `SaveDeviceStatus` | CloudWatch Logs（需额外 IAM） |
| Primary DB | **DynamoDB `DeviceLatestStatus`** | `GetItem` key: `deviceId` = MPPT SN |
| Analytics DB | **Timestream `IQWatchDB.DeviceStatus`** | `SELECT ... WHERE deviceId = '{sn}'` |

---

## 3. HIL 验证记录（2026-05-28）

### 3.1 DynamoDB `DeviceLatestStatus`

- **Key**: `deviceId` = `HQ2513NH99U`
- **MAC**: `EC:E3:34:1A:F9:8C` ✅ 与现场 ESP32 一致
- **last_reported**: `2026-05-28 23:40:57 UTC` ✅ **已恢复**（固件 v2.2.3.22 扁平 payload + WDT/HTTP 修复后复测）

| 字段 | 云端快照 |
|------|----------|
| state | NORMAL |
| battery_voltage | 13.13 V |
| soc | 59.5% |
| solar_power | 0 W |
| load_power | 0 W |

**根因（已修复）**: 固件 payload 缺少 Lambda 所需的 **扁平字段**（`deviceId`、`soc`、`battery_voltage` 等）且 `timestamp` 为 ISO 格式；另 **无 API Key 时仍调用 egress HTTP** 导致 `Task_Comm` 看门狗超时 (~40s) 反复重启，MQTT 无法稳定上报。v2.2.3.22 已对齐 `payload.md` v1 并跳过无 Key 的 HTTP 对账。

### 3.2 Timestream `IQWatchDB.DeviceStatus`

- IAM `Agent-007` 已具备 `timestream:Select`（2026-05-28 授权后复测通过）。
- `HQ2513NH99U`：**5726** 条历史记录，**latest = `2026-03-20 23:26:49 UTC`**（与 DynamoDB 一致）。
- 近 **7 天** `DISTINCT deviceId` 列表中 **无** `HQ2513NH99U`（其他设备有持续上报）。
- 修复后 Timestream 已写入 **`2026-05-28 23:40:57 UTC`** 新点（`battery_voltage`、`soc`、`load_power` 等）。

### 3.3 API Egress

- `IQWATCH_API_KEY` 在主机 `.env`（`verify_telemetry.py` egress **HTTP 200**）；固件 **v2.2.3.24+** 已移除 HTTP 对账，避免双 TLS/WDT 风险。

---

## 4. 007 本地验证命令

```powershell
cd 01-firmware
python tools\aws-verify\verify_telemetry.py --mppt-id HQ2513NH99U --mppt-serial HQ2513NH99U --skip-rut
```

---

## 5. 红线（与 SOP 一致）

- DynamoDB：**禁止 Scan**；仅用 `GetItem` / `Query`
- 密钥：仅 `.env` + `~/.aws/credentials`，不入 Git / 固件
- RUT Timestream 缺口：**2026-03-07 ~ 2026-04-01**（本 MPPT 表不适用，但 RUT 任务需遵守）
