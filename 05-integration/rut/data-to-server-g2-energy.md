# RUT Data to Server — G2 energy telemetry (EDGE-T2)

> **台架**: Homaxi-test `6004727310` · VPN `10.24.121.199` · Cerbo `c0619ab6be37`  
> **契约**: [`09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json`](../../09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json)

---

## Topic 双轨（2026-05-30 生效）

| 轨 | Collection | Topic | 状态 |
|----|------------|-------|------|
| Legacy 路由 | `awsmqtt` | `iot/rut241/status` | ✅ 保留 · period **60 s** |
| **G2 energy** | `g2_energy_telemetry` | `iqedge/g2/dev/energy/telemetry` | ✅ 新增 · period **60 s** |
| ~~IQCloud delta~~ | ~~`collection_modbus_aws`~~ | ~~`iqcloud/energy_telemetry`~~ | ❌ **已禁用** |

---

## Modbus Client

| 项 | 值 |
|----|-----|
| Period | **60 s**（自 300 s 调整） |
| Cerbo | `192.168.20.236` · Unit **100** |
| 寄存器 | 见 [`../cerbo/modbus-register-map.md`](../cerbo/modbus-register-map.md) |

---

## G2 组装

```text
Modbus Client (60s) → Data to Server input g2_modbus_input (json)
        ↓
Collection g2_energy_telemetry (format=lua, /etc/data_sender/format.lua)
        ↓
MQTT → iqedge/g2/dev/energy/telemetry
```

Lua 源文件（仓库）: [`lua/g2_energy_format.lua`](lua/g2_energy_format.lua)  
设备路径: `/etc/data_sender/format.lua`（部署时同步）

**缩放**（Lua 内）:

- `Battery_Voltage`: raw / 10 → `battery.voltage_v`
- `Battery_Power` / `Battery_Voltage` → `battery.current_a`（推导）
- `Battery_SoC` → `battery.soc_pct`
- `PV_Power` → `solar.power_w`
- `load.power_w` = max(0, PV_Power − Battery_Power)

**常量**: `sys_id=IQ-26-60001` · `component_id=c0619ab6be37`

> **007 设备同步**: 修改 `SYS_ID` 后须 scp Lua 并执行 [`rut_apply_g2_edge_t2.sh`](../scripts/rut_apply_g2_edge_t2.sh)。

---

## 部署 / 回滚

```bash
# 经 EC2 跳板（见 docs/vpn-rut-remote-access.md）
scp rut/lua/g2_energy_format.lua ubuntu@52.5.62.66:/tmp/
scp scripts/rut_apply_g2_edge_t2.sh ubuntu@52.5.62.66:/tmp/
ssh ubuntu@52.5.62.66 "bash /tmp/rut_apply_g2_edge_t2.sh /tmp/g2_energy_format.lua"
```

验收:

```bash
ssh root@10.24.121.199 "uci get modbus_client.1.period; uci get data_sender.32.mqtt_topic; uci get data_sender.10.enabled"
ubus call datasender.collection.31 dump
```

---

*Agent 007 · EDGE-T2 台架 · 2026-05-30*
