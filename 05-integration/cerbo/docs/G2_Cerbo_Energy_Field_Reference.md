# G2 · Cerbo 能源字段参考摘要

> **来源**: 提炼自 IQTrailer 研究目录 `003_IQTrailer/02-study/`（VRM 实测语义）  
> **用途**: `05-integration` 边缘组装 **`energy.telemetry.v1`** 时的物理量与映射参考  
> **G2 管道**: Cerbo（Modbus TCP Server）→ RUT 轮询 → MQTT `iqedge/g2/{env}/energy/telemetry` → M4 ingest  
> **非 G2 路径**: Victron **VRM 云端 API**、门户 Widget/Graph、前端图表 — **不**作为 G2 生产 ingest 依据

---

## 1. G2 架构定位（IQTrailer）

| 项 | G2 约定 |
|----|---------|
| **system_type** | `iqtrailer` |
| **energy SSOT** | 单台 **Cerbo GX** 汇聚 MPPT；云端 **不**逐 MPPT 聚合 |
| **component_id** | Cerbo **Venus uniqueId / 出厂序列**（ADR-012 D-3） |
| **component_role** | `cerbo` |
| **发布者** | **RUT** MQTT Client（Cerbo 不直连 AWS） |
| **契约** | [`09-contract/schemas/energy/telemetry.v1.json`](../../../09-contract/schemas/energy/telemetry.v1.json) |
| **系统模型** | [`G2_System_Model.md`](../../../02-backend/docs/G2_System_Model.md) §3.4 |

```text
MPPT ×N → Cerbo GX → [Modbus TCP 读] → RUT → G2 MQTT → Timestream (table_energy)
```

---

## 2. 研究材料中可复用的物理量（Venus / Cerbo 语义）

研究站点为离网 Trailer（SmartShunt + MPPT，经 Cerbo 汇总）。下列 **Victron 属性码** 描述同一套物理量，边缘应从 **Cerbo Modbus / Venus D-Bus** 读取并映射到 G2 `measures.*`，而非调用 VRM HTTP API。

### 2.1 电池 `measures.battery`

| Victron 码 | 物理量 | 单位 | G2 字段 | 符号 / 备注 |
|------------|--------|------|---------|-------------|
| **V** | 电池电压 | V | `voltage_v` | 24 V 系统常见约 22–32 V |
| **I** | 电池电流 | A | `current_a` | **负 = 放电**；**正 = 充电**（Victron / SmartShunt 约定） |
| **SOC** | 荷电状态 | % | `soc_pct` | 0–100 |
| **TTG** | Time to go | s（VRM 显示 h） | `time_to_go_sec` | Modbus **846** raw **`× 100`** → 秒；例 raw 8640 → **864,000 s = 240 h**（见 [`modbus-register-map.md`](../modbus-register-map.md) § Time To Go） |
| **ScS**（MPPT 侧） | 充电阶段 | enum | `charge_state` / `charge_state_code` | Off · Bulk · Absorption · Float · Fault |

告警类（AL、AH、ASoc 等）G2 MVP **可不**上送；若扩展 `status`/`state`，需单独 Schema 版本。

### 2.2 太阳能 `measures.solar`

| Victron 码 | 物理量 | 单位 | G2 字段 | 备注 |
|------------|--------|------|---------|------|
| **ScW** | 当前充电功率 | W | `power_w` | MPPT 送入系统的功率；夜间常为 0 |
| **ScV** | 太阳能侧电压 | V | `voltage_v` | 常接近电池电压 |
| **ScI** | 太阳能侧电流 | A | `current_a` | 可选 |

**勿混淆**: 站点级 stats 码 **`Pdc`** 为整站 DC 相关量，**不等于**纯光伏功率；G2 应用 **`ScW`**（或 Cerbo 汇聚后的等效 solar power）。

### 2.3 负载 `measures.load`（单负载 Trailer）

离网单一直流负载时，由能量平衡推算（与 IQWatch 直读 load 不同）：

```text
P_load(t) = P_solar(t) − V(t) × I(t)
```

| 条件 | 含义 |
|------|------|
| `I < 0`（放电） | `P_load ≈ P_solar + |V×I|` |
| `I > 0`（充电） | 光伏一部分充电池，`P_load` 较小 |
| 夜间 `P_solar ≈ 0` | `P_load ≈ −V×I` |

映射：`load.power_w` = 上式结果（≥0 钳位）；`load.status` = `ON`/`OFF` 按业务阈值。

**仅有光伏功率无法可靠得到负载功率** — 必须同时有 **V + I**。

### 2.4 产量 `measures.yield`

