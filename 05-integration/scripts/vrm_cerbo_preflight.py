#!/usr/bin/env python3
"""
VRM API preflight for IQTrailer Cerbo integration (007 handoff).

Loads credentials from 05-integration/.env (gitignored).
Reuses vrm_client from 003_IQTrailer/04-test when available.

Usage (from repo root):
  python 05-integration/scripts/vrm_cerbo_preflight.py
  python 05-integration/scripts/vrm_cerbo_preflight.py --site 964243
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path

INTEGRATION_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = INTEGRATION_ROOT.parent
TEST_ROOT = REPO_ROOT.parent / "26_Dev" / "003_IQTrailer" / "04-test"

# Prefer G2 .env; fallback to research repo
from dotenv import load_dotenv

load_dotenv(INTEGRATION_ROOT / ".env")
if not os.environ.get("VRM_ACCESS_TOKEN"):
    load_dotenv(REPO_ROOT.parent / "003_IQTrailer" / ".env")

if str(TEST_ROOT) not in sys.path:
    sys.path.insert(0, str(TEST_ROOT))

try:
    from vrm_client import get_json  # type: ignore
except ImportError as exc:
    raise SystemExit(
        f"Need vrm_client from {TEST_ROOT}. pip install -r 04-test/requirements.txt"
    ) from exc

DEFAULT_SITE = "964243"  # ST-03 · IQTrailer lab
BATTERY_INSTANCE = 278
SOLAR_INSTANCE = 277


def extract_widget_metrics(records: dict) -> dict[str, dict]:
    data = records.get("data") or {}
    meta = records.get("meta") or {}
    out: dict[str, dict] = {}
    for key, item in data.items():
        if not isinstance(item, dict) or key in ("hasOldData", "secondsAgo"):
            continue
        code = item.get("code") or (meta.get(str(key)) or {}).get("code")
        if code:
            out[str(code)] = {
                "valueFloat": item.get("valueFloat"),
                "formattedValue": item.get("formattedValue"),
                "valueEnum": item.get("valueEnum"),
                "nameEnum": item.get("nameEnum"),
                "secondsAgo": item.get("secondsAgo"),
            }
    return out


def find_gateway(devices: list) -> dict | None:
    for d in devices:
        name = (d.get("productName") or d.get("name") or "").lower()
        if "cerbo" in name or "gateway" in name:
            return d
    return None


def compute_load_w(solar_w: float, voltage_v: float, current_a: float) -> float:
    return max(0.0, solar_w - voltage_v * current_a)


def main() -> int:
    parser = argparse.ArgumentParser(description="VRM Cerbo preflight for 007")
    parser.add_argument("--site", default=DEFAULT_SITE, help="VRM idSite (default ST-03 964243)")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=INTEGRATION_ROOT / "docs" / "deliveries" / "vrm_preflight_latest.json",
    )
    args = parser.parse_args()
    site = str(args.site)
    user_id = os.environ.get("VRM_USER_ID", "")

    results: dict = {
        "fetchedAtUtc": datetime.now(timezone.utc).isoformat(),
        "idSite": site,
        "tests": {},
        "g2BenchReference": {},
        "pass": True,
    }

    # T1 Auth
    me = get_json("/users/me")
    user = me.get("user") or {}
    results["tests"]["T1_users_me"] = {
        "pass": bool(user.get("id")),
        "userId": user.get("id"),
        "email": user.get("email"),
    }

    # T2 Installations
    inst = get_json(f"/users/{user_id}/installations")
    sites = inst.get("records") or []
    results["tests"]["T2_installations"] = {
        "pass": isinstance(sites, list) and len(sites) > 0,
        "count": len(sites),
        "siteIds": [s.get("idSite") for s in sites if s.get("idSite")],
    }

    # T3 System overview · Cerbo/Gateway id
    overview = get_json(f"/installations/{site}/system-overview")
    devices = (overview.get("records") or {}).get("devices") or []
    gateway = find_gateway(devices)
    gw_id = (gateway or {}).get("identifier") or (gateway or {}).get("machineSerialNumber")
    results["tests"]["T3_system_overview"] = {
        "pass": gw_id is not None,
        "gatewayIdentifier": gw_id,
        "loggingIntervalSec": (gateway or {}).get("loggingInterval"),
        "devices": [
            {
                "instance": d.get("instance"),
                "productName": d.get("productName"),
                "serial": d.get("machineSerialNumber") or d.get("identifier"),
                "role": (
                    "cerbo"
                    if "cerbo" in (d.get("productName") or "").lower()
                    or "gateway" in (d.get("name") or "").lower()
                    else ("mppt" if "solar" in (d.get("productName") or "").lower() else "shunt")
                ),
            }
            for d in devices
        ],
    }

    # T4 Battery realtime (SmartShunt / system battery semantics)
    bat = get_json(f"/installations/{site}/widgets/BatterySummary?instance={BATTERY_INSTANCE}")
    bat_m = extract_widget_metrics(bat.get("records") or {})
    results["tests"]["T4_battery_realtime"] = {
        "pass": all(c in bat_m for c in ("V", "I", "SOC")),
        "instance": BATTERY_INSTANCE,
        "metrics": {k: bat_m[k] for k in ("V", "I", "SOC") if k in bat_m},
    }

    # T5 Solar realtime + yield (MPPT widget · cross-check Modbus 850 / solarcharger)
    sol = get_json(f"/installations/{site}/widgets/SolarChargerSummary?instance={SOLAR_INSTANCE}")
    sol_m = extract_widget_metrics(sol.get("records") or {})
    results["tests"]["T5_solar_realtime"] = {
        "pass": "ScW" in sol_m,
        "instance": SOLAR_INSTANCE,
        "metrics": {k: sol_m[k] for k in ("ScW", "ScV", "ScS", "YT", "YY") if k in sol_m},
    }

    # T6 Stats live_feed (note: total_solar_yield is NOT kWh)
    stats = get_json(f"/installations/{site}/stats?type=live_feed&interval=15mins")
    rec = stats.get("records") or {}
    latest: dict = {}
    for code in ("bv", "bs", "Pdc", "total_solar_yield"):
        pts = rec.get(code)
        if isinstance(pts, list) and pts:
            latest[code] = {"last": pts[-1]}
    results["tests"]["T6_stats_live_feed"] = {
        "pass": "bv" in latest and "bs" in latest,
        "latest": latest,
        "warning": "total_solar_yield is normalized internal scale - use YT/YY widgets for kWh",
    }

    # G2 bench reference snapshot
    v = float(bat_m.get("V", {}).get("valueFloat") or 0)
    i = float(bat_m.get("I", {}).get("valueFloat") or 0)
    soc = float(bat_m.get("SOC", {}).get("valueFloat") or 0)
    solar_w = float(sol_m.get("ScW", {}).get("valueFloat") or 0)
    load_w = compute_load_w(solar_w, v, i)
    max_ago = max(
        bat_m.get("V", {}).get("secondsAgo") or 0,
        sol_m.get("ScW", {}).get("secondsAgo") or 0,
    )

    results["g2BenchReference"] = {
        "sys_id": "IQ-26-60001",
        "component_id_cerbo": gw_id,
        "component_role": "cerbo",
        "registry_mppt_serial": next(
            (d["serial"] for d in results["tests"]["T3_system_overview"]["devices"] if d["role"] == "mppt"),
            None,
        ),
        "measures_expected_at_modbus": {
            "battery.voltage_v": round(v, 2),
            "battery.current_a": round(i, 2),
            "battery.soc_pct": round(soc, 1),
            "solar.power_w": round(solar_w, 1),
            "load.power_w": round(load_w, 1),
            "yield.today_kwh": sol_m.get("YT", {}).get("valueFloat"),
            "yield.yesterday_kwh": sol_m.get("YY", {}).get("valueFloat"),
        },
        "modbus_mvp_registers_unit_100": "840-844, 850 - see cerbo/modbus-register-map.md",
        "data_stale_threshold_min": 25,
        "vrm_seconds_ago": max_ago,
        "vrm_logging_interval_sec": (gateway or {}).get("loggingInterval"),
    }

    for name, t in results["tests"].items():
        if not t.get("pass"):
            results["pass"] = False

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"VRM Cerbo preflight site={site} pass={results['pass']}")
    print(f"  Gateway component_id: {gw_id}")
    print(f"  V={v}V I={i}A SOC={soc}% ScW={solar_w}W load?{load_w:.1f}W")
    print(f"  YT={sol_m.get('YT', {}).get('valueFloat')} YY={sol_m.get('YY', {}).get('valueFloat')} kWh")
    print(f"  secondsAgo={max_ago} (stale if Modbus fail >25min)")
    print(f"Wrote {args.output}")
    return 0 if results["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
