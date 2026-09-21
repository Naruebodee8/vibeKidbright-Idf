// ═══════════════════════════════════════════════════════════════════════════
//  profile.h  —  Trapezoidal Velocity Profile (port from Peter Harrison's
//                mazerunner-core, adapted for ESP32-S3 + FreeRTOS)
//
//  Design hub spec: Section 12 of Micromouse_2027/index.html
//
//  Key features:
//    - Unit-agnostic: same class for forward (mm) and rotation (deg)
//    - State machine: IDLE → ACCELERATING → BRAKING → FINISHED
//    - Trapezoidal speed profile with dynamic braking distance computation
//    - Thread-safe getters via portMUX_TYPE (cross-task safe)
//    - HW-derived termination threshold (configurable per instance)
//    - Continuous-run support via adjust_position()
//
//  Usage:
//    Profile forward(PROFILE_FWD_THRESHOLD_MM);
//    forward.start(300, 200, 0, 1000);   // 300mm, top=200mm/s, stop, accel=1000mm/s²
//    // controlTask calls forward.update() every LOOP_INTERVAL
//    while (!forward.is_finished()) { ... use forward.position() as setpoint ... }
//
//  Adaptations from Peter (Arduino Uno → ESP32-S3):
//    - ATOMIC{}            → portENTER_CRITICAL / portEXIT_CRITICAL
//    - delay(2)            → vTaskDelay(pdMS_TO_TICKS(2))
//    - Hardcoded 0.125     → HW-derived PROFILE_*_THRESHOLD_* from config.h
//    - Hardcoded 5.0       → PROFILE_FINISH_CREEP_SPEED from config.h
//    - +helpers            → wait_until_position(), wait_until_distance()
// ═══════════════════════════════════════════════════════════════════════════
#ifndef PROFILE_H
#define PROFILE_H

#include <Arduino.h>
#include "config.h"

class Profile {
public:
    enum State : uint8_t {
        PS_IDLE         = 0,
        PS_ACCELERATING = 1,
        PS_BRAKING      = 2,
        PS_FINISHED     = 3,
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Constructor
    // threshold: distance (or angle) below which profile declares FINISHED.
    //   Default = PROFILE_FWD_THRESHOLD_MM. Pass PROFILE_ROT_THRESHOLD_DEG for
    //   rotation profile instance.
    // ─────────────────────────────────────────────────────────────────────────
    Profile(float threshold = PROFILE_FWD_THRESHOLD_MM)
        : m_completion_threshold(threshold) {}

