# RUT241 / RUT956 — 4G 边缘发布

> **角色**: Modbus **Client**（读 Cerbo）+ MQTT **Client**（发 AWS IoT）  
> **008 主责**: Topic、证书、Policy、与 `04-cloud` M4/M5 对齐  
> **ADR-012 D-4**: **RutOS Data to Server** + 自定义 G2 JSON 模板

---

## IoT 身份（D-8）

| 项 | 规则 |
|----|------|
| **Thing 名** | RUT **出厂 SN**（例 `RUT241-6001234567`） |
| **证书** | RUT 独立证书 · **G2 IoT Policy** |
| **发布** | 同一 Thing 可发 Legacy + G2 Topic（双轨） |

Policy attach：[`04-cloud/scripts/attach_g2_iot_policy.py`](../../04-cloud/scripts/attach_g2_iot_policy.py)

---

## Topic 对照

| 轨 | Topic 示例 | 说明 |
|----|------------|------|
| **Legacy** | `iot/rut241/status` | 现网 · **不改造**（M5.2） |
| **G2 energy** | `iqedge/g2/{env}/energy/telemetry` | Cerbo 汇聚 · `component_role=cerbo` |
| **G2 network** | `iqedge/g2/{env}/network/telemetry` | RSSI · GPS · 在线状态（M5） |

`{env}` = `dev` | `prod` — 严禁跨环境 publish（G2 IoT Policy Deny）。

---

## EDGE-T2 · Data to Server 流程

```text
Modbus Client (10s) → 变量/寄存器缓存
        ↓
Data to Server 规则 → JSON 组装（sys_id, component_id, measures.*）
        ↓
MQTT publish → iqedge/g2/{env}/energy/telemetry
```

| 配置块 | 说明 |
|--------|------|
| Modbus Client | 见 [`../cerbo/modbus-register-map.md`](../cerbo/modbus-register-map.md) |
| Data to Server | 自定义 JSON · 对齐 [`telemetry-iqtrailer-cerbo-live.v1.json`](../../09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json) |
| `component_id` | Cerbo Venus **uniqueId**（与 Registry 一致 · D-3） |
| `firmware_version` | **省略**（ADR-012 豁免） |
| `data_stale` | RUT 侧：> **25 min** 无成功 Modbus 读 → `true` |
| `state` / `reporting_mode` | **待定** OI-001 — 台架可暂填 `NORMAL` |

---

## 待填

| 文件 | 说明 |
|------|------|
| `mqtt-topic-mapping.md` | Legacy vs G2 字段对照 |
| `data-to-server-template.json` | RutOS 导出模板（EDGE-T2） |
| `docs/` | Teltonika RMS、证书、First Ping |

---

## 运维（008）

- Legacy Lambda：`IQTrailerDataRouter`（Rule 已 disabled · 双轨 ADR-001）

---

## 关联

- [`../cerbo/`](../cerbo/)
- [`04-cloud/docs/G2_Domain_Map.md`](../../04-cloud/docs/G2_Domain_Map.md)
- [`../docs/open_issues.md`](../docs/open_issues.md)
