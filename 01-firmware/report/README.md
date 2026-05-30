# IQEdge-G2 报告目录 (report/)

Agent 007 / 运维产出的**一次性或周期性分析报告**，与 `docs/` 下的常驻设计文档区分。

## 命名约定

```text
<主题>_<类型>_YYYY-MM-DD.md
```

示例：`PreProduction_Audit_Report_2026-05-29.md`、`IQW-9041_Telemetry_Analysis_2026-05-29.md`

## 索引

| 日期 | 报告 | 摘要 |
|------|------|------|
| 2026-05-29 | [PreProduction_Audit_Report_2026-05-29.md](PreProduction_Audit_Report_2026-05-29.md) | 生产前全盘审计，裁决 Conditional GO |
| 2026-05-29 | [IQW-9041_Telemetry_Analysis_2026-05-29.md](IQW-9041_Telemetry_Analysis_2026-05-29.md) | IQW-9041 → HQ2513A69PJ 近 10h 遥测分析 |
| 2026-05-30 | [OTA_IQ-26-00001_v2.3.0_2026-05-30.md](OTA_IQ-26-00001_v2.3.0_2026-05-30.md) | OTA v2.2.3.25→v2.3.0，Legacy 3/3 |
| 2026-05-30 | [NOTICE_007_to_008_IQ-26-00001_2026-05-30.md](NOTICE_007_to_008_IQ-26-00001_2026-05-30.md) | **007→008** 后续行动通知 |

## 关联

- 进展日志：`docs/development_log.md`
- **技能 4**：`.cursor/skills/iqedge-telemetry-analysis/SKILL.md`（本目录报告生成规范）
- 技能索引：`SKILL.md`
- 工具：`tools/aws-verify/analyze_device_window.py`、`find_device.py`
