#include "TaskGps.h"
#include "../config/Config.h"
#include "../core/SystemContext.h"
#include "../hal/GpsDriver.h"
#include <esp_task_wdt.h>

void taskGpsEntry(void* pvParameters) {
    GpsDriver gpsDriver;
    gpsDriver.begin();
    auto& ctx = SystemContext::instance();
    unsigned long lastPrint = 0;

    for (;;) {
        esp_task_wdt_reset();
        if (gpsDriver.loop()) {
            GpsSnapshot snap;
            gpsDriver.populateSnapshot(snap);
            ctx.setGpsSnapshot(snap);
            gpsDriver.syncSystemTime();
        }
        if (DEBUG_MODE && (millis() - lastPrint > 10000)) {
            lastPrint = millis();
            GpsSnapshot s = ctx.getGpsSnapshot();
            Serial.printf("\n[GPS DEBUG] Fix: %d, Sats: %d, Lat: %.6f, Lon: %.6f\n", 
                          s.isValid, s.satellites, s.latitude, s.longitude);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
