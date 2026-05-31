# 008 → 007 通知 — Cerbo EDGE-T3 云端已就绪

> **日期**: 2026-05-30  
> **台架**: RUT VPN `10.24.121.199` · `sys_id=IQ-26-60001` · Cerbo `c0619ab6be37`

---

## 008 已完成（P0）

| 项 | 状态 |
|----|------|
| Registry `IQ-26-60001` · `track=g2` · `iqtrailer` | ✅ dev DDB（Bob 定稿 sys_id 后须 re-seed） |
| M4 Lambda redeploy · **ADR-012 Cerbo fw 豁免** | ✅ |
| G2 IoT Policy on Thing **`RUT241_71DC`** | ✅ |
| IoT Rule `iqedge_g2_dev_rule_energy` | ✅（HIL 已验） |

---

## 007 请配合（联调）

1. **确认 RUT 仍在向 AWS IoT 发 G2**（非仅本地 ubus success）  
   - Topic: `iqedge/g2/dev/energy/telemetry`  
   - 周期: **~60 s**

2. **重新部署 Lua**（`SYS_ID` 已改为 **`IQ-26-60001`**）  
   - 见 [`rut_apply_g2_edge_t2.sh`](../../05-integration/scripts/rut_apply_g2_edge_t2.sh)  
   - **007 已完成** 2026-05-31 · Timestream **PASS**

3. **5–10 分钟后** 008/007 共跑：

```powershell
python 01-firmware/tools/aws-verify/verify_g2_telemetry.py --sys-id IQ-26-60001 --minutes 30
```

4. 若 Timestream **0 行** — 请提供：
   - 一条 MQTT 实包 JSON（或 `ubus call datasender.collection.31 dump`）
   - RUT Data to Server / AWS IoT 端点配置截图或 `uci show data_sender.32`

5. **勿改** Topic；`sys_id` / `component_id` 须与 Registry 一致（`IQ-26-60001`）。

---

## 当前观测

- **Timestream**: `IQ-26-60001` **PASS** — EDGE-T3 **关闭** · [`SIGNOFF`](verification/G2-CERBO-IQ-26-60001-2026-05-30/SIGNOFF-2026-05-31.md)
- **Lambda 日志**: 已归档 [`03-lambda-ingest/`](verification/G2-CERBO-IQ-26-60001-2026-05-30/03-lambda-ingest/)

---

## 文档

- 分工: [`G2_CERBO_008_007_Handoff.md`](G2_CERBO_008_007_Handoff.md)  
- 验收: [`verification/G2-CERBO-IQ-26-60001-2026-05-30/README.md`](verification/G2-CERBO-IQ-26-60001-2026-05-30/README.md)

---

*Agent 008*
