# 交付清单 — M0.1 & M0.2（待 Bob 批准）

> **提交人**: Agent 008  
> **日期**: 2026-05-29  
> **任务来源**: [`G2_Implementation_Task_Breakdown.md`](../G2_Implementation_Task_Breakdown.md) · Module M0  
> **批准前请勿启动**: M0.3（energy Schema）、M1（Foundation Stack deploy）

---

## 1. 交付摘要

| 任务 ID | 名称 | 状态 |
|---------|------|------|
| **M0.1** | 初始化 `04-cloud/cdk/`（TypeScript CDK v2，`-c env=dev\|prod`） | ✅ 待审 |
| **M0.2** | 创建 `09-contract/schemas/{domain}/` 目录约定 | ✅ 待审 |

---

## 2. M0.1 产出物清单

| # | 路径 | 说明 |
|---|------|------|
| 1 | `04-cloud/cdk/package.json` | CDK v2 依赖与 `synth:dev` / `synth:prod` 脚本 |
| 2 | `04-cloud/cdk/cdk.json` | App 入口 `bin/app.ts` |
| 3 | `04-cloud/cdk/tsconfig.json` | TypeScript 严格模式 |
| 4 | `04-cloud/cdk/bin/app.ts` | 读取 `-c env`，实例化 Scaffold Stack |
| 5 | `04-cloud/cdk/lib/config.ts` | `getG2Env()` · `getAwsDeploymentEnv()` |
| 6 | `04-cloud/cdk/lib/naming.ts` | 五域常量 + hyphen/underscore/MQTT 命名助手 |
| 7 | `04-cloud/cdk/lib/stacks/g2-scaffold-stack.ts` | 占位 Stack（Tags + CfnOutput 验证命名） |
| 8 | `04-cloud/cdk/lib/constructs/` | 空目录（M4+ Construct 预留） |
| 9 | `04-cloud/cdk/README.md` | 本地开发与禁止事项 |
| 10 | `04-cloud/cdk/.gitignore` | `node_modules/`, `cdk.out/` |
| 11 | `04-cloud/cdk/package-lock.json` | 依赖锁定（`npm install` 生成） |

### 2.1 验收命令（008 已执行）

```powershell
cd e:\2601-IQEdge-Lite\04-cloud\cdk
npm install
npm run synth:dev
npm run synth:prod
```

**期望**: 两次 `synth` 均成功；`cdk.out/` 生成 CloudFormation 模板；Output 含 `G2Env` 与 `NamingExampleRule`。

### 2.2 验收结果（008 填写）

| 检查项 | dev | prod |
|--------|-----|------|
| `cdk synth` 退出码 0 | ✅ 2026-05-29 | ✅ 2026-05-29 |
| Stack id `iqedge-g2-{env}-scaffold` | ✅ | ✅ |
| Output `NamingExampleRule` | `iqedge-g2-dev-rule-energy` | `iqedge-g2-prod-rule-energy` |
| 无 Legacy 资源引用 | ✅ | ✅ |

---

## 3. M0.2 产出物清单

| # | 路径 | 说明 |
|---|------|------|
| 1 | `09-contract/README.md` | SSOT 说明、版本约定、五域 message-type 表 |
| 2 | `09-contract/schemas/README.md` | 文件命名与 `additionalProperties` 规则 |
| 3 | `09-contract/schemas/energy/README.md` | 占位 → M0.3 |
| 4 | `09-contract/schemas/network/README.md` | 占位 → P2 |
| 5 | `09-contract/schemas/vision/README.md` | 占位 → P4 |
| 6 | `09-contract/schemas/environment/README.md` | 占位 → P5 |
| 7 | `09-contract/schemas/control/README.md` | 占位 → P3 |
| 8 | `09-contract/schemas/registry/README.md` | 占位 → M3.2 |
| 9 | `09-contract/openapi/README.md` | OpenAPI 导出约定 |
| 10 | `09-contract/openapi/v2/.gitkeep` | 生成物目录占位 |

**刻意未交付**（后续任务）: 任何 `.json` Schema 正文（属 **M0.3**）。

---

## 4. 文档与日志更新

| 文件 | 变更 |
|------|------|
| `04-cloud/docs/cloud_backend_log.md` | M0.1/M0.2 完成条目 |
| `04-cloud/docs/G2_Implementation_Task_Breakdown.md` | M0.1/M0.2 勾选 ✅ |
| `04-cloud/README.md` | 增加 `cdk/`、`09-contract/` 链接 |
| 根 `.gitignore` | 忽略 `04-cloud/cdk/node_modules`, `cdk.out` |

---

## 5. Synth 输出摘要（008 执行记录）

```text
npm install        → exit 0
npm run synth:dev  → iqedge-g2-dev-scaffold · G2Env=dev
npm run synth:prod → iqedge-g2-prod-scaffold · G2Env=prod
```

Scaffold 仅 `CDKMetadata` + Outputs；**未 `cdk deploy`**。

---

## 6. 风险与边界

| 项 | 说明 |
|----|------|
| **未 deploy AWS** | M0.1 仅 `synth`，不创建任何云资源 |
| **无 Secret** | 无 `.env`、无 API Key 写入仓库 |
| **Legacy** | 未触碰 `DeviceStatusToLambda` 等老资源 |
| **Node 版本** | 建议 Node 20+；若 synth 失败请检查 `npm install` |

---

## 7. 批准栏

| 角色 | 决定 | 签名 / 日期 |
|------|------|-------------|
| Bob | ☑ **批准** — 可进入 M0.3 + M1（2026-05-29） | |
| Bob | ☐ **驳回** — 意见： | |

**批准后下一任务（建议顺序）**:

1. **M0.3** — `09-contract/schemas/energy/telemetry.v1.json`
2. **M1.1** — `G2FoundationStack` dev deploy
3. **M2.1** — Timestream DB（可与 M1 同 Sprint）

---

*Agent 008 · Delivery M0.1 + M0.2 · 待批准*
