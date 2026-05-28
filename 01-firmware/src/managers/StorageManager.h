#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "../core/SystemContext.h"

// ============================================================
// StorageManager — LittleFS + Certificate Loading
// NO auto-format. Mount failure => EMERGENCY state.
// ============================================================

class StorageManager {
public:
    // Returns false on fatal error (triggers EMERGENCY)
    bool begin();

    bool loadCertificates(WiFiClientSecure& netClient);

    // Generic NVS-style key-value persistence
    bool   writeString(const char* path, const String& data);
    String readString(const char* path);
    bool   fileExists(const char* path);

    // --- V2.1 Daily Ledger Persistence ---
    bool saveLedgerEntry(const DailyLedgerEntry& entry);
    // Returns number of entries loaded (max 30)
    int  loadLedger(DailyLedgerEntry* entries, int maxEntries);

    // --- V2.2.1 WiFi Provisioning ---
    struct WiFiConfig {
        String ssid;
        String password;
        bool   is_valid = false;
    };
    bool saveWiFiConfig(const String& ssid, const String& password);
    WiFiConfig loadWiFiConfig();

    // V2.2: Certificate Getters for OTA/TLS
    const String& getCA()   const { return _ca; }
    const String& getCert() const { return _cert; }
    const String& getKey()  const { return _key; }

private:
    bool _mounted = false;
    String _ca;
    String _cert;
    String _key;

    // Internal helper to enforce 30-day limit
    void _rotateLedger();
};
