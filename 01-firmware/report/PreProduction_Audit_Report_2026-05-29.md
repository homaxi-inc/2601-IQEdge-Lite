# IQEdge-G2 生产前全盘审计报告

| 项 | 值 |
|----|-----|
| **审计日期** | 2026-05-29 |
| **审计方** | Agent 007 |
| **固件基准** | `v2.2.3.25` · `01-firmware` · env `esp32dev` |
| **分区表** | `partitions/default_4MB_littlefs.csv` |
| **关联** | [`development_log.md`](../docs/development_log.md) · [`G2_Architecture_Review.md`](../docs/G2_Architecture_Review.md) · [`OTA_Stress_Test_Plan.md`](../docs/OTA_Stress_Test_Plan.md) · [`WDT_Stress_Test_Plan.md`](../docs/WDT_Stress_Test_Plan.md) |

---

## 1. 执行摘要

| 维度 | 结论 | 说明 |
|------|------|------|
| **云端数据管线** | ✅ 通过 | 扁平 payload + `deviceId` 门控；HIL/现场设备已验证 DDB + Timestream + API |
| **远程 OTA（Jobs）** | ✅ 通过（抽样） | `HQ2513A69PJ` TC-OTA-02：v2.2.3.20→v2.2.3.25，Job SUCCEEDED，遥测恢复 |
| **本地 HIL 闭环** | ✅ 通过 | VE.Direct 16/17、MPPT 采集、MQTT Publish、WDT 单轮启动无循环重启（v2.2.3.22+） |
| **生产镜像合规** | ⚠️ 有条件 | `ENABLE_OTA` 未启用（ArduinoOTA 关闭）；**调试宏与默认 WiFi 明文仍适合 HIL，量产需硬化** |
| **压力/浸泡测试** | ⚠️ 未完成 | WDT/OTA 方案已文档化；多数 TC 未执行或仅部分执行 |
| **总体裁决** | **有条件批准（Conditional GO）** | 可发布 **限定批次/已Provisioning 现场**；全量量产前须闭合 §6 P1 项 |

**一句话**：核心上报与 Jobs OTA 主路径已达生产可用水位；安全硬化与测试矩阵未闭环，不建议在无Provisioning、无回滚策略的情况下做大规模盲推。

---

## 2. 审计范围与方法

### 2.1 范围

- 源码：`src/`（配置、通信、能源、存储、OTA、任务）
- 构建：`platformio.ini`、`inject_env.py`、`partitions/`
- 云端：`device/status` → Lambda → DDB/Timestream（文档与脚本交叉验证）
- 运维：`.cursor/skills/`、`tools/aws-verify`、`tools/ota`
- 密钥：`.gitignore`、固件内是否含 API Key / 证书明文

### 2.2 方法

- 静态代码与配置审查（Grep + 关键路径阅读）
- 本地 `pio run` 编译与 Flash/RAM 占用
- 对照 `GEMINI_ESP32.md` 红线与 `development_log.md` 历史缺陷
- 引用已完成的 HIL/OTA/云端验证记录（2026-05-28～29）

### 2.3 不在本次范围

- Lambda `SaveDeviceStatus` / IoT Rule SQL 的 AWS 控制台逐项复核（需 Bob）
- Secure Boot V2 / 签名 OTA（路线图项）
- 全 fleet 设备逐一烧录与 24h 浸泡

---

## 3. 审计矩阵（按域）

### 3.1 构建与发布

| 检查项 | 状态 | 证据 |
|--------|------|------|
| Release 编译 | ✅ | `pio run` → `[SUCCESS]`；Flash **80.3%**（1,051,893 / 1,310,720 B）；RAM **15.4%** |
| OTA 槽容量 | ✅ | `app0`/`app1` 各 `0x140000`；当前 BIN ~1.06 MB，余量 ~190 KB |
| 分区与 LittleFS | ✅ | 证书区 `0x291000` 与 app 分离；OTA 不写 FS 区（设计 + `OTA_Stress_Test_Plan`） |
| `inject_env.py` | ✅ | 仅日志；**不**向固件注入 `IQWATCH_API_KEY` |
| 版本标识 | ✅ | `FIRMWARE_VERSION` = `v2.2.3.25`；S3 已存在 `firmware_v2.2.3.25.bin` |
| 第三方 Warning | ⚠️ | `VeDirectFrameHandler` `-Wwrite-strings`（非本项目，可接受） |

