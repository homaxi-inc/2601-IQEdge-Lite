---
name: iqedge-telemetry-analysis
description: >-
  Analyzes IQEdge MPPT historical telemetry from AWS (Timestream window, DynamoDB,
  API), resolves site ID vs MPPT SER#, detects anomalies, writes report under report/.
  Use for 遥测分析、历史数据、IQW-xxxx、数据是否正确、analyze_device_window.
---

# IQEdge 历史遥测分析（技能 4）

## 与技能 2 的区别

| | 技能 2 三位一体验证 | 技能 4 历史遥测分析 |
|--|-------------------|-------------------|
| 目的 | 当前快照是否上云、三层一致 | 时间窗内趋势是否正确、可解释 |
| 工具 | `verify_telemetry.py` | `analyze_device_window.py` + `find_device.py` |
| 输出 | 终端 PASS/FAIL | **`report/<主题>_Telemetry_Analysis_YYYY-MM-DD.md`** |

可先技能 2 确认在线，再技能 4 做深度分析。

## 何时使用

- 用户给 **站点编号**（如 `IQW-9041`）或 **MPPT SER#**（`HQ2513…`）
- 问「过去 N 小时数据对不对」「产量/间隔/soc 是否合理」
- OTA/故障排查后的**事后数据回顾**

## 前置

- `.env`：`AWS_*`、`IQWATCH_API_KEY`
- 工作目录：`01-firmware`
- 云端 `deviceId` = MPPT **`SER#`**（非 IQW 项目号，除非已确认映射）

## 执行流程

```text
① 解析 deviceId → ② 拉取时间窗 → ③ 规则分析 → ④ 写 report → ⑤ 更新 report/README.md
```

### ① 解析 deviceId（站点号 vs SER#）

```powershell
cd "d:\2601-IQEdge-Lite\2601-IQEdge-Lite-main\01-firmware"

# 按 MPPT 序列号（首选）
python tools\aws-verify\analyze_device_window.py --device HQ2513A69PJ --hours 10

# 站点号查不到时：扫描云端
python tools\aws-verify\find_device.py 9041
python tools\aws-verify\find_device.py IQW
```

**IQW-xxxx 常见情况**（见 `report/IQW-9041_Telemetry_Analysis_2026-05-29.md`）：

- DDB/API/TS 无 `deviceId=IQW-xxxx` → **404 / 0 条**
- 查 IoT Job 描述、DDB `mac`、Bob 台账 → 映射到 `HQ…` SER#
- 在报告中 **§1 设备 ID 说明** 必须写清映射关系

辅助：Thing `IQEdge_{MAC}` ↔ DDB `mac` ↔ `deviceId`（SER#）

### ② 拉取时间窗

```powershell
python tools\aws-verify\analyze_device_window.py --device <SER#> --hours 10
# 默认 10h；可 --hours 24
```

脚本输出：DDB 最新、API、Timestream 点数/间隔/末 12 快照、自动 **REVIEW** 项。

### ③ 分析规则（人工复核清单）

| 检查 | 正常 | 异常 |
|------|------|------|
| 上报间隔 | 夜间 ~30min、日间 ~5min（见 `Config.h`） | 全天 >1h 无点（非维护窗口） |
| `total_yield_kwh` | 时间窗内单调不减 | 回退 >0.05 kWh |
| `today_yield_kwh` | 可 **日界归零**（H20） | 日中多次大幅回退 |
| 产量量级 | kWh 个位数～数十 | >500 kWh（10× bug） |
| soc / Vbat / solar | soc 0–100%；Vbat 10–14.6V；光伏与日照相关 | 长期全 0 或与光伏矛盾 |
| 故障码 | `error_code=0` | 非 0 持续 |
| 三层末点 | DDB ≈ API ≈ TS 最后时间戳 | 相差 >15 min 或字段严重偏离 |
| load_power | 小负载 W 级（LOAD 端语义） | 误当整机功耗解读 |

**可解释（非缺陷）**：夜间 30min 间隔、`today_yield` 日出附近归零、OTA 后 `firmware_version` 跳变。

### ④ 撰写报告

路径：

```text
report/<主题>_Telemetry_Analysis_YYYY-MM-DD.md
```

`<主题>`：站点号或 SER#（如 `IQW-9041` 或 `HQ2513A69PJ`）

**报告结构模板**（与已有样例对齐）：

1. 元数据（分析时间、窗口、工具）
2. **设备 ID 说明**（IQW → HQ 映射，若适用）
3. 数据覆盖（次数、时间范围、间隔统计）
4. 正确性分析（✅ / ⚠️ / 🟡 分表）
5. **结论**（PASS / REVIEW + 一句摘要）
6. 复现命令

### ⑤ 更新索引

在 `report/README.md` 表格追加一行。

## 推荐链

```text
技能 4 单独（纯云端分析）
技能 2 → 技能 4（先确认在线再深挖）
技能 3 → 技能 2 → 技能 4（OTA 后全链路）
```

## 参考

- 样例报告：`report/IQW-9041_Telemetry_Analysis_2026-05-29.md`
- 管线：`docs/AWS_Cloud_Pipeline.md`
- 产量红线：`src/GEMINI_ESP32.md`（÷1000）
- 索引：`SKILL.md` 技能 4
