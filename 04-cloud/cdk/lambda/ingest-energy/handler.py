"""
G2 energy ingest — IoT Rule → Timestream + Shadow (M4).
ADR-008: Timestream write only when Registry track=g2 and firmware_version >= 2.3.0.
Does not auto-promote track (manual only).
"""
from __future__ import annotations

import json
import logging
import os
import re
from datetime import datetime, timezone
from decimal import Decimal
from typing import Any

import boto3
from botocore.exceptions import ClientError

logger = logging.getLogger()
logger.setLevel(logging.INFO)

METRIC_NAMESPACE = os.environ.get("METRIC_NAMESPACE", "IQEdge/G2/Ingest")

_ts_write = boto3.client("timestream-write")
_ddb = boto3.resource("dynamodb")
_cw = boto3.client("cloudwatch")

FW_PATTERN = re.compile(r"^v[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$", re.I)


def _metric(name: str, value: float = 1.0) -> None:
    try:
        _cw.put_metric_data(
            Namespace=METRIC_NAMESPACE,
            MetricData=[
                {
                    "MetricName": name,
                    "Value": value,
                    "Unit": "Count",
                    "Dimensions": [
                        {"Name": "Domain", "Value": "energy"},
                        {"Name": "Environment", "Value": os.environ.get("G2_ENV", "dev")},
                    ],
                }
            ],
        )
    except ClientError:
        logger.exception("PutMetricData failed")


def _parse_version(version: str) -> tuple[int, ...]:
    s = version.strip()
    if s.lower().startswith("v"):
        s = s[1:]
    return tuple(int(p) for p in s.split(".") if p.isdigit())


def meets_firmware_g2_floor(version: str) -> bool:
    try:
        parts = _parse_version(version)
        major = parts[0] if parts else 0
        minor = parts[1] if len(parts) > 1 else 0
        return major > 2 or (major == 2 and minor >= 3)
    except (ValueError, IndexError):
        return False


def _parse_time_ms(payload: dict[str, Any]) -> int:
    raw = payload.get("timestamp")
    if not raw:
        return int(datetime.now(timezone.utc).timestamp() * 1000)
    if isinstance(raw, (int, float)):
        return int(raw)
    s = str(raw).replace(" UTC", "+00:00").replace(" ", "T")
    if s.endswith("Z"):
        s = s[:-1] + "+00:00"
    dt = datetime.fromisoformat(s)
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    return int(dt.timestamp() * 1000)


def _validate_envelope(payload: dict[str, Any]) -> str | None:
    required = (
        "schema_version",
        "sys_id",
        "component_id",
        "domain",
        "timestamp",
        "firmware_version",
        "status",
        "data_stale",
        "reporting_mode",
    )
    for key in required:
        if key not in payload or payload[key] in (None, ""):
            return f"missing:{key}"
    if payload.get("schema_version") != "energy.telemetry.v1":
        return "bad:schema_version"
    if payload.get("domain") != "energy":
        return "bad:domain"
    fw = str(payload.get("firmware_version", ""))
    if not FW_PATTERN.match(fw):
        return "bad:firmware_version_format"
    measures = payload.get("measures")
    if measures is None and "soc" not in payload:
        return "missing:measures_or_legacy_flat"
    return None


def _collect_measure_values(payload: dict[str, Any]) -> list[dict[str, str]]:
    out: list[dict[str, str]] = []
    measures = payload.get("measures") or {}

    def add(name: str, val: Any, typ: str = "DOUBLE") -> None:
        if val is None:
            return
        out.append({"Name": name, "Value": str(val), "Type": typ})

    bat = measures.get("battery") or {}
    add("battery_soc_pct", bat.get("soc_pct"))
    add("battery_voltage_v", bat.get("voltage_v"))
    add("battery_current_a", bat.get("current_a"))

    sol = measures.get("solar") or {}
    add("solar_power_w", sol.get("power_w"))
    add("solar_voltage_v", sol.get("voltage_v"))

    lod = measures.get("load") or {}
    add("load_power_w", lod.get("power_w"))

    yld = measures.get("yield") or {}
    add("yield_total_kwh", yld.get("total_kwh"))
    add("yield_today_kwh", yld.get("today_kwh"))
    add("days_running", yld.get("days_running"), "BIGINT")

    # Legacy flat fallbacks (already kWh on wire for v2 payloads)
    add("soc", payload.get("soc"))
    add("battery_voltage", payload.get("battery_voltage"))
    add("solar_power", payload.get("solar_power"))
    add("load_power", payload.get("load_power"))
    add("total_yield_kwh", payload.get("total_yield_kwh"))
    add("today_yield_kwh", payload.get("today_yield_kwh"))
    add("days_running", payload.get("days_running"), "BIGINT")

    return out


