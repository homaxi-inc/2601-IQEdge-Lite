#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
// Task_BLE — Core 1, Lowest Priority (PoC)
// BLE advertising + telemetry notify + simple RX commands.
// ============================================================

void taskBleEntry(void* param);
