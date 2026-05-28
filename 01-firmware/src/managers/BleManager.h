#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include "../core/SystemContext.h"

// ============================================================
// BleManager — BLE UART-style telemetry for V2.1 PoC
// Non-blocking loop, optional command RX.
// ============================================================

class BleManager {
public:
    void begin();
    void loop();

private:
    BLEServer*         _server = nullptr;
    BLEService*        _service = nullptr;
    BLECharacteristic* _txChar = nullptr;
    BLECharacteristic* _rxChar = nullptr;

    bool _initialized = false;
    bool _connected = false;
    bool _forceNotify = false;
    unsigned long _notifyIntervalMs = 1000;
    unsigned long _lastNotifyMs = 0;

    void _publishSnapshot();
    void _sendText(const char* txt);
    void _handleCommand(const String& cmd);
    unsigned long _clampInterval(unsigned long ms) const;

    class _ServerCallbacks;
    class _RxCallbacks;
};