def _get_registry_track(sys_id: str) -> tuple[str | None, dict[str, Any] | None]:
    table = _ddb.Table(os.environ["REGISTRY_TABLE"])
    resp = table.get_item(Key={"sys_id": sys_id})
    item = resp.get("Item")
    if not item:
        return None, None
    return str(item.get("track", "")), item


def _write_timestream(
    sys_id: str, component_id: str, time_ms: int, measure_values: list[dict[str, str]]
) -> None:
    if not measure_values:
        measure_values = [{"Name": "heartbeat", "Value": "1", "Type": "BIGINT"}]
    record = {
        "Dimensions": [
            {"Name": "sys_id", "Value": sys_id},
            {"Name": "component_id", "Value": component_id},
        ],
        "MeasureName": "g2_energy",
        "MeasureValueType": "MULTI",
        "MeasureValues": measure_values,
        "Time": str(time_ms),
        "TimeUnit": "MILLISECONDS",
    }
    _ts_write.write_records(
        DatabaseName=os.environ["TIMESTREAM_DATABASE"],
        TableName=os.environ["TIMESTREAM_TABLE"],
        Records=[record],
    )


def _ddb_safe(obj: Any) -> Any:
    """Convert floats for DynamoDB resource (requires Decimal)."""
    if isinstance(obj, float):
        return Decimal(str(obj))
    if isinstance(obj, dict):
        return {k: _ddb_safe(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [_ddb_safe(v) for v in obj]
    return obj


def _update_shadow(sys_id: str, component_id: str, payload: dict[str, Any], time_ms: int) -> None:
    table = _ddb.Table(os.environ["SHADOW_TABLE"])
    table.put_item(
        Item=_ddb_safe(
            {
                "pk": f"SYS#{sys_id}",
                "sk": f"DOMAIN#energy#COMP#{component_id}",
                "sys_id": sys_id,
                "component_id": component_id,
                "domain": "energy",
                "snapshot": payload,
                "updated_at_ms": time_ms,
            }
        )
    )


def _touch_registry_firmware(sys_id: str, firmware_version: str) -> None:
    table = _ddb.Table(os.environ["REGISTRY_TABLE"])
    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    table.update_item(
        Key={"sys_id": sys_id},
        UpdateExpression="SET firmware_version = :fw, updated_at = :u",
        ExpressionAttributeValues={":fw": firmware_version, ":u": now},
    )


def lambda_handler(event: dict[str, Any], context: Any) -> dict[str, Any]:
    logger.info("event=%s", json.dumps(event, default=str)[:4000])

    if isinstance(event.get("payload"), str):
        try:
            payload = json.loads(event["payload"])
        except json.JSONDecodeError:
            _metric("IngestValidationError")
            return {"ok": False, "reason": "invalid_json"}
    else:
        payload = event

    err = _validate_envelope(payload)
    if err:
        logger.warning("validation_failed %s", err)
        _metric("IngestValidationError")
        return {"ok": False, "reason": err}

    sys_id = str(payload["sys_id"])
    component_id = str(payload["component_id"])
    firmware_version = str(payload["firmware_version"])

    track, _ = _get_registry_track(sys_id)
    if track is None:
        logger.warning("registry_miss sys_id=%s", sys_id)
        _metric("IngestValidationError")
        return {"ok": False, "reason": "registry_not_found"}

    if track != "g2":
        logger.info("skip_g2_write track=%s sys_id=%s", track, sys_id)
        _metric("IngestSkippedLegacyTrack")
        return {"ok": True, "skipped": "track_not_g2"}

    if not meets_firmware_g2_floor(firmware_version):
        logger.warning(
            "reject_fw_floor sys_id=%s fw=%s", sys_id, firmware_version
        )
        _metric("IngestValidationError")
        return {"ok": False, "reason": "firmware_below_2_3"}

    time_ms = _parse_time_ms(payload)
    measure_values = _collect_measure_values(payload)

    try:
        _write_timestream(sys_id, component_id, time_ms, measure_values)
        _update_shadow(sys_id, component_id, payload, time_ms)
        _touch_registry_firmware(sys_id, firmware_version)
    except ClientError:
        logger.exception("ingest_write_failed sys_id=%s", sys_id)
        _metric("TimestreamWriteError")
        raise

    _metric("IngestSuccess")
    logger.info(
        "ingest_ok sys_id=%s component_id=%s fw=%s measures=%d",
        sys_id,
        component_id,
        firmware_version,
        len(measure_values),
    )
    return {"ok": True, "sys_id": sys_id, "measures": len(measure_values)}
