# IQEdge G2 — AWS CDK (04-cloud/cdk)

Agent 008 · IaC 主战场。双轨战略下 **仅部署 G2 新轨资源**，不改动 Legacy `DeviceStatusToLambda` 等。

## 前置

- Node.js 20+
- AWS CLI 已配置（`aws sts get-caller-identity`）
- 首次：`npm install`

## 环境上下文

所有 Stack 通过 **`-c env=dev|prod`** 切换命名与 Topic 前缀：

```powershell
cd 04-cloud/cdk
npm run synth:dev    # 等价: npx cdk synth -c env=dev
npm run synth:prod
```

| Context | MQTT 示例 | Timestream DB 示例 |
|---------|-----------|-------------------|
| `dev` | `iqedge/g2/dev/energy/telemetry` | `iqedge_g2_dev_database` |
| `prod` | `iqedge/g2/prod/energy/telemetry` | `iqedge_g2_prod_database` |

命名助手 → `lib/naming.ts` · 宪法 → [`../docs/G2_Domain_Map.md`](../docs/G2_Domain_Map.md)

## 目录

```text
cdk/
  bin/app.ts              # CDK 入口
  lib/
    config.ts             # getG2Env(), deployment env
    naming.ts             # hyphen / underscore / mqtt helpers
    stacks/               # M1+ Foundation, Storage, Ingest…
    constructs/           # DomainIngestPipeline 等（M4+）
  cdk.json
  package.json
```

## 当前阶段 (M1–M4 ✅ · dev 已部署)

- ✅ `G2FoundationStack` — KMS、SSM、Lambda base role、IoT policy
- ✅ `G2StorageStack` — Timestream（5 表）、Shadow DDB、Control logs DDB、S3 vision
- ✅ `G2RegistryStack` — Fleet Registry + alias GSI；HIL 种子 `IQ-26-00001`
- ✅ `G2IngestStack` — energy Rule + ingest Lambda（M4）

```powershell
npx cdk deploy iqedge-g2-dev-ingest -c env=dev   # 依赖 registry
python ../scripts/attach_g2_iot_policy.py --thing IQEdge_1C:69:20:B8:D7:F4 --env dev
```

## 禁止事项

- IoT Rule 使用 `topic #` catch-all
- 修改 Legacy 表/Rule 名称
- 在 `cdk.json` / 代码中写入明文 Secret（用 SSM / Secrets Manager）

## 关联

- 任务分解 → [`../docs/G2_Implementation_Task_Breakdown.md`](../docs/G2_Implementation_Task_Breakdown.md)
- 云架构 → [`../docs/G2_Cloud_Architecture_Design.md`](../docs/G2_Cloud_Architecture_Design.md)
- Payload 契约 → [`../../09-contract/README.md`](../../09-contract/README.md)
