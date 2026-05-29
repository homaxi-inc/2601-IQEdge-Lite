# WDT 压力测试工具

方案全文: [`docs/WDT_Stress_Test_Plan.md`](../../docs/WDT_Stress_Test_Plan.md)

## 解析串口日志

```powershell
cd 01-firmware
python tools\wdt-stress\parse_wdt_log.py debug\wdt_TC01_baseline.txt
```

## 推荐最小执行集

1. TC-02 — 50 次断电冷启动（约 30 min）
2. TC-01 — 4 h 串口浸泡 + 解析
3. TC-04 — MPPT 拔插 5 轮
4. （可选）`env:wdt_stress` 实现后跑 TC-20 / TC-24

日志一律放在 `debug/`，勿写入仓库根目录。
