# G2 Cerbo / IQTrailer 联调 — 008 ↔ 007 分工（EDGE-T3）

> **日期**: 2026-05-30  
> **前置**: 007 **EDGE-T2 ✅** — RUT `10.24.121.199` · G2 Topic 已发 · `datasender` success=1  
> **目标**: **EDGE-T3** — RUT MQTT → M4 ingest → Timestream + Shadow · `sys_id=IQ-26-60001`  
> **关联**: [`05-integration/rut/data-to-server-g2-energy.md`](../../05-integration/rut/data-to-server-g2-energy.md) · ADR-012

---

## 1. 007 现状摘要（008 已读）

| 项 | 状态 |
|----|------|
| EDGE-T1 Modbus 寄存器 | ✅ MVP · 含 846 TTG ×100 |
| EDGE-T2 Data to Server | ✅ `iqedge/g2/dev/energy/telemetry` · 60 s |
| Lua 格式化 | ✅ [`g2_energy_format.lua`](../../05-integration/rut/lua/g2_energy_format.lua) |
| Legacy 双轨 | ✅ `iot/rut241/status` 保留 · IQCloud collection 已禁用 |
| VPN 台架 | ✅ `10.24.121.199` · SN `6004727310` |
| Cerbo `component_id` | **`c0619ab6be37`** |
| Registry `sys_id`（Lua 常量） | **`IQ-26-60001`** |

**007 payload 与契约差异（ingest 可过 · 待迭代）**:

| 字段 | 007 现状 | 契约/Field Ref | 008 建议 |
|------|----------|----------------|----------|
| `battery.current_a` | `Battery_Power / Voltage` 推导 | 优先 Modbus **841** | 联调通过后 007 改 Lua |
| `yield.*` | 未上送 | OI-002 待定 | 不阻塞 EDGE-T3 |
| `charge_state_code` | 未上送 | Modbus 844 | Phase 2 |
| `data_stale` | 固定 `false` | >25 min 无读数 | 007 后续 |
| `state` / `reporting_mode` | 固定 `NORMAL` | **OI-001** 未决 | 不阻塞 ingest |
| `firmware_version` | 省略 | ADR-012 豁免 | ✅ |

---

## 2. 遗留问题确认

| ID | 状态 | EDGE-T3 影响 |
|----|------|--------------|
| **OI-001** | ⬜ 未决 | 007 暂填 `NORMAL` · **不阻塞** M4 |
| **OI-002** | ⬜ 台架 | 无 yield · **不阻塞** |
| **846 TTG** | 文档已定 | Lua **未**输出 `time_to_go_sec` · 不阻塞 |

---

## 3. 008 工作包（按优先级）

### P0 — 联调阻塞（008 执行）

| # | 任务 | 命令 / 产出 | 状态 |
|---|------|-------------|------|
| C0 | **Registry 种子** `IQ-26-60001` · `track=g2` · `iqtrailer` | `seed_registry_item.py` + [`registry-record.v1.json`](../../09-contract/examples/iqtrailer/registry-record.v1.json) | 🟡 Bob 定稿 sys_id · 须 re-seed |
| C1 | **M4 Lambda** 含 ADR-012 Cerbo fw 豁免 | `cdk deploy iqedge-g2-dev-ingest` | ✅ 2026-05-30 |
| C2 | **IoT Rule** 已存在 | `iqedge_g2_dev_rule_energy` · Topic 匹配 | ✅ HIL 已验 |
| C3 | **RUT 发 G2 Topic 权限** | `AllowAllPolicy` 可用 · **已 attach** `iqedge-g2-dev-iot-policy-g2-device` 于 `RUT241_71DC` | ✅ 2026-05-30 |

### P1 — 联调观测（008 + 007）

| # | 任务 | 说明 |
|---|------|------|
| C4 | IoT Core **MQTT test client** 订阅 `iqedge/g2/dev/energy/telemetry` | 确认 60 s 一包 · JSON 与 Lua 一致 |
| C5 | CloudWatch **`/aws/lambda/iqedge-g2-dev-fn-ingest-energy`** | 期望 `ingest_ok` · `iqtrailer=True` · 无 `registry_not_found` |
| C6 | Timestream 查询 `sys_id=IQ-26-60001` | `verify_g2_telemetry.py --sys-id IQ-26-60001` |
| C7 | Shadow DDB `SYS#IQ-26-60001` / `DOMAIN#energy#COMP#c0619ab6be37` | 快照与最后一包 MQTT 一致 |
| C8 | 指标 `IQEdge/G2/Ingest` · `IngestSuccess` | 与 007 发包周期对齐 |

