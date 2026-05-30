# IQEdge-G2 联合开发特工日志 (Development Log)

> **维护者**: Agent 007  
> **更新规则**: 倒序排列（最新日期在最上）；每完成核心任务 / 修复严重 Bug / 每日收尾时追加一节。  
> **关联文档**: [`report/`](../report/README.md) · [`G2_Architecture_Review.md`](G2_Architecture_Review.md) · [`Hardware_Environment.md`](Hardware_Environment.md) · [`AWS_Cloud_Pipeline.md`](AWS_Cloud_Pipeline.md) · [`../SKILL.md`](../SKILL.md) · [`../src/GEMINI_ESP32.md`](../src/GEMINI_ESP32.md)

---

## 📅 [2026-05-29] - G2 固件起线 v2.3.0（008 → 007 · Bob 定稿）

- **ADR-008** — `track=g2` + 固件 **≥ v2.3.0** 才写 G2 Timestream；**晋升 g2 仅人工**；`track=g2` 永不回退。
- **台架 HQ2513A69PJ** — 现网 **v2.2.3.25**；下一版 OTA 目标 **≥ v2.3.0** + G2 Topic + `firmware_version` 字段。
- **必读** — [`G2_HIL_007_Firmware_Requirements.md`](G2_HIL_007_Firmware_Requirements.md) §3.0 · [`09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md`](../../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) §2 · [`04-cloud/docs/G2_Registry_Track_Assignment_SOP.md`](../../04-cloud/docs/G2_Registry_Track_Assignment_SOP.md)

---

## 📅 [2026-05-29] - 技能 4：历史遥测分析

- **`.cursor/skills/iqedge-telemetry-analysis/`** — 时间窗拉取（Timestream/DDB/API）、IQW↔SER# 映射、异常规则、**report/** 报告模板；与技能 2 分工（快照 vs 趋势）。
- **工具** — `analyze_device_window.py`、`find_device.py`；样例 `report/IQW-9041_Telemetry_Analysis_2026-05-29.md`。
- **`SKILL.md`** — 技能索引增至 4 项。

---

## 📅 [2026-05-30] - IQ-26-00001 OTA v2.3.0 + 008 通知

- **OTA** — Job `IQW-OTA-HQ2513A69PJ-v230-20260530-125800` **SUCCEEDED**；`HQ2513A69PJ` → **v2.3.0**。
- **Legacy** — `verify_telemetry.py` **3/3** @ 2026-05-30 19:59:04 UTC；`device/status` → AWS IoT 正常。
- **G2** — `verify_g2_telemetry.py` 未过（待 008 M4 + 固件 G2 Topic）；见 `report/NOTICE_007_to_008_*.md`。
- **Config.h** — `FIRMWARE_VERSION` = `v2.3.0`（G2 起线，ADR-008）。

---

## 📅 [2026-05-29] - 报告目录 `report/`

- 审计、遥测分析等一次性报告迁入 **`report/`**（见 `report/README.md` 索引）。
- 自 `docs/` 迁出：`PreProduction_Audit_Report_2026-05-29.md`、`IQW-9041_Telemetry_Analysis_2026-05-29.md`。

---

## 📅 [2026-05-29] - 生产前全盘审计

- **`report/PreProduction_Audit_Report_2026-05-29.md`** — 构建/安全/云端/OTA/HIL/测试/文档七域审计；裁决 **有条件批准（Conditional GO）**。
- **通过** — 云端管线、Jobs OTA（`HQ2513A69PJ`）、`ENABLE_OTA` 关闭、LittleFS/÷1000 红线、技能与工具链就绪。
- **P1 待闭合** — 量产 WiFi/调试宏硬化、`[env:production]`、OTA 回滚策略、WDT/OTA 压力矩阵与 24h 浸泡。
- **产线清单** — 报告 §6 可复制签收（证书、Thing、三位一体、OTA 追溯）。

---

