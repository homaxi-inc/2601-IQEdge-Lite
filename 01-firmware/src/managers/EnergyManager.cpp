#include "EnergyManager.h"
#include "../config/Config.h"

void EnergyManager::begin(HardwareSerial& serial, StorageManager& storage) {
    _storage = &storage;
    _ved.begin(serial, VEDIRECT_RX_PIN, VEDIRECT_TX_PIN, VEDIRECT_BAUD);
    Serial.printf("[NRG] VE.Direct UART RX=%d TX=%d @ %d baud\n",
                  VEDIRECT_RX_PIN, VEDIRECT_TX_PIN, VEDIRECT_BAUD);
    Serial.println("[NRG] EnergyManager initialized");
}

void EnergyManager::poll() {
    bool newData = _ved.poll();

    bool connected = !_ved.isStale(VEDIRECT_TIMEOUT_MS);
    auto& ctx = SystemContext::instance();
    ctx.setMpptConnected(connected);

    // Edge detection: connected → disconnected transition
    if (_prevMpptConnected && !connected) {
        unsigned long disconnectTime = _ved.lastUpdateMs() + VEDIRECT_TIMEOUT_MS;
        ctx.setMpptDisconnectTime(disconnectTime);
        Serial.printf("[NRG] MPPT DISCONNECTED — last valid frame at T+%lu ms\n",
                      _ved.lastUpdateMs());
    }
    // Edge detection: disconnected → connected recovery
    if (!_prevMpptConnected && connected) {
        ctx.setMpptDisconnectTime(0);
        Serial.println("[NRG] MPPT RECONNECTED");
    }
    _prevMpptConnected = connected;

    if (newData) {
        _updateSystemContext();
    }

    if (DEBUG_MODE && (millis() - _lastDiagPrint > 30000)) {
        if (!connected) {
            Serial.println("[NRG] WARN: MPPT not responding on VE.Direct (check cable/power/pins)");
        }
        _printDiagSummary();
        _lastDiagPrint = millis();
    }
}

void EnergyManager::evaluatePowerPolicy() {
    auto& ctx = SystemContext::instance();
    if (ctx.getState() == SystemState::EMERGENCY) return;

    EnergySnapshot snap = ctx.getEnergySnapshot();
    float soc = snap.battery_soc;
    float vpv = snap.solar_voltage;
    float ppv = snap.solar_power;
    float vbat = snap.battery_voltage;

    // No valid MPPT telemetry yet — do not infer HIBERNATE/NIGHT from zeroed snapshot
    if (!ctx.isMpptConnected() || vbat < 1.0f) {
        _nightCounter = 0;
        ctx.setState(SystemState::NORMAL);
        return;
    }

    // --- Adaptive Reporting Mode Algorithm (Prudent Approach) ---
    // Exit Night Mode / Prevent entering if power is detected or voltage is high
    if (ppv > 1.0f || vpv > (vbat + 1.0f)) {
        _nightCounter = 0;
        if (ppv > PEAK_W_THRESHOLD) {
            if (ctx.getReportingMode() != ReportingMode::PEAK) {
                Serial.println("[NRG] PEAK POWER detected (>100W) -> Switching to High-Freq reporting (1 min)");
                ctx.setReportingMode(ReportingMode::PEAK);
            }
        } else {
            if (ctx.getReportingMode() != ReportingMode::NORMAL) {
                Serial.println("[NRG] Normal daylight conditions -> Restoring 5-min reporting");
                ctx.setReportingMode(ReportingMode::NORMAL);
            }
        }
    } 
    // Enter Night Mode: low V/PV for ~15 min (counter ticks every MPPT_CHECK_INTERVAL_MS)
    else if (vpv < NIGHT_V_THRESHOLD && ppv < 0.1f) {
        unsigned long now = millis();
        if (now - _lastNightTickMs >= MPPT_CHECK_INTERVAL_MS) {
            _lastNightTickMs = now;
            if (_nightCounter < NIGHT_CONFIRM_COUNT) {
                _nightCounter++;
                if (_nightCounter == NIGHT_CONFIRM_COUNT) {
                    Serial.println("[NRG] NIGHT confirmed (15m dark) -> Switching to Conservative reporting (30 min)");
                    ctx.setReportingMode(ReportingMode::NIGHT);
                }
            }
        }
    } else {
        // Twilight/Dusk: Keep current mode but reset counter if conditions improve slightly
        _nightCounter = 0;
    }

    // --- System Battery State Management ---
    if (soc > 60.0f) {
        ctx.setState(SystemState::NORMAL);
    } else if (soc > 30.0f) {
        ctx.setState(SystemState::NORMAL);  // Balanced — still NORMAL but could throttle
    } else if (soc > 15.0f) {
        ctx.setState(SystemState::CONSERVE);
    } else {
        ctx.setState(SystemState::HIBERNATE);
    }
}

