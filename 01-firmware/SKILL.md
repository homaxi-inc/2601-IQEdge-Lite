# Agent 007 — IQEdge-G2 项目技能手册

> **项目**: `01-firmware` · 执行前查阅本索引 + `docs/development_log.md`  
> **细则**: 各技能完整步骤在 `.cursor/skills/<name>/SKILL.md`，本文件仅保留索引与入口命令。

---

## 技能索引

| ID | 技能 | Cursor Skill (`name`) | 状态 |
|----|------|------------------------|------|
| 1 | 本地 HIL 闭环 | `iqedge-hil-loop` | ✅ |
| 2 | 三位一体验证（本地 · 云端 · API） | `iqedge-triple-verify` | ✅ |
| 3 | 远程 OTA（S3 + IoT Jobs） | `iqedge-remote-ota` | ✅ |
| 4 | 历史遥测分析（时间窗 · 写 report） | `iqedge-telemetry-analysis` | ✅ |

**Agent**：接到任务后读取对应目录下 `SKILL.md`，勿仅依赖本文件。

**推荐链**：`1 → 2`（改码上云）· `1 → 3 → 2`（发版 OTA）· `2 → 4` 或 `4`（数据是否正确）· 远程设备可 `3 → 2`（跳过 1 串口）。

---

## 技能 1 — 本地 HIL 闭环

**何时**: 编译/烧录/串口/VE.Direct/MQTT/状态机 · 「修 Bug」「看 COM3」  
**细则**: `.cursor/skills/iqedge-hil-loop/SKILL.md`

```powershell
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
pio run
pio run -t upload --upload-port COM3
pio device monitor --port COM3 --baud 115200 --filter direct
```

---

## 技能 2 — 三位一体验证

**何时**: 遥测/云端停更/OTA 后 · 「三位一体」「verify_telemetry」  
**细则**: `.cursor/skills/iqedge-triple-verify/SKILL.md`

```powershell
python tools\aws-verify\verify_telemetry.py --mppt-id <SER#> --mppt-serial <SER#>
```

**通过**: `Checks passed: N/N`；DDB/API/串口 `mac`、`firmware_version`、时间戳一致。

---

## 技能 3 — 远程 OTA

**何时**: 无 USB 升级 · 「推 Job」「TC-OTA」  
**细则**: `.cursor/skills/iqedge-remote-ota/SKILL.md` · 生产禁 `ENABLE_OTA`

```powershell
pio run
python tools\ota\push_ota_job.py --mac "<MAC>" --version <vX.Y.Z> --wait
```

**事后**: 技能 2 复核 `firmware_version`。

---

## 技能 4 — 历史遥测分析

**何时**: 「过去 N 小时数据对不对」、站点号 `IQW-xxxx`、产量/间隔异常 · 输出 **report/**  
**细则**: `.cursor/skills/iqedge-telemetry-analysis/SKILL.md`

```powershell
python tools\aws-verify\analyze_device_window.py --device <SER#> --hours 10
python tools\aws-verify\find_device.py <站点号片段>
```

**交付**: `report/<主题>_Telemetry_Analysis_YYYY-MM-DD.md` + 更新 `report/README.md`  
**注意**: 云端 `deviceId` = MPPT `SER#`；IQW 项目号需先映射（见样例 `report/IQW-9041_…`）。

---

## 文档速链

| 主题 | 路径 |
|------|------|
| **报告目录** | `report/README.md`（审计、遥测分析等） |
| 硬件 HIL | `docs/Hardware_Environment.md` |
| 架构 | `docs/G2_Architecture_Review.md` |
| 云端管线 | `docs/AWS_Cloud_Pipeline.md` |
| 进展 | `docs/development_log.md` |
| AI 红线 | `src/GEMINI_ESP32.md` |

---

*最后更新: 2026-05-29 · Agent 007 · 技能 1–4*
