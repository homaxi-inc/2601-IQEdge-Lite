# G2 HIL 联调 — 008 ↔ 007 分工与 008 交付清单

> **关联**: [`01-firmware/docs/G2_HIL_007_Firmware_Requirements.md`](../../01-firmware/docs/G2_HIL_007_Firmware_Requirements.md)  
> **日期**: 2026-05-29

---

## 1. 联调目标

在台架 **IQ-26-00001**（MPPT `HQ2513A69PJ`，Thing `IQEdge_1C:69:20:B8:D7:F4`）上：

1. 007 固件发布 `iqedge/g2/dev/energy/telemetry` + 合规 payload  
2. **OTA 强制** 迭代直至稳定  
3. 007 用 **DDB / Timestream 只读** 自证（008 保障 G2 表可读）  
4. **HTTP Fleet API 三位一体** — **后续** 再联调

---

## 2. 008 已完成

| 项 | 状态 |
|----|------|
| M0 契约 + M1 Foundation（IoT Policy、KMS、Lambda base role） | ✅ |
| M2 Storage + M3 Registry（HIL 种子 `IQ-26-00001`） | ✅ |
| **M4** energy ingest（Rule + Lambda + Schema · dev 已部署） | ✅ |
| Thing 证书 + **G2 Policy attach**（`IQEdge_1C:69:20:B8:D7:F4`） | ✅ 2026-05-30 |
| AWS 账户 CLI | ✅ `661631955220` |
| 007 要求文档 | ✅ `G2_HIL_007_Firmware_Requirements.md` |
| **007 通知** | ✅ [`NOTICE_008_to_007_IQ-26-00001_2026-05-30.md`](../../01-firmware/report/NOTICE_008_to_007_IQ-26-00001_2026-05-30.md) |

---

## 3. 007 下一步（008 P0 已关闭）

| 顺序 | 模块 | 负责人 | 状态 |
|------|------|--------|------|
| 1 | G2 MQTT `iqedge/g2/dev/energy/telemetry` + `energy.telemetry.v1` | **007** | ⬜ |
| 2 | OTA ≥ v2.3.0 + Legacy `verify_telemetry.py` | **007** | ⬜ |
| 3 | **`verify_g2_telemetry.py`**（§5.3） | **007** | ⬜ |
| 4 | M4.7 端到端 HIL 签收 | **007** 发包 + **008** 协助查日志 | ⬜ |

---

## 4. Registry 种子（M3 目标 JSON）

```json
{
  "sys_id": "IQ-26-00001",
  "id_format": "iq_y1",
  "system_type": "iqwatch",
  "track": "g2",
  "deployment_state": "qa_bench",
  "cloud_target": "dev",
  "domains_enabled": ["energy", "network", "control"],
  "components": {
    "energy": [
      { "component_id": "HQ2513A69PJ", "role": "mppt", "thing_name": "IQEdge_1C:69:20:B8:D7:F4" }
    ],
    "network": [],
    "vision": [],
    "control": [],
    "environment": []
  },
  "aliases": {
    "legacy_sys_id": "IQW-9041",
    "mppt_serial": "HQ2513A69PJ",
    "legacy_device_id": "HQ2513A69PJ"
  }
}
```

---

## 5. G2 Timestream 验证查询（供 `verify_g2_telemetry.py`）

```sql
SELECT sys_id, component_id, time,
       measure_name, measure_value::double
FROM iqedge_g2_dev_database.iqedge_g2_dev_table_energy
WHERE sys_id = 'IQ-26-00001'
  AND time > ago(30m)
ORDER BY time DESC
LIMIT 20
```

（列名以 M2 ingest 实际 dimension/measure 为准，脚本内封装。）

---

## 6. IoT Policy attach（运维备忘）

```powershell
# 1. 查 Thing 实际证书 ARN（Console 或 list-thing-principals 若已绑定）
# 2. attach-policy
aws iot attach-policy `
  --policy-name iqedge-g2-dev-iot-policy-g2-device `
  --target <certificate-arn> `
  --region us-east-1
```

设备可能仍保留 Legacy Policy；**多 Policy attach 同一证书** 为常见做法。

---

## 7. 007 IAM

确认 `Agent-007`（或 007 使用的 IAM 用户）具备：

- `dynamodb:GetItem` on `DeviceLatestStatus`（已有）
- `timestream:Select` on `iqedge_g2_dev_database`（M2 后由 Bob/008 授权）

---

*Agent 008 · HIL handoff*
