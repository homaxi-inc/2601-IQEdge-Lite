# G2 · Cerbo Modbus 连接摘要

> **来源**: `003_IQTrailer/05-modbus/`（PDF V1.0 + Excel 3.73 · 已提炼）  
> **用途**: EDGE-T1 — RUT 作为 Modbus TCP **Client** 读取 Cerbo，组装 G2 `energy.telemetry.v1`  
> **关联**: [`G2_Cerbo_Energy_Field_Reference.md`](G2_Cerbo_Energy_Field_Reference.md) · [`G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md)

---

## 1. G2 管道中的 Modbus 角色

```text
[RUT241/956]  ──Modbus TCP Client──►  [Cerbo GX :502]  (Venus OS · SSOT)
       │
       └── 轮询寄存器 → 映射 §2 物理量 → MQTT iqedge/g2/{env}/energy/telemetry
```

| 项 | G2 约定 |
|----|---------|
| **Modbus 角色** | Cerbo = **Server**；RUT（或集成主机）= **Client** |
| **与 VRM 云 API** | **分轨** — G2 生产 ingest **不走** VRM HTTP；Modbus 为现场直连读数 |
| **energy SSOT** | Cerbo 汇聚 MPPT；`component_id` = Venus uniqueId · `component_role` = `cerbo` |
| **发布** | Cerbo **不**直连 AWS；由 [`../../rut/`](../../rut/) 发 MQTT |

研究仓 `05-modbus` 定位：存放 **寄存器文档、轮询脚本、台架实验** — 与 `03-vrm-api-v2` 互补，但 G2 边缘以 **本目录 + Modbus** 为 ingest 依据。

---

## 2. 台架网络参考（研究仓记录 · 非契约）

以下为 **003_IQTrailer 实验室** 快照，**不得**写入 `09-contract` 或 Registry 固定字段；部署时以现场 DHCP/静态规划为准。

| 项 | 研究仓值 | G2 说明 |
|----|----------|---------|
| Cerbo GX IP | `192.168.20.236` | Modbus TCP 目标 host |
| 子网网关 | `192.168.20.1` | RUT LAN 侧默认网关（通常 RUT 与 Cerbo 同网段） |
| Modbus 端口 | **502** | TCP · Unit ID **100** |
| RUT 轮询周期 | **10 s** | 见 Register Summary §3 |

**连通性自检（运维）**: RUT 或台架 PC `ping` Cerbo IP → Modbus 读 holding/input 寄存器（工具待 EDGE-T1 脚本化）。

---

## 3. 研究仓现状与 G2 缺口

| 状态 | 内容 |
|------|------|
| ✅ 已提炼 | PDF RUT Client 参数 · Excel 3.73 寄存器 · RUT **+1** 偏移 |
| ✅ G2 文档 | [`G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md) · [`modbus-register-map.md`](../modbus-register-map.md) |
| ⬜ 未提供 | RUT Data to Server / 脚本 → G2 MQTT（EDGE-T2） |

**G2 仓库对应待办**:

| ID | 产出 | 路径 |
|----|------|------|
| **EDGE-T1** | ✅ MVP 寄存器表 | [`modbus-register-map.md`](../modbus-register-map.md) |
| **EDGE-T2** | RUT 轮询 + G2 JSON + MQTT | [`../../rut/`](../../rut/) |

---

## 4. Modbus 读数 → G2 字段（EDGE-T1 ✅）

详见 [`modbus-register-map.md`](../modbus-register-map.md) — Unit ID **100** · 840–850 MVP：

| Modbus（Victron 地址） | G2 `measures` |
|------------------------|---------------|
| 840 Battery Voltage | `battery.voltage_v`（÷10） |
| 841 Battery Current | `battery.current_a`（÷10 · 有符号） |
| 843 SOC | `battery.soc_pct` |
| 850 PV DC Power | `solar.power_w` |
| 784 Yield today* | `yield.today_kwh`（÷10 · solarcharger unit） |

负载 `load.power_w` 按 Field Reference **§2.3**：`P_solar − V×I`。

`data_stale`: RUT 轮询 **10 s**；若 > **20–25 min** 无成功读数则置 `true`（Venus 日志 ~15 min 仅作参考）。

---

## 5. 刻意未纳入

| 排除项 | 原因 |
|--------|------|
| VRM API v2 端点与认证 | 非 G2 Modbus 边缘路径 |
| RUT RutOS REST API（`07-rut-api`） | 属 `05-integration/rut/`，非 Cerbo Modbus 文档 |
| Tesla / 其它 API | 与 IQTrailer Cerbo 无关 |
| 具体 Modbus 寄存器 | 见 Register Summary · **禁止臆造未文档化地址** |

---

## 6. 上游（外部研究仓）

| 路径 | 内容 |
|------|------|
| `003_IQTrailer/05-modbus/IQCloud_RUT_Modbus_Client_Configuration_Guide_V1.0.pdf` | RUT Client 台架参数 |
| `003_IQTrailer/05-modbus/Victron-CCGX-Modbus-TCP-register-list-3.73.xlsx` | 官方寄存器 3.73 |
| `003_IQTrailer/02-study/` | Victron 物理量语义（已提炼为 Field Reference） |

---

*Agent 008 · G2-only · 2026-05-30*
