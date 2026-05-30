# IQCloud 商业战略与产品定位白皮书 (Product & Commercial Strategy)

> **定位**: IQCloud 项目的「商业宪法」与最高产品指南。  
> **阅读要求**: 任何参与前端、后端、架构与固件开发的团队成员，在编写代码前须理解本文，确保技术实现与商业变现目标对齐。  
> **状态**: 战略定稿 · 具体定价与合同条款见内部商务资料（本文不含敏感商业数据）

---

## 1. 我们到底在造什么？ (Product Identity)

IQCloud **不是** 传统的安防视频管理系统 (VMS)。

IQCloud 是 **「离网无人资产的自治化操作系统与商业变现引擎」**  
（The Autonomous OS & Monetization Engine for Off-Grid Assets）。

我们将赋能散落在荒野、工地的监控拖车，赋予其 **「自治求生」** 与 **「主动防御」** 的能力，并将这种能力打包为 **高利润的 SaaS 订阅服务**。

---

## 2. 我们的核心用户是谁？ (Target Audience)

本系统采用 **B2B2C** 商业模式，须同时服务两拨核心利益群体：

### 直接付费方 (Dealer / 安防设备租赁商)

| 维度 | 说明 |
|------|------|
| **痛点** | 规模化带来的运营反噬：电池报废、流量超标、高昂的派工维修成本 |
| **诉求** | 极低成本的资产运维与更高的设备转租溢价 |

### 最终买单方 (Client / 终端工地老板、矿山主管等)

| 维度 | 说明 |
|------|------|
| **痛点** | 不关注技术参数，只关心「我的资产会不会被偷」 |
| **诉求** | 高确定性的安全履约（入侵能自动威慑，运维方可安心值守） |

> **术语**: Client = 终端付费组织（Tenant）。与 User（自然人账号）的区分见 [`G2_Customer_User_Frontend_Blueprint.md`](../../04-cloud/docs/G2_Customer_User_Frontend_Blueprint.md)。

---

## 3. 四大核心商业价值 (为什么他们愿意付钱？)

所有代码架构（边缘断腕求生、多租户隔离、AI 告警等）须 **直接服务于** 以下四项可变现价值：

### 价值一：彻底消灭「Truck Roll」（高昂的出车维修费）

| 项 | 内容 |
|----|------|
| **受众** | Dealer |
| **商业问题** | 北美野外单次派工成本极高，无效出车直接侵蚀利润 |
| **技术落地** | `energy` 域极限求生状态机（低电量自动关闭大功率负载）+ `control` 域远程 PoE 复位 |
| **价值主张** | 显著减少无效出车；节省的 OPEX 应 **远超** SaaS 订阅费 |

### 价值二：封死「通信流量黑洞」(Bandwidth OPEX Protection)

| 项 | 内容 |
|----|------|
| **受众** | Dealer |
| **商业问题** | 4G/5G 资费是离网设备最大的持续成本，流量失控即利润归零 |
| **技术落地** | 与移动网络运营商 API 对接 + 边缘自动降码率、防异常掐流机制 |
| **价值主张** | 为 Dealer 提供「利润保护伞」，流量账单可预测、可管控 |

### 价值三：高确定性的「主动威慑」与「低虚警」(Active Deterrence)

| 项 | 内容 |
|----|------|
| **受众** | Client & 接警中心 |
| **商业问题** | 虚警消耗信任与人力；漏报则安全履约失败 |
| **技术落地** | `vision` 域边缘 AI 过滤 + Alert 告警引擎 → 毫秒级爆闪、警笛等主动威慑 |
| **价值主张** | 过滤环境噪声引发的虚警，提供 **高确定性** 的安全履约 |

### 价值四：赋予 Dealer「SaaS 溢价能力」(The Arbitrage Engine)

| 项 | 内容 |
|----|------|
| **受众** | Dealer |
| **商业问题** | 硬件租赁毛利有限，需软件层创造 recurring revenue |
| **技术落地** | 白标 Client App（终端客户手机端 / 门户）+ 多租户隔离与 RMR 计费能力 |
| **价值主张** | Dealer 向平台支付 **较低** 的平台订阅费，却可向终端 Client 收取 **显著更高** 的智能云服务费。**IQCloud 本质是帮 Dealer 赚取超额 SaaS 利润的套利引擎。** |

---

## 4. 与技术架构的映射（速查）

| 商业价值 | G2 领域 / 能力 |
|----------|----------------|
| Truck Roll 削减 | `energy` · `control` · Registry 远程运维 |
| 流量 OPEX 保护 | `network` · 运营商集成 · 边缘码率策略 |
| 主动威慑 / 低虚警 | `vision` · VQA · `control`（警报灯 / IP Speaker） |
| Dealer SaaS 溢价 | 租户层 Admin/Dealer/Client · 白标 · Customer/User 前端双模块 |

**详细技术文档**:

- [`G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md)
- [`G2_API_Architecture_Draft.md`](../../02-backend/docs/G2_API_Architecture_Draft.md)
- [`G2_Client_Tenant_Model.md`](../../04-cloud/docs/G2_Client_Tenant_Model.md)
- [`G2_Cloud_Architecture_Design.md`](../../04-cloud/docs/G2_Cloud_Architecture_Design.md)

---

## 5. 开发铁律（摘自 AGENTS.md · 全员遵守）

1. 严守 `dev` / `prod` 多环境隔离。
2. 全局贯彻五大业务领域命名：`energy` · `network` · `vision` · `environment` · `control`。
3. 重大技术决策须同步更新 `docs/development_log.md` 或 `docs/cloud_backend_log.md`。

---

*IQCloud Commercial Strategy · 00-strategy · Agent 008 建档*
