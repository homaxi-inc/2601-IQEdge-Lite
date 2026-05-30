# 06 — Lambda · ingest-energy 日志

**函数**: `iqedge-g2-dev-fn-ingest-energy`  
**控制台**: Lambda → **Monitor** → **View CloudWatch logs**

**Log group**: `/aws/lambda/iqedge-g2-dev-fn-ingest-energy`

---

## 通过标准

- [ ] 近期有 `ingest_ok sys_id=IQ-26-00001`
- [ ] Errors 指标 ≈ 0
- [ ] 无大量 `validation_failed` / `firmware_below_2_3`

---

## 证据

| 文件 | 说明 |
|------|------|
| `log-snippet.txt` | 粘贴 3–5 行代表性日志 *(待添加)* |
| `screenshot-metrics.png` | Invocations / Errors *(可选)* |

---

## log-snippet.txt 模板

```text
# 粘贴时间范围:
# 示例行:
# ingest_ok sys_id=IQ-26-00001 component_id=HQ2513A69PJ fw=v2.3.003 measures=...
```
