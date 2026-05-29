"""
verify_telemetry.py
Agent 007 — Secure Telemetry Verification for IQEdge-G2 (SOP V1.0)

Validates DynamoDB latest state + Timestream history (+ optional API egress).
Credentials: local .env only (never hardcode keys).
"""
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

# Load .env from 01-firmware project root (two levels up from this script)
_PROJECT_ROOT = Path(__file__).resolve().parents[2]
_ENV_FILE = _PROJECT_ROOT / ".env"

try:
    from dotenv import load_dotenv
except ImportError:
    print("Missing dependency: pip install python-dotenv boto3 requests")
    sys.exit(1)

load_dotenv(_ENV_FILE)

ACCESS_KEY = os.getenv("AWS_ACCESS_KEY_ID")
SECRET_KEY = os.getenv("AWS_SECRET_ACCESS_KEY")
REGION = os.getenv("AWS_DEFAULT_REGION", "us-east-1")

DDB_MPPT_TABLE = "DeviceLatestStatus"
DDB_RUT_TABLE = "RUTDevices_Attributes"
TS_MPPT_DB = "IQWatchDB"
TS_MPPT_TABLE = "DeviceStatus"
TS_RUT_DB = "RUTDevices_Metrics_DB"
TS_RUT_TABLE = "RUTDevices_Telemetry_Data"
IQWATCH_BASE = os.getenv(
    "IQWATCH_BASE_URL",
    "https://1y9689tax0.execute-api.us-east-1.amazonaws.com/v1",
)
RUT_API_BASE = os.getenv(
    "RUT_API_BASE",
    "https://asifayjlwc.execute-api.us-east-1.amazonaws.com/prod",
)

# Timestream data gap — do not rely on queries in this window (SOP §1.4)
TIMESTREAM_GAP_NOTE = "2026-03-07 .. 2026-04-01 (RUT Timestream gap per SOP)"


def _require_boto3():
    try:
        import boto3
        from botocore.exceptions import ClientError
    except ImportError:
        print("Missing dependency: pip install boto3")
        sys.exit(1)
    return boto3, ClientError


def _session():
    boto3, _ = _require_boto3()
    if not ACCESS_KEY or not SECRET_KEY:
        print("ERROR: AWS credentials missing.")
        print(f"Create {_ENV_FILE} from .env.example (AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY).")
        sys.exit(1)
    return boto3.Session(
        aws_access_key_id=ACCESS_KEY,
        aws_secret_access_key=SECRET_KEY,
        region_name=REGION,
    )


def verify_mppt_latest_status(ddb_client, device_id: str, client_error) -> bool:
    print(f"\n--- DynamoDB Latest Status (MPPT): {device_id} ---")
    try:
        response = ddb_client.get_item(
            TableName=DDB_MPPT_TABLE,
            Key={"deviceId": {"S": device_id}},
        )
        item = response.get("Item")
        if not item:
            print(f"WARN: deviceId '{device_id}' not found in '{DDB_MPPT_TABLE}'.")
            print("Action: Confirm MQTT publish / IoT rule / cloud deviceId mapping.")
            return False

        timestamp = item.get("timestamp", {}).get("S", "N/A")
        vbat = item.get("battery_voltage", {}).get("N", "N/A")
        solar_p = item.get("solar_power", {}).get("N", "N/A")
        load_p = item.get("load_power", {}).get("N", "N/A")
        soc = item.get("soc", {}).get("N", item.get("battery_soc", {}).get("N", "N/A"))
        mac = item.get("mac", {}).get("S", "N/A")
        state = item.get("state", {}).get("S", "N/A")
        today_kwh = item.get("today_yield_kwh", {}).get("N", "N/A")

        print("OK: DynamoDB record found.")
        print(f"   deviceId: {device_id}")
        print(f"   mac: {mac}")
        print(f"   last_reported: {timestamp}")
        print(f"   state: {state} | soc: {soc}% | Vbat: {vbat} | solar_W: {solar_p} | load_W: {load_p}")
        print(f"   today_yield_kwh: {today_kwh}")
        return True
    except client_error as e:
        print(f"ERROR: DynamoDB — {e.response['Error']['Message']}")
        return False


def verify_rut_latest_status(ddb_client, router_sn: str, client_error) -> bool:
    print(f"\n--- DynamoDB Latest Status (RUT): {router_sn} ---")
    try:
        response = ddb_client.get_item(
            TableName=DDB_RUT_TABLE,
            Key={"serial_input4": {"S": router_sn}},
        )
        item = response.get("Item")
        if not item:
            print(f"WARN: serial '{router_sn}' not found in '{DDB_RUT_TABLE}'.")
            return False

        status = item.get("status", {}).get("S", "N/A")
        last_updated = item.get("last_updated", {}).get("S", "N/A")
        iccid = item.get("iccid", {}).get("S", "N/A")

        print("OK: Router ingestion confirmed.")
        print(f"   serial: {router_sn} | status: {status} | iccid: {iccid}")
        print(f"   last_updated: {last_updated}")
        return True
    except client_error as e:
        print(f"ERROR: DynamoDB — {e.response['Error']['Message']}")
        return False


