#pragma once

#include <Arduino.h>
#include "../hal/VeDirectDriver.h"
#include "../core/SystemContext.h"
#include "StorageManager.h"

// ============================================================
// EnergyManager — VE.Direct orchestration + SoC/SoH policy
// Runs exclusively on Core 0 (Task_Energy)
// ============================================================

class EnergyManager {
public:
    void begin(HardwareSerial& serial, StorageManager& storage);

    // Called every cycle by Task_Energy
    void poll();

    // Evaluate system power state and update SystemContext
    void evaluatePowerPolicy();

private:
    VeDirectDriver _ved;
    StorageManager* _storage = nullptr;
    unsigned long _lastDiagPrint = 0;
    int _nightCounter = 0;           // Hysteresis for night detection
    bool _prevMpptConnected = false;  // Edge detection for disconnect event

    void _updateSystemContext();
    void _printDiagSummary();
};
