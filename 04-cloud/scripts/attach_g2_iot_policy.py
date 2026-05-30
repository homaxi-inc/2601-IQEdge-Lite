#!/usr/bin/env python3
"""Attach iqedge-g2-{env}-iot-policy-g2-device to a certificate ARN or Thing principals."""

from __future__ import annotations

import argparse
import sys

import boto3


def main() -> int:
    p = argparse.ArgumentParser(description="Attach G2 IoT device policy to certificate")
    p.add_argument("--env", choices=["dev", "prod"], default="dev")
    p.add_argument("--region", default="us-east-1")
    p.add_argument("--thing", help="Thing name; attach to all principals if --cert omitted")
    p.add_argument("--cert-arn", help="Certificate ARN (overrides --thing lookup)")
    p.add_argument("--dry-run", action="store_true")
    args = p.parse_args()

    policy_name = f"iqedge-g2-{args.env}-iot-policy-g2-device"
    iot = boto3.client("iot", region_name=args.region)

    targets: list[str] = []
    if args.cert_arn:
        targets = [args.cert_arn]
    elif args.thing:
        resp = iot.list_thing_principals(thingName=args.thing)
        targets = resp.get("principals", [])
        if not targets:
            print(f"No principals on thing {args.thing}", file=sys.stderr)
            return 1
    else:
        print("Provide --thing or --cert-arn", file=sys.stderr)
        return 1

    for arn in targets:
        print(f"Attach {policy_name} -> {arn}")
        if not args.dry_run:
            iot.attach_policy(policyName=policy_name, target=arn)

    print("Done")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