def query_timestream_history(ts_client, device_id: str, is_router: bool, client_error) -> bool:
    print(f"\n--- Timestream History (last 10, last 3d): {device_id} ---")
    if is_router:
        print(f"NOTE: Avoid relying on data in {TIMESTREAM_GAP_NOTE}")
        query_string = f"""
            SELECT time, measure_name, measure_value::double
            FROM "{TS_RUT_DB}"."{TS_RUT_TABLE}"
            WHERE (gateway_id = '{device_id}' OR device_serial = '{device_id}' OR serial_input4 = '{device_id}')
              AND time > ago(3d)
            ORDER BY time DESC
            LIMIT 10
        """
    else:
        query_string = f"""
            SELECT time, measure_name, measure_value::double
            FROM "{TS_MPPT_DB}"."{TS_MPPT_TABLE}"
            WHERE deviceId = '{device_id}'
              AND time > ago(3d)
            ORDER BY time DESC
            LIMIT 10
        """

    try:
        response = ts_client.query(QueryString=query_string)
        rows = response.get("Rows", [])
        if not rows:
            print(f"WARN: No Timestream rows for '{device_id}' in last 3 days.")
            return False

        col_names = [c.get("Name") for c in response.get("ColumnInfo", [])]
        print(f"OK: {len(rows)} records.")
        print("   " + " | ".join(col_names))
        print("-" * 65)
        for row in rows:
            data = [val.get("ScalarValue", "NULL") for val in row.get("Data", [])]
            print("   " + " | ".join(data))
        return True
    except client_error as e:
        print(f"ERROR: Timestream — {e.response['Error']['Message']}")
        return False


def verify_iqwatch_api_egress(serial: str) -> bool:
    """Section 5.1 — GET /v1/devices/{serial}/status"""
    api_key = os.getenv("IQWATCH_API_KEY")
    if not api_key:
        print("\n--- IQWatch API egress: skipped (IQWATCH_API_KEY not set) ---")
        return False
    if not serial:
        print("\n--- IQWatch API egress: skipped (no TEST_MPPT_SERIAL) ---")
        return False

    try:
        import requests
    except ImportError:
        print("pip install requests for API egress checks")
        return False

    url = f"{IQWATCH_BASE.rstrip('/')}/devices/{serial}/status"
    print(f"\n--- IQWatch API: GET {url} ---")
    try:
        r = requests.get(url, headers={"x-api-key": api_key}, timeout=30)
        print(f"HTTP {r.status_code}")
        if r.status_code == 200:
            print(r.text[:2000])
            return True
        print(r.text[:500])
        return False
    except requests.RequestException as e:
        print(f"ERROR: {e}")
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Agent 007 AWS telemetry verification")
    parser.add_argument("--mppt-id", default=os.getenv("TEST_MPPT_DEVICE_ID", ""))
    parser.add_argument("--mppt-serial", default=os.getenv("TEST_MPPT_SERIAL", ""))
    parser.add_argument("--rut-sn", default=os.getenv("TEST_RUT_SN", ""))
    parser.add_argument("--api-only", action="store_true", help="Only run IQWatch API egress check")
    parser.add_argument("--skip-rut", action="store_true")
    args = parser.parse_args()

    print("=" * 52)
    print("  IQEdge-G2 Telemetry Verification (Agent 007)")
    print("=" * 52)
    print(f"Env file: {_ENV_FILE} ({'found' if _ENV_FILE.exists() else 'MISSING'})")

    if args.api_only:
        ok = verify_iqwatch_api_egress(args.mppt_serial)
        return 0 if ok else 1

    boto3, ClientError = _require_boto3()
    session = _session()
    ddb = session.client("dynamodb")
    ts = session.client("timestream-query")

    results = []

    if args.mppt_id:
        ok = verify_mppt_latest_status(ddb, args.mppt_id, ClientError)
        results.append(ok)
        if ok:
            results.append(query_timestream_history(ts, args.mppt_id, False, ClientError))
    else:
        print("\nSKIP MPPT DynamoDB/Timestream: set TEST_MPPT_DEVICE_ID in .env or --mppt-id")

    if args.mppt_serial:
        results.append(verify_iqwatch_api_egress(args.mppt_serial))

    if not args.skip_rut and args.rut_sn:
        ok = verify_rut_latest_status(ddb, args.rut_sn, ClientError)
        results.append(ok)
        if ok:
            results.append(query_timestream_history(ts, args.rut_sn, True, ClientError))
    elif not args.skip_rut:
        print("\nSKIP RUT: set TEST_RUT_SN in .env or --rut-sn")

    print("\n" + "=" * 52)
    passed = sum(1 for r in results if r)
    total = len(results)
    print(f"Checks passed: {passed}/{total}" if total else "No checks run — configure .env")
    print("=" * 52)
    return 0 if total and all(results) else (1 if total else 2)


if __name__ == "__main__":
    sys.exit(main())
