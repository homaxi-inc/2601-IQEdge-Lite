#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>

/**
 * @brief OtaManager (V2.2)
 * Handles secure firmware updates via AWS S3 pre-signed URLs.
 */
class OtaManager {
public:
    static OtaManager& getInstance() {
        static OtaManager instance;
        return instance;
    }

    /**
     * @brief Start OTA update from a URL.
     * @param url The pre-signed S3 URL.
     * @param rootCA The root CA certificate for HTTPS validation.
     * @return true if update started successfully.
     */
    bool startUpdate(const char* url, const char* rootCA);

    bool isUpdating() const { return _isUpdating; }

private:
    OtaManager() : _isUpdating(false) {}
    bool _isUpdating;

    static esp_err_t _httpEventHandler(esp_http_client_event_t *evt);
};

#endif
