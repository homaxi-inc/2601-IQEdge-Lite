# IQEdge-G2 OTA 风险分析与测试方案

> **固件基准**: v2.2.3.24+ · AWS IoT **Jobs** + `esp_https_ota`  
> **分区**: `app0` / `app1` 各 1.25 MB · LittleFS @ `0x291000`（**OTA 不写此区**）  
> **关联**: [`WDT_Stress_Test_Plan.md`](WDT_Stress_Test_Plan.md) · [`G2_Architecture_Review.md`](G2_Architecture_Review.md)

---

## 1. OTA 在代码里怎么走

```text
MQTT Jobs 消息 ($aws/things/IQEdge_{MAC}/jobs/...)
    → CommManager::_handleJob()
    → _pendingOta = true（下一圈 loop 执行）
    → OtaManager::startUpdate(url, AmazonRootCA1)
         · esp_task_wdt_delete(Task_Comm)   // 通信任务临时退订 TWDT
         · esp_https_ota() 阻塞下载写入「非当前」OTA 槽
         · esp_task_wdt_add(Task_Comm)
    → 成功: Jobs=SUCCEEDED → esp_restart()
    → 失败: Jobs=FAILED，继续跑旧固件
```

| 路径 | 编译开关 | 说明 |
|------|----------|------|
| **AWS IoT Jobs OTA** | 始终启用 | 生产路径；`Config.h` 注释的 `ENABLE_OTA` **不控制**此路径 |
| **ArduinoOTA** | `#ifdef ENABLE_OTA` | 仅 `TaskComm.cpp`；**当前未定义**，开发用 |

证书与 WiFi 配置在 **LittleFS**，与 `app0/app1` 分区分离；**正常 OTA 不会擦证书区**。

---

## 2. 会不会「死机再也起不来」？

### 2.1 等级定义（与 WDT 方案一致）

| 等级 | OTA 相关表现 |
|------|----------------|
| **P0** | 上电无任何启动迹象（无串口 `rst:` / 无灯变化） |
| **P1** | **Boot Loop**：反复重启，无法稳定 `MQTT Connected` |
| **P2** | 能启动但业务永久异常（如 LittleFS 挂载失败 → SOS，MQTT 永不连） |
| **P3** | OTA 失败可恢复；旧固件继续运行 |
| **P4** | OTA 成功，新固件稳定 |

### 2.2 结论摘要

| 问题 | 是否可能 | 是否「永久死机」 |
|------|----------|------------------|
| 坏固件 / 过大镜像 OTA 后 Boot Loop | **可能** | **否**（USB 烧录 `app0` 可恢复） |
| 下载中断（断电/断网） | **可能** | **否**（`otadata` 未切换则仍启动旧槽） |
| OTA 过程中通信停住数分钟 | **可能** | **否**（MPPT/能源任务仍跑；重启恢复） |
| OTA 写坏 **LittleFS** | **极低**（OTA 不写 FS 区） | 除非新固件主动 `LittleFS.begin(true)` |
| 堆耗尽导致 OTA 中途失败 | **可能** | **否**（走 FAILED 分支） |
| TWDT 在 OTA 期间误杀 | **已缓解** | Comm 退订 WDT + `HTTP_EVENT_ON_DATA` 喂狗 |

**没有任何纯软件 OTA 路径会物理损坏 Flash。** 「再也起不来」在 OTA 场景下 ≈ **P1 启动循环** 或 **P2 证书/FS 逻辑错误的新镜像**，均可通过 **USB 烧录固件 + 必要时重灌 LittleFS** 恢复。

---

## 3. 已识别的高风险点（设计层）

### R1 — 坏镜像 / 不兼容镜像写入备用槽并切换 `otadata`

- **现象**：Jobs 报 SUCCEEDED → 重启后新固件崩溃 → **P1 Boot Loop**。
- **根因**：无 **回滚**（未校验启动计数 / 未做 POST-OTA 健康握手再确认槽位）。
- **缓解**：发布前与 `partitions`、目标板、`FIRMWARE_VERSION` 一致；Jobs 仅推 **已测 BIN**。

### R2 — OTA 期间长时间阻塞 `Task_Comm`

- **现象**：数分钟内无 MQTT 上报；LED 可能 **双闪**（MQTT 断）。
- **根因**：`esp_https_ota()` 在 `loop()` 内同步执行。
- **影响**：非死机；云端以为设备离线。

### R3 — 断电发生在写入末期

- **现象**：偶发无法启动或启动旧版（取决于 `otadata` 是否已提交）。
- **风险**：理论上的 **砖** 概率最高阶段（所有双槽 OTA 通用）。
- **测试**：TC-OTA-12 必做。

### R4 — Jobs 版本判断过于宽松

```296:299:d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware\src\managers\CommManager.cpp
        if (urlStr.indexOf(currentVersionStr) > 0) {
            ...
            _mqtt.publish(updateTopic.c_str(), "{\"status\":\"SUCCEEDED\"}");
```

- URL 子串误匹配可能 **跳过** 真 OTA 或误标 SUCCEEDED（逻辑错误，非砖）。

### R5 — 成功路径先报 SUCCEEDED 再 `esp_restart()`

- 若重启后新固件起不来，云端记录仍为 **成功**（运维陷阱，非设备死）。

### R6 — 镜像大于 `0x140000`（1.25 MB）

- 当前 BIN ~1.08 MB，有余量；超限会导致 OTA **FAILED**，旧固件保留。

### R7 — 新固件含 `LittleFS.begin(true)` 或错误分区表

- 可 **P2** 抹证书；与 OTA 传输无关，与 **发布物** 有关。红线见 `GEMINI_ESP32.md`。

---

## 4. 测试用例矩阵

