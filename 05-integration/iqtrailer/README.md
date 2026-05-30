# IQTrailer — 集成场景总览

> **system_type**: `iqtrailer` · **energy 主组件**: Cerbo GX · **network**: RUT

---

## 端到端验收（EDGE-T3）

| # | 检查 | 路径 |
|---|------|------|
| 1 | Cerbo Modbus 可读 | [`../cerbo/`](../cerbo/) |
| 2 | RUT 发布 G2 energy Topic | [`../rut/`](../rut/) · IoT Test Client |
| 3 | M4 ingest → Timestream | [`04-cloud/docs/deliveries/DELIVERY_M4.md`](../../04-cloud/docs/deliveries/DELIVERY_M4.md) |
| 4 | Legacy `iot/rut241/status` 仍可用 | 双轨共存 |
| 5 | Registry `system_type=iqtrailer` | [`09-contract/examples/iqtrailer/registry-record.v1.json`](../../09-contract/examples/iqtrailer/registry-record.v1.json) |

---

## 与 IQWatch HIL 的区别

| | IQWatch | IQTrailer |
|---|---------|-----------|
| 边缘 | ESP32 · VE.Direct | Cerbo + RUT · Modbus TCP |
| 代码目录 | `01-firmware/` | `05-integration/` |
| energy `component_id` | MPPT SER# | Cerbo ID |
| 007 | ✅ 主责 | ✅ 主责（`05-integration/` · RUT/RutOS） |

---

## 样例 Registry / Payload

- [`09-contract/examples/iqtrailer/registry-record.v1.json`](../../09-contract/examples/iqtrailer/registry-record.v1.json)
- [`09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json`](../../09-contract/examples/energy/telemetry-iqtrailer-cerbo-live.v1.json)
