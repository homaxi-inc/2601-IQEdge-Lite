#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../managers/StorageManager.h"

struct TaskEnergyParams {
    StorageManager* storage;
};

void taskEnergyEntry(void* param);
