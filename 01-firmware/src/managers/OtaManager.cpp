#include "OtaManager.h"
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>

static const char *TAG = "OTA_MGR";

bool OtaManager::startUpdate(const char* url, const char* rootCA) {
    if (_isUpdating) return false;
    _isUpdating = true;

    Serial.printf("[OTA] Initializing update from: %s\n", url);
    
    // Temporarily remove this task from the WDT
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    esp_task_wdt_delete(currentTask);
    Serial.println("[OTA] Watchdog disabled for OTA task.");

    esp_http_client_config_t config = {};
    config.url = url;
    config.cert_pem = rootCA;
    config.event_handler = _httpEventHandler;
    config.timeout_ms = 10000;
    config.keep_alive_enable = true;

    // Use the standard esp_https_ota API for ESP32 Arduino 3.x
    // It takes the http_client_config directly.
    esp_err_t ret = esp_https_ota(&config);
    
    if (ret == ESP_OK) {
        Serial.println("[OTA] Update completed successfully.");
        _isUpdating = false;
        // Re-add the task to the WDT before returning
        esp_task_wdt_add(currentTask);
        Serial.println("[OTA] Watchdog re-enabled for OTA task.");
        return true;
    } else {
        Serial.printf("[OTA] Update failed with error: %d\n", ret);
        _isUpdating = false;
        // Re-add the task to the WDT before returning
        esp_task_wdt_add(currentTask);
        Serial.println("[OTA] Watchdog re-enabled for OTA task.");
        return false;
    }

    return true;
}

esp_err_t OtaManager::_httpEventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            esp_task_wdt_reset();
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
    }
    return ESP_OK;
}
