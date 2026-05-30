# 05 — Timestream · G2 energy 查询

**Database**: `iqedge_g2_dev_database`  
**Table**: `iqedge_g2_dev_table_energy`  
**控制台**: Amazon Timestream → **Query editor**

---

## 推荐查询

见同目录 [`query-energy-30m.sql`](query-energy-30m.sql)

---

## 通过标准

- [ ] 多条 `measure_name = g2_energy`
- [ ] `component_id = HQ2513A69PJ`
- [ ] 间隔约 60 s（联调固件）

---

## 证据

| 文件 | 说明 |
|------|------|
| `query-energy-30m.sql` | 标准查询 |
| `query-result.txt` | Query editor 输出 *(待添加)* |
| `screenshot-query.png` | *(可选)* |

---

## query-result.txt

在此粘贴 Timestream Query editor 结果，或新建该文件。
