#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================================
// SystemContext — Singleton State Machine (Thread-Safe)
// ============================================================

enum class SystemState : uint8_t {
    BOOT,           // Initial startup, hardware init
    NORMAL,         // Full operation
    CONSERVE,       // Low battery — reduce non-essential loads
    HIBERNATE,      // Critical battery — PIR only, RTC wake
    EMERGENCY       // Fatal error (certs missing, FS corrupt)
};

enum class ReportingMode : uint8_t {
    NORMAL,         // Standard 5-min
    NIGHT,          // Conservative 30-min
    PEAK            // Responsive 1-min
};

struct EnergySnapshot {
    float battery_voltage   = 0.0f;
    float battery_current   = 0.0f;
    float battery_soc       = 0.0f;
    float solar_voltage     = 0.0f;
    float solar_power       = 0.0f;
    float solar_current     = 0.0f;
    float load_current      = 0.0f;
    float load_power        = 0.0f;
    float internal_r_mohm   = 0.0f;
    int   charge_state_code = 0;
    bool  soc_from_mppt     = false;
    String charge_state     = "Unknown";
    String load_status      = "OFF";
    unsigned long timestamp = 0;
};

struct SystemDiag {
    String pid;
    String mppt_firmware;
    String mppt_serial;
    unsigned long off_reason = 0;
    int error_code           = 0;
    int mppt_mode            = 0;
};

struct HistoricalData {
    long total_yield_wh          = 0;
    long today_yield_wh          = 0;
    long max_power_w             = 0;
    long yesterday_yield_wh      = 0;
    long yesterday_max_power_w   = 0;
    int  day_sequence            = 0;
};

struct GpsSnapshot {
    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0;
    int satellites = 0;
    float hdop = 99.9;
    bool isValid = false;
    uint32_t timestamp_utc = 0;
};

// --- V2.1 Daily Ledger for 30-day offline reconciliation ---
struct DailyLedgerEntry {
    uint16_t hsds;               // Day sequence from MPPT
    uint32_t total_yield_wh;     // H19 at end of day
    uint32_t day_yield_wh;       // H22 (yesterday's yield)
    uint16_t max_power_w;        // H23 (yesterday's max power)
    uint32_t timestamp_utc;      // NTP time when recorded
};

class SystemContext {
public:
    static SystemContext& instance();

    // State machine
    SystemState getState() const;
    void        setState(SystemState newState);
    const char* stateToString(SystemState s) const;
    const char* reportingModeToString(ReportingMode m) const;

    // Adaptive Reporting
    ReportingMode getReportingMode() const;
    void          setReportingMode(ReportingMode mode);
    unsigned long getDynamicPublishInterval() const;

    // Thread-safe energy data exchange (Energy task writes, Comm task reads)
    void           setEnergySnapshot(const EnergySnapshot& snap);
    EnergySnapshot getEnergySnapshot() const;

    void       setSystemDiag(const SystemDiag& diag);
    SystemDiag getSystemDiag() const;

    void           setHistoricalData(const HistoricalData& hist);
    HistoricalData getHistoricalData() const;
    void setGpsSnapshot(const GpsSnapshot& snap);
    GpsSnapshot getGpsSnapshot() const;

    // Day Rollover Detection (HSDS tracking)
    int  getLastRecordedHsds() const;
    void setLastRecordedHsds(int hsds);

    // Connection flags (set by CommManager, read by Deterrence)
    void setWifiConnected(bool v);
    void setMqttConnected(bool v);
    void setMpptConnected(bool v);
    bool isWifiConnected() const;
    bool isMqttConnected() const;
    bool isMpptConnected() const;
    bool isSystemHealthy() const;

    // Urgent publish mechanism (cross-task signal: Energy → Comm)
    void requestUrgentPublish();
    bool consumeUrgentPublish();

    // MPPT disconnect timestamp (millis at which stale was first detected)
    void          setMpptDisconnectTime(unsigned long ms);
    unsigned long getMpptDisconnectTime() const;

    // Device identity (set once at boot)
    String mac;
    uint64_t chipId = 0;

private:
    SystemContext();
    ~SystemContext() = default;
    SystemContext(const SystemContext&) = delete;
    SystemContext& operator=(const SystemContext&) = delete;

    mutable SemaphoreHandle_t _mutex;
    SystemState    _state           = SystemState::BOOT;
    ReportingMode  _reportingMode   = ReportingMode::NORMAL;
    EnergySnapshot _energySnap;
    SystemDiag     _sysDiag;
    HistoricalData _histData;
    GpsSnapshot _gpsSnap;

    int   _lastRecordedHsds   = -1; // Tracked to detect rollover
    bool _wifiOk  = false;
    bool _mqttOk  = false;
    bool _mpptOk  = false;
    bool _urgentPublish       = false;
    unsigned long _mpptDisconnectMs = 0;
};