### 3.2 安全与密钥

| 检查项 | 状态 | 证据 |
|--------|------|------|
| `.env` 不入库 | ✅ | `.gitignore` 含 `.env` |
| 固件无 API Key | ✅ | 已移除 `_fetchCloudStatus` / `HTTPClient`；`Config.h` 注释说明 API 仅主机工具 |
| LittleFS 禁自动格式化 | ✅ | `StorageManager`: `LittleFS.begin(false)` |
| `ENABLE_OTA`（ArduinoOTA） | ✅ | **未定义**；`TaskComm.cpp` 中 ArduinoOTA 块不编译 |
| **默认 WiFi 明文** | 🔴 P1 | `Config.h`: `WIFI_SSID` / `WIFI_PASSWORD` 硬编码；量产应依赖 LittleFS `wifi.json` 或产线烧录 |
| **串口 `SET WIFI`** | 🟡 P2 | `CommManager::loop` 接受明文改 WiFi；USB 暴露面需现场策略 |
| **调试构建标志** | 🟡 P1 | `DEBUG_MODE true`；`CORE_DEBUG_LEVEL=3`、`DEBUG_ESP_WIFI` — 量产建议独立 `[env:production]` |
| OTA 密码常量 | ✅ N/A | `OTA_PASSWORD` 仅在 `#ifdef ENABLE_OTA` 内（当前未启用） |
| Jobs OTA 路径 | ⚠️ | 始终启用 HTTPS OTA；无 Secure Boot；依赖镜像质量 + USB 救砖 |

### 3.3 云端数据管线

| 检查项 | 状态 | 证据 |
|--------|------|------|
| MQTT 单写通道 | ✅ | 仅 `device/status`；无固件侧 HTTP 回读 |
| Flat v1 字段 | ✅ | `deviceId`、`soc`、`battery_voltage`、`solar_power`、`total_yield_kwh`、`timestamp` 等 |
| Wh→kWh | ✅ | `/1000.0`（`GEMINI_ESP32` 10× bug 已修复） |
| 发布门控 | ✅ | `SER#` 长度 &lt;3 或 MPPT 断开 → SKIP publish |
| MPPT 就绪紧急发布 | ✅ | `EnergyManager` 首次有效序列号 → `requestUrgentPublish` |
| WDT 与 Comm | ✅ | `esp_task_wdt_reset` 于 `loop`/`publish`；`main` loop 任务已加 TWDT |
| HIL 验证 `HQ2513NH99U` | ✅ | DDB 恢复至 2026-05-28 23:40+ UTC |
| 现场验证 `HQ2513A69PJ` | ✅ | OTA 后 `firmware_version` v2.2.3.25 @ 2026-05-29 00:57:34 UTC |
| IoT Rule SQL 版本 | ⚠️ | 文档要求 `2015-10-08` — **需 Bob 控制台确认** |

### 3.4 远程 OTA（AWS IoT Jobs）

| 检查项 | 状态 | 证据 |
|--------|------|------|
| Job 文档格式 | ✅ | `{"url":"<presigned>"}`；与历史 Job `IQW-9041-OTA-DIAG` 一致 |
| Thing 命名 | ✅ | `IQEdge_{MAC}`（含冒号） |
| TC-OTA-02 | ✅ | `IQW-OTA-HQ2513A69PJ-20260529`，~64s SUCCEEDED |
| TC-OTA-01/03/04/10/12 | ⬜ | 未在本轮执行 |
| URL 版本 `indexOf` | 🟡 P2 | 可能误匹配/跳过真升级（`CommManager` L296） |
| OTA 阻塞 Comm | 🟡 P2 | `esp_https_ota` 同步于 `Task_Comm`；能源任务仍运行 |
| 无启动回滚 | 🟡 P1 | 坏镜像可 P1 Boot Loop；USB 可救，无 `esp_ota_mark_app_invalid` 策略 |
| S3/IAM | ✅ | `Agent007OTAPolicy` + `iqwatch-firmware-ota`；工具链 `tools/ota/` |

