#!/usr/bin/env python3
"""Read-only bulk import: Legacy device identifiers → G2 Registry (track=legacy).

Does NOT modify Legacy DDB / Timestream tables. Source data is supplied via JSON export
or future Legacy API — this script is a scaffold for M3.3.

Usage:
  python import_legacy_registry.py --env dev --input legacy_export.json --dry-run
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from seed_registry_item import denormalize_aliases, resolve_table

import boto3


def main() -> int:
    parser = argparse.ArgumentParser(description="Import Legacy fleet rows into G2 Registry")
    parser.add_argument("--env", choices=["dev", "prod"], default="dev")
    parser.add_argument("--region", default="us-east-1")
    parser.add_argument("--table", help="Override registry table name")
    parser.add_argument(
        "--input",
        type=Path,
        required=True,
        help="JSON array of partial registry records (sys_id + aliases required)",
    )
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    if not args.input.is_file():
        print(f"Input not found: {args.input}", file=sys.stderr)
        return 1

    rows = json.loads(args.input.read_text(encoding="utf-8"))
    if not isinstance(rows, list):
        print("Input must be a JSON array", file=sys.stderr)
        return 1

    table_name = resolve_table(args.env, args.table)
    table = boto3.resource("dynamodb", region_name=args.region).Table(table_name)
    ok = 0

    for row in rows:
        row.setdefault("track", "legacy")
        row.setdefault("id_format", "legacy_prefixed")
        if "sys_id" not in row:
            print(f"Skip row without sys_id: {row}", file=sys.stderr)
            continue
        item = denormalize_aliases(row)
        if args.dry_run:
            print(json.dumps(item, indent=2))
        else:
            table.put_item(Item=item)
        ok += 1

    print(f"Processed {ok} record(s) → {table_name}" + (" (dry-run)" if args.dry_run else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