void EnergyManager::_updateSystemContext() {
    auto& ctx  = SystemContext::instance();
    auto  snap = _ved.getSnapshot();
    auto  diag = _ved.getDiag();
    auto  hist = _ved.getHistory();

    ctx.setEnergySnapshot(snap);
    ctx.setSystemDiag(diag);
    ctx.setHistoricalData(hist);

    if (!_serialPublishTriggered && diag.mppt_serial.length() >= 3) {
        _serialPublishTriggered = true;
        ctx.requestUrgentPublish();
        Serial.printf("[NRG] MPPT serial ready (%s) -> urgent cloud publish\n",
                      diag.mppt_serial.c_str());
    }

    // --- V2.1 Day Rollover Ledger Persistence ---
    // If Day Sequence (HSDS) increases, MPPT has just finished a day.
    int currentHsds = hist.day_sequence;
    int lastHsds    = ctx.getLastRecordedHsds();

    if (lastHsds != -1 && currentHsds > lastHsds) {
        Serial.printf("[NRG] Day Rollover detected (%d -> %d)! Archiving yesterday's data.\n", 
                      lastHsds, currentHsds);
        
        DailyLedgerEntry entry;
        entry.hsds           = (uint16_t)lastHsds;
        entry.total_yield_wh = (uint32_t)hist.total_yield_wh;
        entry.day_yield_wh   = (uint32_t)hist.yesterday_yield_wh; // H22
        entry.max_power_w     = (uint16_t)hist.yesterday_max_power_w; // H23
        entry.timestamp_utc  = (uint32_t)time(nullptr);

        if (_storage) {
            _storage->saveLedgerEntry(entry);
        }
        ctx.setLastRecordedHsds(currentHsds);
    }
    
    if (lastHsds == -1 && currentHsds > 0) {
        ctx.setLastRecordedHsds(currentHsds);
    }
}

void EnergyManager::_printDiagSummary() {
    auto snap = _ved.getSnapshot();
    auto hist = _ved.getHistory();

    Serial.println("[NRG] === Data Summary ===");
    Serial.printf("  Battery: %.2fV, %.2fA, %.1f%% (%s) [%s]\n",
                  snap.battery_voltage, snap.battery_current, snap.battery_soc,
                  snap.charge_state.c_str(),
                  snap.soc_from_mppt ? "MPPT" : "V-est");
    Serial.printf("  Solar:   %.2fV, %.2fW, %.2fA\n",
                  snap.solar_voltage, snap.solar_power, snap.solar_current);
    Serial.printf("  Load:    %.2fA, %.2fW, %s\n",
                  snap.load_current, snap.load_power, snap.load_status.c_str());
    Serial.printf("  History: Total=%.1fkWh, Today=%.1fkWh, Days=%d\n",
                  hist.total_yield_wh / 1000.0f,
                  hist.today_yield_wh / 1000.0f,
                  hist.day_sequence);
    uint32_t errs = _ved.parseErrorCount();
    if (errs > 0)
        Serial.printf("  ParseErrors: %lu (corrupt/out-of-range frames rejected)\n", errs);
}
