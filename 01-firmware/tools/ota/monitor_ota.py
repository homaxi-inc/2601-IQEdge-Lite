#!/usr/bin/env python3
"""Poll IoT Job execution + DynamoDB firmware_version after OTA."""
from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
_ENV_FILE = _PROJECT_ROOT / ".env"

try:
    from dotenv import load_dotenv
except ImportError:
    sys.exit("pip install python-dotenv boto3")

load_dotenv(_ENV_FILE)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--job-id", required=True)
    parser.add_argument("--mac", required=True)
    parser.add_argument("--mppt-id", default="")
    parser.add_argument("--expected-fw", default="")
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()

    import boto3

    s = boto3.Session(
        aws_access_key_id=os.getenv("AWS_ACCESS_KEY_ID"),
        aws_secret_access_key=os.getenv("AWS_SECRET_ACCESS_KEY"),
        region_name=os.getenv("AWS_DEFAULT_REGION", "us-east-1"),
    )
    iot = s.client("iot")
    ddb = s.client("dynamodb")
    thing = f"IQEdge_{args.mac}"
    mppt = args.mppt_id or args.mac

    deadline = time.time() + args.timeout
    last_job = None
    last_fw = None

    while time.time() < deadline:
        try:
            ex = iot.describe_job_execution(
                jobId=args.job_id, thingName=thing, executionNumber=1
            )
            st = ex["execution"]["status"]
            if st != last_job:
                print(f"[job] {st}")
                last_job = st
            if st in ("SUCCEEDED", "FAILED", "REJECTED", "TIMED_OUT", "CANCELED"):
                break
        except iot.exceptions.ResourceNotFoundException:
            print("[job] waiting for execution...")

        if args.mppt_id:
            item = ddb.get_item(
                TableName="DeviceLatestStatus",
                Key={"deviceId": {"S": mppt}},
            ).get("Item", {})
            fw = item.get("firmware_version", {}).get("S", "")
            ts = item.get("timestamp", {}).get("S", "N/A")
            if fw != last_fw:
                print(f"[ddb] firmware_version={fw} timestamp={ts}")
                last_fw = fw
            if args.expected_fw and fw == args.expected_fw:
                print(f"[OK] Cloud reports {fw}")
                return 0 if last_job == "SUCCEEDED" else 1

        time.sleep(20)

    print(f"Done. job={last_job} fw={last_fw}")
    if args.expected_fw and last_fw == args.expected_fw and last_job == "SUCCEEDED":
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
