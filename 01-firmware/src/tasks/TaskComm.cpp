#include "TaskComm.h"
#include <esp_task_wdt.h>
#include "../config/Config.h"
#include "../managers/CommManager.h"

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

static CommManager s_comm;

void taskCommEntry(void* param) {
    // Register this task with the global WDT
    esp_task_wdt_add(NULL);

    TaskCommParams* p = static_cast<TaskCommParams*>(param);
    s_comm.begin(*p->storage);

    Serial.println("[TASK] Communication task running on Core 1 (V2.2 OTA Ready)");

    for (;;) {
        esp_task_wdt_reset();
        s_comm.loop();
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
