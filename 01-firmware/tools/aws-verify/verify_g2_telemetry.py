#!/usr/bin/env python3
"""
G2 Timestream verification for Agent 007 (read-only).

Requires M2/M4 deployed by 008. Queries iqedge_g2_{env}_table_energy by sys_id.
"""
from __future__ import annotations

import argparse
import os
import sys
from datetime import datetime, timezone

try:
    import boto3
    from botocore.exceptions import ClientError
except ImportError:
    print("Install: pip install boto3", file=sys.stderr)
    sys.exit(2)

DEFAULT_DB = "iqedge_g2_dev_database"
DEFAULT_TABLE = "iqedge_g2_dev_table_energy"


def main() -> int:
    p = argparse.ArgumentParser(description="Verify G2 energy telemetry in Timestream")
    p.add_argument("--sys-id", required=True, help="e.g. IQ-26-00001")
    p.add_argument("--minutes", type=int, default=30, help="Lookback window")
    p.add_argument("--region", default=os.environ.get("AWS_DEFAULT_REGION", "us-east-1"))
    p.add_argument("--database", default=os.environ.get("G2_TIMESTREAM_DB", DEFAULT_DB))
    p.add_argument("--table", default=os.environ.get("G2_TIMESTREAM_TABLE", DEFAULT_TABLE))
    args = p.parse_args()

    client = boto3.client("timestream-query", region_name=args.region)
    # M4 ingest uses MeasureValueType=MULTI (g2_energy); scalar measure_value::double N/A.
    query = f"""
        SELECT sys_id, component_id, time, measure_name,
               battery_soc_pct, battery_voltage_v, solar_power_w, load_power_w,
               yield_total_kwh, yield_today_kwh
        FROM "{args.database}"."{args.table}"
        WHERE sys_id = '{args.sys_id.replace("'", "''")}'
          AND time > ago({args.minutes}m)
        ORDER BY time DESC
        LIMIT 25
    """

    print(f"Query G2 Timestream: {args.database}.{args.table}")
    print(f"sys_id={args.sys_id}  lookback={args.minutes}m  region={args.region}")
    print("---")

    try:
        resp = client.query(QueryString=query)
    except ClientError as e:
        code = e.response.get("Error", {}).get("Code", "")
        if code in ("ResourceNotFoundException", "ValidationException"):
            print("FAIL: G2 Timestream database/table not found or M2 not deployed yet.")
            print("      Ask Agent 008 to complete M2+M4 dev deploy.")
            print(f"      Detail: {e}")
            return 1
        raise

    rows = resp.get("Rows", [])
    cols = [c["Name"] for c in resp.get("ColumnInfo", [])]
    if not rows:
        print("FAIL: 0 rows — no G2 ingest for this sys_id in the time window.")
        print("      Check: firmware topic, IoT policy attach, M4 ingest Lambda logs.")
        return 1

    print(f"OK: {len(rows)} row(s) (showing up to 25)")
    for row in rows:
        vals = [d.get("ScalarValue", "") for d in row.get("Data", [])]
        print(dict(zip(cols, vals)))

    print("---")
    print(f"PASS at {datetime.now(timezone.utc).isoformat()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
