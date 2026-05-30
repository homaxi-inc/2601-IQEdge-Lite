# 09-contract — G2 API & Ingest 契约层

> **性质**: G2 新轨 **单一事实来源（SSOT）**  for JSON Schema、OpenAPI 与 ingest 校验。  
> **消费者**: `04-cloud/cdk` Lambda ingest · `02-backend` FastAPI · CI 校验门禁  
> **原则**: Legacy `device/status` payload **不在此目录维护**（仅 adapter 层转换）；G2 五域 Schema 由此发布。  
> **sys_id**: ADR-004 `IQ-{YY}-{NNNNN}` — [`decisions/README.md`](../decisions/README.md)

---

## 目录结构

```text
09-contract/
  README.md                 ← 本文件
  schemas/
    README.md               ← Schema 版本与命名约定
    energy/                 ← energy 域 telemetry / 快照
    network/
    vision/                 ← event + VQA telemetry
    environment/
    control/                ← command + audit
    registry/               ← 设备注册表项
  openapi/
    README.md               ← OpenAPI 导出约定（M0.4）
    v2/                     ← 生成物目录（勿手改）
```

---

## 版本约定

| 元素 | 规则 | 示例 |
|------|------|------|
| 文件名 | `{message-type}.v{major}.json` | `telemetry.v1.json` |
| `$id` | `https://iqedge.io/schemas/g2/{domain}/{name}/{version}` | 见各 Schema 内 |
| 破坏性变更 | **递增 major**（v1 → v2） | 禁止静默改字段含义 |
| 与固件对齐 | energy v1 须对齐 `01-firmware/src/payload.md` v2 flat 字段 | M0.3 任务 |

---

## 五域 message-type（草案）

| 域 | 目录 | 典型 message-type | MQTT / API |
|----|------|-------------------|------------|
| energy | `schemas/energy/` | `telemetry` | `…/energy/telemetry` |
| network | `schemas/network/` | `telemetry` | `…/network/telemetry` |
| vision | `schemas/vision/` | `event`, `telemetry` | `…/vision/event` · `…/vision/telemetry` |
| environment | `schemas/environment/` | `telemetry` | `…/environment/telemetry` |
| control | `schemas/control/` | `command`, `audit` | `…/control/command` |
| registry | `schemas/registry/` | `device-record` | Provisioning / DDB |

---

## 校验（路线图）

```powershell
# M0.3 — 校验 energy 样例
cd 09-contract && npm install && npm run validate:energy
```

CI 将在 `09-contract` 变更时运行 Schema 校验（见 M0.5）。

---

## 关联文档

| 文档 | 路径 |
|------|------|
| 五域命名宪法 | [`04-cloud/docs/G2_Domain_Map.md`](../04-cloud/docs/G2_Domain_Map.md) |
| API 架构 | [`02-backend/docs/G2_API_Architecture_Draft.md`](../02-backend/docs/G2_API_Architecture_Draft.md) |
| 系统模型 / Registry | [`02-backend/docs/G2_System_Model.md`](../02-backend/docs/G2_System_Model.md) |
| 固件 Payload 样例 | [`01-firmware/src/payload.md`](../01-firmware/src/payload.md) |

---

*Agent 008 · 09-contract · M0.2 建档*
