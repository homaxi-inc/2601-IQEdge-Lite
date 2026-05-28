#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
// Task_Deterrence — Core 1, Real-time Priority
// LED state machine + alarm/deterrence triggers.
// ============================================================

void taskDeterrenceEntry(void* param);
