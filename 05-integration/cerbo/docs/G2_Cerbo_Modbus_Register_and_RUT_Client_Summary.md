# G2 · Cerbo Modbus 寄存器与 RUT Client 配置摘要

> **来源**  
> - `003_IQTrailer/05-modbus/IQCloud_RUT_Modbus_Client_Configuration_Guide_V1.0.pdf`（RUT Client 生产参数 · 台架已验证）  
> - `003_IQTrailer/05-modbus/Victron-CCGX-Modbus-TCP-register-list-3.73.xlsx`（Victron 官方寄存器 · rev 3.73）  
> **G2 目标**: RUT Modbus Client → 解码 → MQTT **`iqedge/g2/{env}/energy/telemetry`**（`energy.telemetry.v1`）  
> **排除**: VRM 云 API、IQCloud 专有 `delta` JSON 模板、前端/Agent 商业描述 — 不进入 G2 契约

---

## 1. G2 边缘拓扑（与 IQWatch 分轨）

```text
PoE 交换机 ← LAN ← RUT (DHCP + Modbus TCP Client + MQTT)
                    ↕ Modbus TCP :502
                 Cerbo GX (Modbus TCP Server · Venus OS)
                    ↕ VE.Direct / CAN
                 MPPT ×N · SmartShunt …
```

| 角色 | 设备 | G2 说明 |
|------|------|---------|
| Modbus **Server** | Cerbo GX | SSOT；`component_id` = Cerbo |
| Modbus **Client** | RUT241/956 | 轮询 + 组装 G2 JSON + MQTT 发布 |
| Cloud ingest | M4 `ingest-energy` | ADR-008 · `track=g2` + fw ≥ v2.3.0 |

---

## 2. Cerbo 侧前置条件

| 项 | 设置 |
|----|------|
| **Modbus-TCP 服务** | Cerbo UI → Settings → Services → **Modbus-TCP = ON** |
| **系统汇总服务** | Available services 中 `com.victronenergy.system` → **Unit ID = 100** |
| **Unit ID 说明** | Venus ≥ 2.60 可动态分配；以 Cerbo 屏幕 **Available services** 为准（Excel FAQ） |
| **端口** | **502**（TCP） |

台架 IP（研究环境 · **非契约**）：Cerbo `192.168.20.236` · RUT LAN 网关 `192.168.20.1` — 部署须 **Static DHCP lease** 防漂移。

---

## 3. RUT Modbus TCP Client（生产参数摘要）

> 实现细节落 [`../../rut/`](../../rut/) · 下列为 G2 必读参数

| 配置项 | 值 | 说明 |
|--------|-----|------|
| Enabled | ON | |
| Name | `Cerbo_GX_System`（示例） | |
| **Server ID** | **100** | 对应 `com.victronenergy.system` |
| **Address** | Cerbo 静态 LAN IP | 台架 `192.168.20.236` |
| **Port** | **502** | |
| Timeout | 5 s | |
| Always reconnect | ON | 离网重连 |
| **Period** | **10 s** | RUT 轮询周期（与 VRM 门户 ~15 min 日志粒度不同） |
| Function | **Read Holding Registers (3)** | 各 Request 统一 |

**验收**: RutOS Request 行点击 **Test** → `Request successful, result: [...]`。

---

## 4. ⚠️ RUT 寄存器地址 +1 偏移（必做）

Teltonika RUT 的 **First Register** 相对 Victron 手册地址 **+1**（off-by-one）。  
**RUT 填错会直接读到相邻寄存器**（例：填 843 却读到 Power 而非 SOC）。

| Request 名（示例） | Victron 地址 | **RUT First Reg** | 类型 (RUT) | 物理量 |
|--------------------|-------------|-------------------|------------|--------|
| `Battery_SoC` | **843** | **844** | 16bit UINT | SOC % |
| `Battery_Voltage` | **840** | **841** | 16bit UINT | 电压（见 §5 缩放） |
| `Battery_Power` | **842** | **843** | **16bit INT** | 充放电功率 W |
| `PV_Power` | **850** | **851** | 16bit UINT | DC 耦合光伏总功率 W |

---

## 5. Victron 寄存器定义（Unit ID 100 · `com.victronenergy.system`）

摘自 **CCGX Modbus TCP register list 3.73** — MVP **黄金四件套** + 可选电流。

