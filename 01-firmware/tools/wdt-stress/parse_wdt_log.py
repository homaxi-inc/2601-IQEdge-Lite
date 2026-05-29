#!/usr/bin/env python3
"""Parse ESP32 serial log for TWDT / reboot / MQTT publish health (Agent 007)."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

WDT_RE = re.compile(r"task_wdt.*triggered", re.I)
TASK_RE = re.compile(r"task_wdt:\s+-\s+(\S+)", re.I)
RST_RE = re.compile(r"rst:0x([0-9a-fA-F]+)\s+\(([^)]+)\)")
FW_RE = re.compile(r"FW:\s*(v[\d.]+)")
MQTT_OK_RE = re.compile(r"MQTT Connected", re.I)
PUBLISH_OK_RE = re.compile(r"Publish OK", re.I)
FS_FATAL_RE = re.compile(r"LittleFS mount failed|FATAL.*LittleFS", re.I)
GURU_RE = re.compile(r"Guru Meditation", re.I)


def parse(lines: list[str]) -> dict:
    stats = {
        "lines": len(lines),
        "wdt_events": 0,
        "wdt_tasks": [],
        "resets": [],
        "boots": 0,
        "firmware_versions": [],
        "mqtt_connected": 0,
        "publish_ok": 0,
        "fs_fatal": 0,
        "guru": 0,
    }
    for line in lines:
        if WDT_RE.search(line):
            stats["wdt_events"] += 1
        m = TASK_RE.search(line)
        if m:
            stats["wdt_tasks"].append(m.group(1))
        m = RST_RE.search(line)
        if m:
            stats["resets"].append((m.group(1), m.group(2)))
        if FW_RE.search(line):
            stats["boots"] += 1
            stats["firmware_versions"].append(FW_RE.search(line).group(1))
        if MQTT_OK_RE.search(line):
            stats["mqtt_connected"] += 1
        if PUBLISH_OK_RE.search(line):
            stats["publish_ok"] += 1
        if FS_FATAL_RE.search(line):
            stats["fs_fatal"] += 1
        if GURU_RE.search(line):
            stats["guru"] += 1
    return stats


def severity(stats: dict) -> str:
    if stats["guru"] > 0:
        return "P0/P1? (Guru Meditation — investigate)"
    if stats["fs_fatal"] > 0 and stats["publish_ok"] == 0:
        return "P2 (LittleFS FATAL, no publish)"
    if stats["boots"] >= 10 and stats["mqtt_connected"] == 0:
        return "P1 (boot loop, no MQTT)"
    if stats["boots"] >= 10 and stats["publish_ok"] == 0:
        return "P2 (many boots, no Publish OK)"
    if stats["wdt_events"] > 0 and stats["publish_ok"] > 0:
        return "P3/P4 (WDT seen but recovered publish)"
    if stats["wdt_events"] > 10:
        return "WARN (>10 WDT/hour — review Comm/Energy)"
    return "OK (no critical pattern)"


def main() -> int:
    ap = argparse.ArgumentParser(description="Parse IQEdge WDT stress serial log")
    ap.add_argument("logfile", type=Path, help="Serial capture under debug/")
    args = ap.parse_args()
    if not args.logfile.is_file():
        print(f"ERROR: not found: {args.logfile}", file=sys.stderr)
        return 1
    text = args.logfile.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    stats = parse(lines)
    sev = severity(stats)

    print("=" * 56)
    print(f"  WDT Log Analysis: {args.logfile.name}")
    print("=" * 56)
    print(f"  Lines:           {stats['lines']}")
    print(f"  Boot (FW:):      {stats['boots']}")
    if stats["firmware_versions"]:
        print(f"  FW versions:     {', '.join(dict.fromkeys(stats['firmware_versions']))}")
    print(f"  TWDT triggered:  {stats['wdt_events']}")
    if stats["wdt_tasks"]:
        from collections import Counter
        c = Counter(stats["wdt_tasks"])
        print(f"  WDT tasks:       {dict(c)}")
    print(f"  Reset events:    {len(stats['resets'])}")
    for code, name in stats["resets"][:5]:
        print(f"    rst:0x{code} ({name})")
    if len(stats["resets"]) > 5:
        print(f"    ... +{len(stats['resets']) - 5} more")
    print(f"  MQTT Connected:  {stats['mqtt_connected']}")
    print(f"  Publish OK:      {stats['publish_ok']}")
    print(f"  FS FATAL:        {stats['fs_fatal']}")
    print(f"  Guru Meditation: {stats['guru']}")
    print("-" * 56)
    print(f"  Severity hint:   {sev}")
    print("=" * 56)
    return 0


if __name__ == "__main__":
    sys.exit(main())
