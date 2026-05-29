---
name: iqedge-triple-verify
description: >-
  Runs IQEdge-G2 three-layer verification (local serial/HIL, AWS DynamoDB+Timestream,
  IQWatch API egress). Use when validating MQTT pipeline, cloud stall, post-fix HIL,
  or when the user asks for 本地/云端/API 三位一体、遥测验证、verify_telemetry.
---

# IQEdge 三位一体验证（本地 · 云端 · API）

## 何时使用

- 固件/MQTT/云端管线改动后需**端到端自证**
- 用户报告「云端数据停更」「DDB 与现场不一致」
- HIL 烧录完成、OTA 完成后交叉确认
- Bob 要求「查 AWS」「验证遥测」「三位一体」

## 工作目录

`01-firmware`（含 `.env`、`tools/aws-verify/`）

## 三位一体定义

| 层 | 证据来源 | 通过标准 |
|----|----------|----------|
| **本地** | COM 串口 @ 115200 | `[COM] MQTT Connected` + `Publishing payload ... device/status`；`[NRG] MPPT RECONNECTED`；`FW:` 与 `Config.h` 的 `FIRMWARE_VERSION` 一致 |
| **云端** | DynamoDB `DeviceLatestStatus` + Timestream `IQWatchDB.DeviceStatus` | `GetItem` 有记录；`timestamp`/`last_reported` 为**最近**（通常 &lt;15 min）；`mac` 与串口一致 |
| **API** | `GET /v1/devices/{serial}/status` + `x-api-key` | HTTP 200；`deviceId`/`mac`/`firmware_version` 与 DDB 一致 |

三层 **deviceId（MPPT SER#）、mac、firmware_version、时间戳** 必须可对齐；任一层 stale 则整体 **FAIL**。

## 前置配置

1. `.env` 从 `.env.example` 复制（**勿提交**）
2. 必填：`AWS_ACCESS_KEY_ID`、`AWS_SECRET_ACCESS_KEY`、`IQWATCH_API_KEY`
3. 目标设备：
   - `TEST_MPPT_DEVICE_ID` = DynamoDB `deviceId`（通常 = MPPT `SER#`）
   - `TEST_MPPT_SERIAL` = API 路径用的 serial（**与 deviceId 相同**，除非 Bob 另有映射）

从串口或 DDB 获取 MAC → Thing 名为 `IQEdge_{MAC}`（含冒号，见 `CommManager::begin`）。

## 执行顺序（必须）

```text
① 本地串口（可选但强烈建议） → ② 云端脚本 → ③ 交叉比对 API
```

### ① 本地（HIL / 现场 USB）

```powershell
$env:Path = "C:\Users\kevin\.platformio\penv\Scripts;C:\Program Files\Git\cmd;$env:LOCALAPPDATA\Programs\Python\Python312\Scripts;$env:Path"
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
pio device monitor --port COM3 --baud 115200 --filter direct
```

采集 ≥30s，保存到 `debug/verify_local_YYYYMMDD.txt`。必扫标签：`[BOOT]` `[FS]` `[NRG]` `[COM]` `Publish`。

远程设备无 USB 时：跳过 ①，在报告中标注 **「仅云端+API」**。

### ②+③ 云端 + API（一键）

```powershell
pip install -r requirements-aws-verify.txt
python tools/aws-verify/verify_telemetry.py --mppt-id <SER#> --mppt-serial <SER#>
```

- **成功**：末尾 `Checks passed: N/N` 且 N≥3（DDB + Timestream + API）
- **仅 API**：`python tools/aws-verify/verify_telemetry.py --mppt-serial <SER#> --api-only`

### 交叉比对清单

复制并填写：

```text
- [ ] 本地 FW: ___________
- [ ] DDB deviceId / mac / firmware_version / timestamp: ___________
- [ ] API deviceId / mac / firmware_version / timestamp: ___________
- [ ] Timestream 最新点时间 ≈ DDB timestamp（±2 min）
- [ ] 三层一致 → PASS
```

## 常见失败与处置

| 现象 | 可能根因 | 动作 |
|------|----------|------|
| DDB 停更、Timestream 停更 | MQTT 未连、payload 缺 flat 字段、`deviceId` 空 | 查 `CommManager::_buildPayload`、串口 `Publish OK` |
| DDB 新、API 旧 | API 缓存或 serial 填错 | `--mppt-serial` 必须与 MPPT SN 一致 |
| 本地有数据、云端无 | WiFi/证书/NTP | 串口 `[COM]`、`[FS]` |
| Timestream 空、DDB 有 | 查询窗口或 deviceId 错误 | 脚本默认近 3 天 |

**禁止**：`Scan` 生产表；密钥写入固件/Git。RUT Timestream **2026-03-07~04-01** 缺口勿作依据。

## 参考

- 管线：`docs/AWS_Cloud_Pipeline.md`
- 工具说明：`tools/aws-verify/README.md`
- 本地 HIL：`iqedge-hil-loop`（`.cursor/skills/iqedge-hil-loop/SKILL.md`）
- 索引：项目根 `SKILL.md`

时间窗深度分析 → **技能 4** `iqedge-telemetry-analysis`（报告写入 `report/`）。

## 交付模板

```markdown
## 三位一体验证 — <deviceId>

| 层 | 结果 | 要点 |
|----|------|------|
| 本地 | PASS/SKIP/FAIL | FW / Publish / MPPT |
| 云端 DDB | PASS/FAIL | last_reported, mac, fw |
| 云端 TS | PASS/FAIL | 最新点时间 |
| API | PASS/FAIL | firmware_version |

结论：PASS / FAIL — <一句根因或下一步>
```
