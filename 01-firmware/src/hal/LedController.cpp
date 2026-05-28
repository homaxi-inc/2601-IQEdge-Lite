#include "LedController.h"

void LedController::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _ledOn = false;
}

void LedController::setPattern(LedPattern pattern) {
    if (_pattern == pattern) return;
    _pattern  = pattern;
    _seqStep  = 0;
    _seqStart = millis();
    _setHW(pattern == LedPattern::SOLID);
}

void LedController::tick() {
    unsigned long now = millis();
    switch (_pattern) {
        case LedPattern::OFF:        _setHW(false); break;
        case LedPattern::SOLID:      _setHW(true);  break;
        case LedPattern::FAST_BLINK:   _tickFastBlink(now);   break;
        case LedPattern::SLOW_BLINK:   _tickSlowBlink(now);   break;
        case LedPattern::DOUBLE_BLINK: _tickDoubleBlink(now); break;
        case LedPattern::TRIPLE_BLINK: _tickTripleBlink(now); break;
        case LedPattern::SOS:          _tickSOS(now);         break;
    }
}

void LedController::_setHW(bool on) {
    if (_ledOn != on) {
        _ledOn = on;
        digitalWrite(_pin, on ? HIGH : LOW);
    }
}

void LedController::_tickFastBlink(unsigned long now) {
    unsigned long elapsed = (now - _seqStart) % 200;
    _setHW(elapsed < 100);
}

void LedController::_tickSlowBlink(unsigned long now) {
    unsigned long elapsed = (now - _seqStart) % 2000;
    _setHW(elapsed < 1000);
}

void LedController::_tickDoubleBlink(unsigned long now) {
    // on 200, off 200, on 200, off 1000 = 1600ms cycle
    unsigned long elapsed = (now - _seqStart) % 1600;
    if      (elapsed < 200)  _setHW(true);
    else if (elapsed < 400)  _setHW(false);
    else if (elapsed < 600)  _setHW(true);
    else                     _setHW(false);
}

void LedController::_tickTripleBlink(unsigned long now) {
    // on 200, off 200, on 200, off 200, on 200, off 1000 = 2000ms cycle
    unsigned long elapsed = (now - _seqStart) % 2000;
    if      (elapsed < 200)  _setHW(true);
    else if (elapsed < 400)  _setHW(false);
    else if (elapsed < 600)  _setHW(true);
    else if (elapsed < 800)  _setHW(false);
    else if (elapsed < 1000) _setHW(true);
    else                     _setHW(false);
}

void LedController::_tickSOS(unsigned long now) {
    // S: 3 short, O: 3 long, S: 3 short, long pause
    static const uint16_t pattern[] = {
        100, 100, 100, 100, 100, 300,   // S: dot dot dot
        300, 100, 300, 100, 300, 300,   // O: dash dash dash
        100, 100, 100, 100, 100, 1000   // S: dot dot dot + pause
    };
    static const int len = sizeof(pattern) / sizeof(pattern[0]);

    unsigned long total = 0;
    for (int i = 0; i < len; i++) total += pattern[i];
    
    if (total == 0) return;

    unsigned long elapsed = (now - _seqStart) % total;
    unsigned long acc = 0;
    for (int i = 0; i < len; i++) {
        acc += pattern[i];
        if (elapsed < acc) {
            _setHW(i % 2 == 0); // even index = ON
            return;
        }
    }
}
