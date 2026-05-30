# G2 五大业务领域模型（Domain Map）

> **性质**: Bob 定稿 · G2 顶层命名宪法  
> **状态**: 活文档 — 后续逐步补充  
> **适用**: 新轨 G2  only（老架构 `device/status`、`IQWatchDB` 等 **禁止改动**）  
> **关联**: [`008_Strategic_Guide.md`](008_Strategic_Guide.md) · [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md) · [`02-backend/docs/G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)

---

## 建模原则

物理世界所有外设归入 **五个顶级领域**。Topic、API、Timestream/DynamoDB 表名 **必须是这五词的排列组合**，禁止引入混乱旧命名。

| 定名 | 弃用别名 | 选定 |
|------|----------|------|
| 视觉域 | media | **vision** |
| 环境域 | telemetry（作域名） | **environment** |
| 控制域 | actuator | **control** |

---

## 五域定义

| 域 | 本质 | 物理对象 | 数据特征 |
|----|------|----------|----------|
| **energy** | 能量流 | Solar、Battery、MPPT、逆变器、Load | V/I/P、Wh/kWh；发电/储电/用电 |
| **network** | 信息流 | RUT241/956、ESP32 RSSI、SIM 流量、GPS | 在线、连通、运维健康 |
| **vision** | 视频/图像流 | Camera、流媒体、SD 录像、抓拍 | 二进制图像/视频、AI 视觉事件 |
| **environment** | 环境状态流 | 温湿度、风速风向、Lux、气压（Modbus 传感器） | 只读生态气象指标 |
| **control** | 反向控制流 | Relay、DO/DI、警铃、补光灯 | **唯一下行** — 开/关/触发/反转 |

```text
energy        ──► 发电 / 储电 / 用电
network       ──► 在线 / 不失联 / 流量可控
vision        ──► 录像 / 抓拍 / 边缘 AI 事件
environment   ──► 大自然只读指标
control       ──► 改变物理世界（双向中的下行）
```

---

## 命名宪法看板（dev 示例）

> `{env}` = `dev` | `prod` — 全局替换即可。

| 域 | IoT Topic | API 路由 (v2) | Timestream 表 |
|----|-----------|---------------|---------------|
| **energy** | `iqedge/g2/dev/energy/telemetry` | `/api/v2/energy/status` | `iqedge_g2_dev_table_energy` |
| **network** | `iqedge/g2/dev/network/telemetry` | `/api/v2/network/status` | `iqedge_g2_dev_table_network` |
| **vision** | `iqedge/g2/dev/vision/event` | `/api/v2/vision/stream` | `iqedge_g2_dev_table_vision` |
| **environment** | `iqedge/g2/dev/environment/telemetry` | `/api/v2/environment/metrics` | `iqedge_g2_dev_table_environment` |
| **control** | `iqedge/g2/dev/control/command` | `/api/v2/control/execute` | `iqedge_g2_dev_table_control_logs` |

### 共享基础设施命名

| 组件 | dev 命名 |
|------|----------|
| Timestream Database | `iqedge_g2_dev_database` |
| DynamoDB 状态影子表 | `iqedge-g2-dev-table-shadow` |
| IoT Rule（示例） | `iqedge-g2-dev-rule-{domain}` |
| IAM Role | `iqedge-g2-dev-role-iot-ingest` |

### Topic 末段约定

| 末段 | 方向 | 用于 |
|------|------|------|
| `telemetry` | 上行 | energy / network / environment 时序数据 |
| `event` | 上行 | vision AI/告警事件 |
| `command` | 下行 | control 指令 |

---

## 008 / 007 分工

| 层 | 负责人 | 要求 |
|----|--------|------|
| IoT Topic（固件 publish） | **007** | 按域发布至上表 Topic |
| IoT Rule + Lambda + DB | **008** | CDK 按域建 Rule/表，一域一表 |
| API 路由 | **008** | `02-backend` 前缀 `/api/v2/{domain}/…` |
| Legacy 合流 | **008** | 老设备仍走 Legacy API；G2 设备走 v2 域路由 |

---

## 与 Legacy 对照（勿混用）

| G2 域 | 老架构近似物 | 关系 |
|-------|-------------|------|
| energy | `device/status` → `DeviceLatestStatus` / `IQWatchDB` | 老轨锁定；G2 MPPT 进 `table_energy` |
| network | `iot/rut241/status` → RUT DDB/Timestream | 老轨锁定；G2 进 `table_network` |
| vision | `IQCamera*` / `mm8hc3ud9c` API | 待 G2 迁入 `table_vision` |
| environment | — | 新域 |
| control | `device/command` / IoT Jobs OTA | 下行统一进 `control`；OTA 细则待补 |

---

## 待 Bob 后续补充

- [ ] 各域 DynamoDB 影子表是否分表 or 共用 `table-shadow` + domain 分区键
- [ ] control 域 OTA / Jobs 与 `command` Topic 边界
- [ ] vision `event` vs `stream` API 语义细则

---

*2026-05-29 · Bob 定稿 · Agent 008 归档*
