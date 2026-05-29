#!/usr/bin/env python3
import os, json
from pathlib import Path
from dotenv import load_dotenv
load_dotenv(Path(__file__).resolve().parents[2] / ".env")
import boto3

needle = "9041"
s = boto3.Session(
    aws_access_key_id=os.getenv("AWS_ACCESS_KEY_ID"),
    aws_secret_access_key=os.getenv("AWS_SECRET_ACCESS_KEY"),
    region_name="us-east-1",
)
ddb = s.client("dynamodb")
token = None
hits = []
while True:
    kw = {"TableName": "DeviceLatestStatus", "Limit": 50}
    if token:
        kw["ExclusiveStartKey"] = token
    r = ddb.scan(**kw)
    for it in r.get("Items", []):
        blob = json.dumps(it)
        if needle in blob or "IQW-9041" in blob:
            d = {k: list(v.values())[0] for k, v in it.items()}
            hits.append(d)
    token = r.get("LastEvaluatedKey")
    if not token:
        break

print("Hits:", len(hits))
for h in hits:
    print(h.get("deviceId"), h.get("mac"), h.get("system_pid"), h.get("firmware_version"))
