// ============================================================
//  IQEdge V2.0 — Tesla-Grade Firmware Architecture
//  Entry Point: Hardware init + FreeRTOS task creation
//  ALL business logic runs in dedicated tasks.
//  loop() is intentionally empty.
// ============================================================

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config/Config.h"
#include "core/SystemContext.h"
#include "managers/StorageManager.h"
#include "tasks/TaskEnergy.h"
#include "tasks/TaskComm.h"
#include "tasks/TaskDeterrence.h"
#include "tasks/TaskBLE.h"
#include "tasks/TaskGps.h"

static StorageManager   g_storage;
static TaskCommParams   g_commParams;
static TaskEnergyParams g_energyParams;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.printf("  %s  |  FW: %s\n", DEVICE_NAME, FIRMWARE_VERSION);
    Serial.printf("  Chip: %s  |  Cores: %d\n", ESP.getChipModel(), ESP.getChipCores());
    Serial.println("========================================");

    // --- Task Watchdog (Global) ---
    // Panic = true (trigger HW reset on timeout)
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
    esp_task_wdt_add(NULL);  // Arduino loop task must feed TWDT during long SSL/MQTT

    auto& ctx = SystemContext::instance();
    ctx.setState(SystemState::BOOT);

    // --- Filesystem (no auto-format) ---
    if (!g_storage.begin()) {
        ctx.setState(SystemState::EMERGENCY);
        Serial.println("[BOOT] EMERGENCY: Filesystem mount failed. LED SOS.");
        // Fall through — deterrence task will show SOS pattern
    }

    // --- Device Identity ---
    WiFi.mode(WIFI_STA); // Power on WiFi to ensure MAC is available
    ctx.mac    = WiFi.macAddress();
    ctx.chipId = ESP.getEfuseMac();
    Serial.printf("[BOOT] MAC: %s\n", ctx.mac.c_str());

    // --- Prepare task params ---
    g_commParams.storage = &g_storage;
    g_energyParams.storage = &g_storage;

    // --- Launch FreeRTOS Tasks ---

    // Core 0: Energy (isolated from comm/LED workload)
    xTaskCreatePinnedToCore(
        taskEnergyEntry,
        "Task_Energy",
        TASK_ENERGY_STACK,
        &g_energyParams,
        TASK_ENERGY_PRIO,
        nullptr,
        0   // Core 0
    );

    // Core 1: Communication (low priority, may block)
    xTaskCreatePinnedToCore(
        taskCommEntry,
        "Task_Comm",
        TASK_COMM_STACK,
        &g_commParams,
        TASK_COMM_PRIO,
        nullptr,
        1   // Core 1
    );

    if (GPS_ENABLED) {
        xTaskCreatePinnedToCore(taskGpsEntry, "Task_GPS", TASK_GPS_STACK, nullptr, TASK_GPS_PRIO, nullptr, 1);
    }

    // Core 1: Deterrence / LED (real-time, lightweight)
    xTaskCreatePinnedToCore(
        taskDeterrenceEntry,
        "Task_Deterrence",
        TASK_DETERRENCE_STACK,
        nullptr,
        TASK_DETERRENCE_PRIO,
        nullptr,
        1   // Core 1
    );



    // Core 1: BLE PoC telemetry (lowest priority, non-blocking)
    if (BLE_POC_ENABLED) {
        xTaskCreatePinnedToCore(
            taskBleEntry,
            "Task_BLE",
            TASK_BLE_STACK,
            nullptr,
            TASK_BLE_PRIO,
            nullptr,
            1   // Core 1
        );
    }

    if (ctx.getState() != SystemState::EMERGENCY) {
        ctx.setState(SystemState::NORMAL);
    }

    Serial.println("[BOOT] All tasks launched. setup() complete.");
    Serial.println("========================================");
}

void loop() {
    // Feed TWDT for the Arduino loop task (Core 1); Comm SSL can starve idle otherwise.
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
