#include <ArduinoJson.h>
#include "CommManager.h"
#include "OtaManager.h"
#include "../config/Config.h"
#include "../core/SystemContext.h"
#include <time.h>
#include <esp_task_wdt.h>

// Forward static callback to the instance
static CommManager* s_instance = nullptr;

void CommManager::_onMessage(char* topic, byte* payload, unsigned int length) {
    if (s_instance) s_instance->_onMessageInternal(topic, payload, length);
}

void CommManager::begin(StorageManager& storage) {
    s_instance = this;
    _storage = &storage;
    auto& ctx = SystemContext::instance();
    _clientId = "IQEdge_" + ctx.mac;
    
    // AWS IoT Jobs topics
    _jobTopicNotify = "$aws/things/" + _clientId + "/jobs/notify-next";
    _jobTopicGet    = "$aws/things/" + _clientId + "/jobs/$next/get/accepted";

    _mqtt.setClient(_tls);
    _mqtt.setServer(AWS_MQTT_ENDPOINT, AWS_MQTT_PORT);
    _mqtt.setCallback(_onMessage);
    _mqtt.setBufferSize(3072);
    Serial.println("[COM] initialized (V2.2 OTA Job Listening)");
}

void CommManager::loop() {
    esp_task_wdt_reset();

    // --- V2.2.1 Serial Provisioning Listener ---
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.startsWith("SET WIFI ")) {
            int space1 = cmd.indexOf(' ', 9);
            if (space1 > 9) {
                String newSsid = cmd.substring(9, space1);
                String newPass = cmd.substring(space1 + 1);
                newSsid.trim(); newPass.trim();
                Serial.printf("[COM] New WiFi Config Received - SSID: %s\n", newSsid.c_str());
                if (_storage && _storage->saveWiFiConfig(newSsid, newPass)) {
                    Serial.println("[COM] WiFi config saved. Restarting device...");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    ESP.restart();
                }
            } else {
                Serial.println("[COM] ERROR: Invalid format. Use 'SET WIFI <SSID> <PASS>'");
            }
        }
    }

    if (!_ensureWifi()) return;

    // Ensure NTP sync before ANY SSL operation
    if (!_timesynced) {
        _syncTime();
    }

    if (!_ensureMqtt()) return;

    _mqtt.loop();

    if (_pendingOta && _storage) {
        _pendingOta = false;
        String updateTopic = "$aws/things/" + _clientId + "/jobs/" + _pendingOtaJobId + "/update";
        _mqtt.publish(updateTopic.c_str(), "{\"status\":\"IN_PROGRESS\"}");
        
        // Force certificate update if needed, using Amazon CA
        bool success = OtaManager::getInstance().startUpdate(_pendingOtaUrl.c_str(), _storage->getCA().c_str());
        
        if (success) {
            _mqtt.publish(updateTopic.c_str(), "{\"status\":\"SUCCEEDED\"}");
            Serial.println("[COM] Job marked SUCCEEDED. Rebooting...");
            for(int i=0; i<5; i++) {
                _mqtt.loop();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            _mqtt.disconnect();
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        } else {
            _mqtt.publish(updateTopic.c_str(), "{\"status\":\"FAILED\"}");
            Serial.println("[COM] Job marked FAILED.");
        }
    }

    auto& ctx = SystemContext::instance();

    if (_justConnected) {
        _justConnected = false;
        if (publishStatus()) _lastPublish = millis();
    } else if (ctx.consumeUrgentPublish()) {
        if (publishStatus()) {
            _lastPublish = millis();
        } else {
            ctx.requestUrgentPublish();
        }
    } else {
        unsigned long interval = ctx.getDynamicPublishInterval();
        if (millis() - _lastPublish >= interval || _lastPublish == 0) {
            if (publishStatus()) {
                _lastPublish = millis();
            }
        }
    }
}

static void _formatCloudTimestamp(char* buf, size_t len) {
    time_t now = time(nullptr);
    struct tm ti;
    gmtime_r(&now, &ti);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S UTC", &ti);
}

static void _formatIso8601Utc(char* buf, size_t len) {
    time_t now = time(nullptr);
    struct tm ti;
    gmtime_r(&now, &ti);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &ti);
}

static void _formatChipId(uint64_t chipId, char* buf, size_t len) {
    snprintf(buf, len, "%012llX", (unsigned long long)chipId);
}

