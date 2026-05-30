# VRM Cerbo preflight — 007 交接报告

> **日期**: 2026-05-30 UTC  
> **执行**: Agent 008  
> **站点**: **ST-03** · VRM `idSite=964243`（IQTrailer 实验室）  
> **用途**: Modbus/RUT 集成前 **VRM 基线** — 007 台架读数应与此同量级（非生产 ingest）

---

## 结论（007 可开工）

| 项 | 结果 |
|----|------|
| **总体** | ✅ **PASS** — Token、站点、设备清单、实时能源量均可读 |
| **Cerbo `component_id`** | **`c0619ab6be37`**（Gateway `identifier` · ADR-012 D-3） |
| **Modbus 对照** | 见下表 · 007 读 Unit **100** 寄存器后应与 VRM 偏差在合理范围（±缩放/轮询延迟） |
| **Yield OI-002** | VRM **YT/YY 来自 MPPT instance 277**；Modbus 须验证 **Unit 100** 是否有等价寄存器 |

---

## 测试项

| ID | 接口 | 结果 | 说明 |
|----|------|------|------|
| **T1** | `GET /users/me` | ✅ | User `608184` · bobw@homaxi.com |
| **T2** | `GET /users/608184/installations` | ✅ | **7** 个安装（含 ST-02/03/10 与 040826-*） |
| **T3** | `GET /installations/964243/system-overview` | ✅ | Gateway + SmartShunt + MPPT |
| **T4** | `BatterySummary` instance **278** | ✅ | **V · I · SOC** 齐全 |
| **T5** | `SolarChargerSummary` instance **277** | ✅ | **ScW · YT · YY · ScS** |
| **T6** | `stats?type=live_feed` | ✅ | `bv`/`bs`/`Pdc` 可读；`total_solar_yield` **≠ kWh** |

**采集时刻**: `2026-05-30T22:48:55Z` · VRM `secondsAgo` ≈ **449 s**（~7.5 min，正常；网关 `loggingInterval=900` s）

---

## ST-03 设备清单（Registry D-7 参考）

| 角色 | 型号 | Serial / ID | VRM instance |
|------|------|-------------|--------------|
| **cerbo**（energy SSOT） | Gateway | **`c0619ab6be37`** | — |
| **mppt** | SmartSolar MPPT 150/45 rev3 | `HQ2441HWTFQ` | 277 |
| **shunt** | SmartShunt 500A | `HQ2245QEEW2` | 278 |

> 台架 Cerbo IP（Modbus）：见 [`cerbo/docs/G2_Cerbo_Modbus_Connectivity.md`](../cerbo/docs/G2_Cerbo_Modbus_Connectivity.md) · `192.168.20.236`

---

## G2 映射基线（VRM 实时 · 007 Modbus 对照用）

| G2 `measures` | VRM 来源 | 实测值 | Modbus 寄存器 (Unit 100) |
|---------------|----------|--------|---------------------------|
| `battery.voltage_v` | SmartShunt **V** | **27.54 V** | 840 ÷10 |
| `battery.current_a` | SmartShunt **I** | **-0.60 A**（放电） | 841 ÷10 |
| `battery.soc_pct` | SmartShunt **SOC** | **66.0 %** | 843 |
| `solar.power_w` | MPPT **ScW** | **33 W** | 850（系统 PV 汇总） |
| `load.power_w` | 公式 `ScW − V×I` | **≈ 49.5 W** | RUT 边缘计算 |
| `yield.today_kwh` | MPPT **YT** | **0.40 kWh** | OI-002 · system 100 TBD |
| `yield.yesterday_kwh` | MPPT **YY** | **0.44 kWh** | OI-002 · system 100 TBD |
| `battery.charge_state_code` | MPPT **ScS**=Float (enum 5) | 浮充 | 844（system · 与 MPPT ScS 语义不同） |

**负载验算**:

```text
P_load = 33 − 27.54 × (−0.60) = 33 + 16.52 ≈ 49.5 W
```

---

## 007 行动清单

1. **Modbus Client** — 按 [`modbus-register-map.md`](../cerbo/modbus-register-map.md) 配 RUT（Remember **+1** First Reg）  
2. **对照** — 轮询成功后与上表 V/I/SOC/ScW 比较（允许 ±1 寄存器刻度、10 s 轮询差）  
3. **`component_id`** — MQTT/Registry 用 **`c0619ab6be37`**（非样例 JSON 里的占位符）  
4. **Yield** — 先完成 840–850/844 MVP；784/786 待 OI-002 台架确认 Unit 100  
5. **EDGE-T2** — Data to Server 模板对齐 [`telemetry-iqtrailer-cerbo-live.v1.json`](../../09-contract/examples/energy/telemetry.v1.json)（无 `firmware_version`）  
6. **复跑预检** — `python 05-integration/scripts/vrm_cerbo_preflight.py`

---

## 陷阱（Field Reference 已述）

| 勿用 | 应用 |
|------|------|
| `stats.total_solar_yield` (~0.01) | Widget **YT / YY**（kWh） |
| `stats.Pdc`  alone | **ScW** 或 Modbus **850** |
| VRM HTTP 生产 ingest | Modbus → RUT → G2 MQTT |

---

## 原始输出

| 文件 | 位置 |
|------|------|
| 设备列表 | `003_IQTrailer/04-test/output_devices.json` |
| ST-03 overview | `003_IQTrailer/04-test/output_system_overview_964243.json` |
| 实时 Solar/Battery | `003_IQTrailer/04-test/output_964243_realtime_battery_solar.json` |
| 机器可读预检 | [`vrm_preflight_latest.json`](vrm_preflight_latest.json) |

---

*008 → 007 · VRM 交叉校验基线 · Cerbo 集成可启动*
