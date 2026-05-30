# G2 Registry — `track` 赋值与固件版本门禁 SOP

> **状态**: Bob 定稿 · 2026-05-29  
> **ADR**: [ADR-008](../../decisions/README.md#adr-008--registry-track--firmware-gate)  
> **读者**: 008（Ingest/API）· 007（固件/OTA）· 运维

---

## 1. 目的

定义 `Registry.track`（`legacy` | `g2`）与 **固件版本** 如何共同决定：

- API **读路径**（Legacy DDB vs G2 Timestream）
- Ingest **是否写入 Timestream**
- 老现场 ~70 台 **升级晋升** 流程

---

## 2. 权威原则（Bob 定稿）

| # | 规则 |
|---|------|
| R1 | **设备上报** `firmware_version` 为权威（MQTT payload 顶层字段，与 Legacy 一致） |
| R2 | **G2 Timestream 写入门槛**：`track=g2` **且** `firmware_version` **≥ v2.3.0**（见 §3） |
| R3 | **新生产** Registry 默认 **`track=g2`** |
| R4 | **`IQ-26-00001`（HIL）** 固定 **`track=g2`** |
| R5 | **Legacy 批量导入** 初始 **`track=legacy`** |
| R6 | 老机升到 **≥ v2.3.0** 后可纳入 **g2** — **`track` 晋升仅人工**（工单 / Admin / `seed_registry_item.py`），**Ingest 不得自动改 track** |
| R7 | **`track=g2` 永不回退** 为 `legacy`（即使固件降级） |
| R8 | Registry **`firmware_version`** 字段：有则写入；无则允许空，首包或运维补录 |
| R9 | **量产 OTA 门禁**：不得对量产发布 **&lt; v2.3.0** 的 BIN（007/发布流程） |
| R10 | 产线误刷 **&lt; v2.3.0** 可发货，云端 **`track=legacy`** 直至返工/OTA 达标后 **人工** 改为 `g2` |
| R11 | **`batch_id` / `HQ2513*`** 不作为 `track` 依据（仅报表/追溯） |

---

## 3. 版本字符串与比较

### 3.1 格式（沿用现网 · HQ2513A69PJ 实测）

| 项 | 值 |
|----|-----|
| 字段名 | `firmware_version` |
| 现网样例 | `v2.2.3.25`（`Config.h` → `CommManager` payload） |
| **G2 起线版本** | **`v2.3.0`**（007 首个 G2 量产/联调目标） |
| 建议 pattern | `^v[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$` |

### 3.2 比较算法（Ingest / 工具共用）

```text
1. 去掉可选前缀 "v"（大小写不敏感）
2. 按 "." 拆分为整数段 [major, minor, patch, build?]
3. G2 达标 ⟺ (major > 2) OR (major == 2 AND minor >= 3)
   例：v2.2.3.25 → 不达标；v2.3.0 / v2.3.0.1 → 达标
```

---

## 4. `track` 赋值矩阵

| 场景 | `track` | Timestream 写入 |
|------|---------|-----------------|
| 新生产（正常） | `g2`（Provisioning 默认） | 仅当上报 ≥ v2.3.0 |
| HIL `IQ-26-00001` | `g2`（已种子） | 仅当上报 ≥ v2.3.0 |
| Legacy 导入 ~70 台 | `legacy`（初始） | **不** 写 G2 表 |
| 老机 OTA 至 ≥ v2.3.0 | **人工** PATCH → `g2` | 改 track 后且上报 ≥ v2.3.0 |
| 产线误刷 &lt; v2.3.0 | `legacy`（直至返工） | 否 |
| 已为 `g2` 但版本 &lt; 2.3 | 保持 `g2`（不回退） | **拒写** + 告警（M4） |

---

## 5. 人工晋升流程（legacy → g2）

1. 007 OTA 至 **≥ v2.3.0** 且 G2 Topic 发包正常（§007 HIL 文档阶段 B/C）。
2. 运维/008 核对 `firmware_version`（DDB Legacy 或 MQTT 抓包）。
3. 执行其一：
   - `PATCH /api/v2/fleet/systems/{sys_id}`（`track=g2`，`reason=…`，待 API 实现）
   - `python 04-cloud/scripts/seed_registry_item.py --env dev --file <record.json>`
4. **不得** 依赖 Ingest 自动改 `track`。
5. 记录工单号 / OTA Job ID 备查。

---

## 6. M4 Ingest 行为（008 实现约束）

| 条件 | 行为 |
|------|------|
| `track=g2` + 版本 ≥ 2.3 + G2 Topic + Schema OK | `WriteRecords` Timestream + Shadow；可更新 Registry `firmware_version` |
| `track=g2` + 版本 &lt; 2.3 或缺版本 | **拒写** Timestream；CloudWatch 告警；**不改 track** |
| `track=legacy` + G2 Topic | **拒写** G2 表；Legacy 管道若仍发 `device/status` 不受影响 |
| 任意 | **禁止** Ingest 自动 `track` 晋升/降级 |

---

## 7. 007 必读

→ [`09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md`](../../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) §「G2 固件版本线」  
→ [`01-firmware/docs/G2_HIL_007_Firmware_Requirements.md`](../../01-firmware/docs/G2_HIL_007_Firmware_Requirements.md) §3.0

---

## 8. 相关文档

| 文档 | 说明 |
|------|------|
| [`02-backend/docs/G2_System_Model.md`](../../02-backend/docs/G2_System_Model.md) §6 | Registry JSON |
| [`G2_Cloud_Architecture_Design.md`](G2_Cloud_Architecture_Design.md) | 双轨合流 |
| [`G2_HIL_008_007_Handoff.md`](G2_HIL_008_007_Handoff.md) | 联调分工 |
