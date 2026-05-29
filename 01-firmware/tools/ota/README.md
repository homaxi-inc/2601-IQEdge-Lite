# IQEdge 远程 OTA（AWS IoT Jobs）

## 目标设备示例（HQ2513A69PJ）

| 项 | 值 |
|----|-----|
| MPPT SN | `HQ2513A69PJ` |
| MAC | `1C:69:20:B8:D7:F4` |
| IoT Thing | `IQEdge_1C:69:20:B8:D7:F4` |
| S3 桶 | `iqwatch-firmware-ota` |
| Job 文档 | `{"url":"<presigned-https-url>"}` |

## IAM

- 内联策略 **`Agent007OTAPolicy`** 已挂载 **`Agent-007`**：`s3:PutObject`（桶 `iqwatch-firmware-ota`）、`iot:CreateJob` 等。
- 若 `boto3` 报 `CreateJob` AccessDenied，可用 **AWS CLI** 同凭证重试（IAM 传播延迟）。
- 可选 `.env`：`OTA_AWS_ACCESS_KEY_ID` / `OTA_AWS_SECRET_ACCESS_KEY` 覆盖默认密钥。

## 一键脚本

```powershell
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
pio run
python tools\ota\push_ota_job.py --mac "1C:69:20:B8:D7:F4" --version v2.2.3.25 --wait
python tools\ota\monitor_ota.py --job-id <JOB_ID> --mac "1C:69:20:B8:D7:F4" --mppt-id HQ2513A69PJ --expected-fw v2.2.3.25
```

## AWS CLI 等价

```powershell
$AWS = "C:\Program Files\Amazon\AWSCLIV2\aws.exe"
$BIN = ".pio\build\esp32dev\firmware.bin"
$KEY = "firmware_v2.2.3.25.bin"
$THING = "IQEdge_1C:69:20:B8:D7:F4"
$JOB = "IQW-OTA-HQ2513A69PJ-$(Get-Date -Format yyyyMMdd-HHmmss)"

& $AWS s3 cp $BIN "s3://iqwatch-firmware-ota/$KEY" --content-type application/octet-stream
$URL = & $AWS s3 presign "s3://iqwatch-firmware-ota/$KEY" --expires-in 86400
$DOC = "{`"url`": `"$URL`"}"
& $AWS iot create-job --job-id $JOB --targets "arn:aws:iot:us-east-1:661631955220:thing/$THING" --document $DOC --target-selection SNAPSHOT --description "OTA v2.2.3.25 HQ2513A69PJ"
```

## 验证（三位一体）

```powershell
python tools\aws-verify\verify_telemetry.py --mppt-id HQ2513A69PJ --mppt-serial HQ2513A69PJ
```

期望：Job **SUCCEEDED**；DDB/API `firmware_version` 与目标版本一致。

详见 **`.cursor/skills/iqedge-remote-ota/SKILL.md`** 与 **`docs/OTA_Stress_Test_Plan.md`**。
