# 03 — IoT Rule · energy ingest

**Rule 名**: `iqedge_g2_dev_rule_energy`  
**控制台**: IoT Core → **Message routing** → **Rules**

---

## 核对项

- [ ] SQL: `SELECT * FROM 'iqedge/g2/dev/energy/telemetry'`
- [ ] Action: Lambda → `iqedge-g2-dev-fn-ingest-energy`
- [ ] Rule 状态: Enabled
- [ ] Error action: CloudWatch Logs `/aws/iot/rules/iqedge_g2_dev_rule_energy`

---

## 证据

| 类型 | 文件 | 说明 |
|------|------|------|
| 截图 | `screenshot-rule-config.png` | *(待添加)* |
| 说明 | `notes.md` | 粘贴 SQL / Action ARN 等 |

---

## notes.md

```markdown
# Rule 核对记录

- 查看时间:
- Rule ARN:
- Lambda ARN:
- 备注:
```
