#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../managers/StorageManager.h"

// ============================================================
// Task_Communication — Core 1, Low Priority
// WiFi, MQTT, AWS IoT, OTA. Allowed to block.
// ============================================================

struct TaskCommParams {
    StorageManager* storage;
};

void taskCommEntry(void* param);
