#!/usr/bin/env python3
import os, sys
from pathlib import Path
from dotenv import load_dotenv
load_dotenv(Path(__file__).resolve().parents[2] / ".env")
import boto3

needle = sys.argv[1] if len(sys.argv) > 1 else "9041"
s = boto3.Session(
    aws_access_key_id=os.getenv("AWS_ACCESS_KEY_ID"),
    aws_secret_access_key=os.getenv("AWS_SECRET_ACCESS_KEY"),
    region_name="us-east-1",
)
ddb = s.client("dynamodb")
ts = s.client("timestream-query")
iot = s.client("iot")

print("DDB scan contains", needle)
resp = ddb.scan(
    TableName="DeviceLatestStatus",
    FilterExpression="contains(deviceId, :p)",
    ExpressionAttributeValues={":p": {"S": needle}},
)
for it in resp.get("Items", []):
    d = {k: list(v.values())[0] for k, v in it.items()}
    print(" ", d.get("deviceId"), d.get("mac"), d.get("timestamp", d.get("last_reported")), d.get("firmware_version"))

q = f"""
SELECT DISTINCT deviceId
FROM "IQWatchDB"."DeviceStatus"
WHERE time > ago(30d) AND deviceId LIKE '%{needle}%'
"""
print("Timestream deviceIds:")
r = ts.query(QueryString=q)
for row in r.get("Rows", []):
    print(" ", row["Data"][0]["ScalarValue"])

print("IoT things (sample):")
for t in iot.list_things(maxResults=50).get("things", []):
    if needle.lower() in t["thingName"].lower():
        print(" ", t["thingName"])