| Victron 地址 | dbus 路径 | 类型 | Scale | 单位 | 解码 | G2 `measures` |
|-------------|-----------|------|-------|------|------|---------------|
| **840** | `/Dc/Battery/Voltage` | uint16 | **10** | V | `raw / 10` | `battery.voltage_v` |
| **841** | `/Dc/Battery/Current` | int16 | **10** | A | `raw / 10` | `battery.current_a` |
| **842** | `/Dc/Battery/Power` | int16 | 1 | W | 有符号；**+充电 / −放电** | （可用于校验负载公式） |
| **843** | `/Dc/Battery/Soc` | uint16 | 1 | % | `raw` 即 % | `battery.soc_pct` |
| **850** | `/Dc/Pv/Power` | uint16 | 1 | W | 所有 MPPT 汇总 | `solar.power_w` |

**电流符号**（841 / SmartShunt 一致）：**正 = 充电，负 = 放电**（与 [`G2_Cerbo_Energy_Field_Reference.md`](G2_Cerbo_Energy_Field_Reference.md) 一致）。

**负载推算**（单负载 Trailer）：

```text
P_load ≈ P_solar − V × I     （850/840/841 解码后）
       → measures.load.power_w
```

**`Battery_Power` (842)** 可与 `V×I` 交叉校验；G2 payload 仍优先显式 `battery.current_a`。

---

## 6. MVP 扩展寄存器（ADR-012 D-9 · Unit 100）

| Victron 地址 | RUT First+1 | Scale | G2 字段 | 备注 |
|-------------|-------------|-------|---------|------|
| **841** | 842 | 10 | `battery.current_a` | 负载公式必需 |
| **844** | 845 | 1 | `battery.charge_state_code` | 0=idle 1=charging 2=discharging |

### Yield（D-10 · 条件性 · OI-002）

仅当 **Unit 100** 台架验证存在 system 级 yield 等价寄存器时才配置。**禁止** MVP 默认读 solarcharger 独立 Unit ID。

---

## 7. G2 MQTT 载荷（替代 IQCloud `delta` 模板）

PDF 附录 `delta` JSON（`pwr.soc/vol/pwr/sol`）为 **IQCloud 旧轨**，G2 生产须改为：

- **Topic**: `iqedge/g2/{env}/energy/telemetry`
- **Schema**: [`energy.telemetry.v1`](../../../09-contract/schemas/energy/telemetry.v1.json)
- **身份**: `sys_id` · `component_id`（Cerbo）· `component_role=cerbo` · `system_type=iqtrailer`

解码后映射示例：

```json
"measures": {
  "battery": { "voltage_v": 26.5, "current_a": -0.4, "soc_pct": 72 },
  "solar":   { "power_w": 39 },
  "load":    { "power_w": 10.6, "status": "ON" },
  "yield":   { "today_kwh": 0.2 }
}
```

**network 域**（RSSI/GPS）走 `iqedge/g2/{env}/network/telemetry`（M5）— **不要**塞进 energy payload。

---

## 8. `data_stale` 与轮询

| 源 | 粒度 | G2 建议 |
|----|------|---------|
| RUT Modbus Period | **10 s** | 联调 ingest 活跃 |
| Venus 数据日志 | ~**900 s** | `data_stale` 阈值 **> 25 min** 无成功 Modbus 读（D-5） |

---

## 9. 刻意未纳入

| 排除 | 原因 |
|------|------|
| VRM API 限流 / 流量对比 | 非 Modbus 边缘路径 |
| IQCloud Data to Server `delta` 模板 | 由 G2 Topic + Schema 取代 |
| 降功耗 Agent / 数字孪生文案 | 非 G2 契约 |
| 全表 962 行寄存器 | 仅摘 MVP + yield 扩展；全表留研究仓 xlsx |

---

## 10. 相关 G2 文档

| 文档 | 路径 |
|------|------|
| 字段语义 | [`G2_Cerbo_Energy_Field_Reference.md`](G2_Cerbo_Energy_Field_Reference.md) |
| 连接摘要 | [`G2_Cerbo_Modbus_Connectivity.md`](G2_Cerbo_Modbus_Connectivity.md) |
| 寄存器表（运维） | [`../modbus-register-map.md`](../modbus-register-map.md) |
| RUT 实现 | [`../../rut/README.md`](../../rut/README.md) |

---

*Agent 008 · Victron 3.73 + RUT V1.0 指南 · G2-only · 2026-05-30*