| Victron 码 | 物理量 | G2 字段 | 备注 |
|------------|--------|---------|------|
| **YT** | 今日发电 | `today_kwh` | **kWh**，业务可读 |
| **YY** | 昨日发电 | `yesterday_kwh` | kWh |
| （累计） | 总发电 | `total_kwh` | 来自 Cerbo 累计寄存器 |
| **days_running** | 运行天数 | `days_running` | 与 flat 字段勿重复（M4 ingest 去重） |

**重要**: VRM stats 序列 **`total_solar_yield` / `Pb`** 为内部归一化小数值（约 0.01 量级），**不是** kWh。**G2 产量以 kWh 标度的 Cerbo/YT 类读数为准**。

---

## 3. G2 `energy.telemetry.v1` 映射表（EDGE-T2 目标）

RUT（或集成脚本）轮询 Cerbo 后组装：

```json
{
  "schema_version": "energy.telemetry.v1",
  "sys_id": "IQ-26-xxxxx",
  "component_id": "<venus_unique_id>",
  "component_role": "cerbo",
  "domain": "energy",
  "system_type": "iqtrailer",
  "status": "running",
  "state": "NORMAL",
  "data_stale": false,
  "reporting_mode": "NORMAL",
  "measures": {
    "battery": {
      "voltage_v": 25.89,
      "current_a": -0.40,
      "soc_pct": 72.5,
      "charge_state": "Off",
      "charge_state_code": 0
    },
    "solar": {
      "power_w": 0,
      "voltage_v": 25.9
    },
    "load": {
      "power_w": 10.4,
      "status": "ON"
    },
    "yield": {
      "today_kwh": 0.20,
      "yesterday_kwh": 0.21,
      "total_kwh": 35.6,
      "days_running": 76
    }
  }
}
```

| G2 必填 / 建议 | 来源 |
|----------------|------|
| `measures.battery.soc_pct` | SOC |
| `measures.battery.voltage_v` | V |
| `measures.battery.current_a` | I（保留符号） |
| `measures.solar.power_w` | ScW |
| `measures.yield.today_kwh` | YT |
| `data_stale` | 读数年龄 **> 25 min**（D-5） |
| `timestamp` | RUT 采样时刻 UTC ISO8601 |

Ingest：`track=g2` 即可写 Timestream；**豁免** ADR-008 ESP32 固件门禁（ADR-012）。`firmware_version` **可省略**。

---

## 4. 数据新鲜度与上报节奏

| 观察（研究站点） | G2 含义 |
|------------------|---------|
| Venus 日志周期约 **900 s**（15 min） | IQTrailer `data_stale` 阈值 **> 25 min**（ADR-012 D-5） |
| `secondsAgo` ≈ 576 s 仍属正常 | 非离线；勿与 IQWatch 60 s 联调节奏混用 |
| stats 15 min 桶 `[ts, mean, min, max]` | **历史趋势**参考；G2 MQTT 发**当前快照**即可，不必在 payload 内带 24 h 序列 |

---

## 5. 离网 Trailer 边界（G2 五域）

| 域 | 本摘要范围 |
|----|------------|
| **energy** | ✅ 本文 |
| **network** | RUT RSSI/GPS → `network/telemetry`（M5）；**不在此文件** |
| **vision / control / environment** | 未涉及 |

研究站点无并网：`grid_history_*`、`total_consumption` 等为 false/0 — IQTrailer 默认离网，**勿**向 energy payload 塞电网字段。

---

## 6. 刻意未纳入（避免 G2 干扰）

以下存在于 `02-study/` 但 **与 G2 边缘 ingest 无关**，本文不引用：

- VRM API v2 认证、限流、OpenAPI 端点索引  
- 前端 Solar 图表、`overallstats`、Widget 排版  
- `04-test/` 脚本与 JSON 文件名约定  
- VRM `idSite` / `instance` 作为生产配置（仅研究站点 964243 语义参考）

---

## 7. 待办（EDGE-T1 / T2）

| ID | 内容 |
|----|------|
| EDGE-T1 | 本文 §2 → Cerbo **Modbus 寄存器地址** 对照表（[`modbus-register-map.md`](../modbus-register-map.md)） |
| EDGE-T2 | RUT 轮询周期 + G2 JSON 组装实现（`../rut/`） |
| 契约 | `09-contract/examples/iqtrailer/` 样例 payload + Registry 种子 |

Modbus 连接与台架网络：[`G2_Cerbo_Modbus_Connectivity.md`](G2_Cerbo_Modbus_Connectivity.md)

---

## 8. 上游文档（外部研究仓）

| 文件 | 用途 |
|------|------|
| `VRM-realtime-battery-solar-字段解读.md` | Widget 码 V/I/SOC/ScW/YT 语义 |
| `VRM-realtime-stats-字段解读.md` | bv/bs/Pdc 与 15 min 桶格式（趋势参考） |
| `VRM-Single-Load-Power-Calculation.md` | §2.3 负载功率公式 |

---

*Agent 008 · G2-only 提炼 · 2026-05-30*
