# RUT241 / RUT956 — 4G 边缘发布

> **角色**: Modbus **Client**（读 Cerbo）+ MQTT **Client**（发 AWS IoT）  
> **008 主责**: Topic、证书、Policy、与 `04-cloud` M4/M5 对齐

---

## Topic 对照

| 轨 | Topic 示例 | 说明 |
|----|------------|------|
| **Legacy** | `iot/rut241/status` | 现网 · **不改造**（M5.2） |
| **G2 energy** | `iqedge/g2/{env}/energy/telemetry` | Cerbo 汇聚 payload · `component_role=cerbo_gx` |
| **G2 network** | `iqedge/g2/{env}/network/telemetry` | RSSI · GPS · 在线状态（M5） |

`{env}` = `dev` | `prod` — 严禁跨环境 publish（G2 IoT Policy Deny）。

---

## 待填

| 文件 | 说明 |
|------|------|
| `mqtt-topic-mapping.md` | Legacy vs G2 字段对照 |
| `scripts/` | RUT 侧 Lua/Python、配置导出（若有） |
| `docs/` | Teltonika RMS、证书、First Ping |

---

## 运维（008）

- G2 Policy attach：[`04-cloud/scripts/attach_g2_iot_policy.py`](../../04-cloud/scripts/attach_g2_iot_policy.py)
- Legacy Lambda：`IQTrailerDataRouter`（Rule 已 disabled · 双轨 ADR-001）

---

## 关联

- [`../cerbo/`](../cerbo/)
- [`04-cloud/docs/G2_Domain_Map.md`](../../04-cloud/docs/G2_Domain_Map.md)
