# 05 — Timestream · G2 energy 查询

**Database**: `iqedge_g2_dev_database`  
**Table**: `iqedge_g2_dev_table_energy`  
**控制台**: Amazon Timestream → **Query editor**

---

## 推荐查询

将以下 SQL 保存为 `query-energy-30m.sql`，结果粘贴到 `query-result.txt`：

```sql
SELECT sys_id, component_id, time, measure_name,
       battery_soc_pct, solar_power_w, yield_total_kwh
FROM iqedge_g2_dev_database.iqedge_g2_dev_table_energy
WHERE sys_id = 'IQ-26-00001'
  AND time > ago(2h)
ORDER BY time DESC
LIMIT 20
```

---

## 通过标准

- [ ] 多条 `measure_name = g2_energy`
- [ ] `component_id = HQ2513A69PJ`
- [ ] 间隔约 60 s（联调固件）

---

## 证据

| 文件 | 说明 |
|------|------|
| `query-energy-30m.sql` | *(可复制上方 SQL)* |
| `query-result.txt` | Query editor 输出 *(待添加)* |
| `screenshot-query.png` | *(可选)* |
