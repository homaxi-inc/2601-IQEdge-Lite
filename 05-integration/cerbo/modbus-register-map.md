# Modbus 寄存器映射 — IQTrailer G2 MVP

> **权威摘要**: [`docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md)  
> **ADR-012**: Bob 定稿 D-9 MVP · D-10 yield 条件性  
> **Victron 手册**: CCGX Modbus TCP register list **3.73** · Unit ID **100** (`com.victronenergy.system`)  
> **RUT**: First Register = Victron 地址 **+1**

---

## MVP — Unit 100（台架必配）

| Request 名 | Victron | RUT First | FC | RUT 类型 | Scale | 解码 → G2 |
|------------|---------|-----------|-----|----------|-------|-----------|
| `Battery_Voltage` | 840 | **841** | 3 | UINT16 | 10 | `battery.voltage_v = raw/10` |
| `Battery_Current` | 841 | **842** | 3 | **INT16** | 10 | `battery.current_a = raw/10` |
| `Battery_Power` | 842 | **843** | 3 | **INT16** | 1 | 校验 W（±） |
| `Battery_SoC` | 843 | **844** | 3 | UINT16 | 1 | `battery.soc_pct` |
| `Battery_State` | 844 | **845** | 3 | UINT16 | 1 | `battery.charge_state_code` |
| `PV_Power` | 850 | **851** | 3 | UINT16 | 1 | `solar.power_w` |

**连接**: host = Cerbo 静态 IP · port **502** · server id **100**

`load.power_w` = `solar.power_w − battery.voltage_v × battery.current_a`（≥ 0）

---

## Yield — 条件性（D-10 · 台架验证）

**规则**: 仅当 **Unit 100** 存在与 784/786 等价的 system 级寄存器时才配置 RUT Request。  
**禁止** MVP 默认读 solarcharger 独立 Unit ID（待 OI-002 关闭）。

| Request 名 | Victron（待验证） | RUT First | Scale | G2 | 状态 |
|------------|-------------------|-----------|-------|-----|------|
| `Yield_Today` | TBD @ unit 100 | TBD+1 | 10? | `yield.today_kwh` | ⬜ OI-002 |
| `Yield_Yesterday` | TBD @ unit 100 | TBD+1 | 10? | `yield.yesterday_kwh` | ⬜ OI-002 |

---

## RUT Client 全局参数

| 项 | 值 |
|----|-----|
| Period | 10 s |
| Timeout | 5 s |
| Always reconnect | ON |
| Function | Read Holding Registers (3) |

## `data_stale`（D-5）

距上次 **Modbus 成功读** > **25 min** → `data_stale: true`

---

## EDGE-T2 · Data to Server

RUT 侧实现：**RutOS Data to Server** + 自定义 JSON → `iqedge/g2/{env}/energy/telemetry`  
详见 [`../../rut/README.md`](../../rut/README.md)

---

## 验收

1. RutOS 各 Request **Test** 返回合理数组  
2. RUT → G2 MQTT → M4 ingest（IQTrailer **无** fw 门禁 · ADR-012）  
3. Yield Request 仅在 OI-002 关闭后启用

---

*台架 IP 见 Connectivity 摘要 · 非契约常量*
