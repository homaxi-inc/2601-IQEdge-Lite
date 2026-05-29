# IQEdge-G2 沙箱物理硬件环境（HIL）

> **文档性质**: 项目物理硬件资产 / Hardware-in-the-Loop（HIL）基准拓扑  
> **适用范围**: `01-firmware` 固件开发、代码审查、重构与联调  
> **维护责任**: 凡变更现场接线、MCU 底板或 MPPT 型号，须同步更新本文档  
> **关联文档**: [`G2_Architecture_Review.md`](G2_Architecture_Review.md) · [`../src/GEMINI_ESP32.md`](../src/GEMINI_ESP32.md)

---

## 0. 文档约束（Agent / 开发者必读）

在分析或重构 `src/` 中下列模块时，**必须以本文档描述的物理闭环为准**，不得仅凭代码注释或历史配置推断现场接线：

| 代码域 | 关键文件 | 必须对齐的硬件事实 |
|--------|----------|-------------------|
| VE.Direct 协议解析 | `hal/VeDirectDriver.*` | MPPT 75/15 · 19200 8N1 · ESP32↔VE.direct 专线 |
| 能源采集与连接状态 | `managers/EnergyManager.*` | 串口引脚、超时、MPPT 在线判定 |
| Load 端子语义 | `VeDirectDriver`（`IL`/`LOAD`） | **本沙箱** LOAD 仅供电 IQEdge 底板；`load_power` 反映 LOAD 回路，非整机功耗 |
| ESP32 串口配置 | `EnergyManager::begin`、`config/Config.h` | HIL 实装 **GPIO 16/17**（见 §5） |
| 上报 Payload | `managers/CommManager.*` | 电池/光伏数据来自 MPPT；负载字段含义受 LOAD 接线限制 |

---

## 1. 系统概览

本环境为 **单 MPPT + 单电池 + 单 IQEdge** 的桌面级 HIL 沙箱，用于 G2 固件标杆验证（VE.Direct 采集、MQTT 上报、OTA/配网等）。

```text
                    ┌─────────────────────────────────────┐
  Solar Panel ─────►│  Victron BlueSolar MPPT 75/15       │
  (PV 输入)         │  · PV 输入                           │
                    │  · [BATT] ──► 12V LiFePO4 20Ah      │
                    │  · [LOAD] ──► 12V ──► IQEdge 底板    │
                    │  · [VE.direct] ◄──UART──► ESP32     │
                    └─────────────────────────────────────┘
                                          │
                                          ▼
                    ┌─────────────────────────────────────┐
                    │  IQEdge 定制底板 + ESP32 模组        │
                    │  12V in → DC-DC → 5V → ESP32        │
                    │  USB 调试口 ◄── USB-TTL ◄── 开发 PC  │
                    └─────────────────────────────────────┘
```

---

## 2. 设备清单

| # | 角色 | 型号 / 规格 | 数量 | 在环中的作用 |
|---|------|-------------|------|--------------|
| 1 | 充电控制器 (MPPT) | Victron **BlueSolar MPPT 75/15** | 1 | PV 充电、电池管理、LOAD 输出、VE.Direct 遥测源 |
| 2 | 电池 (Battery) | **12 V / 20 Ah** LiFePO4 | 1 | 储能；直接接 MPPT **[BATT]** 端子 |
| 3 | 主控 (MCU) | **IQEdge**（ESP32 模组 + 定制底板） | 1 | 固件运行、VE.Direct 主站、WiFi/MQTT 上云 |
| 4 | 调试链路 (Debug) | USB 转 TTL / 板载 USB 调试口 | 1 | `pio run -t upload` 烧录、Serial Monitor |

---

## 3. 电气连接与端子表

### 3.1 MPPT 端子接线

