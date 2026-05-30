#!/usr/bin/env python3
"""Upsert a G2 Registry record from JSON.

Usage:
  python seed_registry_item.py --env dev --file ../../09-contract/examples/registry/hil-iq-26-00001.v1.json
"""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import boto3


def denormalize_aliases(item: dict[str, Any]) -> dict[str, Any]:
    aliases = item.get("aliases") or {}
    if mppt := aliases.get("mppt_serial"):
        item["alias_mppt_serial"] = mppt
    if legacy := aliases.get("legacy_device_id"):
        item["alias_legacy_device_id"] = legacy
    return item


def resolve_table(env: str, table: str | None) -> str:
    if table:
        return table
    return f"iqedge-g2-{env}-table-registry"


def main() -> int:
    parser = argparse.ArgumentParser(description="Seed G2 Registry DynamoDB item")
    parser.add_argument("--env", choices=["dev", "prod"], default="dev")
    parser.add_argument("--region", default="us-east-1")
    parser.add_argument("--table", help="Override DDB table name")
    parser.add_argument("--file", type=Path, required=True)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    if not args.file.is_file():
        print(f"File not found: {args.file}", file=sys.stderr)
        return 1

    item = denormalize_aliases(json.loads(args.file.read_text(encoding="utf-8")))
    if "sys_id" not in item:
        print("Missing required field: sys_id", file=sys.stderr)
        return 1

    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    item.setdefault("created_at", now)
    item["updated_at"] = now

    table_name = resolve_table(args.env, args.table)
    print(f"Target: {table_name} sys_id={item['sys_id']} track={item.get('track')}")

    if args.dry_run:
        print(json.dumps(item, indent=2))
        return 0

    table = boto3.resource("dynamodb", region_name=args.region).Table(table_name)
    table.put_item(Item=item)
    print("PutItem OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