## 📅 [2026-05-29] - Agent 技能扩展（三位一体 + 远程 OTA）

- **`.cursor/skills/iqedge-triple-verify/`** — 本地串口 + DynamoDB/Timestream + IQWatch API 交叉验证流程、失败处置表、交付模板。
- **`.cursor/skills/iqedge-remote-ota/`** — S3 `iqwatch-firmware-ota`、IoT Job 下发、`push_ota_job`/`monitor_ota`、TC-OTA-02、`ENABLE_OTA` 合规红线。
- **`.cursor/skills/iqedge-hil-loop/`** — 技能 1 细则外迁；与技能 2/3 串联说明（`1→2`、`1→3→2`）。
- **根目录 `SKILL.md`** — 精简为索引 + 入口命令；三技能统一 Cursor Skill 路径。

---

## 📅 [2026-05-29] - 远程 OTA 实测通过 (HQ2513A69PJ)

- **IAM** — `Agent007OTAPolicy` 挂载 `Agent-007`；S3 `iqwatch-firmware-ota` 上传 + IoT Job 下发。
- **Job** — `IQW-OTA-HQ2513A69PJ-20260529` → Thing `IQEdge_1C:69:20:B8:D7:F4`；约 64s **SUCCEEDED**。
- **版本** — `v2.2.3.20` → **v2.2.3.25**；DDB/API `last_reported` **2026-05-29 00:57:34 UTC**；遥测恢复（TC-OTA-02）。
- **产物** — `s3://iqwatch-firmware-ota/firmware_v2.2.3.25.bin`；工具 `tools/ota/push_ota_job.py`、`monitor_ota.py`。
- **合规** — 生产镜像保持 `ENABLE_OTA` 注释；仅 AWS IoT Jobs 路径 OTA（非 ArduinoOTA）。

---

## 📅 [2026-05-29] - OTA 风险分析与测试方案

- **docs/OTA_Stress_Test_Plan.md** — AWS IoT Jobs + `esp_https_ota` 路径；P0～P4 与 WDT 方案对齐；TC-OTA-01～33（合法升级/断网断电/坏镜像救砖）。
- **结论** — OTA **可能** P1 Boot Loop（坏镜像），**不会**物理毁 Flash；LittleFS/证书区与 app 分区分离；Comm 在 OTA 期间阻塞、TWDT 对 Task_Comm 退订。

---

## 📅 [2026-05-29] - TWDT 压力测试方案

- **docs/WDT_Stress_Test_Plan.md** — 用例 TC-01～TC-33：浸泡/冷启动/断网/MPPT 拔线/WDT 耐力/压力注入/回归；P0～P4 死机定义。
- **tools/wdt-stress/parse_wdt_log.py** — 串口日志统计 WDT、复位、`Publish OK`，输出严重度提示。
- **待做**: `platformio.ini` → `[env:wdt_stress]` + 串口触发阻塞钩子（TC-20～24）。

---

## 📅 [2026-05-28] - 移除固件 API 调用 (v2.2.3.24)

### 分析结论

| 路径 | 职责 | 固件是否需要 |
|------|------|--------------|
| MQTT `device/status` | 写入 DynamoDB + Timestream | **是**（唯一上传通道） |
| API Gateway GET `/devices/{sn}/status` | 运维/脚本**读取**云端快照 | **否** |

原 `_fetchCloudStatus()` 仅用于：拉云端 `total_yield_kwh` → 比对本地 → 可选 ledger MQTT 回补。每次 MQTT payload 已含 H19/H20 转 `total_yield_kwh`/`today_yield_kwh`，Lambda 会更新 DDB；设备回读 API 冗余，且引发 WDT/第二路 TLS 失败。

### 删除项

- `CommManager::_fetchCloudStatus`、`_cloudSyncTick`、`HTTPClient`
- `SystemContext` 云端对账状态（`isSyncedWithCloud` 等）
- `inject_env.py` 向固件注入 `IQWATCH_API_KEY`（`.env` 仍供 `tools/aws-verify`）

