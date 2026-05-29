#!/usr/bin/env python3
"""Analyze MPPT Timestream + DDB for a time window (Skill 4: iqedge-telemetry-analysis)."""
from __future__ import annotations

import argparse
import os
import statistics
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

_PROJECT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(_PROJECT))

try:
    from dotenv import load_dotenv
except ImportError:
    sys.exit("pip install python-dotenv boto3 requests")

load_dotenv(_PROJECT / ".env")

TS_DB = "IQWatchDB"
TS_TABLE = "DeviceStatus"
DDB_TABLE = "DeviceLatestStatus"


def _ddb_item_to_dict(item: dict) -> dict:
    out = {}
    for k, v in item.items():
        if "S" in v:
            out[k] = v["S"]
        elif "N" in v:
            try:
                out[k] = float(v["N"])
            except ValueError:
                out[k] = v["N"]
        elif "BOOL" in v:
            out[k] = v["BOOL"]
    return out


def _parse_ts(s: str) -> datetime:
    # Timestream: 2026-05-29 12:00:00.000000000
    s = s.split(".")[0].strip()
    return datetime.strptime(s, "%Y-%m-%d %H:%M:%S").replace(tzinfo=timezone.utc)


def fetch_timestream(ts_client, device_id: str, hours: int) -> list[tuple[str, str, float | None]]:
    q = f"""
        SELECT time, measure_name, measure_value::double
        FROM "{TS_DB}"."{TS_TABLE}"
        WHERE deviceId = '{device_id}'
          AND time > ago({hours}h)
        ORDER BY time ASC
    """
    rows: list[tuple[str, str, float | None]] = []
    resp = ts_client.query(QueryString=q)
    while True:
        for row in resp.get("Rows", []):
            data = row["Data"]
            t = data[0]["ScalarValue"]
            m = data[1]["ScalarValue"]
            sv = data[2].get("ScalarValue")
            v = float(sv) if sv is not None else None
            rows.append((t, m, v))
        token = resp.get("NextToken")
        if not token:
            break
        resp = ts_client.query(QueryString=q, NextToken=token)
    return rows


