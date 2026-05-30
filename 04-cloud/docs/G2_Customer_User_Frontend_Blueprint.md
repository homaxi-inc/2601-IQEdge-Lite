# G2 Customer vs User — 前端架构蓝图（开放议题）

> **性质**: Bob 定稿 · B2B2C SaaS 身份模型 · Agent 008 存档  
> **状态**: ⏸ **方案待决** — 前端/React/PWA 与后端 RBAC 细则后续确定  
> **日期**: 2026-05-29  
> **关联**: [`G2_Client_Tenant_Model.md`](G2_Client_Tenant_Model.md) · [`G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)

---

## 0. 核心辨析（最易踩坑）

在 B2B2C Fleet Management + 物理硬件 SaaS 中，**Customer** 与 **User** 必须 **完全解耦**：

| 概念 | 本质 | 类比 | 拥有什么 |
|------|------|------|----------|
| **Customer** | 商业实体 / **Tenant（租户）** | 「加州第一建筑公司」 | 资产（`sys_id`）、账单、品牌 |
| **User** | 自然人 / **登录账号** | 包工头 John、接警员 Mary | **不拥有**拖车；被邀请加入 Customer，带 **Role** |

```text
❌ 混淆：一个 Login = 一个公司 = 拥有设备
✅ 正解：Customer 拥有 sys_id[] · User ∈ Customer · Role 决定 UI/API
```

### 与既有「Client」术语映射（待法务/UI 统一）

| 本文 | [`G2_Client_Tenant_Model.md`](G2_Client_Tenant_Model.md) | 说明 |
|------|----------------------------------------------------------|------|
| **Customer** | **Client**（终端付费组织） | 同一商业实体；Registry 字段暂用 `client_id`，可演进为 `customer_id` |
| **Dealer** | **Dealer** | 不变；Dealer 本身也可能是 IQCloud 的 **Customer**（向 Admin 付费） |
| **User** | （未单独成文） | 本文补齐 |

**租户栈（草案）**:

```text
Admin (IQCloud)
  └── Dealer (Customer of IQCloud)
        └── Customer / Client (终端租户)
              └── User[] + Role
                    └── 可见 sys_id[] ⊆ Customer 分配
```

---

## 1. 全平台角色一览（待决枚举）

| 层级 | 实体 | 典型角色 |
|------|------|----------|
| 平台 | Admin | IQCloud 大管家 |
| 经销商 | Dealer | Dealer Admin / Dealer Operator |
| 终端租户 | Customer | Customer Admin / Operator / Viewer |
| 自然人 | User | 跨 Customer **不可**（除非 Admin  impersonation） |

**RBAC 角色示例（Customer 内）**:

| Role | 能力方向 |
|------|----------|
| **Admin** | 全菜单、邀请 User、资产只读、团队管理 |
| **Operator** | 大屏、视频、报警、有限 control（警报灯） |
| **Viewer** | 仅看视频；control 置灰 |

Dealer 侧角色 **待扩展**（IT 运维 vs 销售）。

---

## 2. Customer 模块（组织 / 商业 · 前端独立域）

**定位**: 商业契约、资产分配、品牌隔离。  
**主要用户**: Admin、Dealer Owner。

### 2.1 Client Roster — 客户生命周期看板

| UI | 说明 |
|----|------|
| Data Grid | 搜索、过滤、分页 |
| Dealer 列 | 客户名、部署拖车数、流量报警、账单 Active/Suspended |
| Drawer | 点击行 → 滑出详情 → 该 Customer 的 `sys_id` 列表 |

**API 预留**: `GET /api/v2/tenants/customers` · `GET .../customers/{customer_id}/systems`

### 2.2 Asset Allocation — 资产划拨引擎

**痛点**: 拖车流动 — 今月 A 工地，下月 B 工地。

| UI | 说明 |
|----|------|
| Transfer List / 拖拽看板 | 左：Dealer **Inventory** · 右：目标 **Customer** |
| 动作 | 勾选 ST-05 → 划拨至「甲工地」 |

**API 预留**: `PATCH /api/v2/fleet/systems/{sys_id}/allocation`  
Body: `{ "customer_id": "...", "effective_at": "..." }` → 更新 Registry `client_id` / `customer_id`

### 2.3 White-labeling — 白标设置

| UI | 说明 |
|----|------|
| 品牌表单 | Logo、Primary HEX、自定义域名 `portal.dealer-name.com` |

**存储**: Customer 或 **Dealer 级** Tenant 配置表（**待决**层级）。

### 2.4 Billing & Quota — 账单与用量

| UI | 说明 |
|----|------|
| Stripe Customer Portal | iframe / 内嵌 |
| 用量 | SIM 聚合流量进度条；超标预警 |

---

## 3. User 模块（身份 / 团队 · 前端独立域）

**定位**: 谁可以登录、登录后能干什么。  
**主要用户**: Dealer Admin、Customer Admin。

### 3.1 Auth Flows — 身份验证

| 要求 | 说明 |
|------|------|
| 登录 / 注册 / 忘密 | 标准流程 |
| **MFA 强引导** | 视频隐私合规；SMS / TOTP |
| **禁止自助乱入** | B2B **邀请制** 为主（见 3.2） |

**后端**: Cognito / 等价 IdP（**待决**）。

### 3.2 Team & Invitations — 团队邀请

| UI | 说明 |
|----|------|
| 邀请 Modal | 邮箱 + Role → 发邮件 |
| 状态列 | Active / Pending |

**API 预留**: `POST /api/v2/tenants/customers/{customer_id}/invitations`

**规则**: User **不能**自行注册绑定 Customer（防越权）。

### 3.3 RBAC — 视觉降级（UI Degradation）

**原则**: 权限判定在 **后端 + JWT**；前端只做 **菜单隐藏 / 按钮 Disabled**，不做安全边界。

```text
JWT claims (草案):
  user_id, customer_id, dealer_id, role, scopes[]
```

| role | 隐藏示例 |
|------|----------|
| Operator | Billing、Team、Asset Allocation |
| Viewer | control 全部 Disabled |

### 3.4 Profile & Audit — 个人中心与审计展示

| UI | 说明 |
|----|------|
| Profile | 头像、改密、MFA 管理 |
| 资产页审计条 | 「5/29 20:00 · User John (1024) 远程关闭 PoE」 |

**数据源**: `control_logs` + `actor_user_id` · `actor_customer_id`

---

## 4. Dealer 视图 vs Customer(User) 视图（UI 信息架构）

| 模块 | Dealer Portal | Customer Portal (User) |
|------|---------------|---------------------------|
| 首页 | 全州设备地图、SoC、SIM 告警 | 分配拖车实况、AI 报警列表 |
| Customer 模块 | ✅ Roster、划拨、白标 | ❌ 不可见 |
| User 模块 | ✅ 本 Dealer 团队 | ✅ 本 Customer 团队 |
| Vision | 运维级 | 4K 流、回放轴 |
| Control | 全量运维 | 受限（Arm/Disarm、警报灯） |
| Billing | Dealer 向 IQCloud + 向下游 | 仅自己的订阅（可选） |

---

## 5. 后端 / Registry 预留（与前端解耦）

| 实体 | ID 字段 | 关系 |
|------|---------|------|
| Dealer | `dealer_id` | 1:N Customer |
| Customer | `customer_id`（=`client_id` 演进） | 1:N User；N:M `sys_id`（随时间） |
| User | `user_id` | N:1 Customer；带 `role` |
| System | `sys_id` | `customer_id` 可空 = Inventory |

**control_logs 扩展**:

```json
{
  "actor_user_id": "user-1024",
  "actor_customer_id": "customer-buildco-alpha",
  "actor_dealer_id": "dealer-phoenix",
  "action": "disarm"
}
```

---

## 6. 前端工程原则（[`03-frontend/`](../../03-frontend/)）

```text
03-frontend/                    ← 仓库目录已创建 · 2026-05-30
  modules/
    customer/     ← Roster, Allocation, White-label, Billing
    user/         ← Auth, Team, RBAC guards, Profile
    fleet/        ← 共用 sys_id 看板（按 JWT 过滤）
  shared/
    auth/         ← JWT parse, role hooks
```

**铁律**: `customer` 与 `user` **不同路由域、不同 Store 切片**；禁止 `useLogin()` 同时承载组织与自然人状态。

---

## 7. 待决清单

- [ ] 统一命名：`client_id` → `customer_id` 是否一次性迁移
- [ ] Dealer 是否同时是 IQCloud 的 Stripe Customer（双层 Billing）
- [ ] MFA 强制策略：Dealer Admin vs Customer Viewer 是否分级
- [ ] Impersonation（Admin 代登录 Dealer）合规流程
- [ ] 一 User 隶属多 Customer 是否允许（兼职保安场景）
- [ ] 03-frontend 技术栈（React vs Vue vs PWA-only）— 目录已建，见 [`03-frontend/README.md`](../../03-frontend/README.md)

---

## 8. 008 立场

1. **Customer ≠ User** — 前端必须双模块；与 [`G2_Client_Tenant_Model.md`](G2_Client_Tenant_Model.md) **互补**（商业 Why + 本文件 UI How）。
2. **Fleet API `sys_id` 设计不变**；租户过滤在 Auth 层。
3. **今日仅存档**；不实现 Cognito / 前端脚手架。

---

*Agent 008 · Customer/User Frontend Blueprint · 方案后续再定*