bool CommManager::publishStatus() {
    esp_task_wdt_reset();
    if (!_mqtt.connected()) return false;

    auto& ctx = SystemContext::instance();
    String serial = ctx.getSystemDiag().mppt_serial;

    // SaveDeviceStatus keys DynamoDB/Timestream by MPPT serial (deviceId)
    if (serial.length() < 3) {
        Serial.println("[COM] SKIP publish: deviceId (MPPT SER#) not available yet");
        return false;
    }

    if (!ctx.isMpptConnected()) {
        Serial.println("[COM] SKIP publish: MPPT data stale / disconnected");
        return false;
    }

    char buf[2048];
    _buildPayload(buf, sizeof(buf));
    Serial.printf("[COM] Publishing payload (%d bytes) to topic: %s\n", strlen(buf), MQTT_TOPIC_STATUS);
    
    esp_task_wdt_reset();
    bool ok = _mqtt.publish(MQTT_TOPIC_STATUS, buf);
    esp_task_wdt_reset();
    
    if (ok) {
        Serial.println("[COM] Legacy publish OK");
    } else {
        Serial.printf("[COM] Legacy publish FAILED, MQTT state=%d\n", _mqtt.state());
    }

    char g2buf[2048];
    _buildG2Payload(g2buf, sizeof(g2buf));
    Serial.printf("[COM] G2 publish (%d bytes) topic: %s\n", strlen(g2buf), G2_MQTT_TOPIC_ENERGY);
    esp_task_wdt_reset();
    bool g2ok = _mqtt.publish(G2_MQTT_TOPIC_ENERGY, g2buf);
    esp_task_wdt_reset();
    if (g2ok) {
        Serial.println("[COM] G2 publish OK");
    } else {
        Serial.printf("[COM] G2 publish FAILED, MQTT state=%d\n", _mqtt.state());
    }

    return ok && g2ok;
}

bool CommManager::_ensureWifi() {
    esp_task_wdt_reset();
    if (WiFi.status() == WL_CONNECTED) {
        SystemContext::instance().setWifiConnected(true);
        return true;
    }
    
    SystemContext::instance().setWifiConnected(false);
    unsigned long now = millis();
    if (_wifiBackoff > 0 && (now - _wifiLastAttempt) < _wifiBackoff) return false;
    
    // Try to load dynamic config first
    String ssid = WIFI_SSID;
    String pass = WIFI_PASSWORD;
    if (_storage) {
        auto cfg = _storage->loadWiFiConfig();
        if (cfg.is_valid) {
            ssid = cfg.ssid;
            pass = cfg.password;
            if (DEBUG_MODE) Serial.printf("[COM] Using dynamic WiFi: %s\n", ssid.c_str());
        }
    }

    WiFi.begin(ssid.c_str(), pass.c_str());
    _wifiLastAttempt = now;
    _wifiBackoff = _nextBackoff(_wifiBackoff, WIFI_RECONNECT_BASE_MS, WIFI_RECONNECT_MAX_MS);
    return false;
}

bool CommManager::_ensureMqtt() {
    esp_task_wdt_reset();
    if (_mqtt.connected()) return true;
    
    // We are not connected. Ensure the context flag is cleared.
    SystemContext::instance().setMqttConnected(false);

    // CRITICAL: Check if system time is actually synced (not 1970)
    time_t now_utc = time(nullptr);
    if (now_utc < 1000000) {
        Serial.println("[COM] MQTT: Postponing connection - System time NOT SYNCED yet.");
        _syncTime(); 
        return false;
    }

    unsigned long now_ms = millis();
    if (_mqttBackoff > 0 && (now_ms - _mqttLastAttempt) < _mqttBackoff) return false;
    if (!_storage) return false;

    if (!_certsLoaded) {
        if (_storage->loadCertificates(_tls)) {
            _certsLoaded = true;
            Serial.println("[COM] Certificates loaded successfully");
        } else {
            Serial.println("[COM] ERROR: Certificate loading failed from LittleFS");
            _mqttBackoff = _nextBackoff(_mqttBackoff, MQTT_RECONNECT_BASE_MS, MQTT_RECONNECT_MAX_MS);
            _mqttLastAttempt = now_ms;
            return false;
        }
    }

    _mqttLastAttempt = now_ms;
    Serial.printf("[COM] MQTT: Connecting to %s:%d... (Time: %ld)\n", AWS_MQTT_ENDPOINT, AWS_MQTT_PORT, (long)now_utc);
    esp_task_wdt_reset();
    if (_mqtt.connect(_clientId.c_str())) {
        esp_task_wdt_reset();
        _mqtt.subscribe(MQTT_TOPIC_CMD);
        _mqtt.subscribe(_jobTopicNotify.c_str());
        _mqtt.subscribe(_jobTopicGet.c_str());
        _mqttBackoff = 0;
        _justConnected = true;
        Serial.println("[COM] MQTT Connected & Job Topics Subscribed");

        // Request pending jobs
        String jobGetReq = "$aws/things/" + _clientId + "/jobs/$next/get";
        _mqtt.publish(jobGetReq.c_str(), "{}");

        SystemContext::instance().setMqttConnected(true);
        return true;
    }
    Serial.printf("[COM] MQTT: Connection failed, state=%d\n", _mqtt.state());
    _mqttBackoff = _nextBackoff(_mqttBackoff, MQTT_RECONNECT_BASE_MS, MQTT_RECONNECT_MAX_MS);
    return false;
}

