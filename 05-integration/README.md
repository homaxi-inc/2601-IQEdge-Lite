# 05-integration — 现场网关集成（Cerbo · RUT）

> **状态**: 脚手架 · 2026-05-30  
> **主场景**: **IQTrailer** — Victron Cerbo GX + Teltonika RUT241/956  
> **非职责**: ESP32 固件（→ `01-firmware/`）· AWS CDK（→ `04-cloud/`）· Payload Schema（→ `09-contract/`）

---

## 数据管道（IQTrailer）

```text
MPPT ×N ──► Cerbo GX (Venus OS · Modbus TCP Server)
                  ▲
                  │ Modbus TCP 轮询 / 读寄存器
            RUT241/956 (4G · MQTT Client)
                  │
                  ▼
            AWS IoT Core
       ├─ Legacy: iot/rut241/status …（现网 · M5 不改造）
       └─ G2:     iqedge/g2/{env}/energy/telemetry
                  iqedge/g2/{env}/network/telemetry
                  ▼
            04-cloud M4/M5 Ingest → Timestream / Shadow / Registry
```

与 **IQWatch** 分轨：Watch 为 ESP32 **VE.Direct** → MQTT（`01-firmware/`）；Trailer 不经 ESP32 读 MPPT。

---

## 目录

| 路径 | 职责 |
|------|------|
| [`cerbo/`](cerbo/) | Victron Cerbo · Modbus 寄存器 · Venus OS 读数 |
| [`rut/`](rut/) | RUT MQTT 发布 · Topic 映射 · 脚本/配置 |
| [`iqtrailer/`](iqtrailer/) | Trailer 场景总览 · 验收清单 |
| [`docs/`](docs/) | 集成开发日志 |

---

## Agent 分工

| 角色 | 范围 |
|------|------|
| **Bob / 现场集成** | Cerbo 配置 · Modbus 映射 · 台架验收 |
| **008** | RUT↔AWS · G2 Topic · M4/M5 ingest · Legacy 双轨 |
| **007** | **不总责**；仅 Trailer 上另有 ESP32 子系统时改 `01-firmware/` |

---

## 契约与云端

| 层 | 路径 |
|----|------|
| Energy payload | [`09-contract/schemas/energy/telemetry.v1.json`](../09-contract/schemas/energy/telemetry.v1.json) · `component_role=cerbo_gx` |
| 系统模型 §3.4 | [`02-backend/docs/G2_System_Model.md`](../02-backend/docs/G2_System_Model.md) |
| M4/M5 里程碑 | [`04-cloud/docs/G2_Implementation_Task_Breakdown.md`](../04-cloud/docs/G2_Implementation_Task_Breakdown.md) |

---

## 边缘子任务（文档跟踪）

| ID | 内容 | 状态 |
|----|------|------|
| EDGE-T1 | Cerbo Modbus 寄存器图 | ⬜ |
| EDGE-T2 | RUT → G2 JSON 组装 + Topic | ⬜ |
| EDGE-T3 | 台架验收（对齐 M4/M5） | ⬜ |

---

*Bob · 008 协助 RUT/AWS 对齐 · ADR-011*
