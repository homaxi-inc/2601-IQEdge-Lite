#!/usr/bin/env python3
"""Upload firmware BIN to S3 and create AWS IoT Job for IQEdge OTA (TC-OTA-02)."""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
_ENV_FILE = _PROJECT_ROOT / ".env"

try:
    from dotenv import load_dotenv
except ImportError:
    print("pip install python-dotenv boto3")
    sys.exit(1)

load_dotenv(_ENV_FILE)

BUCKET = "iqwatch-firmware-ota"
REGION = os.getenv("AWS_DEFAULT_REGION", "us-east-1")
ACCOUNT = "661631955220"


def _session():
    import boto3

    # OTA_AWS_* overrides read-only Agent-007 creds when present
    key = os.getenv("OTA_AWS_ACCESS_KEY_ID") or os.getenv("AWS_ACCESS_KEY_ID")
    secret = os.getenv("OTA_AWS_SECRET_ACCESS_KEY") or os.getenv("AWS_SECRET_ACCESS_KEY")
    if not key or not secret:
        print(f"Missing AWS creds in {_ENV_FILE} (or OTA_AWS_* for upload/create-job)")
        sys.exit(1)
    who = "OTA_AWS_*" if os.getenv("OTA_AWS_ACCESS_KEY_ID") else "AWS_*"
    print(f"Using credentials: {who}")
    return boto3.Session(
        aws_access_key_id=key,
        aws_secret_access_key=secret,
        region_name=REGION,
    )


def mac_to_thing(mac: str) -> str:
    return f"IQEdge_{mac}"


def upload_and_presign(s3, local_bin: Path, s3_key: str, expires_sec: int) -> str:
    size = local_bin.stat().st_size
    print(f"Uploading {local_bin} ({size} bytes) -> s3://{BUCKET}/{s3_key}")
    s3.upload_file(
        str(local_bin),
        BUCKET,
        s3_key,
        ExtraArgs={"ContentType": "application/octet-stream"},
    )
    url = s3.generate_presigned_url(
        "get_object",
        Params={"Bucket": BUCKET, "Key": s3_key},
        ExpiresIn=expires_sec,
    )
    print(f"Presigned URL (expires {expires_sec}s): {url[:80]}...")
    return url


def create_job(iot, thing_name: str, job_id: str, url: str, description: str) -> None:
    target = f"arn:aws:iot:{REGION}:{ACCOUNT}:thing/{thing_name}"
    document = json.dumps({"url": url})
    print(f"Creating job {job_id} -> {target}")
    iot.create_job(
        jobId=job_id,
        targets=[target],
        document=document,
        description=description,
        targetSelection="SNAPSHOT",
    )


def wait_job(iot, thing_name: str, job_id: str, timeout_sec: int = 600) -> str:
    deadline = time.time() + timeout_sec
    last = None
    while time.time() < deadline:
        try:
            r = iot.describe_job_execution(
                jobId=job_id, thingName=thing_name, executionNumber=1
            )
            status = r["execution"]["status"]
            if status != last:
                print(f"  Job execution status: {status}")
                last = status
            if status in ("SUCCEEDED", "FAILED", "REJECTED", "TIMED_OUT", "CANCELED"):
                return status
        except iot.exceptions.ResourceNotFoundException:
            print("  Waiting for job execution to appear...")
        time.sleep(15)
    return "TIMEOUT"


def main() -> int:
    parser = argparse.ArgumentParser(description="Push IQEdge OTA via AWS IoT Jobs")
    parser.add_argument("--mac", required=True, help="ESP32 MAC e.g. 1C:69:20:B8:D7:F4")
    parser.add_argument(
        "--bin",
        default=str(_PROJECT_ROOT / ".pio" / "build" / "esp32dev" / "firmware.bin"),
    )
    parser.add_argument("--version", default="v2.2.3.25", help="Version tag in S3 key")
    parser.add_argument("--job-id", default="", help="IoT Job ID (auto if empty)")
    parser.add_argument("--expires", type=int, default=86400, help="Presign TTL seconds")
    parser.add_argument("--wait", action="store_true", help="Poll job until terminal state")
    parser.add_argument("--no-upload", action="store_true", help="Use existing S3 key only")
    args = parser.parse_args()

    thing = mac_to_thing(args.mac)
    s3_key = f"firmware_{args.version}.bin"
    job_id = args.job_id or f"IQW-OTA-{args.mac.replace(':', '')}-{datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S')}"

    session = _session()
    s3 = session.client("s3")
    iot = session.client("iot")

    local_bin = Path(args.bin)
    if not args.no_upload:
        if not local_bin.is_file():
            print(f"Missing firmware: {local_bin}")
            return 1
        url = upload_and_presign(s3, local_bin, s3_key, args.expires)
    else:
        url = s3.generate_presigned_url(
            "get_object",
            Params={"Bucket": BUCKET, "Key": s3_key},
            ExpiresIn=args.expires,
        )

    create_job(
        iot,
        thing,
        job_id,
        url,
        f"Agent007 OTA test {args.version} for {thing}",
    )
    print(f"Job created: {job_id}")
    print(f"Thing: {thing}")

    if args.wait:
        status = wait_job(iot, thing, job_id)
        print(f"Final status: {status}")
        return 0 if status == "SUCCEEDED" else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
