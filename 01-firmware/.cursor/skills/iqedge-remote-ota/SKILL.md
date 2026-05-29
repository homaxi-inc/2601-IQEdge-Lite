---
name: iqedge-remote-ota
description: >-
  Pushes IQEdge-G2 firmware via AWS S3 and IoT Jobs (esp_https_ota), monitors job
  execution and cloud firmware_version. Use for remote OTA, TC-OTA, firmware upgrade,
  push_ota_job, or when the user mentions IoT Job / iqwatch-firmware-ota.
---

# IQEdge 远程 OTA（AWS IoT Jobs）

## 何时使用

- 现场/远程设备升级（无 USB）
- TC-OTA-01/02/10 等测试
- 验证 `Agent007OTAPolicy` 或新镜像发布

## 路径说明（合规）

| 路径 | 生产 | 说明 |
|------|------|------|
| **AWS IoT Jobs** + `OtaManager::esp_https_ota` | ✅ 始终启用 | 本技能覆盖 |
| **ArduinoOTA**（`ENABLE_OTA`） | ❌ 生产禁止 | `Config.h` 保持注释；仅维护构建可开 |

OTA **不写** LittleFS（证书 @ `0x291000`）；`app0`/`app1` 双槽 ~1.25 MB。

## 目标设备映射

| 字段 | 来源 |
|------|------|
| MPPT SN | VE.Direct `SER#` → payload `deviceId` |
| MAC | 串口 / DDB `mac` |
| IoT Thing | `IQEdge_{MAC}`（冒号保留） |
| Job 文档 | `{"url":"<presigned-https-url>"}` |

固件 Job 处理：`CommManager::_handleJob` — URL 含**当前** `FIRMWARE_VERSION` 子串则 **SUCCEEDED 不重启**。

## IAM 与凭证

- 桶：`iqwatch-firmware-ota`
- 用户：`Agent-007` + 内联策略 `Agent007OTAPolicy`（`s3:PutObject`、`iot:CreateJob` 等）
- `.env`：`AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY`；可选 `OTA_AWS_*` 覆盖
- `boto3` 偶发 `CreateJob` 被拒时：用 **AWS CLI**（同凭证）创建 Job

## 标准流程（TC-OTA-02）

```text
① 确认设备在线 → ② 递增 FIRMWARE_VERSION → ③ pio run → ④ 上传 S3 → ⑤ 创建 Job → ⑥ 监控 → ⑦ 三位一体验证
```

### ① 设备在线

```powershell
python tools/aws-verify/verify_telemetry.py --mppt-id <SER#> --mppt-serial <SER#>
```

记录 DDB `firmware_version`（版本 A）、`mac`。

### ②③ 构建镜像 B

`src/config/Config.h` → `FIRMWARE_VERSION` 递增（如 `v2.2.3.25`）。

```powershell
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
pio run
```

产物：`.pio/build/esp32dev/firmware.bin`（须 &lt; 0x140000）。

### ④⑤ 下发 OTA

```powershell
python tools/ota/push_ota_job.py --mac "<MAC>" --version <vX.Y.Z> --wait
```

S3 key 约定：`firmware_<version>.bin`（如 `firmware_v2.2.3.25.bin`）。

**CLI 备选**（`CreateJob` 失败时）：

```powershell
$AWS = "C:\Program Files\Amazon\AWSCLIV2\aws.exe"
& $AWS s3 cp .pio/build/esp32dev/firmware.bin s3://iqwatch-firmware-ota/firmware_v2.2.3.25.bin --content-type application/octet-stream
$URL = & $AWS s3 presign s3://iqwatch-firmware-ota/firmware_v2.2.3.25.bin --expires-in 86400
& $AWS iot create-job --job-id "IQW-OTA-<SER#>-<timestamp>" --targets "arn:aws:iot:us-east-1:661631955220:thing/IQEdge_<MAC>" --document "{`"url`": `"$URL`"}" --target-selection SNAPSHOT
```

### ⑥ 监控

```powershell
python tools/ota/monitor_ota.py --job-id <JOB_ID> --mac "<MAC>" --mppt-id <SER#> --expected-fw <vX.Y.Z> --timeout 900
```

期望：Job **SUCCEEDED**（通常 1–10 min）；重启后 DDB/API `firmware_version` = B。

### ⑦  post-OTA 验证

使用技能 **iqedge-triple-verify**（至少云端+API；有 USB 则加本地串口）。

## 成功 / 失败判定

| 结果 | Job | 设备 FW | 遥测 |
|------|-----|---------|------|
| **PASS** | SUCCEEDED | DDB/API = B | &lt;5 min 内新 `timestamp` |
| **FAIL** | FAILED / 超时 | 仍为 A | 查 URL 404、断网、镜像过大 |
| **SKIP 重启** | SUCCEEDED 无重启 | URL 含当前版本串 | TC-OTA-01 |

## 禁止与救砖

- 禁止 `LittleFS.begin(true)`、禁止 `uploadfs`（除非 TC-OTA-FS）
- 禁止未测 BIN 推生产
- P1 Boot Loop：USB `pio run -t upload`（不擦 FS）救砖 → 见 `docs/OTA_Stress_Test_Plan.md` TC-OTA-04

## 参考

- `tools/ota/README.md`、`tools/ota/push_ota_job.py`、`monitor_ota.py`
- `docs/OTA_Stress_Test_Plan.md`
- 本地构建：`iqedge-hil-loop` · 事后验证：`iqedge-triple-verify`
- 实测：`HQ2513A69PJ` Job `IQW-OTA-HQ2513A69PJ-20260529` v2.2.3.20→v2.2.3.25