    // ─────────────────────────────────────────────────────────────────────────
    // reset() — clear all state to IDLE
    // ─────────────────────────────────────────────────────────────────────────
    void reset() {
        portENTER_CRITICAL(&m_mux);
        m_position = 0;
        m_speed = 0;
        m_target_speed = 0;
        m_state = PS_IDLE;
        portEXIT_CRITICAL(&m_mux);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // start() — begin a profile
    //   distance: total distance (can be negative for reverse / CCW)
    //   top_speed: peak speed during constant phase (units/s, always positive in spec)
    //   final_speed: speed at completion (0 = stop, top_speed = never-stop continuous)
    //   acceleration: ramp rate (units/s², always positive)
    // ─────────────────────────────────────────────────────────────────────────
    void start(float distance, float top_speed, float final_speed, float acceleration) {
        int8_t sign = (distance < 0) ? -1 : +1;
        if (distance < 0) distance = -distance;

        // Trivial-distance shortcut
        if (distance < m_completion_threshold) {
            portENTER_CRITICAL(&m_mux);
            m_state = PS_FINISHED;
            portEXIT_CRITICAL(&m_mux);
            return;
        }

        if (final_speed > top_speed) final_speed = top_speed;

        float one_over_acc = (acceleration >= 1.0f) ? (1.0f / acceleration) : 1.0f;

        // Commit all fields atomically so update() sees a consistent profile
        portENTER_CRITICAL(&m_mux);
        m_sign            = sign;
        m_position        = 0;
        m_final_position  = distance;
        m_target_speed    = sign * fabsf(top_speed);
        m_final_speed     = sign * fabsf(final_speed);
        m_acceleration    = fabsf(acceleration);
        m_one_over_acc    = one_over_acc;
        m_state           = PS_ACCELERATING;
        portEXIT_CRITICAL(&m_mux);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // move() — start + busy-wait until finished (blocking convenience)
    // ─────────────────────────────────────────────────────────────────────────
    void move(float distance, float top_speed, float final_speed, float acceleration) {
        start(distance, top_speed, final_speed, acceleration);
        wait_until_finished();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // stop() — abort: target_speed=0, then finish()
    // Note: doesn't physically stop the robot. Downstream PD/motor controller
    // ramps speed → 0 based on the new target_speed.
    // ─────────────────────────────────────────────────────────────────────────
    void stop() {
        portENTER_CRITICAL(&m_mux);
        m_target_speed = 0;
        portEXIT_CRITICAL(&m_mux);
        finish();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // finish() — force state = FINISHED, snap speed to target_speed
    // ─────────────────────────────────────────────────────────────────────────
    void finish() {
        portENTER_CRITICAL(&m_mux);
        m_speed = m_target_speed;
        m_state = PS_FINISHED;
        portEXIT_CRITICAL(&m_mux);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // update() — advance one tick. Called from controlTask @ LOOP_FREQUENCY Hz.
    // SINGLE WRITER: only controlTask calls this.
    // ─────────────────────────────────────────────────────────────────────────
    void update() {
        // Only IDLE returns early. FINISHED still ramps speed → 0 and stops
        // position naturally. Returning early in FINISHED would freeze the
        // setpoint at non-zero values and cause downstream runaway (A6 lesson).
        if (m_state == PS_IDLE) return;

        float delta_v   = m_acceleration * LOOP_INTERVAL;
        float remaining = fabsf(m_final_position) - fabsf(m_position);

        // Phase transition: ACCELERATING → BRAKING
        if (m_state == PS_ACCELERATING) {
            if (remaining < get_braking_distance()) {
                m_state = PS_BRAKING;
                if (m_final_speed == 0) {
                    // Magic creep speed: prevents getting stuck at speed=0
                    // before reaching threshold. See R4 in design hub.
                    m_target_speed = m_sign * PROFILE_FINISH_CREEP_SPEED;
                } else {
                    m_target_speed = m_final_speed;
                }
            }
        }

        // Ramp speed toward target (one-sided, with clamping)
        if (m_speed < m_target_speed) {
            m_speed += delta_v;
            if (m_speed > m_target_speed) m_speed = m_target_speed;
        }
        if (m_speed > m_target_speed) {
            m_speed -= delta_v;
            if (m_speed < m_target_speed) m_speed = m_target_speed;
        }

        // Integrate position
        m_position += m_speed * LOOP_INTERVAL;

        // Termination
        if (m_state != PS_FINISHED && remaining < m_completion_threshold) {
            portENTER_CRITICAL(&m_mux);
            m_state = PS_FINISHED;
            m_target_speed = m_final_speed;
            portEXIT_CRITICAL(&m_mux);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Getters (thread-safe via portMUX)
    // ─────────────────────────────────────────────────────────────────────────
    bool is_finished() {
        bool f;
        portENTER_CRITICAL(&m_mux);
        f = (m_state == PS_FINISHED);
        portEXIT_CRITICAL(&m_mux);
        return f;
    }

    float position() {
        float p;
        portENTER_CRITICAL(&m_mux);
        p = m_position;
        portEXIT_CRITICAL(&m_mux);
        return p;
    }

    float speed() {
        float s;
        portENTER_CRITICAL(&m_mux);
        s = m_speed;
        portEXIT_CRITICAL(&m_mux);
        return s;
    }

    float acceleration() const {
        return m_acceleration;
    }

    uint8_t state() {
        uint8_t s;
        portENTER_CRITICAL(&m_mux);
        s = m_state;
        portEXIT_CRITICAL(&m_mux);
        return s;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // get_braking_distance()
    //   Distance required to ramp from current speed → final speed at current accel.
    //   Useful for triggering decisions (e.g., "start sensing turn 50mm before braking").
    // ─────────────────────────────────────────────────────────────────────────
    float get_braking_distance() const {
        return fabsf(m_speed * m_speed - m_final_speed * m_final_speed) * 0.5f * m_one_over_acc;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Setters (use carefully — bypass profile generator)
    // ─────────────────────────────────────────────────────────────────────────
    void set_speed(float s) {
        portENTER_CRITICAL(&m_mux);
        m_speed = s;
        portEXIT_CRITICAL(&m_mux);
    }

    void set_target_speed(float s) {
        portENTER_CRITICAL(&m_mux);
        m_target_speed = s;
        portEXIT_CRITICAL(&m_mux);
    }

    void set_position(float p) {
        portENTER_CRITICAL(&m_mux);
        m_position = p;
        portEXIT_CRITICAL(&m_mux);
    }

    // adjust_position(delta) — relative shift, used for continuous-run cell-frame reset
    //   e.g., adjust_position(-FULL_CELL) after crossing cell boundary
    void adjust_position(float delta) {
        portENTER_CRITICAL(&m_mux);
        m_position += delta;
        portEXIT_CRITICAL(&m_mux);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Wait helpers (FreeRTOS-friendly: yield via vTaskDelay)
    // ─────────────────────────────────────────────────────────────────────────
    void wait_until_finished() {
        while (!is_finished()) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    void wait_until_position(float target) {
        while (position() < target) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    void wait_until_distance(float distance) {
        float start_pos = position();
        while (position() < start_pos + distance) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

private:
    portMUX_TYPE m_mux = portMUX_INITIALIZER_UNLOCKED;

    volatile uint8_t m_state         = PS_IDLE;
    volatile float   m_speed         = 0;
    volatile float   m_position      = 0;
    int8_t           m_sign          = 1;
    float            m_acceleration  = 0;
    float            m_one_over_acc  = 1;
    float            m_target_speed  = 0;
    float            m_final_speed   = 0;
    float            m_final_position = 0;
    float            m_completion_threshold;
};

#endif // PROFILE_H
