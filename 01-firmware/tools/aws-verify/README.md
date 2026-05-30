# AWS 云端遥测验证 (Agent 007)

依据 `agent_007_aws_verification_sop.md`（S90）与 [`docs/AWS_Cloud_Pipeline.md`](../docs/AWS_Cloud_Pipeline.md) 搭建，**只读**访问 DynamoDB / Timestream，可选 API Gateway 回退。

**MPPT 写入管线**: `device/status` → IoT Rule `DeviceStatusToLambda` (SQL **2015-10-08**) → `SaveDeviceStatus` → DynamoDB `DeviceLatestStatus` + Timestream `IQWatchDB.DeviceStatus`。

## 快速开始

```powershell
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
copy .env.example .env
# 编辑 .env，填入 AWS 密钥与设备 ID（勿提交 .env）

pip install -r requirements-aws-verify.txt
python tools\aws-verify\verify_telemetry.py
```

## `.env` 必填项

| 变量 | 说明 |
|------|------|
| `AWS_ACCESS_KEY_ID` | IAM `Agent-007-Telemetry-Verifier` Access Key |
| `AWS_SECRET_ACCESS_KEY` | 对应 Secret |
| `TEST_MPPT_DEVICE_ID` | DynamoDB `DeviceLatestStatus` 的 `deviceId` |
| `TEST_MPPT_SERIAL` | MPPT `SER#`，用于 IQWatch API `/devices/{serial}/status` |

可选：`TEST_RUT_SN`、`IQWATCH_API_KEY`、`RUT_API_KEY`

## HIL 现场参考（2026-05-28）

- ESP32 MAC: `EC:E3:34:1A:F9:8C`
- MQTT Client ID: `IQEdge_EC:E3:34:1A:F9:8C`（固件 `CommManager`）
- MPPT `SER#` 需从串口 `[NRG]` 摘要或 payload `system.serial` 读取后填入 `.env`

## 技能 4 — 历史遥测分析

| 脚本 | 用途 |
|------|------|
| `verify_g2_telemetry.py --sys-id IQ-26-00001` | **G2** Timestream（需 008 M2+M4） |
| `analyze_device_window.py --device <SER#> --hours 10` | 时间窗统计 + 异常提示 |
| `find_device.py <片段>` | 按 SER# 片段扫描 DDB |

报告输出规范：`.cursor/skills/iqedge-telemetry-analysis/SKILL.md` → 写入 `report/`。

## G2 HIL 联调（IQ-26-00001 / HQ2513A69PJ）

**007 必读**: [`docs/G2_HIL_007_Firmware_Requirements.md`](docs/G2_HIL_007_Firmware_Requirements.md)  
**008 分工**: [`04-cloud/docs/G2_HIL_008_007_Handoff.md`](../04-cloud/docs/G2_HIL_008_007_Handoff.md)

---

- 禁止 Scan 生产表；`find_device.py` 仅小范围 `contains` 扫描，优先已知 SER#
- 禁止将密钥写入固件或 Git
- RUT Timestream：**2026-03-07 ~ 2026-04-01** 数据缺口，勿作依据