### 3.5 硬件与 HIL

| 检查项 | 状态 | 证据 |
|--------|------|------|
| VE.Direct 引脚 | ✅ | RX=16 TX=17 @ 19200（`Config.h` + `Hardware_Environment.md`） |
| LOAD 功率语义 | ✅ | 文档与 `GEMINI_ESP32` 已声明：非整机功耗 |
| GPS / BLE | ✅ | `GPS_ENABLED=false`、`BLE_POC_ENABLED=false` |
| 证书Provisioning | ⚠️ | 依赖产线 `uploadfs`/预置 LittleFS；无证书则 MQTT 失败 |

### 3.6 可靠性与测试

| 检查项 | 状态 | 证据 |
|--------|------|------|
| TWDT 超时 | ✅ | 30s；OTA 期间任务退订 WDT + HTTP 喂狗 |
| WDT 压力矩阵 | ⬜ | `WDT_Stress_Test_Plan` TC-01～33 大多待执行 |
| OTA 压力矩阵 | ⬜ | 仅 TC-OTA-02 完成；24h 浸泡（TC-OTA-03）未做 |
| 电源循环 TC-02 | ⚠️ | 部分日志；USB 断开导致监控中断 |
| 内存泄漏 | ⚠️ | 架构审查：长期 `String`/WiFi 重连可能碎片化；无长稳实测 |

### 3.7 文档与运维就绪

| 检查项 | 状态 | 证据 |
|--------|------|------|
| 架构 / 硬件 / 管线 | ✅ | `G2_Architecture_Review`、`Hardware_Environment`、`AWS_Cloud_Pipeline` |
| Agent Skills 1～3 | ✅ | `.cursor/skills/iqedge-hil-loop|triple-verify|remote-ota` + 根 `SKILL.md` |
| 验证脚本 | ✅ | `verify_telemetry.py`、`push_ota_job.py`、`monitor_ota.py` |
| 开发日志 | ✅ | `development_log.md` 倒序维护至 2026-05-29 |

---

## 4. 已验证设备快照

| deviceId | MAC | 固件（审计时） | 最后上报（UTC） | 备注 |
|----------|-----|----------------|-----------------|------|
| `HQ2513NH99U` | `EC:E3:34:1A:F9:8C` | v2.2.3.24（HIL） | 2026-05-29 00:46:12 | 桌面 HIL 沙箱 |
| `HQ2513A69PJ` | `1C:69:20:B8:D7:F4` | **v2.2.3.25** | 2026-05-29 00:57:34 | 远程 OTA 实测通过 |

---

## 5. 发现项汇总（按严重度）

### 🔴 P1 — 量产前强烈建议闭合

| ID | 发现 | 建议 |
|----|------|------|
| P1-01 | **默认 WiFi 凭据硬编码**于 `Config.h` | 产线写入 `wifi.json`；量产构建使用占位或空默认 + 强制 FS 配置 |
| P1-02 | **调试宏开启**（`DEBUG_MODE`、`CORE_DEBUG_LEVEL=3`） | 新增 `platformio.ini` `[env:production]`：`DEBUG_MODE false`、`CORE_DEBUG_LEVEL=1`、去掉 `DEBUG_ESP_WIFI` |
| P1-03 | **无 OTA 启动健康检查/回滚** | 实现启动后 N 分钟内 MQTT 成功否则标记无效槽位；或量产仅推签名镜像 |
| P1-04 | **压力/浸泡测试未闭环** | 至少完成 TC-OTA-01/03/04、WDT TC-01/02/11 后再扩批次 |

### 🟡 P2 — 下一迭代或批次扩大前

| ID | 发现 | 建议 |
|----|------|------|
| P2-01 | OTA Job URL `indexOf(FIRMWARE_VERSION)` | 改为解析 Job document 版本字段 |
| P2-02 | OTA 在 `Task_Comm` 内同步阻塞 | 独立任务 + 进度上报 |
| P2-03 | 串口 `SET WIFI` 无认证 | 产线模式禁用或加令牌 |
| P2-04 | `wifi.json` 缺失日志噪音 | 收敛首次启动日志 |
| P2-05 | `ReadOnlyAccess` 与 OTA 策略并存 | 运维文档说明 CLI/boto3 传播延迟 |