| MPPT 端子 | 连接对象 | 电压/信号 | 说明 |
|-----------|----------|-----------|------|
| **PV** | 光伏板（沙箱可用模拟源或面板） | PV 额定内 | 75/15：PV 开路电压上限须符合 Victron 手册 |
| **[BATT]** | 12 V / 20 Ah LiFePO4 | 12 V 标称 | 电池 **直接** 接 BATT，不经过 IQEdge |
| **[LOAD]** | IQEdge 定制底板电源输入 | 12 V（MPPT LOAD 输出） | 为本沙箱 **唯一** 由 LOAD 供电的负载；底板内 DC-DC 至 5 V 供 ESP32 |
| **[VE.direct]** | IQEdge 底板 VE.Direct 接口 | 异步串口 | 与 ESP32 UART 专用线缆连接；**禁止**与调试 USB 串口混用 |

### 3.2 IQEdge 底板接口

| 底板接口 | 连接对象 | 说明 |
|----------|----------|------|
| 电源输入 | MPPT **[LOAD]** | 12 V 输入 → 板载降压 → **5 V** 供 ESP32 |
| VE.Direct 端口 | MPPT **[VE.direct]** | 与 `Serial2`（或硬件映射 UART）相连，见 §5 |
| USB 调试口 | 开发 PC USB | 烧录与 `Serial` 日志（115200，与 `main.cpp` 一致） |
| （可选）WiFi 天线 | 板载 / 外置 | 与云端通信，非 HIL 功率路径 |

### 3.3 调试链路

| 项目 | 配置 |
|------|------|
| 物理连接 | USB 转 TTL 或板载 USB ↔ 本机 USB 口 |
| 用途 | PlatformIO `upload`、串口监视、串口配网命令（`SET WIFI ...`） |
| 与 VE.Direct 关系 | **独立通道**；调试口不参与 VE.Direct 协议 |

---

## 4. 电源级联拓扑

| 级数 | 节点 | 电压 | 能量/控制说明 |
|------|------|------|---------------|
| L0 | 光伏 → MPPT PV | PV 变量 | MPPT 算法调节充电 |
| L1 | MPPT **[BATT]** | 12 V | 电池充放电管理；**SoC/电压主要数据源**（经 VE.Direct 上报） |
| L2 | MPPT **[LOAD]** | 12 V（开关/限流由 MPPT LOAD 逻辑控制） | 本沙箱：**仅 IQEdge 底板** 挂接于 LOAD |
| L3 | 底板 DC-DC | 12 V → **5 V** | ESP32 模组供电 |
| L4 | ESP32 | 3.3 V（模组 LDO） | 运行固件、WiFi、UART |

**电源级联示意（ASCII）**

```text
  PV ──► [ MPPT 75/15 ] ──BATT──► 12V LiFePO4 20Ah
              │
              └──LOAD(12V)──► [ IQEdge 底板 DC-DC ] ──► 5V ──► ESP32
```

### 4.1 与固件「Load」字段的对应关系

| 概念 | 本沙箱物理事实 | 固件中的体现 |
|------|----------------|--------------|
| MPPT **LOAD 端子** | 向 IQEdge 底板供电（12 V） | `LOAD` / `IL` 字段描述 **该端子** 上的负载 |
| **整机功耗** | 无 PoE/大负载直连电池；沙箱较简单 | `load_power` **不能** 代表「电池侧总放电」；估算整机需用夜间 SOC 衰减等方法（见 `GEMINI_ESP32.md`） |
| **LOAD 输出控制** | Victron 可通过 VE.Direct / 配置影响 LOAD 开关 | 固件当前 **未实现** 对 LOAD 继电器的直接 GPIO 控制；分析时勿假设存在本地 LOAD 开关引脚 |

---

## 5. 通信链路

### 5.1 VE.Direct（MPPT ↔ ESP32）

| 参数 | 值 | 备注 |
|------|-----|------|
| 协议 | Victron **VE.Direct** 文本帧 | 由 `VeDirectFrameHandler` + `VeDirectDriver` 解析 |
| 波特率 | **19200** | `Config.h` → `VEDIRECT_BAUD` |
| 数据格式 | 8N1 | `VeDirectDriver::begin` |
| 物理接口 | MPPT **[VE.direct]** ↔ 底板 ↔ ESP32 UART | 专用线缆，与 USB 调试分离 |
| 超时判定 | 60 s 无有效帧 | `VEDIRECT_TIMEOUT_MS` → `isMpptConnected()` |

