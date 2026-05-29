# IQEdge-G2 任务看门狗（TWDT）压力测试方案

> **固件基准**: `v2.2.3.24+` · `WDT_TIMEOUT_SEC = 30` · `esp_task_wdt_init(30, true)`（超时 **panic → 复位**）  
> **HIL**: COM3 @ 115200 · MPPT `HQ2513NH99U` · ESP32 MAC `EC:E3:34:1A:F9:8C`  
> **日志**: 一律写入 `debug/`，见 [`debug/README.md`](../debug/README.md)

---

## 1. 测试目标与「死机」定义

### 1.1 目标

| # | 目标 |
|---|------|
| G1 | 验证 TWDT 在各类阻塞/高负载下是否 **可预期地复位**，而非挂死无串口 |
| G2 | 验证 **反复 WDT 复位** 后设备仍能进入正常业务（MQTT Publish、MPPT 数据） |
| G3 | 排除 **永久性失效**（见下表） |
| G4 | 回归已修复路径（历史上 `Task_Comm` 长时间阻塞触发 WDT） |

### 1.2 「再也起不来」判定（必须区分）

| 等级 | 现象 | 是否算测试失败 | 典型原因 |
|------|------|----------------|----------|
| **P0 真死机** | 断电再上电后 **无任何串口**（无 `rst:` 行） | **失败** | 硬件/供电/Flash 损坏（极少见） |
| **P1 软死机** | 有串口但 **持续 Boot Loop**（5 min 内 ≥10 次 `FW:` 行且无 `MQTT Connected`） | **失败** | 启动即崩溃、证书/NVS 损坏、重复 panic |
| **P2 业务死** | 能启动但 **>10 min** 无 `Publish OK` 且 WiFi/MPPT 正常 | **失败** | 逻辑卡死未触发 WDT（未注册任务阻塞） |
| **P3 可恢复** | 出现 `task_wdt` / `rst:0xc` 后 **≤2 min** 内恢复 `Publish OK` | **通过** | 设计内的 TWDT 恢复 |
| **P4 预期复位** | 注入阻塞后 **~30s** 出现 WDT 并重启 | **通过**（压力项） | 压力钩子生效 |

**说明**: TWDT panic 后 **应** 看到 `rst:0xc (SW_CPU_RESET)` 或 `rst:0x10` 等软件复位，**不是** Flash 磨损坏。

---

## 2. 被测系统（TWDT 订阅一览）

```text
esp_task_wdt_init(30, panic=true)
├── Arduino loop()          [Core 1]  esp_task_wdt_reset 每 1s
├── Task_Comm               [Core 1]  loop 入口 + publish/MQTT 路径
├── Task_Energy             [Core 0]  每 100ms
├── Task_Deterrence         [Core 1]  每 20ms
├── Task_BLE / Task_GPS     默认关闭 (GPS_ENABLED=false, BLE_POC_ENABLED=false)
└── OTA 路径                临时 esp_task_wdt_delete(当前任务)
```

**关键代码**:

- `Config.h`: `WDT_TIMEOUT_SEC = 30`
- `main.cpp`: 初始化 + `loop()` 喂狗
- `TaskComm.cpp` / `TaskEnergy.cpp` / `TaskDeterrence.cpp`: 各任务注册并周期 `esp_task_wdt_reset()`

**已知历史缺陷（已修）**: `Task_Comm` 在单次 `loop()` 内阻塞 >30s（如 HTTP 对账）→ 整任务未喂狗 → panic。v2.2.3.24 已移除固件 API 调用。

---

## 3. 测试环境准备

| 项 | 要求 |
|----|------|
| 固件 | 生产配置烧录一次即可；**压力注入**需 `env:wdt_stress`（见 §7） |
| 串口 | COM3，115200，测试期间勿占用 Monitor |
| 网络 | 用例 A/B 需 IQWatch WiFi；用例 C/D 可断网 |
| 采集 | 每项测试独立日志 `debug/wdt_TCxx_YYYYMMDD_HHMM.txt` |
| 解析 | `python tools/wdt-stress/parse_wdt_log.py <logfile>` |

