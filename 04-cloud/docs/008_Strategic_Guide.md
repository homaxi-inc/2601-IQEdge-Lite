# Agent 008 战略指导 — 双轨并行（Dual-Track）

> **性质**: Bob 定稿思路，008 启动工作的首要参考  
> **状态**: 活文档 — 后续逐步补充  
> **关联**: [`G2_Cloud_Deployment_Audit_2026-05-29.md`](G2_Cloud_Deployment_Audit_2026-05-29.md) · [`G2_Domain_Map.md`](G2_Domain_Map.md) · [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md)

---

## 核心原则

**禁止「一刀切」重构。** 不为迁就 G2 而打烂已在线运行的老设备与老云端。

| 轨道 | 范围 | 008 职责 |
|------|------|----------|
| **老轨 Legacy** | 已部署设备 + 现有 AWS 资源（DDB `DeviceLatestStatus`、现有 IoT Rules 等） | **纯维护 / 不折腾**；锁流量在老架构内 |
| **新轨 G2** | 新批次设备 + 新部署 | **IaC 全量新建**（CDK/CFN）；dev/prod 隔离；模块化一键部署 |

---

## 三条铁律

### 1. 老设备 → 老架构（求稳）

- 固件、证书、DDB 表结构、IoT Rules **已通过时间检验** → 进入维护态，**不重构**。
- 老流量物理隔离在老架构 → 为 G2 提供安全后方。

### 2. 新部署 → G2 新架构（轻装）

- 新批次：**全新** Timestream 表、IoT Rules、Lambda — 与老资源 **绝对隔离**。
- 新架构目标：高并发、低成本、AI 联动；IaC 一行命令为新批次开辟干净环境。
- 仓库映射：`01-firmware` · `02-backend` · `04-cloud`（IaC 主战场）。

### 3. 唯一纽带 → `02-backend` 数据合流

新老在 **硬件 / 固件 / 云端存储** 层完全隔离；在 **后端 API** 层统一出口：

```text
API 请求 (deviceId)
    │
    ├─ 老设备 ID ──► Legacy 路径 ──► 老 DynamoDB ──► 老算法转换
    │
    └─ G2 设备 ID ──► G2 路径     ──► 新 Timestream ──► 正确比例 (如 kWh /1000.0)
```

- 前端 / App / 业务层 **无感** — 同一看板展示新老设备。
- **008 核心交付**：设备 ID 路由逻辑 + 双轨读路径 + 统一 API 契约。

---

## 008 工作边界（当前）

| ✅ 做 | ❌ 不做 |
|-------|---------|
| G2 新轨 CDK / Timestream / IoT Rules | 迁移或改造老 DDB / 老 Rules |
| `02-backend` 双轨 API 合流层 | 强制老设备升级 G2 固件 |
| dev 沙箱一键部署 | 线上老架构「大重构」 |
| 五域命名（energy 等）仅用于 **新轨** | 用新命名覆盖老表/老 API |

> **五域细则** → 见 [`G2_Domain_Map.md`](G2_Domain_Map.md)（Topic / API / 表名完整矩阵）

---

## G2 命名体系（Bob 定稿）

> **仅适用于新轨 G2 资源**；老架构名称（如 `device/status`、`DeviceLatestStatus`）**禁止改动**。  
> **责任人**：云端资源 → **008**（CDK）；固件 MQTT Topic → **007**（须与下表对齐）。

### 命名模式

```text
iqedge-g2-{env}-{resource-type}-{purpose}     # IoT Rule / DDB / IAM（连字符）
iqedge_g2_{env}_{resource-type}_{purpose}     # Timestream DB/Table（下划线）
iqedge/g2/{env}/{domain}/{telemetry|event|command}   # MQTT Topic（五域分层）
/api/v2/{domain}/{action}                     # 后端 API（02-backend）
```

**五域**: `energy` · `network` · `vision` · `environment` · `control` — 完整看板见 [`G2_Domain_Map.md`](G2_Domain_Map.md)。

`{env}` = `dev` | `prod`（铁律：dev/prod 物理隔离）

### 对照表（dev 示例 — 以 energy 为代表）

| AWS 组件 | ❌ 老架构（勿动） | 🟢 G2 新命名（dev） |
|----------|------------------|---------------------|
| IoT Core Topic | `device/status` | `iqedge/g2/dev/energy/telemetry` |
| IoT Rule | `DeviceStatusToLambda` | `iqedge-g2-dev-rule-energy` |
| Timestream Database | `IQWatchDB` | `iqedge_g2_dev_database` |
| Timestream Table | `DeviceStatus` | `iqedge_g2_dev_table_energy` |
| DynamoDB 状态影子表 | `DeviceLatestStatus` | `iqedge-g2-dev-table-shadow` |
| IAM Role / Policy | `iot_lambda_role` | `iqedge-g2-dev-role-iot-ingest` |
| API 路由 | `/devices/{id}/status` | `/api/v2/energy/status` |

其余四域 Topic / API / 表名 → [`G2_Domain_Map.md`](G2_Domain_Map.md)。

### prod 替换规则

将路径/名称中的 `dev` 换为 `prod` 即可，例如：

- Topic: `iqedge/g2/prod/energy/telemetry`
- Rule: `iqedge-g2-prod-rule-energy`
- Table: `iqedge_g2_prod_table_energy`

### 008 执行要点

1. CDK Stack 内 **所有** 新资源必须遵循上表，禁止 `My*`、`device_*` 等历史随意命名。
2. Lambda 函数名建议同前缀：`iqedge-g2-dev-fn-telemetry-ingest`（后续 Bob 可补充 fn 细则）。
3. 固件仍发 `device/status` 的设备走 **老轨**；G2 新批次按 **五域 Topic** 发布（如 `…/energy/telemetry`），与老 Rule 并存。

---

## 待 Bob 后续补充

- [x] ~~sys_id 发号规则~~ → 见 [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §4
- [ ] G2 新轨首批上线批次与时间线
- [ ] dev / prod 账号或 Stack 隔离细则
- [ ] Lambda / API Gateway 命名细则

---

*2026-05-29 · Bob 定稿 · Agent 008 归档 · 命名体系同日追加*
