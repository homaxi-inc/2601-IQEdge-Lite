# IQEdge (ESP32) Firmware - AI Developer Guidelines

## 1. Context & Architecture Rules
This file contains critical context and historical bug records for the IQEdge (ESP32) firmware (`src/` directory). All future AI assistants MUST review this file before making any code modifications.

## 2. Critical Bug Records & Known Issues

### 2.1 The VE.DIRECT 10x Yield Bug (Identified 2026-03-18)
*   **Severity:** CRITICAL
*   **Component:** `src/managers/CommManager.cpp` (`_buildPayload` function)
*   **Issue:** The firmware incorrectly scales the `today_yield_kwh` and `total_yield_kwh` values before sending them to AWS IoT.
*   **Root Cause:** The VE.DIRECT driver (`src/hal/VeDirectDriver.cpp`) correctly parses `H19` and `H20` (which are in 0.01 kWh units from Victron) into Watt-hours (Wh) by multiplying by 10 (e.g., `_hist.today_yield_wh = v * 10;`). However, in `CommManager.cpp`, when converting these Wh values back to kWh for the JSON payload, the code divides by `100.0` instead of `1000.0`.
*   **Impact:** This results in a **10x inflation** of all reported yield metrics on the cloud Dashboard (e.g., a real yield of 0.2 kWh is reported as 2.0 kWh). This severely broke the energy reconciliation logic and masked severe solar generation issues on deployed systems (e.g., IQW-9006 in Los Angeles).
*   **Action Required:** Any future updates to the payload building logic MUST ensure that Wh to kWh conversions divide by `1000.0`.

### 2.2 The Missing Load Payload Bug (Identified 2026-03-18)
*   **Severity:** HIGH
*   **Component:** `src/managers/CommManager.cpp` (`_buildPayload` function)
*   **Issue:** The ESP32 parses `IL` (Load Current) and `LOAD` (Load Status) from the MPPT and calculates `load_power`, but these metrics were completely omitted from the JSON payload sent to AWS.
*   **Root Cause:** The `load` JSON object was missing in the `_buildPayload` construction.
*   **Impact:** AWS Timestream recorded `0.0` for `load_power` due to cloud-side default fallbacks when the field was missing.
*   **Status:** Fixed by AI on 2026-03-18 (added `load` object to `_buildPayload`).

## 3. Hardware Topology Awareness
*   **Load Measurement Limitation:** In the standard IQWatch topology, large loads (like PoE switches and cameras) are connected directly to the battery, bypassing the MPPT's LOAD terminals.
*   **Consequence:** The `load_power` and `load_current` metrics parsed from VE.DIRECT will ONLY reflect the power consumed by small devices connected directly to the MPPT LOAD terminals (like the ESP32 itself). It does NOT represent total system power consumption.
*   **System Power Estimation:** To estimate total system power, use the "Nighttime SOC Decay" method (calculating energy lost over time when solar power is 0W) based on the known battery capacity (e.g., 80Ah / 1024Wh).

## 4. Development Redlines
*   **Do Not Use Auto-Format for LittleFS:** Ensure `LittleFS.begin(false)` is used. Auto-formatting will destroy provisioned AWS certificates on production devices.
*   **Core Isolation:** Adhere to the dual-core design. Core 0 handles Energy (VE.DIRECT parsing), and Core 1 handles Communications (WiFi/MQTT/TLS). Do not mix these concerns.