### P2 — 验收收口（EDGE-T3）

| # | 任务 | 产出 |
|---|------|------|
| C9 | 验证目录 | [`verification/G2-CERBO-IQ-26-60001-2026-05-30/`](verification/G2-CERBO-IQ-26-60001-2026-05-30/README.md) | ✅ SIGNOFF 2026-05-31 |
| C10 | Bob 签收 | [`SIGNOFF-2026-05-31.md`](verification/G2-CERBO-IQ-26-60001-2026-05-30/SIGNOFF-2026-05-31.md) | ✅ 008 归档 |
| C11 | `cloud_backend_log.md` + `05-integration/development_log` | EDGE-T3 关闭 | ✅ |

### P3 — 联调后（非阻塞）

| # | 任务 |
|---|------|
| C12 | Fleet API `GET .../energy` adapter（M10） |
| C13 | RUT Thing 正式化：独立 Thing SN + 仅 G2 Policy（替换 AllowAllPolicy） |
| C14 | network 域 M5 · `iqedge/g2/dev/network/telemetry` |

---

## 4. 008 验收命令（复制即用）

```powershell
# Registry
python 04-cloud/scripts/seed_registry_item.py --env dev `
  --file 09-contract/examples/iqtrailer/registry-record.v1.json

# G2 Policy（台架 Thing · 若不用 AllowAllPolicy）
python 04-cloud/scripts/attach_g2_iot_policy.py --env dev --thing RUT241_71DC

# Lambda  redeploy（ADR-012 豁免）
cd 04-cloud/cdk
npm run build
npx cdk deploy iqedge-g2-dev-ingest --require-approval never

# Timestream（007 发包 5–10 min 后）
python 01-firmware/tools/aws-verify/verify_g2_telemetry.py --sys-id IQ-26-60001 --minutes 30

# IoT Rule
aws iot get-topic-rule --rule-name iqedge_g2_dev_rule_energy --region us-east-1

# Ingest 日志（最近 15 min）
aws logs filter-log-events --log-group-name /aws/lambda/iqedge-g2-dev-fn-ingest-energy `
  --start-time $([int64]((Get-Date).AddMinutes(-15).ToUniversalTime()-[datetime]'1970-01-01').TotalMilliseconds) `
  --filter-pattern "IQ-26-60001" --region us-east-1
```

---

## 5. 007 联调配合清单

| # | 007 动作 | 008 期望 |
|---|----------|----------|
| 1 | 确认 RUT **`datasender.collection.*` mqtt success=1** 持续 | IoT 测试客户端可见消息 |
| 2 | 提供 **1 条实包 JSON**（MQTT 抓包或 ubus dump） | 008 对照 Schema / Lambda 日志 |
| 3 | Modbus 与 VRM 对照（可选） | Timestream 数值同量级 |
| 4 | 勿改 Topic / `sys_id` / `component_id` 常量 | 与 Registry 一致 |
| 5 | Legacy `iot/rut241/status` 仍正常 | 双轨 ADR-001 |

---

## 6. 失败排障矩阵

| 症状 | 最可能原因 | 008/007 动作 |
|------|------------|--------------|
| MQTT 无 G2 消息 | Data to Server / AWS 端点 / 证书 | 007 ubus dump · 查 RUT log |
| MQTT 有 · Lambda 无 | IoT Rule 禁用 / Lambda 错 | 008 Rule + CW Logs |
| Lambda `registry_not_found` | 未 seed `IQ-26-60001` | **008 C0** |
| Lambda `firmware_below_2_3` | 旧 Lambda 无 ADR-012 | **008 C1 redeploy** |
| Lambda OK · Timestream 0 行 | `track!=g2` / 写库失败 | Registry + CW `TimestreamWriteError` |
| 数值离谱 | Lua 缩放 / Modbus +1 | 007 对照 VRM preflight |

---

## 7. Registry 目标 JSON（dev · IQTrailer 台架）

见 [`09-contract/examples/iqtrailer/registry-record.v1.json`](../../09-contract/examples/iqtrailer/registry-record.v1.json) — `sys_id=IQ-26-60001` · Cerbo `c0619ab6be37` · RUT Thing `RUT241_71DC`。

---

*Agent 008 · EDGE-T3 规划 · 2026-05-30*
