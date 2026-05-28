#include "TaskEnergy.h"
#include <esp_task_wdt.h>
#include "../config/Config.h"
#include "../core/SystemContext.h"
#include "../managers/EnergyManager.h"

static EnergyManager s_energy;

void taskEnergyEntry(void* param) {
    // Register this task with the global WDT
    esp_task_wdt_add(NULL);

    TaskEnergyParams* p = static_cast<TaskEnergyParams*>(param);
    s_energy.begin(Serial2, *p->storage);

    Serial.printf("[TASK] Energy task running on Core %d\n", xPortGetCoreID());
    const unsigned long bootMs = millis();

    for (;;) {
        // Feed the dog
        esp_task_wdt_reset();

        auto& ctx = SystemContext::instance();
        // Boot-time guard: avoid stressing UART path while MQTT/TLS stack is
        // in first bring-up phase on the other core.
        bool inBringupWindow = (millis() - bootMs) < 120000UL;
        bool deferUartPoll = inBringupWindow && ctx.isWifiConnected() && !ctx.isMqttConnected();

        if (!deferUartPoll) {
            s_energy.poll();
        }
        s_energy.evaluatePowerPolicy();

        vTaskDelay(pdMS_TO_TICKS(100));  // 10 Hz sampling
    }
}