### 5.2 固件 UART 引脚映射（沙箱实装）

| 信号 | ESP32 GPIO（HIL 实装） | `Config.h` | 说明 |
|------|------------------------|------------|------|
| VE.Direct RX | **GPIO 16** | `VEDIRECT_RX_PIN` = 16 | MPPT VE.Direct → ESP32 UART RX |
| VE.Direct TX | **GPIO 17** | `VEDIRECT_TX_PIN` = 17 | MPPT VE.Direct → ESP32 UART TX |
| 调试 Serial | USB/UART0 | — | `Serial.begin(115200)` |

> **重构红线**: VE.Direct 引脚以 `Config.h` 中 `VEDIRECT_RX_PIN` / `VEDIRECT_TX_PIN` 为准（HIL 沙箱为 **16/17**），`EnergyManager::begin` 必须引用这两处宏，禁止再硬编码其他 GPIO。

### 5.3 上云链路（逻辑层）

| 链路 | 路径 | 说明 |
|------|------|------|
| 遥测上报 | ESP32 WiFi → AWS IoT MQTT | 非 HIL 专用线；依赖现场 WiFi |
| 证书存储 | LittleFS | 生产设备禁止 `LittleFS.begin(true)` 自动格式化 |
| 时间同步 | NTP（`pool.ntp.org`） | TLS 连接前置条件 |

---

## 6. 关键 VE.Direct 寄存器与硬件量对应

以下为固件已解析、与 **本沙箱** 直接相关的字段（详见 `VeDirectDriver::_parseFrame`）：

| VE.Direct 键 | 物理量 | 单位换算（驱动内） |
|--------------|--------|-------------------|
| `V` | 电池电压 | mV → V（÷1000） |
| `I` | 电池电流 | mA → A（÷1000） |
| `SOC` | 电池 SoC | 0.1% → %（÷10） |
| `VPV` / `PPV` | 光伏电压 / 功率 | 与面板及光照相关 |
| `IL` / `LOAD` | LOAD 电流 / 状态 | 本沙箱：**主要为 IQEdge 底板功耗** |
| `H19`–`H23`, `HSDS` | 历史产量 / 日序 | Wh 与 kWh 上报见 `CommManager`（÷1000） |

---

## 7. HIL 沙箱 vs 量产现场（差异提示）

| 项目 | 本 HIL 沙箱 | 量产 IQWatch 典型拓扑 |
|------|-------------|------------------------|
| LOAD 负载 | 仅 IQEdge 底板 | 可能另有 PoE、摄像机等 **直连电池** |
| `load_power` 语义 | 接近「边缘节点自耗」 | 往往 **远低于** 系统总负载 |
| 电池容量 | 20 Ah | 现场常见 80 Ah 等 |
| MPPT 型号 | 75/15 | 以现场 Victron 型号为准 |

分析代码时：本沙箱用于 **验证协议与固件逻辑**；解释云端「负载为 0」或偏差时，须区分 **字段缺失**、**单位错误** 与 **接线拓扑限制**。

---

## 8. 开发操作速查

| 操作 | 命令 / 接口 | 物理前提 |
|------|-------------|----------|
| 编译烧录 | `pio run -t upload` | USB 调试口已连接 |
| 串口监视 | `pio device monitor` 或 IDE Monitor | 115200 baud |
| WiFi 串口配网 | `SET WIFI <SSID> <PASS>` | 同上；写入 LittleFS 后重启 |
| MPPT 数据可见性 | 串口 `[NRG]` / `[VED]` 日志 | VE.Direct 线接通且 MPPT 上电 |
| 云端 Payload | MQTT `device/status` | WiFi + 证书 + NTP 就绪 |

---

## 9. 修订记录

| 日期 | 版本 | 说明 |
|------|------|------|
| 2026-05-28 | 1.0 | 初版：HIL 沙箱拓扑固化（MPPT 75/15 · 12V 20Ah · IQEdge · USB 调试） |

---

*本文档为 IQEdge-G2 物理硬件在环（HIL）基准，与固件仓库同步维护。*