### 4.1 前置条件

| 项 | 要求 |
|----|------|
| 设备 | HIL：`HQ2513NH99U`，12V +（调试时）USB |
| 云端 | AWS IoT Jobs 可下发；S3 预签名 URL 有效 |
| 镜像 A | 当前生产 BIN `v2.2.3.24`（已知良好） |
| 镜像 B | 下一版 **递增版本号** 的合法 BIN（同分区表） |
| 镜像 C | **故意坏** BIN（见 TC-OTA-20，仅实验室） |
| 日志 | `debug/ota_TCxx_*.txt`；禁止 `uploadfs` 除非测 TC-OTA-FS |

---

### 4.2 第一组：安全与恢复（必做）

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-OTA-01** | 同版本 Job | 下发 URL 含当前 `FIRMWARE_VERSION` 的 Job | Jobs **SUCCEEDED**，**无** 重启，MQTT 不停 |
| **TC-OTA-02** | 合法升级 A→B | 下发镜像 B（版本号不同） | 一次重启后 `FW:` 为 B；≤3 min 内 `MQTT Connected` + `Publish OK`；LittleFS 证书仍在 |
| **TC-OTA-03** | 升级后 24h 浸泡 | TC-OTA-02 通过后继续运行 24h | 无 P1；`getMinFreeHeap` 无单调暴跌 |
| **TC-OTA-04** | USB 救砖 | TC-OTA-02 后仅 `pio run -t upload`（不擦 FS） | 可烧回 A 或 B；MQTT 恢复 |

---

### 4.3 第二组：失败与中断（必做）

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-OTA-10** | 错误 URL | Job 内 URL 404/过期 | Jobs **FAILED**；**保持** 版本 A；≤5 min 恢复 Publish |
| **TC-OTA-11** | 下载中断网 | OTA 进行中拔路由器/关 AP 30s | **FAILED** 或超时失败；重启后仍为 A；无 P1 |
| **TC-OTA-12** | 下载中断电 | OTA 进度约 50% 时断 12V 10s 再上电 | 启动 **旧固件 A**（或稳定槽）；无 P0；若 P1 则记缺陷 |
| **TC-OTA-13** | 超大镜像 | 推送 `> 0x140000` 的 BIN（测服） | **FAILED**，旧固件运行 |

---

### 4.4 第三组：砖化与回滚（实验室，谨慎）

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-OTA-20** | 坏镜像 Boot Loop | 推送 **故意崩溃** 的 B'（如 `abort()` on boot） | 预期 **P1**；USB 可烧回 A → **P4** |
| **TC-OTA-21** | 错误分区表镜像 | 推送 **另一分区 CSV 编出** 的 BIN | 预期 **FAILED** 或 USB 可恢复；**不得** 唯一槽位 P0 |
| **TC-OTA-22** | 连续两次 OTA | A→B→A（合法） | 两次均最终可 Publish；`otadata` 切换正常 |

---

### 4.5 第四组：并发与观测

| ID | 名称 | 步骤 | 通过标准 |
|----|------|------|----------|
| **TC-OTA-30** | OTA 中 MPPT | OTA 进行时观察 VE.Direct | `[NRG]` 仍更新；能源任务不受阻 |
| **TC-OTA-31** | OTA 时长与 LED | 记录 OTA 总时长 | 有界（如 <10 min）；LED 双闪可接受 |
| **TC-OTA-32** | 重复 Job 轰炸 | 连续下发 3 个 Job | 不重复 `_pendingOta` 错乱；最终状态一致 |
| **TC-OTA-33** | 云端 DDB | OTA 成功后查 `HQ2513NH99U` | `firmware_version` 与 B 一致 |

---

## 5. 观测关键字

```text
[OTA] Initializing update from:
[OTA] Update completed successfully. / Update failed with error:
[COM] OTA Job Received / for current version / Marked SUCCEEDED|FAILED
rst:0x1 (POWERON) / rst:0xc (SW_CPU_RESET)
[FS] LittleFS mounted OK          ← 升级后必须有
[FS] FATAL: LittleFS mount failed ← P2 红旗
[COM] MQTT Connected / Publish OK
Guru Meditation / boot loop (无间隔 FW 横幅)
```

---

## 6. 结果记录模板

```markdown
### TC-OTA-xx
- 镜像 A/B 版本、JobId、日期
- 结果: PASS / FAIL (P0|P1|P2|P3|P4)
- 断电/断网时机:
- 恢复方式: 自恢复 / 软重启 / USB 烧录 / 重灌 FS
```

---

## 7. 发布与架构建议（降低 OTA 死机概率）

| 建议 | 优先级 |
|------|--------|
| Jobs 仅允许 **已 HIL 验证** 的 BIN + 显式 `FIRMWARE_VERSION` | 高 |
| 增加 **启动健康检查**（启动后 N 分钟内 MQTT 成功否则 `esp_ota_mark_app_invalid_cancel_rollback()`） | 高 |
| OTA 下载改 **独立任务** + 进度上报，避免 Comm `loop` 长期阻塞 | 中 |
| 严格化版本比较（解析 Job document 字段，非 URL `indexOf`） | 中 |
| 量产启用 **Secure Boot + 签名 OTA**（`Config.h` 路线图） | 长期 |

---

## 8. 推荐执行顺序

1. **TC-OTA-01** → **TC-OTA-02** → **TC-OTA-04**（确认主路径 + USB 可救）  
2. **TC-OTA-10** → **TC-OTA-12**（失败与断电）  
3. 有测服环境再做 **TC-OTA-20**  
4. 现场部署前 **TC-OTA-03**（24h）

---

*文档版本: 2026-05-29 · 对齐分区 `default_4MB_littlefs.csv`*
