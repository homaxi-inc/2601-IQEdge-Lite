#include "VeDirectDriver.h"
#include "../config/Config.h"
#include <ctype.h>
#include <errno.h>

// ============================================================
// Physical range constants for sanity-check clamping.
// Values outside these ranges are rejected as corrupt data.
// ============================================================
static constexpr float VBAT_MIN     =  0.0f;    // V (battery terminal)
static constexpr float VBAT_MAX     = 60.0f;    // V (covers up to 16S LiFePO4)
static constexpr float IBAT_MIN     = -50.0f;   // A (discharge)
static constexpr float IBAT_MAX     =  50.0f;   // A (charge)
static constexpr float VPV_MIN      = -10.0f;   // V (allow negative drift at night)
static constexpr float VPV_MAX      = 250.0f;   // V (max open-circuit for large panels)
static constexpr float PPV_MIN      = -50.0f;   // W (allow negative drift at night)
static constexpr float PPV_MAX      = 5000.0f;  // W (covers SmartSolar 250/100)
static constexpr float IPV_MIN      =  0.0f;    // A
static constexpr float IPV_MAX      = 100.0f;   // A
static constexpr float IL_MIN       =  0.0f;    // A (load current)
static constexpr float IL_MAX       = 50.0f;    // A
static constexpr float SOC_MIN_VAL  =  0.0f;    // %
static constexpr float SOC_MAX_VAL  = 100.0f;   // %
static constexpr long  H_YIELD_MIN  =  0;       // 0.01 kWh
static constexpr long  H_YIELD_MAX  = 999999;   // ~10,000 kWh lifetime
static constexpr long  H_POWER_MAX  = 10000;    // W
static constexpr int   HSDS_MAX     = 36500;    // ~100 years of solar days

// ============================================================
// Safe parsing helpers
// ============================================================

bool VeDirectDriver::_isValidNumeric(const char* s) {
    if (!s || *s == '\0') return false;
    const char* p = s;
    if (*p == '-' || *p == '+') p++;
    if (*p == '\0') return false;
    bool hasDot = false;
    bool hasDigit = false;
    while (*p) {
        if (isdigit((unsigned char)*p)) {
            hasDigit = true;
        } else if (*p == '.' && !hasDot) {
            hasDot = true;
        } else {
            return false;
        }
        p++;
    }
    return hasDigit;
}

float VeDirectDriver::_safeAtof(const char* s, float fallback) {
    if (!_isValidNumeric(s)) return fallback;
    return atof(s);
}

long VeDirectDriver::_safeAtol(const char* s, long fallback) {
    if (!_isValidNumeric(s)) return fallback;
    return atol(s);
}

int VeDirectDriver::_safeAtoi(const char* s, int fallback) {
    if (!_isValidNumeric(s)) return fallback;
    return atoi(s);
}

float VeDirectDriver::_clamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

