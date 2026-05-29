#!/usr/bin/env python3
"""TC-02: Monitor COM3 during power-cycle stress test."""

from __future__ import annotations

import argparse
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
except ImportError:
    print("pip install pyserial", file=sys.stderr)
    sys.exit(1)

KEYWORDS = (
    "rst:",
    "task_wdt",
    "Guru",
    "[BOOT]",
    "MQTT Connected",
    "Publish OK",
    "LittleFS",
    "FATAL",
    "EMERGENCY",
    "MPPT RECONNECTED",
    "abort()",
)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM3")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--target-boots", type=int, default=50)
    ap.add_argument("--max-minutes", type=int, default=75)
    ap.add_argument("--idle-stop-sec", type=int, default=120)
    ap.add_argument(
        "--wait-port-min",
        type=int,
        default=10,
        help="Wait up to N minutes for COM port to appear",
    )
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[2]
    out = root / "debug" / f"wdt_TC02_powercycle_{datetime.now().strftime('%Y%m%d_%H%M')}.txt"

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    def _log(msg: str) -> None:
        print(msg, flush=True)

    out.parent.mkdir(parents=True, exist_ok=True)
    log_fp = open(out, "w", encoding="utf-8")
    log_fp.write(f"# TC-02 started {datetime.now().isoformat()}\n")
    log_fp.flush()

    _log("=" * 60)
    _log(f"TC-02 Power cycle monitor — {args.port} @ {args.baud}")
    _log(f"Log: {out}")
    _log(f"Target: {args.target_boots} boots, max {args.max_minutes} min")
    _log("=" * 60)

    ser = None
    wait_deadline = time.time() + args.wait_port_min * 60
    while ser is None and time.time() < wait_deadline:
        try:
            ser = serial.Serial(args.port, args.baud, timeout=0.5)
            _log(f"Port {args.port} open OK.")
        except serial.SerialException as e:
            _log(f"Waiting for {args.port}... ({e})")
            time.sleep(2)
    if ser is None:
        _log(f"ERROR: {args.port} not available after {args.wait_port_min} min")
        log_fp.close()
        return 1

    stats = {
        "boot": 0,
        "rst": 0,
        "wdt": 0,
        "mqtt": 0,
        "pub": 0,
        "guru": 0,
        "fs_fatal": 0,
    }
    start = time.time()
    max_sec = args.max_minutes * 60
    last_boot = 0.0

    try:
        while time.time() - start < max_sec:
            raw = ser.readline()
            if not raw:
                if (
                    stats["boot"] >= args.target_boots
                    and time.time() - last_boot > args.idle_stop_sec
                ):
                    _log(
                        f"\n>>> {stats['boot']} boots done, idle "
                        f"{args.idle_stop_sec}s — stop."
                    )
                    break
                continue
            line = raw.decode("utf-8", errors="replace").rstrip()
            if not line:
                continue
            ts = datetime.now().strftime("%H:%M:%S")
            log_fp.write(f"[{ts}] {line}\n")
            log_fp.flush()

            if "FW:" in line and "IQEdge" in line:
                stats["boot"] += 1
                last_boot = time.time()
                _log(f"[{ts}] *** BOOT #{stats['boot']} *** {line[:72]}")
            elif any(k in line for k in KEYWORDS):
                if "rst:" in line:
                    stats["rst"] += 1
                if "task_wdt" in line:
                    stats["wdt"] += 1
                if "MQTT Connected" in line:
                    stats["mqtt"] += 1
                if "Publish OK" in line:
                    stats["pub"] += 1
                if "Guru" in line:
                    stats["guru"] += 1
                if "FATAL" in line or "LittleFS mount failed" in line:
                    stats["fs_fatal"] += 1
                _log(f"[{ts}] {line[:110]}")
    finally:
        ser.close()
        log_fp.close()
    elapsed = int(time.time() - start)
    summary = f"""
============================================================
TC-02 SUMMARY ({elapsed}s)
  Log: {out.name}
  Boot (FW banner): {stats['boot']}
  Reset (rst:):     {stats['rst']}
  TWDT:             {stats['wdt']}
  MQTT Connected:   {stats['mqtt']}
  Publish OK:       {stats['pub']}
  Guru:             {stats['guru']}
  FS FATAL:         {stats['fs_fatal']}
============================================================
"""
    _log(summary)
    with open(out, "a", encoding="utf-8") as f:
        f.write(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
