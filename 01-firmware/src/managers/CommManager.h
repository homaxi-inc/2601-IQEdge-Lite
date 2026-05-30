#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "../core/SystemContext.h"
#include "StorageManager.h"

/**
 * @brief CommManager — WiFi + AWS MQTT with Exponential Backoff
 * V2.2: Integrated AWS IoT Jobs for Secure OTA.
 */
class CommManager {
public:
    void begin(StorageManager& storage);
    void loop();
    bool publishStatus();

    bool isWifiConnected() const { return WiFi.status() == WL_CONNECTED; }
    bool isMqttConnected()       { return _mqtt.connected(); }

private:
    WiFiClientSecure _tls;
    PubSubClient     _mqtt;
    StorageManager*  _storage = nullptr;

    bool _certsLoaded   = false;
    bool _timesynced    = false;
    bool _justConnected = false;
    bool _isSyncing     = false;

    unsigned long _wifiBackoff      = 0;
    unsigned long _wifiLastAttempt  = 0;
    unsigned long _mqttBackoff      = 0;
    unsigned long _mqttLastAttempt  = 0;
    unsigned long _lastPublish      = 0;

    String _clientId;
    String _jobTopicNotify;
    String _jobTopicGet;

    bool _pendingOta = false;
    String _pendingOtaUrl;
    String _pendingOtaJobId;

    bool _ensureWifi();
    bool _ensureMqtt();
    void _syncTime();
    void _buildPayload(char* buf, size_t bufLen);
    void _buildG2Payload(char* buf, size_t bufLen);
    unsigned long _nextBackoff(unsigned long current, unsigned long base, unsigned long max);

    // V2.2 OTA & MQTT Callback
    void _onMessageInternal(char* topic, byte* payload, unsigned int length);
    static void _onMessage(char* topic, byte* payload, unsigned int length);
    void _handleJob(char* payload, unsigned int length);
};