### 🟢 P3 — 技术债 / 长期

| ID | 发现 | 建议 |
|----|------|------|
| P3-01 | 头文件 `../` 耦合 | 目录重组时批量修正 |
| P3-02 | `CONSERVE`/`HIBERNATE` 部分无硬件动作 | 产品定义对齐或实现省电 |
| P3-03 | Secure Boot + 签名 OTA | `Config.h` Phase 2 路线图 |

---

## 6. 生产前检查清单（Bob / 产线）

复制用于签收：

```text
固件与构建
[ ] 发布使用 [env:production]（或确认接受当前 debug 构建风险）
[ ] FIRMWARE_VERSION 与 S3 对象名、Release Notes 一致
[ ] firmware.bin < 0x140000；分区表 default_4MB_littlefs.csv 未改

Provisioning（每台）
[ ] LittleFS 已烧录 AmazonRootCA1 + device cert + key
[ ] wifi.json 或现场 SSID 策略（非仅默认 Config.h）
[ ] IoT Thing IQEdge_{MAC} 已注册并绑定证书

云端（一次性）
[ ] DeviceStatusToLambda SQL = 2015-10-08
[ ] SaveDeviceStatus 抽样日志无解析错误

发布后验证（每台抽样）
[ ] 技能 2：verify_telemetry.py --mppt-id <SER#> --mppt-serial <SER#> → 3/3
[ ] 串口：Publish OK、FW 版本正确（技能 1）

OTA（若本批次含升级）
[ ] 仅推 HIL 验证过的 BIN；Job ID 可追溯
[ ] 技能 3 + 技能 2 事后验证
[ ] ENABLE_OTA 未在生产镜像开启
```

---

## 7. 裁决与建议

### 7.1 裁决：**有条件批准（Conditional GO）**

**允许：**

- 向 **已完成证书 + WiFi Provisioning** 的现场单元推送 **`v2.2.3.25`**（或同分区后续版本）；
- 使用 **AWS IoT Jobs OTA** 作为远程升级主路径；
- 依赖现有 **三位一体** 脚本做发布后验收。

**不允许（在未闭合 P1 前）：**

- 将当前镜像作为 **无Provisioning 模板** 全量复制到未知网络环境；
- 在未完成 OTA 失败/断电用例前，对大批设备连续推送未测 BIN；
- 开启 `ENABLE_OTA` 的 ArduinoOTA 生产构建。

### 7.2 建议发布顺序

1. **Pilot**：5～10 台，HIL 技能 1 → 烧录 → 技能 2；观察 24h（TC-OTA-03 简化版）。
2. **OTA 批次**：仅 `firmware_v2.2.3.25.bin` 已测设备；Job 记录入 `development_log`。
3. **量产硬化**：闭合 P1-01/02/03 后 bump `v2.2.4.0` 或下一生产 tag。

---

## 8. 附录

### A. 关键文件索引

| 路径 | 审计关注点 |
|------|------------|
| `src/config/Config.h` | 版本、WiFi、DEBUG、ENABLE_OTA |
| `src/managers/CommManager.cpp` | Payload、OTA Job、发布门控 |
| `src/managers/StorageManager.cpp` | LittleFS、证书、WiFi JSON |
| `src/managers/OtaManager.cpp` | HTTPS OTA、WDT |
| `platformio.ini` | 调试级别、分区、COM3 |
| `tools/aws-verify/verify_telemetry.py` | 三位一体 |
| `tools/ota/push_ota_job.py` | OTA 下发 |

### B. 审计执行记录

- 编译：`pio run` @ 2026-05-29，SUCCESS，Flash 80.3%
- 代码审查：全 `src/` 红线项 + Comm/Energy/Storage/OTA 主路径
- 回归依据：`development_log.md` 2026-05-28～29 验证条目

---

*报告版本 1.0 · 存放路径 `report/PreProduction_Audit_Report_2026-05-29.md` · 下次重大版本发布前复审计*
