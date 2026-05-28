# Payload Samples

## Payload v1

```json
{
  "device": "IQEdge",
  "mac": "00:4B:12:2F:15:88",
  "chipid": "88152F124B00",
  "firmware_version": "iqe_v01_r01.06.28",
  "timestamp": "2026-03-19 21:22:32 UTC",
  "status": "running",
  "data_stale": false,
  "last_update": 129000430,
  "battery": {
    "voltage": 13.80000019,
    "current": -0.050000001,
    "soc": 100,
    "charge_state": "Float",
    "charge_state_code": 5
  },
  "solar": {
    "voltage": 21.76000023,
    "current": 0.067024127,
    "power": 13
  },
  "load": {
    "voltage": 13.80000019,
    "current": 0.899999976,
    "status": "ON",
    "power": 12.42000008
  },
  "system": {
    "temp": 0,
    "output": 0,
    "pid": "0XA07D",
    "firmware": "166",
    "serial": "HQ2443XPTQY"
  },
  "history": {
    "total_yield_kwh": 6.245,
    "today_yield_kwh": 0.027,
    "yesterday_yield_kwh": 0.03,
    "month_yield_kwh": 0.09,
    "max_power_w": 89,
    "days_running": 218,
    "avg_daily_yield_kwh": 0.02864679,
    "today_vs_yesterday_pct": -10.00000238
  }
}
```

---

## Payload v2

```json
{
  "device": "IQEdge",
  "mac": "00:4B:12:2F:15:88",
  "is_reconciliation": false,
  "state": "RUNNING",
  "timestamp": "2026-03-19T21:22:32Z",
  "battery": {
    "voltage": 13.8,
    "soc": 100
  },
  "solar": {
    "power": 13.0
  },
  "load": {
    "power": 12.42,
    "current": 0.9,
    "status": "ON"
  },
  "system": {
    "serial": "HQ2443XPTQY"
  },
  "history": {
    "total_yield_kwh": 6.25,
    "today_yield_kwh": 0.03
  }
}
```
