# G2 SIM 激活死锁（Chicken-and-Egg）— 开放架构议题

> **性质**: Agent 008 记录 · Bob 业务直觉归档  
> **状态**: ⏸ **方案待决** — 需与 Granite（或当前 M2M 供应商）能力对齐后选型  
> **日期**: 2026-05-29  
> **关联**: [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §5 · §12 · [`G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)

---

## 1. 问题陈述

物联网部署中的经典 **「先有鸡还是先有蛋」死锁**：

```text
需要设备连网发心跳  →  才能触发 SIM 正式激活 / 计费
但 SIM 未激活       →  设备根本连不上网
```

若不在架构层预先打破此死锁，现场通电后 RUT **长期红灯、无法 MQTT**，存在 **「设备变砖」** 风险 — 尤其 IQWatch / IQTrailer 现场部署与租赁场景。

**008 结论**: 必须在 G2 Provisioning 设计阶段明确解法，**不可假设「出厂即 Active SIM」**。

---

## 2. 候选解法（行业惯例 · 二选一或组合）

> ⚠️ **尚未定案**。选型依赖 Granite（或替代运营商）是否支持解法一。

### 解法一 · Test Ready / Pre-active（运营商微流量过渡态）🌟

**体验**: 现场 **Zero-Touch**，通电即用。

| 阶段 | SIM 状态 | 行为 |
|------|----------|------|
| 出厂 | **Test Ready** / Pre-active | 可注册 4G，**极小配额**（如 100KB–1MB）或 **AWS IoT 域名白名单**；**无月租** |
| 现场首次通电 | RUT 用「生命通道」连 AWS IoT | First Ping / First MQTT |
| 云端闭环 | Lambda 收首包 → 调运营商 API | Test Ready → **Active**；并行 Stripe 开账 |

```text
RUT 通电 → MQTT First Ping → Lambda
    ├─► Granite API: SIM Test Ready → Active
    └─► Stripe: 开启订阅计费
```

| 优点 | 前提 |
|------|------|
| 安装工零触碰 | Granite **必须** 提供无月租 Test Ready / 测试流量池 |
| 自动化程度最高 | IoT 域名白名单需运营商或 APN 侧配合 |

---

### 解法二 · Out-of-Band Activation（带外激活 · 扫码）🛡️

**体验**: 安装工 **手机 5G** 打破死锁；SIM 出厂可 **彻底 Deactivated**，仓库 **零偷跑流量**。

| 阶段 | 状态 |
|------|------|
| 出厂 | SIM **Deactivated**；RUT 通电无网 |
| 现场 | 铭牌 **二维码**（`sys_id` 和/或 **ICCID**） |
| 安装工 | IQCloud **PWA / Dealer Portal** 扫码 →「部署上线 Deploy & Activate」 |
| 后端 | `POST /api/v2/fleet/systems/{sys_id}/activate` |
| 顺序 | ① Granite API 激活 SIM → ② 等待 30s–2min Modem 注网 → ③ 确认 IoT 心跳 → ④ Stripe 扣费 |

```text
安装工手机 (5G)
    → POST .../systems/{sys_id}/activate
        → Granite: Deactivated → Active
        → (wait) RUT 绿灯 → MQTT 基线
        → Stripe 订阅
```

| 优点 | 成本 |
|------|------|
| 极稳健；仓库期无流量风险 | 需 **Dealer Portal** 前端 + 扫码 SOP |
| 不依赖运营商微流量态 | 安装工多一步操作 |

**与遗留项关联**: 即 [`cloud_backend_log.md`](cloud_backend_log.md) 中 **「Provisioning 扫码 SOP」** 的业务根因。

---

## 3. 对比摘要

| 维度 | 解法一 Test Ready | 解法二 OOB 扫码 |
|------|-------------------|-----------------|
| 现场操作 | 零触碰 | 扫码 + 一键激活 |
| 仓库 SIM | 微流量态（需管控配额） | 完全停机 |
| 运营商依赖 | **高**（Granite 能力） | **低**（API 激活即可） |
| 云端 | First Ping Lambda | Activate API + 心跳确认 |
| 前端 | 可选 | **PWA Dealer Portal** |
| 计费 | First Ping → Stripe | 心跳确认 → Stripe |

---

## 4. G2 预留（方案未定，先占位）

### 4.1 Registry / deployment_state

| 字段 / 状态 | 用途 |
|-------------|------|
| `components.network[].iccid` | Granite API 激活键 |
| `sim_lifecycle` | `deactivated` \| `test_ready` \| `active`（待定枚举） |
| `deployment_state` | 激活成功后 → `deployed` |
| `first_ping_at` | 解法一闭环时间戳 |
| `activated_by` | 解法二：`installer_user_id` / OOB |

### 4.2 API 预留（解法二 · 解法一也可能复用确认端点）

```http
POST /api/v2/fleet/systems/{sys_id}/activate
Content-Type: application/json
Authorization: Bearer <installer-token>

{
  "iccid": "8944...",
  "site_ref": "optional",
  "confirm_stripe": true
}
```

响应：`202 Accepted` + `activation_job_id`；异步轮询或 Webhook 确认 IoT 首心跳。

### 4.3 云端组件（待选方案后 CDK 化）

| 组件 | 解法一 | 解法二 |
|------|--------|--------|
| Lambda `fn-sim-first-ping` | ✅ IoT Rule 触发 | 可选（心跳确认） |
| Lambda `fn-sim-activate` | 调 Granite API | ✅ Activate API 调用 |
| Secrets | Granite API Key | 同左 |
| Integration | Stripe webhook | 同左 |

### 4.4 硬件 / 产线

- 机箱外 **二维码铭牌**：`sys_id` + ICCID（解法二必备；解法一亦建议用于追溯）
- 出厂 SIM 状态写入 Registry（`sim_lifecycle`）

---

## 5. 待决事项（Bob / 商务 / Granite）

- [ ] **向 Granite 确认**：是否支持 **Test Ready / Pre-active**、配额、AWS 域名白名单、无月租测试池
- [ ] **择一或组合**：Test Ready 优先 vs 纯 OOB vs Test Ready 失败时 OOB 降级
- [ ] Stripe 与 SIM Active 的 **严格顺序**（先 SIM 还是先合同）
- [ ] IQCloud PWA Dealer Portal 是否纳入 Phase 1
- [ ] 安装商身份鉴权（installer JWT / 一次性激活码）

---

## 6. 008 立场

1. **今日仅记录，不实施** — 避免在无 Granite 能力确认前写死 CDK。
2. **倾向调研顺序**：先问 Granite（解法一成本最低）→ 若不支持，解法二为 **可靠兜底**。
3. **无论哪解法**，Registry 必须存 **ICCID** 并与 `sys_id` 绑定；**禁止**假设出厂 SIM 已 Active。

---

*Agent 008 · Open Architecture Issue · 方案后续再定*