---

## 📅 [2026-05-28] - 云端写入管线修复 (v2.2.3.22)

### 根因与修复

| 问题 | 修复 |
|------|------|
| MQTT payload 仅 v2 嵌套 JSON，Lambda `SaveDeviceStatus` 读不到 `deviceId`/扁平度量 | `CommManager::_buildPayload` 增加 v1 扁平字段 + `YYYY-MM-DD HH:MM:SS UTC` 时间戳 |
| 无 `IQWATCH_API_KEY` 仍 `HTTP GET` 对账，阻塞 >30s → `Task_Comm` WDT 重启 | 无 Key 跳过 `_fetchCloudStatus`；Comm/loop 全程 `esp_task_wdt_reset` |
| 紧急发布在 MQTT 未连时被 consume 丢失 | 发布失败时 `requestUrgentPublish()`；MQTT 连上立即 `publishStatus()` |
| MPPT 序列号就绪后未触发上报 | `EnergyManager` 首次有效 `SER#` → `requestUrgentPublish()` |

### 验证

- 串口：`[COM] Publishing payload (827 bytes)` → `Publish OK`，**单轮启动无 WDT 重启**。
- DynamoDB `HQ2513NH99U`：`last_reported` **2026-05-28 23:40:57 UTC**（原停滞 2026-03-20）。
- Timestream：同时间戳新写入 `soc` / `battery_voltage` / `load_power` 等。

---

## 📅 [2026-05-28] - 今日特工大本营进展汇总

### 🟢 1. 今日已物理重构/完成的内容