```powershell
cd 01-firmware
$env:PYTHONIOENCODING = 'utf-8'
pio device monitor --port COM3 --baud 115200 2>&1 | Tee-Object -FilePath debug\wdt_TC01_baseline.txt
```

---

## 4. 测试用例矩阵

### 4.1 第一组：无注入（生产固件）

| ID | 名称 | 步骤 | 时长 | 通过标准 |
|----|------|------|------|----------|
| **TC-01** | 稳态浸泡 | 正常供电，MPPT+WiFi 在线，仅串口采集 | **4 h** | 无 P0/P1；WDT 次数 **0** 或偶发 ≤2 且均 P3；`Publish OK` 间隔 ≤6 min（夜间模式可能 30 min） |
| **TC-02** | 快速启停 | 断电 5s → 上电，重复 | **50 次** | 每次 90s 内出现 `MQTT Connected` + 至少 1 次 `Publish OK`；无 P1 |
| **TC-03** | WiFi 抖动 | 测试中用 Faraday 袋/关 AP 每 2 min 断连 1 min | **1 h** | 允许 `DOUBLE_BLINK`；恢复后 3 min 内 `Publish OK`；无 P1 |
| **TC-04** | MPPT 拔线 | 运行中拔 VE.Direct 2 min 再插回 | **30 min** 窗口内执行 5 轮 | 允许 `TRIPLE_BLINK`；**不得** WDT（Energy/Comm 不应 >30s 阻塞）；插回后 `MPPT RECONNECTED` |
| **TC-05** | 串口洪水 | 打开 Monitor，不向设备发命令 | **2 h** | 与 TC-01 相同；验证 Serial 处理不拖死 `Task_Comm` |

### 4.2 第二组：WDT 复位耐力（无注入）

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-10** | 自然 WDT 计数 | 跑 TC-01 日志，统计 `task_wdt` / `rst:0xc` | 记录基线；若 >10 次/h → 开缺陷单 |
| **TC-11** | 复位后业务 | 每次观察到 WDT 后，等 3 min，查是否 `Publish OK` | 100% 可恢复为 P3 |
| **TC-12** | 冷启动 after WDT | WDT 发生后断电 10s 再上电 | 单次启动必须 P3，不得 P1 |

### 4.3 第三组：压力注入（需 `wdt_stress` 构建，§7）

> 目的：**主动** 让某订阅任务停止喂狗，确认 panic 机制与恢复，而非等现场偶发。

| ID | 注入点 | 钩子行为 | 预期现象 | 通过标准 |
|----|--------|----------|----------|----------|
| **TC-20** | `Task_Comm` | `publishStatus` 入口 `delay(35000)` 且不喂狗 | ~30s 后 `task_wdt: Task_Comm`，`rst:0xc` | 重启后 3 min 内 P3；**不得** P1 |
| **TC-21** | `Task_Comm` | `_ensureMqtt` 内 `connect` 前阻塞 35s（模拟 SSL 卡死） | 同上 | 同上 |
| **TC-22** | `Task_Energy` | `poll()` 前阻塞 35s | `task_wdt: Task_Energy` | 同上 |
| **TC-23** | `loop()` | 注释 `esp_task_wdt_reset` 或 `delay(35000)` | 可能报 `loopTask` 或 IDLE 相关 | 记录实际任务名；仍须 P3 |
| **TC-24** | 连续注入 | TC-20 连续触发 **20 次**（脚本断电或等自动重启） | 20 次均 P3/P4 | **不得** P0/P1；Flash 仅 app 区擦除（见 Flash 评估文档） |

### 4.4 第四组：边界与回归

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-30** | NTP 最长等待 | 断网启动，观察 NTP 30 次 × 1s | 仅见 `[COM] Waiting for NTP`，**无** WDT；联网后恢复 |
| **TC-31** | 大 Payload Publish | 正常发布，确认 ~800B payload | `Publish OK`；无 WDT |
| **TC-32** | API 回归 | 用 **v2.2.3.22**（含 HTTP）烧录 1 次对比 | 应复现 `Task_Comm` WDT（对照组）；再烧 **v2.2.3.24** 应消失 |
| **TC-33** | OTA Job 模拟 | （仅维护窗口）下发 Jobs 或跳过 | OTA 期间 Comm 任务临时退订 WDT，**不得** 导致 P1；完成后恢复 |

