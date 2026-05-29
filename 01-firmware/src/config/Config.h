#pragma once

// ============================================================
// IQEdge V2.0 — Global Configuration
// ============================================================

#define DEVICE_NAME     "IQEdge V2.3"
#define FIRMWARE_VERSION "v2.2.3.25"

// --- Build Flags ---
#define DEBUG_MODE          true
#define PRODUCTION_VERIFY   true
//#define ENABLE_OTA       // Uncomment ONLY for development/maintenance builds.
                            // Production firmware MUST NOT have OTA enabled without
                            // Secure Boot V2 + Signed OTA (see Phase 2 roadmap).

// --- Pin Definitions ---
#define LED_BUILTIN_PIN     2
#define LED_STATUS_PIN      2
#define VEDIRECT_RX_PIN     16  // HIL: VE.Direct UART (ESP32 RX=16, TX=17)
#define VEDIRECT_TX_PIN     17
#define VEDIRECT_BAUD       19200

// --- WiFi ---
constexpr const char* WIFI_SSID     = "IQWatch";
constexpr const char* WIFI_PASSWORD = "Homaxi2526";

// --- AWS IoT & API ---
constexpr const char* AWS_MQTT_ENDPOINT = "a3vcfgcj3um9l2-ats.iot.us-east-1.amazonaws.com";
constexpr uint16_t    AWS_MQTT_PORT     = 8883;
constexpr const char* MQTT_TOPIC_STATUS = "device/status";
constexpr const char* MQTT_TOPIC_CMD    = "device/command";
// IQWatch REST API (1y9689tax0) is host/tools only — see tools/aws-verify, not used by firmware.

// --- Timing ---
constexpr unsigned long PUBLISH_INTERVAL_NORMAL_MS = 300000;   // 5 min (Default)
constexpr unsigned long PUBLISH_INTERVAL_NIGHT_MS  = 1800000;  // 30 min (Production)
constexpr unsigned long PUBLISH_INTERVAL_PEAK_MS   = 60000;    // 1 min (Peak > 100W)
constexpr unsigned long VEDIRECT_TIMEOUT_MS        = 60000;    // 1 min
constexpr unsigned long MPPT_CHECK_INTERVAL_MS     = 5000;
constexpr unsigned long HEARTBEAT_INTERVAL_MS      = 60000;

// --- Adaptive Reporting Algorithm Thresholds ---
constexpr float NIGHT_V_THRESHOLD     = 5.0f;     // Solar V < 5V is dark
constexpr float PEAK_W_THRESHOLD      = 100.0f;   // Solar P > 100W is peak
constexpr int   NIGHT_CONFIRM_COUNT   = 180;      // 180 * 5s = 15 mins to confirm night
constexpr unsigned long WIFI_RECONNECT_BASE_MS  = 2000;     // Exponential backoff base
constexpr unsigned long WIFI_RECONNECT_MAX_MS   = 120000;   // Max 2 min
constexpr unsigned long MQTT_RECONNECT_BASE_MS  = 3000;
constexpr unsigned long MQTT_RECONNECT_MAX_MS   = 180000;   // Max 3 min

// --- OTA (development only, guarded by ENABLE_OTA) ---
#ifdef ENABLE_OTA
constexpr const char* OTA_PASSWORD = "IQEdge!R0t@2026$ecure";
#endif

// --- Watchdog ---
constexpr uint32_t WDT_TIMEOUT_SEC = 30;

// --- FreeRTOS Task Config ---
constexpr uint32_t TASK_ENERGY_STACK      = 8192;   // Reduced from 20480
constexpr uint32_t TASK_COMM_STACK        = 16384;  // Reduced from 32768 (Still plenty for SSL)
constexpr uint32_t TASK_DETERRENCE_STACK  = 4096;   // Reduced from 8192
constexpr uint32_t TASK_BLE_STACK         = 4096;   // Reduced from 8192
constexpr uint32_t TASK_GPS_STACK         = 4096;   // Dedicated GPS stack
constexpr UBaseType_t TASK_ENERGY_PRIO    = 5;    // High: Core 0
constexpr UBaseType_t TASK_COMM_PRIO      = 2;    // Low: Core 1
constexpr UBaseType_t TASK_DETERRENCE_PRIO = 4;   // Real-time: Core 1
constexpr UBaseType_t TASK_GPS_PRIO       = 3;    // Med: Core 1
constexpr UBaseType_t TASK_BLE_PRIO       = 1;    // Lowest: Core 1

// --- GPS Module (Disabled by default, HW not ready) ---
constexpr bool GPS_ENABLED                      = false;

// --- BLE PoC (V2.1 Concept Validation) ---
// Keep BLE disabled by default for V2.0 production stability.
// Enable explicitly only when doing V2.1 BLE PoC validation.
constexpr bool BLE_POC_ENABLED                  = false;
constexpr const char* BLE_DEVICE_NAME           = "IQEdge_V2";
constexpr unsigned long BLE_NOTIFY_INTERVAL_MS  = 1000;   // 1 Hz default
constexpr unsigned long BLE_NOTIFY_MIN_MS       = 500;    // 2 Hz max
constexpr unsigned long BLE_NOTIFY_MAX_MS       = 2000;   // 0.5 Hz min

// --- LiFePO4 Voltage-to-SoC Lookup ---
constexpr float SOC_VOLTAGES[] = {13.6, 13.5, 13.4, 13.2, 13.0, 12.8, 12.4, 12.1, 10.4, 10.0};
constexpr float SOC_PERCENTS[] = {100,  99,   90,   70,   40,   30,   20,   10,   1,    0   };
constexpr int   SOC_TABLE_LEN  = sizeof(SOC_VOLTAGES) / sizeof(SOC_VOLTAGES[0]);

// --- LittleFS Certificate Paths ---
constexpr const char* CERT_CA_PATH   = "/AmazonRootCA1.pem";
constexpr const char* CERT_CRT_PATH  = "/certificate.pem.crt";
constexpr const char* CERT_KEY_PATH  = "/private.pem.key";
