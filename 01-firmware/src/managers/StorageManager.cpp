#include <ArduinoJson.h>
#include "StorageManager.h"
#include "LittleFS.h"
#include "../config/Config.h"

static void _normalizePem(String& s) {
    // Remove UTF-8 BOM if present.
    if (s.length() >= 3 &&
        static_cast<uint8_t>(s[0]) == 0xEF &&
        static_cast<uint8_t>(s[1]) == 0xBB &&
        static_cast<uint8_t>(s[2]) == 0xBF) {
        s.remove(0, 3);
    }
    // Normalize CRLF to LF.
    s.replace("\r\n", "\n");
    s.replace("\r", "\n");
    // Drop embedded NULs and non-printable control chars.
    String cleaned;
    cleaned.reserve(s.length());
    for (size_t i = 0; i < s.length(); ++i) {
        const char c = s[i];
        if (c == '\0') continue;
        if (c == '\n' || c == '\t' || (c >= 32 && c <= 126)) {
            cleaned += c;
        }
    }
    s = cleaned;
}

static bool _extractPemBlock(String& s, const char* beginTag, const char* endTag) {
    if (!beginTag || !endTag) return false;
    int beginPos = s.indexOf(beginTag);
    if (beginPos < 0) return false;
    int endPos = s.indexOf(endTag, beginPos);
    if (endPos < 0) return false;
    
    endPos += static_cast<int>(strlen(endTag));
    s = s.substring(beginPos, endPos);
    
    s.trim();
    s += "\n"; // Crucial trailing newline for mbedTLS compatibility
    return true;
}

static bool _containsPemMarkers(const String& s, const char* beginTag, const char* endTag) {
    if (!beginTag || !endTag) return false;
    const char* p = s.c_str();
    if (!p) return false;
    return strstr(p, beginTag) != nullptr && strstr(p, endTag) != nullptr;
}

bool StorageManager::begin() {
    // CRITICAL: Do NOT pass true — never auto-format production filesystems
    if (!LittleFS.begin(false)) {
        Serial.println("[FS] FATAL: LittleFS mount failed. Entering EMERGENCY.");
        _mounted = false;
        return false;
    }
    _mounted = true;
    Serial.println("[FS] LittleFS mounted OK");
    return true;
}