---

## 5. 观测清单（串口关键字）

测试人员 / 脚本按时间线扫描：

```text
[通过]  FW: v2.2.3.24
[通过]  [BOOT] All tasks launched
[通过]  [COM] MQTT Connected
[通过]  [COM] Publish OK
[通过]  [NRG] MPPT RECONNECTED

[WDT]   E (xxxxx) task_wdt: Task watchdog got triggered
[WDT]   task_wdt:  - Task_Comm (CPU 1)    ← 记录任务名
[复位]  rst:0xc (SW_CPU_RESET)
[失败]  Guru Meditation / Brownout / rst:0x0 (POWERON) 循环无业务输出
[失败]  [FS] FATAL: LittleFS mount failed  → EMERGENCY SOS，属 P2
```

---

## 6. 结果记录模板

每项测试填写：

```markdown
### TC-xx — <名称>
- 日期 / 固件 / 操作人
- 日志: debug/wdt_TCxx_....txt
- WDT 次数: __  复位次数: __  Publish OK 次数: __
- 最高严重度: P0 / P1 / P2 / P3 / P4
- 结论: PASS / FAIL
- 备注:
```

汇总表建议放在 `docs/development_log.md` 或单独 `debug/wdt_summary.md`（gitignore）。

---

## 7. 压力注入构建（可选实现）

在 `platformio.ini` 增加 **仅 HIL 使用** 的环境（勿用于量产）：

```ini
[env:wdt_stress]
extends = env:esp32dev
build_flags =
    ${env:esp32dev.build_flags}
    -DWDT_STRESS_TEST=1
```

固件内用 `#ifdef WDT_STRESS_TEST` 在下列位置插入 **可串口触发** 的阻塞（避免误触发生产）：

| 宏 | 行为 |
|----|------|
| `WDT_STRESS_BLOCK_COMM_PUBLISH` | `publishStatus()` 开头 `vTaskDelay(35000)` 且 **不** 调用 `esp_task_wdt_reset` |
| `WDT_STRESS_BLOCK_COMM_MQTT` | `_ensureMqtt()` connect 前同上 |
| `WDT_STRESS_BLOCK_ENERGY` | `taskEnergyEntry` 循环内 `poll` 前同上 |

串口触发示例（实现时）: `SET WDT STRESS COMM` / `SET WDT STRESS OFF`。

**未实现 `-DWDT_STRESS_TEST` 时**: 第三组用例 TC-20～24 **跳过**，第一、二、四组仍可执行。

---

## 8. 自动化辅助

```powershell
# 采集 1 小时并解析
pio device monitor --port COM3 --baud 115200 2>&1 | Tee-Object debug\wdt_TC01_run.txt
python tools/wdt-stress/parse_wdt_log.py debug\wdt_TC01_run.txt
```

解析器输出：WDT 次数、复位类型统计、Boot 次数、`Publish OK` 间隔、是否疑似 P1。

---

## 9. 风险与注意事项

| 风险 | 缓解 |
|------|------|
| 反复 panic 复位 | 仅 **TC-24** 做 20 连击；日常用 TC-01/02；**不要** 每次测试 `uploadfs` |
| 对照组 v2.2.3.22 | TC-32 会故意 WDT，烧回 v2.2.3.24 结束 |
| 未注册 WDT 的死锁 | 若 TC-01 出现 P2（无 WDT 无输出），查是否有任务未 `esp_task_wdt_add` 却长期占满 CPU |
| 与 Flash 关系 | WDT 复位 **不写 Flash**；仅 USB 烧录擦 app 区。见 Flash 擦写评估 |

---

## 10. 推荐执行顺序（最小集）

1. **TC-02**（50 次冷启动）— 30 min，快速排除 P1  
2. **TC-01**（4 h 浸泡）— 过夜，建立基线  
3. **TC-04 / TC-03** — 各 1 h，验证断链不触发 WDT  
4. 实现 `wdt_stress` 后跑 **TC-20 + TC-24** — 验证「WDT 后仍能起来」  
5. **TC-32** — 回归对照（可选）

---

*文档版本: 2026-05-29 · 对齐固件 v2.2.3.24*
