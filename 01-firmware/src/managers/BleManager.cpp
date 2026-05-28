#include "BleManager.h"
#include <BLE2902.h>
#include "../config/Config.h"

namespace {
constexpr const char* kUartServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr const char* kUartRxCharUuid  = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr const char* kUartTxCharUuid  = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BleManager* s_bleInstance = nullptr;
}  // namespace

class BleManager::_ServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        (void)pServer;
        if (!s_bleInstance) return;
        s_bleInstance->_connected = true;
        s_bleInstance->_forceNotify = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        (void)pServer;
        if (!s_bleInstance) return;
        s_bleInstance->_connected = false;
        BLEDevice::startAdvertising();
    }
};

class BleManager::_RxCallbacks : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic* pCharacteristic) override {
        if (!s_bleInstance || !pCharacteristic) return;
        std::string value = pCharacteristic->getValue();
        if (value.empty()) return;
        String cmd(value.c_str());
        cmd.trim();
        s_bleInstance->_handleCommand(cmd);
    }
};

void BleManager::begin() {
    if (!BLE_POC_ENABLED || _initialized) return;

    _notifyIntervalMs = _clampInterval(BLE_NOTIFY_INTERVAL_MS);
    BLEDevice::init(BLE_DEVICE_NAME);

    _server = BLEDevice::createServer();
    _service = _server->createService(kUartServiceUuid);
    _txChar = _service->createCharacteristic(
        kUartTxCharUuid,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _txChar->addDescriptor(new BLE2902());

    _rxChar = _service->createCharacteristic(
        kUartRxCharUuid,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );

    s_bleInstance = this;
    _server->setCallbacks(new _ServerCallbacks());
    _rxChar->setCallbacks(new _RxCallbacks());

    _service->start();
    BLEAdvertising* ad = BLEDevice::getAdvertising();
    ad->addServiceUUID(kUartServiceUuid);
    ad->setScanResponse(false);
    ad->setMinPreferred(0x06);
    ad->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    _initialized = true;
    Serial.printf("[BLE] Started (%s), interval=%lu ms\n", BLE_DEVICE_NAME, _notifyIntervalMs);
}

void BleManager::loop() {
    if (!_initialized || !_connected) return;

    unsigned long now = millis();
    if (_forceNotify || (now - _lastNotifyMs >= _notifyIntervalMs)) {
        _publishSnapshot();
        _lastNotifyMs = now;
        _forceNotify = false;
    }
}

void BleManager::_publishSnapshot() {
    if (!_txChar) return;

    auto& ctx = SystemContext::instance();
    auto snap = ctx.getEnergySnapshot();
    auto diag = ctx.getSystemDiag();

    char payload[224];
    int n = snprintf(
        payload,
        sizeof(payload),
        "{\"ts\":%lu,\"st\":\"%s\",\"bv\":%.2f,\"bc\":%.2f,\"soc\":%.1f,\"sv\":%.2f,\"sp\":%.2f,\"mm\":%d}",
        millis(),
        ctx.stateToString(ctx.getState()),
        snap.battery_voltage,
        snap.battery_current,
        snap.battery_soc,
        snap.solar_voltage,
        snap.solar_power,
        diag.mppt_mode
    );

    if (n <= 0) return;
    size_t outLen = static_cast<size_t>(n);
    if (outLen >= sizeof(payload)) outLen = sizeof(payload) - 1;

    _txChar->setValue(reinterpret_cast<uint8_t*>(payload), outLen);
    _txChar->notify();
}

void BleManager::_sendText(const char* txt) {
    if (!_txChar || !txt) return;
    _txChar->setValue(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(txt)), strlen(txt));
    _txChar->notify();
}

void BleManager::_handleCommand(const String& cmd) {
    if (cmd.length() == 0) return;

    String u = cmd;
    u.toUpperCase();

    if (u == "PING") {
        _sendText("{\"ok\":true,\"cmd\":\"PING\"}");
        return;
    }
    if (u == "GET_NOW") {
        _forceNotify = true;
        _sendText("{\"ok\":true,\"cmd\":\"GET_NOW\"}");
        return;
    }
    if (u.startsWith("SET_RATE=")) {
        unsigned long ms = static_cast<unsigned long>(u.substring(9).toInt());
        if (ms == 0) {
            _sendText("{\"ok\":false,\"err\":\"bad_rate\"}");
            return;
        }
        _notifyIntervalMs = _clampInterval(ms);
        char ack[72];
        snprintf(ack, sizeof(ack), "{\"ok\":true,\"cmd\":\"SET_RATE\",\"ms\":%lu}", _notifyIntervalMs);
        _sendText(ack);
        return;
    }

    _sendText("{\"ok\":false,\"err\":\"unknown_cmd\"}");
}

unsigned long BleManager::_clampInterval(unsigned long ms) const {
    if (ms < BLE_NOTIFY_MIN_MS) return BLE_NOTIFY_MIN_MS;
    if (ms > BLE_NOTIFY_MAX_MS) return BLE_NOTIFY_MAX_MS;
    return ms;
}
