---
name: iqedge-hil-loop
description: >-
  IQEdge-G2 local HIL workflow: code review, PlatformIO build/flash, serial monitor
  on COM3, minimal fix loop. Use for compile/upload bugs, VE.Direct pins, MQTT/NRG
  issues, or when the user asks for 编译烧录、串口、HIL、修固件、pio run.
---

# IQEdge 本地 HIL 闭环

## 何时使用

- 固件缺陷、引脚/协议、编译失败、MPPT 全 0、MQTT/状态机异常
- 改码后**本机自证**（USB + COM3）
- 远程 OTA / 云端验证**之前**的本地构建与串口证据

## 与其他技能的关系

```text
技能 1（本技能）: 审查 → 编译 → 烧录 → 串口 → 修复
        ↓ 管线/MQTT/云端相关
技能 2 iqedge-triple-verify: 本地(可选) + DDB/TS + API
        ↓ 无 USB 升级
技能 3 iqedge-remote-ota: S3 + IoT Job（仍须 ① 中 pio run 产出 BIN）
```

- **仅 USB 台架**：做到技能 1 即可；要证明上云 → 接技能 2。
- **OTA 前**：技能 3 依赖本技能 ② 的 `firmware.bin`；OTA 后 → 技能 2。

## 工作目录

`d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware`

HIL 默认：**COM3** @ 115200（monitor）/ 921600（upload），VE.Direct **GPIO 16/17**。

## 前置必读（约 30 秒）

| 文档 | 用途 |
|------|------|
| `docs/Hardware_Environment.md` | 接线、MPPT、LOAD 语义 |
| `docs/G2_Architecture_Review.md` | 模块、状态机、风险点 |
| `src/GEMINI_ESP32.md` | LittleFS 禁 format；Wh→kWh **÷1000** |
| `docs/development_log.md` | 最新 Blockers（倒序） |
| `docs/WDT_Stress_Test_Plan.md` | TWDT / P0～P4 |
| `docs/OTA_Stress_Test_Plan.md` | OTA 与 HIL 边界 |

## 闭环流程（按序，不跳步）

```text
① 审查 → ② pio run → ③ upload → ④ 串口 ≥30s → ⑤ 根因 → ⑥ 最小修复 → ⑦ 回到 ②
```

### ① 审查代码

- 对齐 `Config.h` 引脚与 `Hardware_Environment.md`。
- Grep：`setState`、`isMpptConnected`、`_buildPayload`、`VEDIRECT_*`、`LittleFS.begin`。
- **最小 diff**；不扩 scope。

### ② 编译

```powershell
$env:Path = "C:\Users\kevin\.platformio\penv\Scripts;C:\Program Files\Git\cmd;$env:LOCALAPPDATA\Programs\Python\Python312\Scripts;$env:Path"
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"
pio run
```

- 成功：末尾 `[SUCCESS]`，存在 `.pio/build/esp32dev/firmware.bin`。
- 常见：缺 `inject_env.py`、`partitions/*.csv`、Git 未装。

### ③ 烧录

```powershell
pio run -t upload --upload-port COM3
```

- 烧录前关闭占用 COM3 的 Monitor。
- 改 `FIRMWARE_VERSION` 时在此步前于 `Config.h` 递增。

### ④ 串口

```powershell
pio device monitor --port COM3 --baud 115200 --filter direct
# 存档：
pio device monitor --port COM3 --baud 115200 2>&1 | Tee-Object -FilePath debug\serial_capture.txt
```

| 标签 | 关注 |
|------|------|
| `[BOOT]` / `[FS]` | 启动、LittleFS |
| `[NRG]` / `[VED]` | `MPPT RECONNECTED` / `MPPT not responding` |
| `[COM]` / `[SYS]` | WiFi/MQTT/NTP、`NORMAL`/`HIBERNATE` |
| `[TASK]` | 任务启动 |
| `rst:` / `Guru Meditation` | 复位、崩溃 |

采集 **≥30s**（含 NTP、首次 Publish、NRG 周期摘要）。日志只放 `debug/`。

### ⑤ 根因速查

| 现象 | 先查 |
|------|------|
| 电池/光伏全 0 | VE.Direct **16/17**、线缆、MPPT 上电 |
| 误 HIBERNATE/NIGHT | `EnergyManager::evaluatePowerPolicy` |
| MQTT 失败 | NTP、`state=`、LittleFS 证书 |
| Payload 异常 | `CommManager::_buildPayload` 单位与 flat 字段 |

### ⑥ 修复与迭代

- 改码后 **必须** 重走 ②→④。
- 收尾更新 `docs/development_log.md`（倒序一节）。

## 禁止事项

- 禁止 `LittleFS.begin(true)` 格式化生产 FS。
- 禁止未看串口就断定 MPPT 硬件损坏。
- 禁止只改引脚不 **编译+烧录+串口**。

## 交付模板

```markdown
## HIL 闭环 — <简述>

- 根因: …
- 改动: `file1`, `file2`
- 编译: SUCCESS / FAIL
- 烧录: COM3 OK / FAIL
- 串口: `[NRG]` … / `[COM] Publish …` / `FW: v…`
- 下一步: 无 / 技能 2 三位一体 / 技能 3 OTA
```

## 参考

- 项目索引：`SKILL.md`（根目录，仅索引）
- 云端验证：`.cursor/skills/iqedge-triple-verify/SKILL.md`
- 远程 OTA：`.cursor/skills/iqedge-remote-ota/SKILL.md`
