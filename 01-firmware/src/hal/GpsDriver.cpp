#include "GpsDriver.h"
#include "../config/Config.h"
#include <sys/time.h>

#define GPS_RX_PIN 33
#define GPS_TX_PIN 32
#define GPS_BAUD   9600

GpsDriver::GpsDriver() : _serial(1) {}

void GpsDriver::begin() {
    _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] UART1 init on RX:33, TX:32");
}

bool GpsDriver::loop() {
    bool newData = false;
    while (_serial.available() > 0) {
        char c = _serial.read();
        if (_gps.encode(c)) {
            newData = true;
        }
    }
    return newData;
}

void GpsDriver::populateSnapshot(GpsSnapshot& snap) {
    if (_gps.location.isValid()) {
        snap.isValid   = true;
        snap.latitude  = _gps.location.lat();
        snap.longitude = _gps.location.lng();
    } else {
        snap.isValid   = false;
    }
    if (_gps.satellites.isValid()) snap.satellites = _gps.satellites.value();
    if (_gps.hdop.isValid())       snap.hdop       = _gps.hdop.hdop();
    if (_gps.altitude.isValid())   snap.altitude   = _gps.altitude.meters();

    if (_gps.date.isValid() && _gps.time.isValid() && _gps.date.year() > 2000) {
        struct tm tm;
        tm.tm_year = _gps.date.year() - 1900;
        tm.tm_mon  = _gps.date.month() - 1;
        tm.tm_mday = _gps.date.day();
        tm.tm_hour = _gps.time.hour();
        tm.tm_min  = _gps.time.minute();
        tm.tm_sec  = _gps.time.second();
        tm.tm_isdst = 0;
        snap.timestamp_utc = (uint32_t)mktime(&tm);
    }
}

bool GpsDriver::syncSystemTime() {
    if (_isTimeSynced) return true;
    if (_gps.date.isValid() && _gps.time.isValid() && _gps.date.year() > 2024) {
        struct tm tm;
        tm.tm_year = _gps.date.year() - 1900;
        tm.tm_mon  = _gps.date.month() - 1;
        tm.tm_mday = _gps.date.day();
        tm.tm_hour = _gps.time.hour();
        tm.tm_min  = _gps.time.minute();
        tm.tm_sec  = _gps.time.second();
        tm.tm_isdst = 0;
        time_t t = mktime(&tm);
        time_t now = time(nullptr);
        if (now < 1700000000) {
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            _isTimeSynced = true;
            Serial.printf("[GPS] Time Synced: %ld\n", (long)t);
            return true;
        }
    }
    return false;
}
