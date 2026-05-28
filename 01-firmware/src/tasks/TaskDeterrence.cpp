#include "TaskDeterrence.h"
#include <esp_task_wdt.h>
#include "../config/Config.h"
#include "../core/SystemContext.h"
#include "../hal/LedController.h"

static LedController s_led;

static LedPattern _resolvePattern() {
    auto& ctx = SystemContext::instance();

    if (ctx.getState() == SystemState::EMERGENCY)
        return LedPattern::SOS;

    if (!ctx.isWifiConnected())
        return LedPattern::FAST_BLINK;

    if (!ctx.isMqttConnected())
        return LedPattern::DOUBLE_BLINK;

    if (!ctx.isMpptConnected())
        return LedPattern::TRIPLE_BLINK;

    if (ctx.isSystemHealthy())
        return LedPattern::SOLID;

    return LedPattern::SLOW_BLINK;
}

void taskDeterrenceEntry(void* param) {
    // Register this task with the global WDT
    esp_task_wdt_add(NULL);

    s_led.begin(LED_STATUS_PIN);

    Serial.println("[TASK] Deterrence task running on Core 1");

    for (;;) {
        // Feed the dog
        esp_task_wdt_reset();

        s_led.setPattern(_resolvePattern());
        s_led.tick();

        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz for smooth LED
    }
}
