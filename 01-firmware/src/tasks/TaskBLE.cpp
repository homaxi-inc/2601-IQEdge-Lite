#include "TaskBLE.h"
#include <esp_task_wdt.h>
#include "../config/Config.h"
#include "../core/SystemContext.h"
#include "../managers/BleManager.h"

static BleManager s_ble;

void taskBleEntry(void* param) {
    (void)param;
    if (!BLE_POC_ENABLED) {
        vTaskDelete(NULL);
        return;
    }

    // Register this task with the global WDT
    esp_task_wdt_add(NULL);

    // Defer BLE startup until comm path completed initial bring-up
    // (avoids boot-time contention with cert loading on the same core).
    auto& ctx = SystemContext::instance();
    while (!ctx.isMqttConnected() && ctx.getState() != SystemState::EMERGENCY) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    if (ctx.getState() == SystemState::EMERGENCY) {
        vTaskDelete(NULL);
        return;
    }

    s_ble.begin();
    Serial.printf("[TASK] BLE task running on Core %d\n", xPortGetCoreID());

    for (;;) {
        esp_task_wdt_reset();
        s_ble.loop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
