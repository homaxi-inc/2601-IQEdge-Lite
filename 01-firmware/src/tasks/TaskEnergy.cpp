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

    for (;;) {
        // Feed the dog
        esp_task_wdt_reset();

        s_energy.poll();
        s_energy.evaluatePowerPolicy();

        vTaskDelay(pdMS_TO_TICKS(100));  // 10 Hz sampling
    }
}