bool StorageManager::loadCertificates(WiFiClientSecure& netClient) {
    if (!_mounted) {
        Serial.println("[FS] Cannot load certs — filesystem not mounted");
        return false;
    }

    Serial.println("[FS] Loading AWS IoT certificates from LittleFS...");

    // Process CA Certificate
    if (_ca.length() == 0) {
        File f = LittleFS.open(CERT_CA_PATH, "r");
        if (!f) { Serial.printf("[FS] Missing: %s\n", CERT_CA_PATH); return false; }
        _ca = f.readString();
        f.close();
        _normalizePem(_ca);
        if (!_extractPemBlock(_ca, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----")) {
            Serial.println("[FS] Invalid CA PEM");
            _ca = "";
            return false;
        }
    }
    netClient.setCACert(_ca.c_str());
    if (DEBUG_MODE) Serial.printf("[FS] CA loaded (%d bytes)\n", _ca.length());

    // Process Device Certificate
    if (_cert.length() == 0) {
        File f = LittleFS.open(CERT_CRT_PATH, "r");
        if (!f) { Serial.printf("[FS] Missing: %s\n", CERT_CRT_PATH); return false; }
        _cert = f.readString();
        f.close();
        _normalizePem(_cert);
        if (!_extractPemBlock(_cert, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----")) {
            Serial.println("[FS] Invalid Device Cert PEM");
            _cert = "";
            return false;
        }
    }
    netClient.setCertificate(_cert.c_str());
    if (DEBUG_MODE) Serial.printf("[FS] Cert loaded (%d bytes)\n", _cert.length());

    // Process Private Key
    if (_key.length() == 0) {
        File f = LittleFS.open(CERT_KEY_PATH, "r");
        if (!f) { Serial.printf("[FS] Missing: %s\n", CERT_KEY_PATH); return false; }
        _key = f.readString();
        f.close();
        _normalizePem(_key);
        bool keyOk = _extractPemBlock(_key, "-----BEGIN PRIVATE KEY-----", "-----END PRIVATE KEY-----") ||
                     _extractPemBlock(_key, "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----") ||
                     _extractPemBlock(_key, "-----BEGIN EC PRIVATE KEY-----", "-----END EC PRIVATE KEY-----");
        if (!keyOk) {
            Serial.println("[FS] Invalid Private Key PEM");
            _key = "";
            return false;
        }
    }
    netClient.setPrivateKey(_key.c_str());
    if (DEBUG_MODE) Serial.printf("[FS] Key loaded (%d bytes)\n", _key.length());

    Serial.println("[FS] All certificates loaded into TLS client");
    return true;
}

bool StorageManager::writeString(const char* path, const String& data) {
    if (!_mounted) return false;
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    f.print(data);
    f.close();
    return true;
}

String StorageManager::readString(const char* path) {
    if (!_mounted) return "";
    File f = LittleFS.open(path, "r");
    if (!f) return "";
    String s = f.readString();
    f.close();
    return s;
}

bool StorageManager::fileExists(const char* path) {
    if (!_mounted) return false;
    return LittleFS.exists(path);
}

// --- V2.1 Daily Ledger Implementation ---

#define LEDGER_PATH "/data/ledger.bin"
#define MAX_LEDGER_ENTRIES 30

bool StorageManager::saveLedgerEntry(const DailyLedgerEntry& entry) {
    if (!_mounted) return false;

    // 1. Read existing ledger to avoid duplicates and handle rotation
    DailyLedgerEntry history[MAX_LEDGER_ENTRIES];
    int count = loadLedger(history, MAX_LEDGER_ENTRIES);

    // Check if this HSDS is already recorded (rare, but prevents duplicates on reboot)
    for (int i = 0; i < count; i++) {
        if (history[i].hsds == entry.hsds) {
            Serial.printf("[FS] Ledger: HSDS %d already recorded, updating.\n", entry.hsds);
            history[i] = entry;
            // Overwrite file with updated list
            File f = LittleFS.open(LEDGER_PATH, "w");
            if (f) {
                f.write((uint8_t*)history, count * sizeof(DailyLedgerEntry));
                f.close();
            }
            return true;
        }
    }

    // 2. Append or Rotate
    File f;
    if (count < MAX_LEDGER_ENTRIES) {
        // Simple append
        f = LittleFS.open(LEDGER_PATH, "a");
        if (f) {
            f.write((uint8_t*)&entry, sizeof(DailyLedgerEntry));
            f.close();
            Serial.printf("[FS] Ledger: Added HSDS %d (Count: %d)\n", entry.hsds, count + 1);
            return true;
        }
    } else {
        // Rotate: Shift everything left by 1 and put new at end
        for (int i = 0; i < MAX_LEDGER_ENTRIES - 1; i++) {
            history[i] = history[i+1];
        }
        history[MAX_LEDGER_ENTRIES - 1] = entry;
        
        f = LittleFS.open(LEDGER_PATH, "w");
        if (f) {
            f.write((uint8_t*)history, MAX_LEDGER_ENTRIES * sizeof(DailyLedgerEntry));
            f.close();
            Serial.printf("[FS] Ledger: Rotated. New HSDS %d at end.\n", entry.hsds);
            return true;
        }
    }
    
    return false;
}

// --- V2.2.1 WiFi Provisioning Implementation ---

#define WIFI_CONFIG_PATH "/config/wifi.json"

bool StorageManager::saveWiFiConfig(const String& ssid, const String& password) {
    if (!_mounted) return false;
    
    // Ensure parent directory exists
    if (!LittleFS.exists("/config")) LittleFS.mkdir("/config");

    StaticJsonDocument<256> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["updated_at"] = millis();

    File f = LittleFS.open(WIFI_CONFIG_PATH, "w");
    if (!f) return false;
    
    serializeJson(doc, f);
    f.close();
    Serial.printf("[FS] WiFi config saved: %s\n", ssid.c_str());
    return true;
}

StorageManager::WiFiConfig StorageManager::loadWiFiConfig() {
    WiFiConfig cfg;
    if (!_mounted || !LittleFS.exists(WIFI_CONFIG_PATH)) return cfg;

    File f = LittleFS.open(WIFI_CONFIG_PATH, "r");
    if (!f) return cfg;

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if (!error) {
        cfg.ssid = doc["ssid"].as<String>();
        cfg.password = doc["password"].as<String>();
        cfg.is_valid = (cfg.ssid.length() > 0);
    }
    return cfg;
}

int StorageManager::loadLedger(DailyLedgerEntry* entries, int maxEntries) {
    if (!_mounted || !entries) return 0;
    if (!LittleFS.exists(LEDGER_PATH)) return 0;

    File f = LittleFS.open(LEDGER_PATH, "r");
    if (!f) return 0;

    size_t size = f.size();
    int count = size / sizeof(DailyLedgerEntry);
    if (count > maxEntries) count = maxEntries;

    size_t readSize = f.read((uint8_t*)entries, count * sizeof(DailyLedgerEntry));
    f.close();

    return readSize / sizeof(DailyLedgerEntry);
}
