# Architecture Decisions (ADR Index)

> **Purpose**: Mandatory index for all AI agents (007, 008, …) — conclusions only; see linked docs for detail.  
> **Maintenance**: One table row per decision; owning agent appends on major changes.

---

## Decision log

| ID | Date | Summary | Detail |
|----|------|---------|--------|
| **ADR-001** | 2026-05-29 | **Dual-track** — no Legacy AWS/firmware refactor; G2 new-track IaC + API | [`04-cloud/docs/008_Strategic_Guide.md`](../04-cloud/docs/008_Strategic_Guide.md) |
| **ADR-002** | 2026-05-29 | **Five domains** — `energy` · `network` · `vision` · `environment` · `control` | [`04-cloud/docs/G2_Domain_Map.md`](../04-cloud/docs/G2_Domain_Map.md) |
| **ADR-003** | 2026-05-29 | **Fleet PK `sys_id`** — neutral asset ID; no Watch/Box/Trailer in ID; `system_type` assignable at prod/deploy | [`02-backend/docs/G2_System_Model.md`](../02-backend/docs/G2_System_Model.md) §4 |
| **ADR-004** | 2026-05-29 | **`sys_id` format Y1** — `IQ-{YY}-{NNNNN}` (e.g. `IQ-26-00001`); issuance year immutable; serial resets yearly | This file §ADR-004 |
| **ADR-005** | 2026-05-29 | **Legacy `sys_id`** — keep `IQW-*` / `IQB-*` / `IQT-*`; new units from 2026 use `IQ-YY-NNNNN` only | This file §ADR-005 |
| **ADR-006** | 2026-05-29 | **`deployed` requires `system_type`**; `unassigned` allowed pre-deploy | System Model §5 |
| **ADR-007** | 2026-05-29 | **Cross-class re-profile** — `system_type` PATCH needs Admin + audit + component checks | System Model §5 |
| **ADR-008** | 2026-05-29 | **Registry `track` + firmware gate** — Timestream iff `track=g2` and fw ≥ v2.3.0; promotion **manual only** | [`04-cloud/docs/G2_Registry_Track_Assignment_SOP.md`](../04-cloud/docs/G2_Registry_Track_Assignment_SOP.md) |

---

## ADR-004 · sys_id format (approved · Y1)

```text
IQ-{YY}-{NNNNN}

e.g. IQ-26-00001  →  first unit issued in 2026
     IQ-27-00001  →  new pool each calendar year
```

| Rule | Value |
|------|--------|
| Storage | Uppercase `IQ`; normalize `iq-26-6001` → `IQ-26-06001` |
| `YY` | Issuance year (2 digits); **never changes** after assignment |
| `NNNNN` | 5-digit zero-padded serial; atomic increment per year |
| Product shape | **`system_type` not in sys_id** — Registry only |
| dev/prod | **Shared** issuance pool; env isolation via MQTT/Timestream |

**Schema regex**: `^IQ-[0-9]{2}-[0-9]{5}$` (new) · legacy `^IQ[WBT]-…`

**Note**: Historical `IQW-9041` labels in `01-firmware/report/` are site names, not the new format.

---

## ADR-005 · Legacy coexistence

Grandfathered IDs remain valid in Registry and Schema. New provisioning uses **ADR-004** only.

---

## ADR-008 · Registry track & firmware gate (approved)

| Rule | Value |
|------|--------|
| G2 firmware floor | **`v2.3.0`**+ (`firmware_version` on device payload) |
| Timestream write | `track=g2` **and** fw ≥ 2.3 |
| New production Registry | default **`track=g2`** |
| Legacy import (~70) | initial **`track=legacy`** |
| Upgrade to g2 | **`track` change manual only** (no ingest auto-promote) |
| Downgrade | **`track=g2` never reverts** to legacy |
| `batch_id` / `HQ2513*` | **not** used for `track` |

Full SOP → [`04-cloud/docs/G2_Registry_Track_Assignment_SOP.md`](../04-cloud/docs/G2_Registry_Track_Assignment_SOP.md).

---

## Agent quick links

| Role | Read |
|------|------|
| All | This file + [`AGENTS.md`](../AGENTS.md) |
| 008 cloud/API | [`04-cloud/docs/008_Strategic_Guide.md`](../04-cloud/docs/008_Strategic_Guide.md) |
| 007 firmware | [`09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md`](../09-contract/schemas/energy/FIRMWARE_ALIGNMENT_007.md) |
| Commercial | [`00-strategy/docs/IQCLOUD_COMMERCIAL_STRATEGY.md`](../00-strategy/docs/IQCLOUD_COMMERCIAL_STRATEGY.md) |

---

*Keep new rows to one-line summaries; link out for long-form specs.*
