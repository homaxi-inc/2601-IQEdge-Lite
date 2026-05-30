# 01 — IoT MQTT · G2 energy telemetry

**Topic**: `iqedge/g2/dev/energy/telemetry`  
**控制台**: IoT Core → **MQTT test client** → Subscribe to a topic

---

## 通过标准

- [ ] 约每 **60 s**（联调 `esp32dev`）收到一条消息
- [ ] `schema_version` = `energy.telemetry.v1`
- [ ] `sys_id` = `IQ-26-00001`
- [ ] `component_id` = `HQ2513A69PJ`
- [ ] `firmware_version` ≥ `v2.3.0`
- [ ] `measures.*` 结构完整

---

## 已录入样本

| # | 文件 | 控制台时间 (本地) | UTC |
|---|------|-------------------|-----|
| 001 | [sample-001-2026-05-30T205927Z.json](sample-001-2026-05-30T205927Z.json) | 2026-05-30 13:59:27 (UTC-0700) | 2026-05-30T20:59:27Z |

---

## 追加方式

1. 在 MQTT test client 复制 JSON → 保存为 `sample-00N-{ISO时间}.json`
2. 在上表增加一行
3. 可选：粘贴截图 → `screenshot-00N.png`

---

## notes.md（可选）

在此文件或新建 `notes.md` 记录观察（信号质量、间隔、字段异常等）。
