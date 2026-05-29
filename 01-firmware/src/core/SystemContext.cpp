#include "SystemContext.h"
#include "../config/Config.h"

SystemContext& SystemContext::instance() {
    static SystemContext ctx;
    return ctx;
}

SystemContext::SystemContext() {
    _mutex = xSemaphoreCreateMutex();
    configASSERT(_mutex);
}

// --- State Machine ---

SystemState SystemContext::getState() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    SystemState s = _state;
    xSemaphoreGive(_mutex);
    return s;
}

void SystemContext::setState(SystemState newState) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_state != newState) {
        Serial.printf("[SYS] State: %s -> %s\n",
                      stateToString(_state), stateToString(newState));
        _state = newState;
    }
    xSemaphoreGive(_mutex);
}

const char* SystemContext::stateToString(SystemState s) const {
    switch (s) {
        case SystemState::BOOT:      return "BOOT";
        case SystemState::NORMAL:    return "NORMAL";
        case SystemState::CONSERVE:  return "CONSERVE";
        case SystemState::HIBERNATE: return "HIBERNATE";
        case SystemState::EMERGENCY: return "EMERGENCY";
        default:                     return "UNKNOWN";
    }
}

const char* SystemContext::reportingModeToString(ReportingMode m) const {
    switch (m) {
        case ReportingMode::NIGHT: return "NIGHT";
        case ReportingMode::PEAK:  return "PEAK";
        default:                   return "NORMAL";
    }
}

// --- Energy Data (thread-safe) ---

void SystemContext::setEnergySnapshot(const EnergySnapshot& snap) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _energySnap = snap;
    xSemaphoreGive(_mutex);
}

EnergySnapshot SystemContext::getEnergySnapshot() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    EnergySnapshot copy = _energySnap;
    xSemaphoreGive(_mutex);
    return copy;
}

void SystemContext::setSystemDiag(const SystemDiag& diag) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _sysDiag = diag;
    xSemaphoreGive(_mutex);
}

SystemDiag SystemContext::getSystemDiag() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    SystemDiag copy = _sysDiag;
    xSemaphoreGive(_mutex);
    return copy;
}

void SystemContext::setHistoricalData(const HistoricalData& hist) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _histData = hist;
    xSemaphoreGive(_mutex);
}

HistoricalData SystemContext::getHistoricalData() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    HistoricalData res = _histData;
    xSemaphoreGive(_mutex);
    return res;
}

void SystemContext::setGpsSnapshot(const GpsSnapshot& snap) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _gpsSnap = snap;
    xSemaphoreGive(_mutex);
}

GpsSnapshot SystemContext::getGpsSnapshot() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    GpsSnapshot res = _gpsSnap;
    xSemaphoreGive(_mutex);
    return res;
}

int SystemContext::getLastRecordedHsds() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    int res = _lastRecordedHsds;
    xSemaphoreGive(_mutex);
    return res;
}

void SystemContext::setLastRecordedHsds(int hsds) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _lastRecordedHsds = hsds;
    xSemaphoreGive(_mutex);
}


// --- Connection Flags ---

void SystemContext::setWifiConnected(bool v) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _wifiOk = v;
    xSemaphoreGive(_mutex);
}

void SystemContext::setMqttConnected(bool v) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _mqttOk = v;
    xSemaphoreGive(_mutex);
}

void SystemContext::setMpptConnected(bool v) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _mpptOk = v;
    xSemaphoreGive(_mutex);
}

bool SystemContext::isWifiConnected() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool v = _wifiOk;
    xSemaphoreGive(_mutex);
    return v;
}

bool SystemContext::isMqttConnected() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool v = _mqttOk;
    xSemaphoreGive(_mutex);
    return v;
}

bool SystemContext::isMpptConnected() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool v = _mpptOk;
    xSemaphoreGive(_mutex);
    return v;
}

bool SystemContext::isSystemHealthy() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool healthy = _wifiOk && _mqttOk && _mpptOk &&
                   (_state == SystemState::NORMAL);
    xSemaphoreGive(_mutex);
    return healthy;
}

// --- Urgent Publish (one-shot flag, consumed by CommManager) ---

void SystemContext::requestUrgentPublish() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _urgentPublish = true;
    xSemaphoreGive(_mutex);
}

bool SystemContext::consumeUrgentPublish() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool val = _urgentPublish;
    _urgentPublish = false;
    xSemaphoreGive(_mutex);
    return val;
}

// --- MPPT Disconnect Timestamp ---

void SystemContext::setMpptDisconnectTime(unsigned long ms) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _mpptDisconnectMs = ms;
    xSemaphoreGive(_mutex);
}

unsigned long SystemContext::getMpptDisconnectTime() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    unsigned long res = _mpptDisconnectMs;
    xSemaphoreGive(_mutex);
    return res;
}

// --- Adaptive Reporting ---

ReportingMode SystemContext::getReportingMode() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    ReportingMode m = _reportingMode;
    xSemaphoreGive(_mutex);
    return m;
}

void SystemContext::setReportingMode(ReportingMode mode) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_reportingMode != mode) {
        _reportingMode = mode;
    }
    xSemaphoreGive(_mutex);
}

unsigned long SystemContext::getDynamicPublishInterval() const {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    ReportingMode m = _reportingMode;
    xSemaphoreGive(_mutex);

    switch (m) {
        case ReportingMode::NIGHT: return PUBLISH_INTERVAL_NIGHT_MS;
        case ReportingMode::PEAK:  return PUBLISH_INTERVAL_PEAK_MS;
        default:                   return PUBLISH_INTERVAL_NORMAL_MS;
    }
}
