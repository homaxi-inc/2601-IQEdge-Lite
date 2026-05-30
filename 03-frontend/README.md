# 03-frontend — IQCloud Fleet UI / PWA

> **状态**: 脚手架目录（Bob 本地已创建 · 2026-05-30）  
> **消费**: [`02-backend/`](../02-backend/) Fleet API · [`09-contract/`](../09-contract/) Schema  
> **设计**: [`04-cloud/docs/G2_Customer_User_Frontend_Blueprint.md`](../04-cloud/docs/G2_Customer_User_Frontend_Blueprint.md)

---

## 职责

| 项 | 说明 |
|----|------|
| **Customer 域** | 租户 Roster、资产分配、白标、账单入口 |
| **User 域** | 登录、团队、RBAC、Profile（与 Customer **解耦**） |
| **Fleet 域** | `sys_id` 看板 · energy / network / vision / control 五域 UI |
| **非职责** | 固件 · AWS CDK · 现场 Cerbo/RUT 集成 |

---

## 建议目录（待实现）

```text
03-frontend/
  README.md                 ← 本文件
  docs/                     ← 前端专属开发日志、UI 决策
  modules/
    customer/               ← 租户 / Client 管理
    user/                   ← Auth · Team · RBAC guards
    fleet/                  ← 设备看板（JWT 过滤 sys_id）
  shared/
    auth/                   ← JWT parse · role hooks
```

**铁律**（见 Blueprint §6）：`customer` 与 `user` **不同路由域、不同 Store 切片**。

---

## 待决

- [ ] 技术栈：React / Vue / PWA-only
- [ ] 与 Legacy IQWatch API 过渡策略
- [ ] Cognito / Auth 提供方
- [ ] 部署目标（S3 + CloudFront · 现有 `iqedge-pwa-mobile-test-*` 等）

---

## 关联

| 文档 | 路径 |
|------|------|
| Customer vs User 蓝图 | [`G2_Customer_User_Frontend_Blueprint.md`](../04-cloud/docs/G2_Customer_User_Frontend_Blueprint.md) |
| 租户模型 | [`G2_Client_Tenant_Model.md`](../04-cloud/docs/G2_Client_Tenant_Model.md) |
| Fleet API | [`G2_API_Architecture_Draft.md`](../02-backend/docs/G2_API_Architecture_Draft.md) |
| 任务分解 M17+ | [`G2_Implementation_Task_Breakdown.md`](../04-cloud/docs/G2_Implementation_Task_Breakdown.md) |

---

*Bob · 前端工程入口 · Agent 008 仅协助 API/契约对齐*