void CommManager::_syncTime() {
    Serial.println("[COM] Syncing time via NTP (pool.ntp.org)...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.apple.com");
    
    int attempts = 0;
    time_t now = time(nullptr);
    while (now < 1000000 && attempts < 30) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
        now = time(nullptr);
        attempts++;
        if (attempts % 5 == 0) Serial.printf("[COM] Waiting for NTP... (%d/30)\n", attempts);
    }

    if (now >= 1000000) {
        _timesynced = true;
        struct tm ti;
        gmtime_r(&now, &ti);
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &ti);
        Serial.printf("[COM] Time synced SUCCESS: %s\n", ts);
    } else {
        Serial.println("[COM] ERROR: NTP Sync TIMEOUT. Check Internet/UDP 123.");
    }
}

void CommManager::_onMessageInternal(char* topic, byte* payload, unsigned int length) {
    if (String(topic) == _jobTopicNotify || String(topic) == _jobTopicGet) {
        _handleJob((char*)payload, length);
    }
}

void CommManager::_handleJob(char* payload, unsigned int length) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, (const char*)payload, length);
    if (error) return;

    JsonObject execution = doc["execution"];
    if (execution.isNull()) {
        // AWS Job notify-next might have a slightly different root for "jobs" vs "job"
        execution = doc["jobs"][0]; // Check first pending job
    }
    if (execution.isNull()) return;

    const char* jobId = execution["jobId"];
    const char* url   = execution["jobDocument"]["url"];
    
    if (url) {
        String updateTopic = "$aws/things/" + _clientId + "/jobs/" + String(jobId) + "/update";
        String urlStr = String(url);
        String currentVersionStr = String(FIRMWARE_VERSION);

        if (urlStr.indexOf(currentVersionStr) > 0) {
            Serial.printf("[COM] OTA Job is for current version %s. Marking SUCCEEDED.\n", FIRMWARE_VERSION);
            _mqtt.publish(updateTopic.c_str(), "{\"status\":\"SUCCEEDED\"}");
            return;
        }

        Serial.printf("[COM] OTA Job Received: %s. Scheduling update...\n", jobId);
        _pendingOtaUrl = urlStr;
        _pendingOtaJobId = String(jobId);
        _pendingOta = true;
    }
}

void CommManager::_buildPayload(char* buf, size_t bufLen) {
    auto& ctx  = SystemContext::instance();
    auto  snap = ctx.getEnergySnapshot();
    auto  diag = ctx.getSystemDiag();
    auto  hist = ctx.getHistoricalData();

    const float totalKwh = round((hist.total_yield_wh / 1000.0) * 100) / 100.0;
    const float todayKwh = round((hist.today_yield_wh / 1000.0) * 100) / 100.0;

    StaticJsonDocument<2048> doc;
    doc["device"]            = DEVICE_NAME;
    doc["mac"]               = ctx.mac;
    doc["firmware_version"]  = FIRMWARE_VERSION;
    doc["is_reconciliation"] = false;

    char chipBuf[17];
    _formatChipId(ctx.chipId, chipBuf, sizeof(chipBuf));
    doc["chipid"] = chipBuf;

    // --- Cloud ingest flat fields (SaveDeviceStatus / DeviceStatusToLambda) ---
    doc["deviceId"]        = diag.mppt_serial;
    doc["status"]          = "running";
    doc["data_stale"]      = false;
    doc["state"]           = ctx.stateToString(ctx.getState());
    doc["reporting_mode"]  = ctx.reportingModeToString(ctx.getReportingMode());
    doc["soc"]             = snap.battery_soc;
    doc["battery_voltage"] = snap.battery_voltage;
    doc["solar_power"]     = snap.solar_power;
    doc["load_power"]      = snap.load_power;
    doc["total_yield_kwh"] = totalKwh;
    doc["today_yield_kwh"] = todayKwh;
    doc["days_running"]    = hist.day_sequence;
    doc["last_update"]     = (uint32_t)time(nullptr);

    char tsCloud[64];
    _formatCloudTimestamp(tsCloud, sizeof(tsCloud));
    doc["timestamp"] = tsCloud;

    auto gpsSnap = ctx.getGpsSnapshot();
    if (gpsSnap.isValid) {
        JsonObject gps = doc.createNestedObject("gps");
        gps["lat"] = gpsSnap.latitude;
        gps["lon"] = gpsSnap.longitude;
        gps["sats"] = gpsSnap.satellites;
    }

    // --- Nested blocks (dashboard / backward compatible with payload v1/v2) ---
    JsonObject h = doc.createNestedObject("history");
    h["total_yield_kwh"] = totalKwh;
    h["today_yield_kwh"] = todayKwh;
    h["max_power_w"]     = hist.max_power_w;
    h["days_running"]    = hist.day_sequence;

    JsonObject bat = doc.createNestedObject("battery");
    bat["voltage"]           = snap.battery_voltage;
    bat["current"]           = snap.battery_current;
    bat["soc"]               = snap.battery_soc;
    bat["charge_state"]      = snap.charge_state;
    bat["charge_state_code"] = snap.charge_state_code;

    JsonObject sol = doc.createNestedObject("solar");
    sol["voltage"] = snap.solar_voltage;
    sol["current"] = snap.solar_current;
    sol["power"]   = snap.solar_power;

    JsonObject load = doc.createNestedObject("load");
    load["power"]   = snap.load_power;
    load["current"] = snap.load_current;
    load["status"]  = snap.load_status;

    JsonObject sys = doc.createNestedObject("system");
    sys["serial"]   = diag.mppt_serial;
    sys["firmware"] = diag.mppt_firmware;
    sys["pid"]      = diag.pid;

    serializeJson(doc, buf, bufLen);
}

