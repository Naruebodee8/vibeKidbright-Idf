// ═══════════════════════════════════════════════════════════════════════════
//  ble_debug.h — Minimal BLE wireless telemetry (Nordic UART Service)
//
//  Layer: 2 (glue)
//  Dependencies: config.h, ESP32 built-in BLE (BLEDevice.h)
//
//  WHY BLE (not WiFi)
//    The Sharp IR sensors sit on GPIO 11-14 = ADC2. WiFi hard-locks ADC2 →
//    analogRead garbage. BLE does NOT: Project 1 ran Sharp-on-ADC2 with BLE
//    on the whole time and still mapped mazes — proof BLE + ADC2 coexist on
//    this hardware. This module is the first increment toward a full wireless
//    debug dashboard; for now it streams telemetry text only.
//
//  SERVICE
//    Nordic UART Service (NUS) — recognised by generic BLE terminal apps
//    (nRF Connect, "Serial Bluetooth Terminal" in BLE mode, Web Bluetooth).
//    TX characteristic NOTIFYs telemetry; RX characteristic accepts writes
//    (logged to USB only for now — commands arrive in the next increment).
//
//  PUBLIC API
//    ble_debug_init(name)     start the stack + advertise   (call on demand)
//    ble_debug_stop()         deinit — frees the radio, ADC2 fully clean
//    ble_debug_is_on()        stack started?
//    ble_debug_is_connected() a central is connected?
//    ble_debug_send(text)     NOTIFY a line (no-op if not connected)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef BLE_DEBUG_H
#define BLE_DEBUG_H

#include <Arduino.h>
#include "config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Nordic UART Service UUIDs (standard — do not change; apps key off these)
#define BLE_NUS_SERVICE  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_NUS_RX_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // phone → robot
#define BLE_NUS_TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // robot → phone

// ─────────────────────────────────────────────────────────────────────────────
// State
// ─────────────────────────────────────────────────────────────────────────────
inline bool              _ble_on        = false;
inline volatile bool     _ble_connected = false;
inline BLEServer*        _ble_server    = nullptr;
inline BLECharacteristic* _ble_tx       = nullptr;
inline uint32_t          _ble_tx_count  = 0;
inline char              _ble_rx_buf[64] = {0};  // accumulates the chars written
inline volatile int      _ble_rx_len     = 0;    // chars in the buffer so far
inline volatile bool     _ble_rx_eol     = false;// a newline has been received
inline volatile uint32_t _ble_rx_last    = 0;    // millis() of the last write

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────
class _BleSrvCb : public BLEServerCallbacks {
    void onConnect(BLEServer* /*s*/) override {
        _ble_connected = true;
    }
    void onDisconnect(BLEServer* s) override {
        _ble_connected = false;
        s->startAdvertising();          // let the central reconnect
    }
};

class _BleRxCb : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        // APPEND the written chars to the line buffer. Some BLE terminal apps
        // send a multi-char command one character per write — appending (not
        // overwriting) + a newline/idle flush in ble_debug_take_line() lets a
        // path like "FRF" arrive whole regardless of how the app chunks it.
        String v = c->getValue();
        for (size_t i = 0; i < v.length(); i++) {
            char ch = v[i];
            if (ch == '\n' || ch == '\r') {
                _ble_rx_eol = true;
            } else if (ch >= ' ' && ch < 127 &&          // KEEP space (0x20): commands
                       _ble_rx_len < (int)sizeof(_ble_rx_buf) - 1) {  // like "kp 1.5", "60 80"
                _ble_rx_buf[_ble_rx_len++] = ch;
            }
        }
        _ble_rx_last = millis();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
// Final advertised name = "<name>_<chipID>" so 20 robots in one room never
// collide. The suffix is the low 24 bits of the ESP32 eFuse MAC (unique/board).
inline char _ble_adv_name[40] = {0};
inline const char* ble_debug_name(void) { return _ble_adv_name; }

inline bool ble_debug_init(const char* name) {
    if (_ble_on) return true;

    uint32_t id = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
    snprintf(_ble_adv_name, sizeof(_ble_adv_name), "%s_%06X", name, id);
    Serial.printf("\n========================================\n");
    Serial.printf("  THIS ROBOT'S BLE NAME:  %s\n", _ble_adv_name);
    Serial.printf("  (เลือกชื่อนี้ตอนสแกนใน mm_ui.py)\n");
    Serial.printf("========================================\n");

    BLEDevice::init(_ble_adv_name);
    BLEDevice::setMTU(185);             // allow a full telemetry line per notify

    _ble_server = BLEDevice::createServer();
    _ble_server->setCallbacks(new _BleSrvCb());

    BLEService* svc = _ble_server->createService(BLE_NUS_SERVICE);

    _ble_tx = svc->createCharacteristic(
        BLE_NUS_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    _ble_tx->addDescriptor(new BLE2902());

    BLECharacteristic* rx = svc->createCharacteristic(
        BLE_NUS_RX_UUID, BLECharacteristic::PROPERTY_WRITE);
    rx->setCallbacks(new _BleRxCb());

    svc->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_NUS_SERVICE);
    adv->setScanResponse(true);
    BLEDevice::startAdvertising();

    _ble_on = true;
    return true;
}

inline void ble_debug_stop(void) {
    if (!_ble_on) return;
    BLEDevice::deinit(true);            // release the radio fully
    _ble_on        = false;
    _ble_connected = false;
    _ble_server    = nullptr;
    _ble_tx        = nullptr;
}

inline bool     ble_debug_is_on(void)        { return _ble_on; }
inline bool     ble_debug_is_connected(void) { return _ble_connected; }
inline uint32_t ble_debug_tx_count(void)     { return _ble_tx_count; }

// Pull a complete line written by a BLE central into `out`. A line counts as
// complete when a newline was received, OR the input has been idle ~500 ms.
// The newline is the primary signal (clients append '\n'); the long idle
// window only exists for clients that send no newline, and is generous enough
// that BLE link-layer chunking of one message never splits it. Returns false
// if no complete line is waiting yet.
inline bool ble_debug_take_line(char* out, int maxlen) {
    if (_ble_rx_len == 0) { _ble_rx_eol = false; return false; }
    if (!_ble_rx_eol && (millis() - _ble_rx_last) < 500) return false;
    int i = 0;
    for (; i < maxlen - 1 && i < _ble_rx_len; i++) out[i] = _ble_rx_buf[i];
    out[i] = 0;
    _ble_rx_len = 0;
    _ble_rx_eol = false;
    return true;
}

// Discard any pending RX input.
inline void ble_debug_flush_rx(void) { _ble_rx_len = 0; _ble_rx_eol = false; }

// NOTIFY one line. No-op unless a central is connected.
inline void ble_debug_send(const char* s) {
    if (!_ble_on || !_ble_connected || _ble_tx == nullptr) return;
    _ble_tx->setValue((uint8_t*)s, strlen(s));
    _ble_tx->notify();
    _ble_tx_count++;
}

#endif // BLE_DEBUG_H
