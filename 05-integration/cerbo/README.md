# Cerbo GX — Victron 能源网关

> **角色**: IQTrailer **energy SSOT** · 单台 Cerbo 汇聚多 MPPT（Modbus）  
> **协议**: **Modbus TCP**（Cerbo 为 Server，RUT 或集成 PC 为 Client）

---

## 范围（Bob 定稿）

- 每台 Trailer **1× Cerbo GX**（不考虑多 Cerbo）。
- 云端 **不逐 MPPT 聚合**；API/Registry 以 **Cerbo `component_id`** 为主读组件。
- 明细优先来自 **Cerbo / Venus OS**，不在云端拆 MPPT。

---

## 待填文档

| 文件 | 说明 |
|------|------|
| [`docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md) | **寄存器 + RUT Client**（PDF V1.0 + Excel 3.73） |
| [`docs/G2_Cerbo_Modbus_Connectivity.md`](docs/G2_Cerbo_Modbus_Connectivity.md) | Modbus 连接与台架网络 |
| [`docs/G2_Cerbo_Energy_Field_Reference.md`](docs/G2_Cerbo_Energy_Field_Reference.md) | 物理量 → G2 Schema |
| [`modbus-register-map.md`](modbus-register-map.md) | MVP 寄存器快查（ADR-012 D-9） |
| [`docs/open_issues.md`](docs/open_issues.md) | D-6 / D-10 遗留问题 |
| `venus-os-notes.md` | 设备 ID、网络、Modbus 端口（默认 502） |

---

## 与 RUT 的接口

Cerbo **不直连 AWS**。读数由 [`../rut/`](../rut/) 轮询 Modbus 后组装 G2 JSON 发布 MQTT。

---

## 关联

- [`../iqtrailer/README.md`](../iqtrailer/README.md)
- [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §3.4
