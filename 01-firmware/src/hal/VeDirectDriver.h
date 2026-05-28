#pragma once

#include <Arduino.h>
#include <VeDirectFrameHandler.h>
#include "../core/SystemContext.h"

// ============================================================
// VeDirectDriver — Victron VE.Direct Protocol Parser
// Runs on Core 0, feeds EnergySnapshot + HistoricalData
// ============================================================

class VeDirectDriver {
public:
    void begin(HardwareSerial& serial, int rxPin, int txPin, int baud);

    // Call from energy task loop — reads serial, parses frames
    bool poll();

    // Parsed outputs
    EnergySnapshot getSnapshot() const { return _snap; }
    SystemDiag     getDiag()     const { return _diag; }
    HistoricalData getHistory()  const { return _hist; }

    unsigned long lastUpdateMs() const { return _lastUpdate; }
    bool          isStale(unsigned long timeoutMs) const;

    uint32_t parseErrorCount() const { return _parseErrors; }

private:
    VeDirectFrameHandler _handler;
    HardwareSerial*      _serial = nullptr;
    unsigned long        _lastUpdate = 0;
    uint32_t             _parseErrors = 0;

    EnergySnapshot _snap;
    SystemDiag     _diag;
    HistoricalData _hist;

    void _parseFrame();
    float _voltageToSoC(float voltage) const;
    String _cleanString(const char* input) const;
    String _chargeStateToString(int code) const;

    static bool   _isValidNumeric(const char* s);
    static float  _safeAtof(const char* s, float fallback = 0.0f);
    static long   _safeAtol(const char* s, long fallback = 0);
    static int    _safeAtoi(const char* s, int fallback = 0);
    static float  _clamp(float val, float lo, float hi);
    static long   _clampL(long val, long lo, long hi);
};
