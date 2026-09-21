// ═══════════════════════════════════════════════════════════════════════════
//  hal_led.h — WS2812 RGB status LED (single-file inline header)
//
//  Layer: 1 (HAL)
//  Dependencies: config.h, Arduino.h (neopixelWrite built into ESP32 core 3.x)
//
//  Purpose:
//    Visual status for button-triggered (untethered) debug flow.
//    When USB is disconnected, the LED is the only feedback the user has
//    about robot state. Colors chosen to be distinguishable at a glance.
//
//  Color map:
//    BOOT       white   — powering up / gyro calibrating (DO NOT touch robot)
//    READY_FWD  green   — ready, START button will run forward-profile test
//    READY_SPIN blue    — ready, START button will run spin-turn test
//    COUNTDOWN  amber   — countdown running, motion about to start
//    RUNNING    red     — motors active, test in progress
//    DONE       cyan    — test complete, data in RAM (dump via USB 'd')
//    ERROR      magenta — battery low / aborted
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_LED_H
#define HAL_LED_H

#include <Arduino.h>
#include "config.h"

enum LedStatus : uint8_t {
    LED_OFF          = 0,
    LED_BOOT         = 1,   // white
    LED_READY_FWD    = 2,   // green   — Lab 1 forward selected
    LED_READY_SPIN   = 3,   // blue    — spin sequence selected
    LED_READY_SMOOTH = 4,   // purple  — smooth turn selected
    LED_COUNTDOWN    = 5,   // amber
    LED_RUNNING      = 6,   // red
    LED_DONE         = 7,   // cyan
    LED_ERROR        = 8    // magenta
};

// Brightness kept low (0-90 of 255) — onboard WS2812 is very bright up close
inline void hal_led_set(uint8_t status) {
    switch (status) {
        case LED_OFF:          neopixelWrite(PIN_RGB_LED,  0,  0,  0); break;
        case LED_BOOT:         neopixelWrite(PIN_RGB_LED, 50, 50, 50); break; // white
        case LED_READY_FWD:    neopixelWrite(PIN_RGB_LED,  0, 70,  0); break; // green
        case LED_READY_SPIN:   neopixelWrite(PIN_RGB_LED,  0, 15, 80); break; // blue
        case LED_READY_SMOOTH: neopixelWrite(PIN_RGB_LED, 45,  0, 80); break; // purple
        case LED_COUNTDOWN:    neopixelWrite(PIN_RGB_LED, 80, 45,  0); break; // amber
        case LED_RUNNING:      neopixelWrite(PIN_RGB_LED, 90,  0,  0); break; // red
        case LED_DONE:         neopixelWrite(PIN_RGB_LED,  0, 60, 65); break; // cyan
        case LED_ERROR:        neopixelWrite(PIN_RGB_LED, 75,  0, 55); break; // magenta
        default:               neopixelWrite(PIN_RGB_LED,  0,  0,  0); break;
    }
}

inline void hal_led_init(void) {
    hal_led_set(LED_OFF);
}

// Blink helper — toggles between a color and OFF n times (blocking)
inline void hal_led_blink(uint8_t status, uint8_t count, uint16_t period_ms) {
    for (uint8_t i = 0; i < count; i++) {
        hal_led_set(status);
        delay(period_ms / 2);
        hal_led_set(LED_OFF);
        delay(period_ms / 2);
    }
}

#endif // HAL_LED_H