long VeDirectDriver::_clampL(long val, long lo, long hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

// ============================================================
// Core driver
// ============================================================

void VeDirectDriver::begin(HardwareSerial& serial, int rxPin, int txPin, int baud) {
    _serial = &serial;
    _serial->setRxBufferSize(1024);
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
    Serial.println("[VED] VE.Direct driver initialized");
}

bool VeDirectDriver::poll() {
    if (!_serial) return false;

    bool hasNewData = false;
    // Bound per-cycle work to keep task responsive and avoid pathological loops.
    int budget = 256;
    while (budget-- > 0 && _serial->available()) {
        int b = _serial->read();
        if (b < 0) break;
        _handler.rxData(static_cast<uint8_t>(b));
        hasNewData = true;
    }

    if (!hasNewData) {
        _handler.veEnd = 0;
        return false;
    }

    // Defensive guard: if parser metadata is corrupted, drop this cycle safely.
    if (_handler.veEnd < 0 || _handler.veEnd > buffLen) {
        _parseErrors++;
        _handler.veEnd = 0;
        return false;
    }

    if (_handler.veEnd > 0) {
        _parseFrame();
        _lastUpdate = millis();
        return true;
    }
    return false;
}

bool VeDirectDriver::isStale(unsigned long timeoutMs) const {
    if (_lastUpdate == 0) return true;
    return (millis() - _lastUpdate) > timeoutMs;
}

void VeDirectDriver::_parseFrame() {
    _snap.soc_from_mppt = false;
    _snap.solar_current = 0.0f;

    int safeEnd = _handler.veEnd;
    if (safeEnd < 0) {
        _parseErrors++;
        return;
    }
    if (safeEnd > buffLen) {
        _parseErrors++;
        safeEnd = buffLen;
    }

    for (int i = 0; i < safeEnd; i++) {
        const char* name = _handler.veName[i];
        const char* val  = _handler.veValue[i];

        if (!name || !val || val[0] == '\0') {
            _parseErrors++;
            continue;
        }

        if (strcmp(name, "V") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            float v = raw / 1000.0f;
            if (v < VBAT_MIN || v > VBAT_MAX) { _parseErrors++; continue; }
            _snap.battery_voltage = v;
        }
        else if (strcmp(name, "I") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            float a = raw / 1000.0f;
            if (a < IBAT_MIN || a > IBAT_MAX) { _parseErrors++; continue; }
            _snap.battery_current = a;
        }
        else if (strcmp(name, "SOC") == 0) {
            int raw = _safeAtoi(val, -1);
            if (raw < 0) { _parseErrors++; continue; }
            float soc = raw / 10.0f;
            if (soc < SOC_MIN_VAL || soc > SOC_MAX_VAL) { _parseErrors++; continue; }
            _snap.battery_soc   = soc;
            _snap.soc_from_mppt = true;
        }
        else if (strcmp(name, "CS") == 0) {
            int cs = _safeAtoi(val, -1);
            if (cs < 0 || cs > 255) { _parseErrors++; continue; }
            _snap.charge_state_code = cs;
            _snap.charge_state      = _chargeStateToString(cs);
        }
        else if (strcmp(name, "VPV") == 0 || strcmp(name, "VPV1") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            float v = raw / 1000.0f;
            if (v < VPV_MIN || v > VPV_MAX) { _parseErrors++; continue; }
            _snap.solar_voltage = v;
        }
        else if (strcmp(name, "PPV") == 0 || strcmp(name, "PPV1") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            if (raw < PPV_MIN || raw > PPV_MAX) { _parseErrors++; continue; }
            _snap.solar_power = raw;
        }
        else if (strcmp(name, "IPV") == 0 || strcmp(name, "IPV1") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            float a = raw / 1000.0f;
            if (a < IPV_MIN || a > IPV_MAX) { _parseErrors++; continue; }
            _snap.solar_current = a;
        }
        else if (strcmp(name, "LOAD") == 0) {
            _snap.load_status = _cleanString(val);
        }
        else if (strcmp(name, "IL") == 0) {
            float raw = _safeAtof(val, NAN);
            if (isnan(raw)) { _parseErrors++; continue; }
            float a = raw / 1000.0f;
            if (a < IL_MIN || a > IL_MAX) { _parseErrors++; continue; }
            _snap.load_current = a;
        }
        else if (strcmp(name, "PID")  == 0) _diag.pid           = _cleanString(val);
        else if (strcmp(name, "FW")   == 0) _diag.mppt_firmware = _cleanString(val);
        else if (strcmp(name, "SER#") == 0) _diag.mppt_serial   = _cleanString(val);
        else if (strcmp(name, "OR")   == 0) {
            if (!_isValidNumeric(val) && val[0] != '0') {
                bool hexOk = true;
                for (const char* p = val; *p; p++) {
                    if (!isxdigit((unsigned char)*p)) { hexOk = false; break; }
                }
                if (!hexOk) { _parseErrors++; continue; }
            }
            _diag.off_reason = strtoul(val, NULL, 16);
        }
        else if (strcmp(name, "MPPT") == 0) {
            int m = _safeAtoi(val, -1);
            if (m < 0 || m > 255) { _parseErrors++; continue; }
            _diag.mppt_mode = m;
        }
        else if (strcmp(name, "ERR") == 0) {
            int e = _safeAtoi(val, -999);
            if (e == -999) { _parseErrors++; continue; }
            _diag.error_code = e;
        }
        else if (strcmp(name, "H19") == 0) {
            long v = _safeAtol(val, -1);
            if (v < H_YIELD_MIN || v > H_YIELD_MAX) { _parseErrors++; continue; }
            _hist.total_yield_wh = v * 10;
        }
        else if (strcmp(name, "H20") == 0) {
            long v = _safeAtol(val, -1);
            if (v < H_YIELD_MIN || v > H_YIELD_MAX) { _parseErrors++; continue; }
            _hist.today_yield_wh = v * 10;
        }
        else if (strcmp(name, "H21") == 0) {
            long v = _safeAtol(val, -1);
            if (v < 0 || v > H_POWER_MAX) { _parseErrors++; continue; }
            _hist.max_power_w = v;
        }
        else if (strcmp(name, "H22") == 0) {
            long v = _safeAtol(val, -1);
            if (v < H_YIELD_MIN || v > H_YIELD_MAX) { _parseErrors++; continue; }
            _hist.yesterday_yield_wh = v * 10;
        }
        else if (strcmp(name, "H23") == 0) {
            long v = _safeAtol(val, -1);
            if (v < 0 || v > H_POWER_MAX) { _parseErrors++; continue; }
            _hist.yesterday_max_power_w = v;
        }
        else if (strcmp(name, "HSDS") == 0) {
            int d = _safeAtoi(val, -1);
            if (d < 0 || d > HSDS_MAX) { _parseErrors++; continue; }
            _hist.day_sequence = d;
        }
    }

    // Derived calculations
    _snap.load_power = _snap.load_current * _snap.battery_voltage;

    if (_snap.solar_current == 0.0f && _snap.solar_voltage > 0.1f)
        _snap.solar_current = _snap.solar_power / _snap.solar_voltage;

    if (!_snap.soc_from_mppt)
        _snap.battery_soc = _voltageToSoC(_snap.battery_voltage);

    _snap.timestamp = millis();
}

float VeDirectDriver::_voltageToSoC(float voltage) const {
    if (voltage >= SOC_VOLTAGES[0]) return SOC_PERCENTS[0];
    if (voltage <= SOC_VOLTAGES[SOC_TABLE_LEN - 1]) return SOC_PERCENTS[SOC_TABLE_LEN - 1];
    for (int i = 0; i < SOC_TABLE_LEN - 1; ++i) {
        if (voltage <= SOC_VOLTAGES[i] && voltage > SOC_VOLTAGES[i + 1]) {
            float v1 = SOC_VOLTAGES[i], v2 = SOC_VOLTAGES[i + 1];
            float s1 = SOC_PERCENTS[i], s2 = SOC_PERCENTS[i + 1];
            return s1 + (voltage - v1) * (s2 - s1) / (v2 - v1);
        }
    }
    return 0.0f;
}

String VeDirectDriver::_cleanString(const char* input) const {
    String result;
    if (!input) return result;
    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];
        if (c >= 32 && c <= 126) result += c;
    }
    return result;
}

String VeDirectDriver::_chargeStateToString(int code) const {
    switch (code) {
        case 0: return "Off";
        case 2: return "Standby";
        case 3: return "Bulk";
        case 4: return "Absorption";
        case 5: return "Float";
        case 7: return "Equalize";
        default: return "Unknown";
    }
}
