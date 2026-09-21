// ═══════════════════════════════════════════════════════════════════════════
//  hal_battery.h — Battery Monitoring HAL (single-file inline header)
//
//  Layer: 1 (HAL)
//  Dependencies: config.h, Arduino.h
//
//  Ported from Project1 hal_battery.cpp/.h (proven against multimeter).
//  Single-file inline style to match test_05_v2 conventions.
//
//  Design:
//    - ADC reading via analogReadMilliVolts() — handles ESP32-S3 ADC
//      calibration internally (core 3.x). No manual eFuse calibration needed.
//    - Voltage divider scaling: V_bat = V_adc × (BATTERY_DIVIDER_X100 / 100)
//    - EMA filter (α ≈ 0.10) for stable readings
//    - Integer math (mV) — float reserved for caller convenience
//
//  Usage in controlTask (A6):
//    hal_battery_update();    // every 100ms or so
//    float vbat = hal_battery_volts();
//    int pwm = MAX_PWM × desired_voltage / vbat;
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_BATTERY_H
#define HAL_BATTERY_H

#include <Arduino.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Status levels
// ─────────────────────────────────────────────────────────────────────────────
enum BatteryLevel : uint8_t {
    BATT_LEVEL_CRITICAL = 0,    // < BATT_CRITICAL_MV — stop motors
    BATT_LEVEL_LOW      = 1,    // < BATT_LOW_MV — warn (yellow)
    BATT_LEVEL_NOMINAL  = 2,    // < BATT_FULL_MV — green
    BATT_LEVEL_FULL     = 3     // >= BATT_FULL_MV — fresh charge
};

// ─────────────────────────────────────────────────────────────────────────────
// State (inline — header-only style, C++17)
// ─────────────────────────────────────────────────────────────────────────────
inline uint16_t _battery_avg_mV = 0;
inline bool     _battery_first  = true;

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

// Read single instantaneous voltage in mV. ~50μs.
inline uint16_t hal_battery_read_mV(void) {
    // analogReadMilliVolts handles ADC reference + attenuation calibration
    // Returns voltage at PIN (0-3300 mV after attenuation)
    uint32_t v_adc_mV = analogReadMilliVolts(PIN_BATTERY_ADC);

    // Scale up by voltage divider ratio
    return (uint16_t)((v_adc_mV * BATTERY_DIVIDER_X100) / 100);
}

// Initialize ADC pin + prime EMA filter
// Fix for ESP32-S3 ADC warmup:
//   First read(s) after analogSetPinAttenuation may return invalid data.
//   Discard 3 reads, then average 8 reads to seed filter — gives stable start.
inline void hal_battery_init(void) {
    analogReadResolution(ADC_RESOLUTION_BITS);

    // 11dB attenuation → ~0-3.3V input range
    // Use per-pin set (global analogSetAttenuation may not work in core 3.x)
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
    pinMode(PIN_BATTERY_ADC, INPUT);
    delay(5);   // let ADC settle after config change

    // Warmup: discard first 3 reads (may be invalid after config change)
    for (int i = 0; i < 3; i++) {
        (void)analogReadMilliVolts(PIN_BATTERY_ADC);
        delay(2);
    }

    // Prime filter with average of 8 valid reads (smooth start, no EMA warmup)
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += hal_battery_read_mV();
        delay(2);
    }
    _battery_avg_mV = (uint16_t)(sum / 8);
    _battery_first  = false;
}

// Update EMA filter — call periodically (10-100 Hz)
//   avg = (α × raw + (256-α) × avg) / 256
//   At α=26/256 ≈ 0.10: time constant ≈ 10 samples
//
// ROBUSTNESS: ESP32-S3 analogReadMilliVolts occasionally returns 0 right after
// boot (ADC warmup race with BLE init). The init's 8-read seed sometimes
// captures all zeros, leaving _battery_avg_mV stuck at 0 (= "battery shows 0"
// symptom seen during testing). We now:
//   1) SKIP raw readings < 1000 mV (no real battery is this low)
//   2) If the cached EMA is invalid (< 1000 mV) but a valid reading arrives,
//      SNAP to it instead of EMA-converging from 0 over 50 samples
inline void hal_battery_update(void) {
    uint16_t raw = hal_battery_read_mV();
    if (raw < 1000) return;          // invalid ADC read — keep last good value

    if (_battery_first || _battery_avg_mV < 1000) {
        _battery_avg_mV = raw;        // first read, or recover from stuck-0
        _battery_first  = false;
    } else {
        uint32_t a = BATT_EMA_ALPHA_X256;
        _battery_avg_mV = (uint16_t)((a * raw + (256 - a) * _battery_avg_mV) / 256);
    }
}

// Get filtered voltage (last hal_battery_update result)
inline uint16_t hal_battery_get_avg_mV(void) {
    return _battery_avg_mV;
}

// Convenience: filtered voltage in Volts (for motors_controller use)
inline float hal_battery_volts(void) {
    return _battery_avg_mV * 0.001f;
}

// Battery level enum (FULL / NOMINAL / LOW / CRITICAL)
inline uint8_t hal_battery_get_level(void) {
    uint16_t v = _battery_avg_mV;
    if (v >= BATT_FULL_MV)     return BATT_LEVEL_FULL;
    if (v >= BATT_LOW_MV)      return BATT_LEVEL_NOMINAL;
    if (v >= BATT_CRITICAL_MV) return BATT_LEVEL_LOW;
    return BATT_LEVEL_CRITICAL;
}

// True if battery below critical threshold (controlTask should disable motors)
inline bool hal_battery_is_critical(void) {
    return (_battery_avg_mV < BATT_CRITICAL_MV);
}

// Human-readable level name (for debug print)
inline const char* hal_battery_level_name(uint8_t lvl) {
    switch (lvl) {
        case BATT_LEVEL_FULL:     return "FULL";
        case BATT_LEVEL_NOMINAL:  return "NOMINAL";
        case BATT_LEVEL_LOW:      return "LOW";
        case BATT_LEVEL_CRITICAL: return "CRITICAL";
        default:                  return "?";
    }
}

#endif // HAL_BATTERY_H