def analyze(device_id: str, hours: int) -> int:
    import boto3
    import requests
    from botocore.exceptions import ClientError

    key = os.getenv("AWS_ACCESS_KEY_ID")
    secret = os.getenv("AWS_SECRET_ACCESS_KEY")
    if not key or not secret:
        print("Missing AWS credentials in .env")
        return 1

    session = boto3.Session(
        aws_access_key_id=key,
        aws_secret_access_key=secret,
        region_name=os.getenv("AWS_DEFAULT_REGION", "us-east-1"),
    )
    ddb = session.client("dynamodb")
    ts = session.client("timestream-query")

    print("=" * 60)
    print(f"Device: {device_id}  |  Window: last {hours}h")
    print("=" * 60)

    # DynamoDB
    print("\n--- DynamoDB latest ---")
    try:
        r = ddb.get_item(TableName=DDB_TABLE, Key={"deviceId": {"S": device_id}})
        item = r.get("Item")
        if not item:
            print(f"NOT FOUND: deviceId={device_id}")
            scan = ddb.scan(
                TableName=DDB_TABLE,
                FilterExpression="contains(deviceId, :p)",
                ExpressionAttributeValues={":p": {"S": "9041"}},
                Limit=15,
            )
            cands = [x.get("deviceId", {}).get("S") for x in scan.get("Items", [])]
            if cands:
                print("Scan candidates containing 9041:", cands)
        else:
            d = _ddb_item_to_dict(item)
            for k in sorted(d.keys()):
                print(f"  {k}: {d[k]}")
            ddb_snap = d
    except ClientError as e:
        print("DDB error:", e.response["Error"]["Message"])
        ddb_snap = {}

    # API
    print("\n--- IQWatch API ---")
    api_key = os.getenv("IQWATCH_API_KEY")
    base = os.getenv(
        "IQWATCH_BASE_URL",
        "https://1y9689tax0.execute-api.us-east-1.amazonaws.com/v1",
    )
    api_snap = {}
    if api_key:
        url = f"{base.rstrip('/')}/devices/{device_id}/status"
        try:
            resp = requests.get(url, headers={"x-api-key": api_key}, timeout=30)
            print(f"HTTP {resp.status_code}")
            if resp.ok:
                api_snap = resp.json()
                for k in (
                    "deviceId",
                    "mac",
                    "firmware_version",
                    "timestamp",
                    "soc",
                    "battery_voltage",
                    "solar_power",
                    "load_power",
                    "total_yield_kwh",
                    "today_yield_kwh",
                    "state",
                    "error_code",
                    "off_reason",
                ):
                    if k in api_snap:
                        print(f"  {k}: {api_snap[k]}")
            else:
                print(resp.text[:400])
        except Exception as e:
            print("API error:", e)
    else:
        print("SKIP: no IQWATCH_API_KEY")

    # Timestream
    print(f"\n--- Timestream ({hours}h) ---")
    try:
        raw = fetch_timestream(ts, device_id, hours)
    except ClientError as e:
        print("Timestream error:", e.response["Error"]["Message"])
        return 1

    if not raw:
        print("NO rows in window.")
        return 1

    by_ts: dict[str, dict[str, float | None]] = defaultdict(dict)
    for t, m, v in raw:
        by_ts[t][m] = v

    times = sorted(by_ts.keys())
    print(f"Measure points: {len(raw)}  |  Report timestamps: {len(times)}")
    print(f"Range: {times[0][:19]} .. {times[-1][:19]} UTC")

    gaps_sec = []
    for i in range(1, len(times)):
        gaps_sec.append((_parse_ts(times[i]) - _parse_ts(times[i - 1])).total_seconds())
    if gaps_sec:
        print(
            f"Gap between reports (s): min={min(gaps_sec):.0f} "
            f"median={statistics.median(gaps_sec):.0f} max={max(gaps_sec):.0f}"
        )

    # Per-measure stats
    measures = sorted({m for _, m, _ in raw})
    print(f"Measures seen: {', '.join(measures)}")

    issues: list[str] = []

    def check_range(name: str, vals: list[float], lo: float, hi: float, unit: str):
        bad = [v for v in vals if v is not None and (v < lo or v > hi)]
        if bad:
            issues.append(f"{name} out of range [{lo},{hi}] {unit}: samples={bad[:5]}")
        return vals

    soc_vals = [by_ts[t].get("soc") for t in times if by_ts[t].get("soc") is not None]
    vbat_vals = [
        by_ts[t].get("battery_voltage")
        for t in times
        if by_ts[t].get("battery_voltage") is not None
    ]
    solar_vals = [
        by_ts[t].get("solar_power") for t in times if by_ts[t].get("solar_power") is not None
    ]
    today_vals = [
        by_ts[t].get("today_yield_kwh")
        for t in times
        if by_ts[t].get("today_yield_kwh") is not None
    ]
    total_vals = [
        by_ts[t].get("total_yield_kwh")
        for t in times
        if by_ts[t].get("total_yield_kwh") is not None
    ]

    if soc_vals:
        check_range("soc", soc_vals, 0, 100, "%")
        if max(soc_vals) - min(soc_vals) > 40 and len(times) < 20:
            issues.append(f"soc swing {min(soc_vals):.1f}-{max(soc_vals):.1f}% in {len(times)} reports")
    if vbat_vals:
        check_range("battery_voltage", vbat_vals, 10.0, 14.6, "V")
    if solar_vals:
        check_range("solar_power", solar_vals, 0, 200, "W")

    # Monotonic yield
    if len(total_vals) >= 2:
        for i in range(1, len(total_vals)):
            if total_vals[i] < total_vals[i - 1] - 0.05:
                issues.append(
                    f"total_yield_kwh decreased: {total_vals[i-1]} -> {total_vals[i]}"
                )
                break
    if len(today_vals) >= 2:
        for i in range(1, len(today_vals)):
            if today_vals[i] < today_vals[i - 1] - 0.01:
                t_prev, t_curr = times[i - 1], times[i]
                issues.append(
                    f"today_yield_kwh decreased: {today_vals[i-1]} -> {today_vals[i]} "
                    f"at {t_curr[:19]} (likely midnight H20 reset if ~00:00 UTC)"
                )
                break

    # 10x yield heuristic
    if total_vals and max(total_vals) > 500:
        issues.append(f"total_yield_kwh very large (10x bug?): max={max(total_vals)}")

    # Night solar
    night_solar = [v for v in solar_vals if v and v > 50]
    if len(night_solar) > len(solar_vals) * 0.3 and max(solar_vals or [0]) > 20:
        issues.append("high solar_power on many samples — check night misclassification")

    print("\n--- Last 12 snapshots ---")
    for t in times[-12:]:
        d = by_ts[t]
        print(
            f"  {t[:19]} | soc={d.get('soc')} V={d.get('battery_voltage')} "
            f"solar={d.get('solar_power')}W load={d.get('load_power')}W "
            f"today={d.get('today_yield_kwh')} total={d.get('total_yield_kwh')}"
        )

    # DDB vs API vs last TS
    print("\n--- Cross-layer consistency (latest) ---")
    last = by_ts[times[-1]]
    if ddb_snap:
        for field, ts_key in [
            ("soc", "soc"),
            ("battery_voltage", "battery_voltage"),
            ("solar_power", "solar_power"),
            ("firmware_version", "firmware_version"),
        ]:
            dv = ddb_snap.get(field)
            tv = last.get(ts_key)
            if dv is not None and tv is not None and abs(float(dv) - float(tv)) > 0.5:
                if field == "soc" and abs(float(dv) - float(tv)) > 5:
                    issues.append(f"DDB vs TS last mismatch {field}: DDB={dv} TS={tv}")
                elif field != "soc" and field != "firmware_version":
                    issues.append(f"DDB vs TS last mismatch {field}: DDB={dv} TS={tv}")
    if api_snap and last:
        for field in ("soc", "battery_voltage", "solar_power"):
            av = api_snap.get(field)
            tv = last.get(field)
            if av is not None and tv is not None and abs(float(av) - float(tv)) > 5:
                issues.append(f"API vs TS last mismatch {field}: API={av} TS={tv}")

    print("\n--- Analysis verdict ---")
    if not issues:
        print("PASS: No obvious anomalies in range, monotonicity, or cross-layer checks.")
    else:
        print("REVIEW:")
        for i, msg in enumerate(issues, 1):
            print(f"  {i}. {msg}")

    return 0


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--device", default="IQW-9041")
    p.add_argument("--hours", type=int, default=10)
    args = p.parse_args()
    return analyze(args.device, args.hours)


if __name__ == "__main__":
    sys.exit(main())
