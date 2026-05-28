#pragma once

#include <Arduino.h>

// ============================================================
// LedController — Non-Blocking LED State Machine
// ============================================================

enum class LedPattern : uint8_t {
    OFF,
    SOLID,
    SLOW_BLINK,     // 1s on / 1s off — system anomaly
    FAST_BLINK,     // 100ms / 100ms — WiFi disconnected
    DOUBLE_BLINK,   // 2x flash + pause — MQTT disconnected
    TRIPLE_BLINK,   // 3x flash + pause — MPPT disconnected
    SOS             // Emergency mode
};

class LedController {
public:
    void begin(uint8_t pin);
    void setPattern(LedPattern pattern);
    LedPattern getPattern() const { return _pattern; }

    // Must be called at high frequency from the deterrence task
    void tick();

private:
    uint8_t    _pin           = 0;
    LedPattern _pattern       = LedPattern::OFF;
    bool       _ledOn         = false;
    uint8_t    _seqStep       = 0;
    unsigned long _seqStart   = 0;

    void _setHW(bool on);

    // Pattern timing tables (on_ms, off_ms pairs per step)
    void _tickFastBlink(unsigned long now);
    void _tickSlowBlink(unsigned long now);
    void _tickDoubleBlink(unsigned long now);
    void _tickTripleBlink(unsigned long now);
    void _tickSOS(unsigned long now);
};