- **docs/G2_Architecture_Review.md** — 全盘扫描 `src/` 后固化 G2 架构：模块拓扑、搬迁风险点、状态机与启动流。
- **docs/Hardware_Environment.md** — 固化 HIL 沙箱物理拓扑（MPPT 75/15 · 12V 20Ah · IQEdge · VE.Direct · USB 调试）；**VE.Direct 引脚确认为 GPIO 16/17**。
- **partitions/** — 从 GitHub `homaxi-inc/2601-IQEdge-Lite` 同步 `default_4MB_littlefs.csv`、`huge_app.csv`（本地 zip 拉取，无 git CLI）。
- **inject_env.py** — 新增 PlatformIO 预构建占位脚本（`platformio.ini` 引用，避免缺文件导致构建失败）。
- **.cursorrules** — 于 `01-firmware/` 根目录创建空规则文件，供后续联合开发约束逐步写入。
- **开发环境** — 安装 Python 3.12、PlatformIO Core 6.1.19、Git 2.54；Cursor 安装 `platformio.platformio-ide`、`ms-vscode.cpptools`；CP2102 驱动（COM3）。
- **platformio.ini** — 固定 `monitor_port` / `upload_port` = COM3；`.vscode/settings.json` 配置 PIO 自定义 PATH。
- **src/config/Config.h** — `VEDIRECT_RX/TX_PIN` 对齐现场 **16/17**；固件版本递增至 **v2.2.3.21**。
- **src/managers/EnergyManager.cpp/.h** — 修复无 MPPT 数据时误进 `HIBERNATE`；夜间模式计数改为每 5s 递增（原 100ms 循环导致约 18s 误判「15 分钟夜间」）；MPPT 未连接时保持 `NORMAL` 并打印 WARN。
- **src/tasks/TaskEnergy.cpp** — 移除启动期 120s VE.Direct 轮询推迟逻辑，避免 MQTT 建连期间长时间不读 MPPT。
- **烧录验证** — 16/17 引脚修复后出现 `[NRG] MPPT RECONNECTED`，电池约 13.21V、负载约 1.32W 可读。
- **AWS 验证环境** — `.env` / `tools/aws-verify/verify_telemetry.py`；IAM `Agent-007` @ Account `661631955220`。
- **docs/AWS_Cloud_Pipeline.md** — 固化管线：`device/status` → `DeviceStatusToLambda` (SQL **2015-10-08**) → `SaveDeviceStatus` → DDB + Timestream。
- **云端 DDB 验证** — `DeviceLatestStatus` / `deviceId=HQ2513NH99U` 命中，MAC 与 HIL 一致；**last_reported 停滞于 2026-03-20**（与当日 MQTT 实时数据不同步）。

### 🛠️ 2. 当前编译与健康状态 (HIL & Build Status)

- **本地 PlatformIO 编译状态**: **成功**（`pio run`，env: `esp32dev`，约 35–52s）
- **烧录状态**: **成功**（`pio run -t upload --upload-port COM3`）
- **HIL 串口**: COM3 @ 115200；CP210x 驱动 v6.7.6.2130；VE.Direct **已连通**（16/17）
- **0 Error / 0 Warning 达成率**: **编译 0 Error**；第三方库 `VeDirectFrameHandler` 仍有 `-Wwrite-strings` **Warning**（非本项目源码，未清零 Warning）
- **AWS 云端**: DynamoDB **可读**；Timestream **无 Select 权限**；API `1y9689tax0` 无 Key 时 **403**

### 🛑 3. 挂起的问题与技术卡点 (Blockers & Todo)

- ~~**DynamoDB 快照滞后**~~ — **已解决**（v2.2.3.22 扁平 payload + WDT/HTTP 修复，见上节）。
- ~~**Timestream 无新点**~~ — **已恢复写入**（2026-05-28 23:40:57 UTC）。
- ~~**IQWATCH_API_KEY**~~ — 已配置于 `.env`；`verify_telemetry.py` API egress **3/3 通过**。
- ~~**inject_env.py** no-op~~ — 已从 `.env` 注入 `IQWATCH_API_KEY` 至 `CPPDEFINES`。
- **MQTT 偶发 `state=-4`** — 烧录后曾见连接失败并重试；需 Bob 确认是否为 WiFi 信号、并发建连或证书链时序问题。
- **LittleFS `wifi.json`** — 首次启动 VFS 报错 `open(): /littlefs/config/wifi.json does not exist`（逻辑上走默认 `Config.h` WiFi，日志噪音待收敛）。
- ~~**固件 API 对账**~~ — 已移除；`IQWATCH_API_KEY` 仅用于主机 `verify_telemetry.py`。
- **GPS / BLE** — `GPS_ENABLED=false`、`BLE_POC_ENABLED=false`；模块代码在库内但未在 HIL 启用。
- **架构债** — 头文件内大量 `../` include、部分状态（`CONSERVE`/`HIBERNATE`）仅写上下文无硬件动作（见架构审查 §2/§3）。

### 🚀 4. 明日首要攻坚目标 (Next Steps)

- Bob 侧：核对 **DeviceStatusToLambda** SQL 版本 = `2015-10-08`，查 **SaveDeviceStatus** 日志；补 **Timestream Select** + **IQWATCH_API_KEY**。
- 007：Timestream 权限到位后重跑 `verify_telemetry.py`；对比串口 payload 与 DDB/Timestream 时间戳。
- 在 Monitor 中确认 **MQTT 稳定连接** 与 **device/status** Payload 字段（电池/光伏/负载）与 MPPT 现场一致。
- 若 Bob 确认对账策略：清洗 **CommManager::_buildPayload** 与云端 reconciliation 路径，对照 `payload.md` v2。
- 将今日引脚与 HIL 结论 **同步写入 `.cursorrules`**（VE.Direct 16/17、禁止 LittleFS auto-format、Wh→kWh ÷1000）。
- 视需要更新 **G2_Architecture_Review.md** 中「partitions 缺失 / 引脚漂移」条目为已解决状态。

---

<!-- 以下为历史日志，新条目请插入本行上方 -->
