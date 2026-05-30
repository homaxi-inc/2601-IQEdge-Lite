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
| [`docs/vrm-api-local-credentials.md`](docs/vrm-api-local-credentials.md) | VRM API 本地凭据说明（`.env` gitignored） |
| [`docs/`](docs/) | 集成开发日志 · open issues |

---

## 007 本地起步

1. 复制凭据：`cp 05-integration/.env.example 05-integration/.env`（或从 `003_IQTrailer/.env` 同步 · **勿提交**）  
2. VRM 预检：`python 05-integration/scripts/vrm_cerbo_preflight.py`  
3. 交接报告：[`docs/deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md`](docs/deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md)  
4. Modbus 寄存器：[`cerbo/modbus-register-map.md`](cerbo/modbus-register-map.md) · EDGE-T2：[`rut/README.md`](rut/README.md)

---

| 角色 | 范围 |
|------|------|
| **007** | **主责** `05-integration/` — Cerbo / Venus OS · Modbus 映射 · **RUT / RutOS**（Modbus Client · Data to Server）· 台架验收 · EDGE-T1/T2/T3；`01-firmware/` 仍主责 ESP32 |
| **008** | **云端对齐** — G2 Topic · IoT Policy/Thing · M4/M5 ingest · Legacy 双轨；与 007 联调 payload，**不另改 Schema**（`09-contract/`） |
| **Bob** | 产品决策 · 现场资源 · 验收签收 |

> **授权（Bob）**: 007 可读写 **`05-integration/`** 全目录；RUT/RutOS 底层协议以 007 专长为准，008 负责 AWS 侧契约与 ingest 闭环。

---

## 契约与云端

| 层 | 路径 |
|----|------|
| Energy payload | [`09-contract/schemas/energy/telemetry.v1.json`](../09-contract/schemas/energy/telemetry.v1.json) · `component_role=cerbo` |
| 系统模型 §3.4 | [`02-backend/docs/G2_System_Model.md`](../02-backend/docs/G2_System_Model.md) |
| M4/M5 里程碑 | [`04-cloud/docs/G2_Implementation_Task_Breakdown.md`](../04-cloud/docs/G2_Implementation_Task_Breakdown.md) |

---

## 边缘子任务（文档跟踪）

| ID | 内容 | 状态 |
|----|------|------|
| EDGE-T1 | Cerbo Modbus 寄存器图 | ✅ MVP（ADR-012 D-9） |
| EDGE-T2 | RUT Data to Server → G2 JSON | ⬜ |
| EDGE-T3 | 台架验收（对齐 M4/M5） | ⬜ |

---

*007 主责边缘集成 · 008 云端对齐 · ADR-011*
