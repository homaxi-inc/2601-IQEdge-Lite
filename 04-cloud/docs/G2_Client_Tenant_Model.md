# G2 Client 租户层 — SaaS 商业架构（开放议题）

> **性质**: Bob 定稿商业逻辑 · Agent 008 架构存档  
> **状态**: ⏸ **方案待决** — 租户模型 / RBAC / API 隔离细则后续确定  
> **日期**: 2026-05-29  
> **关联**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) · [`G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md) · [`G2_Customer_User_Frontend_Blueprint.md`](G2_Customer_User_Frontend_Blueprint.md)

---

## 0. 黄金三角（Admin · Dealer · Client）
Admin（IQCloud 平台）  →  造平台、定规则、收平台费
Dealer（区域集成商）   →  管资产、SIM、部署、赚服务费
Client（终端付费方）   →  用安防、看录像、布撤防、付 RMR
```

**纯卖硬件** 模式可能 **不需要 Client 层**；  
**IQCloud「卖服务 + 履约 + SaaS 资产运营」** 闭环中，Client 层 **必须存在** — 是高毛利 **RMR（Recurring Monthly Revenue）** 的核心支点。

### 0.1 术语：Client ≈ Customer（组织）

本文 **Client** = 终端付费 **商业组织（Tenant）** = 前端蓝图中的 **Customer**。  
与 **User（自然人）** 的区分见 [`G2_Customer_User_Frontend_Blueprint.md`](G2_Customer_User_Frontend_Blueprint.md)。Registry 字段 `client_id` 可演进为 `customer_id`（**待决**）。

---

## 1. Client 是谁？（Who）

**Client** = **为拖车租金和监控服务最终买单的人**。

| 典型 Client 群体 | 场景 |
|------------------|------|
| **建筑承包商** (Construction PM) | 新楼盘工地防盗（钢筋、铜线） |
| **汽车 4S 店总经理** | 露天停车场防盗窃、三元催化器 |
| **农场主 / 矿山主管** | 偏远重型机械、水泵看护 |
| **警察局 / 大型活动主办方** | 音乐节、游行等 **临时租用** |
| **居民小区 HOA** | 无电死角监控塔（经 Dealer 部署） |

---

## 2. 为什么 Client 层绝对必须？（Why）

### 2.1 隐私隔离（Data Privacy & Siloing）

**反例（无 Client 层）**: Dealer 把同一账号给 10 个工地 → 工地甲能看到 4S 店录像、别人家拖车位置 → **北美/欧洲隐私诉讼风险**。

**正例（有 Client 层）**: 工地甲 **独立账号** → 仅见 **分配给自己的 sys_id**（如 3 台 IQTrailer）。

### 2.2 SaaS 变现引擎（Monetization）

| 仅工具后台 | + Client 门户 / App |
|------------|-------------------|
| 看电量、4G 流量 | Dealer 对 Client 加收 **+$50/月**：“专属 App、录像、推送” |
| 低溢价 | **Client 层 = Dealer 的「包装盒」** → 高毛利 RMR |

### 2.3 运营减负（Self-Service）

**痛点**: 工地加班需 **临时撤防** → 无 Client 权限则只能打 Dealer 电话 → Dealer 被琐事淹没。

**Client 能力（待决权限）**: 手机 **Arm / Disarm**、**暂停报警 2h** 等 **局部 control** — 见 control 域 `disarm` 命令。

### 2.4 责任审计（Liability Audit Trail）

误报 / 漏报扯皮 → **`control_logs` + Client 身份** 铁证：

```text
2026-05-29 20:00 UTC · Client (工地甲 · 保安 John)
  → POST .../control/command · action=disarm · sys_id=IQ-26-06077
```

---

## 3. Dealer 视图 vs Client 视图

| 维度 | 👨‍🔧 **Dealer** | 👷 **Client** |
|------|----------------|---------------|
| **关注点** | 设备坏没坏、流量、电池 SoC | 昨晚有没有小偷、现场人在干嘛 |
| **地图** | 全州 50 台 **全局** | **仅分配** 的几台 |
| **高频界面** | SIM 超额、SoC 预测、上下线日志 | 4K 实况、AI 报警列表、录像回放 |
| **control** | 全量运维命令 | **受限**：布撤防、暂停报警（待决白名单） |
| **commercial** | 租约、计费、Client 分配 | 仅自己的订阅状态（待决） |

---

## 4. G2 技术预留（方案待决 ⏸）

> MVP **不实现** 多租户；Registry / API / Cognito 设计 **须留扩展位**，避免日后推倒重来。

### 4.1 租户层级（草案）

```text
tenant_iqcloud (Admin)
  └── dealer_id
        └── client_id
              └── sys_id[]   （Fleet 资源挂载点）
```

### 4.2 Registry 扩展字段（占位）

```json
{
  "sys_id": "IQ-26-06077",
  "dealer_id": "dealer-homaxi-phoenix",
  "client_id": "client-buildco-site-alpha",
  "client_assigned_at": "2026-06-01T00:00:00Z",
  "commercial": {
    "ownership_model": "leased",
    "lessee_ref": "client-buildco-site-alpha"
  }
}
```

### 4.3 API / 鉴权预留

| 角色 | Fleet API 范围 |
|------|----------------|
| `admin` | 全平台 |
| `dealer` | `dealer_id` 下全部 `sys_id` |
| `client` | **仅** `client_id` 绑定的 `sys_id` |

**Scope 草案（Cognito / JWT）**: `dealer:read` · `client:read` · `client:control:disarm`（细粒度 **待决**）

**列表过滤**:

```text
GET /api/v2/fleet/systems?dealer_id=...     # Dealer
GET /api/v2/fleet/systems                   # Client → 后端强制 filter by client_id
```

### 4.4 与现有模型关系

| 已有概念 | 关系 |
|----------|------|
| `sys_id` | 物理资产 ID **不变**；Client 是 **使用权** 挂载 |
| `commercial.lessee_ref` | 可能与 `client_id` **合并或映射**（待决） |
| `deployment_state` | Dealer 运维；Client 仅见 `deployed` 且已分配设备 |
| `control_logs` | **必须** 记录 `actor_type: dealer|client` + `actor_id` |

### 4.5 前端（Roadmap · 不在 008 MVP）

| 产品 | 用户 |
|------|------|
| **Dealer Portal** | IT / 资产健康 |
| **Client App / PWA** | 安防、录像、布撤防 |
| IQCloud Admin | IQ 内部 |

---

## 5. 待决清单

- [ ] `client_id` 发号规则 vs 复用 CRM UUID
- [ ] 一 `sys_id` 是否可 **临时** 租给多 Client（音乐节场景）
- [ ] Client **control 白名单**（仅 disarm？还是含 Talk-down？）
- [ ] Stripe：Dealer 统一扣费 vs Client 直付
- [ ] 与 **租赁 `lease_status`** 联动（overdue 是否冻结 Client 登录）
- [ ] 数据驻留 / 合规（GDPR、CPRA）按 Client 隔离级别

---

## 6. 008 立场

1. **纯硬件出货** 可不启用 Client 租户；**SaaS RMR 路径必须预留 Client 层**。
2. **Fleet API** 当前以 `sys_id` 为中心设计 **正确**；上线多租户时在 **Auth 中间件 + Registry 过滤** 加层，**不改 sys_id 主键**。
3. **今日仅存档**；不在 CDK / FastAPI 实现 RBAC。

---

*Agent 008 · Client Tenant Model · 方案后续再定*