void CommManager::_buildG2Payload(char* buf, size_t bufLen) {
    auto& ctx  = SystemContext::instance();
    auto  snap = ctx.getEnergySnapshot();
    auto  diag = ctx.getSystemDiag();
    auto  hist = ctx.getHistoricalData();

    const float totalKwh = round((hist.total_yield_wh / 1000.0) * 100) / 100.0;
    const float todayKwh = round((hist.today_yield_wh / 1000.0) * 100) / 100.0;

    StaticJsonDocument<2048> doc;
    doc["schema_version"]    = "energy.telemetry.v1";
    doc["sys_id"]            = G2_SYS_ID;
    doc["component_id"]      = diag.mppt_serial;
    doc["component_role"]    = "mppt";
    doc["domain"]            = "energy";
    doc["ingest_mode"]       = "live";
    doc["firmware_version"]  = FIRMWARE_VERSION;
    doc["mac"]               = ctx.mac;
    doc["is_reconciliation"] = false;
    doc["status"]            = "running";
    doc["data_stale"]        = false;
    doc["state"]             = ctx.stateToString(ctx.getState());
    doc["reporting_mode"]    = ctx.reportingModeToString(ctx.getReportingMode());

    char chipBuf[17];
    _formatChipId(ctx.chipId, chipBuf, sizeof(chipBuf));
    doc["chipid"] = chipBuf;

    char tsIso[32];
    _formatIso8601Utc(tsIso, sizeof(tsIso));
    doc["timestamp"] = tsIso;

    JsonObject measures = doc.createNestedObject("measures");

    JsonObject bat = measures.createNestedObject("battery");
    bat["voltage_v"]           = snap.battery_voltage;
    bat["current_a"]           = snap.battery_current;
    bat["soc_pct"]             = snap.battery_soc;
    bat["charge_state"]        = snap.charge_state;
    bat["charge_state_code"]   = snap.charge_state_code;

    JsonObject sol = measures.createNestedObject("solar");
    sol["voltage_v"] = snap.solar_voltage;
    sol["current_a"] = snap.solar_current;
    sol["power_w"]   = snap.solar_power;

    JsonObject load = measures.createNestedObject("load");
    load["voltage_v"] = snap.battery_voltage;
    load["current_a"] = snap.load_current;
    load["power_w"]   = snap.load_power;
    load["status"]    = snap.load_status;

    JsonObject yld = measures.createNestedObject("yield");
    yld["total_kwh"]    = totalKwh;
    yld["today_kwh"]    = todayKwh;
    yld["days_running"] = hist.day_sequence;
    yld["max_power_w"]  = hist.max_power_w;

    doc["soc"]             = snap.battery_soc;
    doc["battery_voltage"] = snap.battery_voltage;
    doc["solar_power"]     = snap.solar_power;
    doc["load_power"]      = snap.load_power;
    doc["total_yield_kwh"] = totalKwh;
    doc["today_yield_kwh"] = todayKwh;
    // days_running only in measures.yield — flat duplicate breaks M4 MULTI ingest

    JsonObject leg = doc.createNestedObject("legacy");
    leg["deviceId"] = diag.mppt_serial;
    leg["device"]   = DEVICE_NAME;

    serializeJson(doc, buf, bufLen);
}

unsigned long CommManager::_nextBackoff(unsigned long current, unsigned long base, unsigned long max) {
    if (current == 0) return base;
    unsigned long next = current * 2;
    return (next > max) ? max : next;
}
