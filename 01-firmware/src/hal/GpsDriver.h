#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "../core/SystemContext.h"

class GpsDriver {
public:
    GpsDriver();
    void begin();
    bool loop();
    void populateSnapshot(GpsSnapshot& snap);
    bool syncSystemTime();
private:
    HardwareSerial _serial;
    TinyGPSPlus _gps;
    bool _isTimeSynced = false;
};
