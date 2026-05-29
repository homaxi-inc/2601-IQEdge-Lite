# IQW-9041 遥测数据分析（近 10 小时）

| 项 | 值 |
|----|-----|
| **分析时间** | 2026-05-29（UTC） |
| **查询窗口** | `ago(10h)` |
| **工具** | `tools/aws-verify/analyze_device_window.py` |

---

## 1. 设备 ID 说明（重要）

在 AWS 中 **不存在** `deviceId = IQW-9041`：

| 数据源 | `IQW-9041` 查询结果 |
|--------|-------------------|
| DynamoDB `DeviceLatestStatus` | 无记录 |
| Timestream `IQWatchDB.DeviceStatus` | 近 10h **0 条** |
| IQWatch API `GET /devices/IQW-9041/status` | **HTTP 404** |

`IQW-9041` 为 **项目/站点编号**（历史 OTA Job `IQW-9041-OTA-DIAG-20260518-1836` 描述字段），云端键为 MPPT 序列号 **`SER#`**。

**已确认关联（同机）**：

| 云端 `deviceId` | ESP32 MAC | IoT Thing |
|-----------------|-----------|-----------|
| **HQ2513A69PJ** | `1C:69:20:B8:D7:F4` | `IQEdge_1C:69:20:B8:D7:F4` |

以下分析基于 **HQ2513A69PJ**（即 IQW-9041 对应现场单元）。

---

## 2. 数据覆盖（近 10h）

| 指标 | 值 |
|------|-----|
| 上报次数 | **56** 次 |
| 时间范围 | **2026-05-29 06:37:38** ~ **16:20:42 UTC** |
| 上报间隔 | 夜间段 **~30 min**；午后段 **~5 min**（median 300s） |
| 度量字段 | soc, Vbat, solar_*, load_*, today/total_yield_kwh 等 |
| 最新固件 | **v2.2.3.25**（OTA 后） |

DDB / API / Timestream 最新点 **一致**（16:20:42 UTC）。

---

## 3. 正确性分析

### 3.1 通过项 ✅

| 检查 | 结论 |
|------|------|
| **三层一致** | DDB = API = TS 末点（soc 61%、Vbat 13.14V、solar 25W、fw v2.2.3.25） |
| **累计产量** | `total_yield_kwh` 由 ~35.06 缓升至 **35.11**，日内单调递增 |
| **物理量范围** | soc 0–100%；Vbat 13.0–13.28V；solar 0–82W（午后光照合理） |
| **故障码** | `error_code=0`，`off_reason=0`，`state=NORMAL` |
| **10× 产量 bug** | 无；total/today 为 **kWh 量级**（非数百 kWh） |
| **负载语义** | load 0–1.3W，符合 MPPT LOAD 端仅 IQEdge 小负载（非整机 PoE） |
| **充电与光伏** | 午后 solar↑ 时 soc 由 ~40% 升至 ~78%，与 Bulk 充电一致 |

### 3.2 可解释现象 ⚠️（非缺陷）

| 现象 | 解释 |
|------|------|
| **上报间隔 30 min**（06:37–12:37 UTC） | 与固件 **夜间/低光伏** 模式 `PUBLISH_INTERVAL_NIGHT_MS`（30 min）一致 |
| **07:37 UTC `today_yield` 0.47→0** | MPPT **日计数器 H20 归零**（当地接近日出/跨日），非云端错误 |
| **12:37 后间隔恢复 ~5 min** | 转入 **NORMAL** 动态上报（`PUBLISH_INTERVAL_NORMAL_MS` 5 min） |
| **12:37→12:50 间 777s 单段 gap** | 模式切换或单次 MQTT/网络延迟；前后数据连续，无停滞整天 |

### 3.3 需关注点 🟡

| 项 | 说明 |
|----|------|
| **站点编号 vs 云端 ID** | 运维/看板若用 `IQW-9041` 查 API 会 404；应使用 **HQ2513A69PJ** 或维护映射表 |
| **soc 阶跃**（如 15:25→15:35：40%→62%） | 10 分钟内光伏 18W→64W，SOC 估算表响应偏快但可接受；非 10× 产量类错误 |
| **06:37 前 10h 窗口外** | 若需覆盖当地昨夜，可拉长 `--hours 14` |

---

## 4. 结论

**在将 IQW-9041 映射为 MPPT `HQ2513A69PJ` 的前提下：近 10 小时云端数据整体正确、可用。**

- 管线正常：持续写入 Timestream，DDB/API 与末点一致。
- 未发现停滞、数量级错误、累计产量回退或故障码异常。
- 夜间 30 min / 日间 5 min 间隔与固件设计一致；`today_yield` 下降为日界重置。

**若您掌握的 MPPT `SER#` 不是 HQ2513A69PJ**，请提供正确序列号后重新跑：

```powershell
python tools\aws-verify\analyze_device_window.py --device <SER#> --hours 10
```

---

*Agent 007 · 关联 OTA：`IQW-9041-OTA-DIAG-20260518-1836` → Thing `IQEdge_1C:69:20:B8:D7:F4`*
