// ═══════════════════════════════════════════════════════════════════════════
//  control_task.h — FreeRTOS control loop (500Hz on core 1)
//
//  Layer: 2 (Control — glue)
//  Dependencies: profile, hal_encoder, hal_imu, hal_battery, motors_controller
//
//  Architecture:
//    FreeRTOS task pinned to core 1, priority high, period 2ms (500Hz)
//    Uses vTaskDelayUntil for periodic scheduling (simpler than esp_timer).
//
//  Each tick (in _ctl_tick):
//    1. encoder_update() + imu_update()
//    2. forward_profile.update() + rotation_profile.update()
//    3. measure deltas:
//         fwd_change = (L_dist + R_dist)/2 - prev_fwd        ← encoder
//         rot_change = gyro_z_dps × LOOP_INTERVAL            ← gyro (D15)
//    4. battery critical check → disarm if low
//    5. motors_update(profile.speed, profile.omega, fwd_change, rot_change, steering)
//    6. update telemetry snapshot
//
//  Heading source (D15):
//    Forward (mm): encoder differential (no other choice — only encoder measures distance)
//    Rotation (deg): gyro Z rate × dt (no slip artifacts, accurate per-tick)
//    Long-term drift correction via fusion: deferred — if observed in field tests
//
//  Safety:
//    - Battery critical → auto disarm + motors off
//    - control_arm() must be called before motors will turn — boot defaults disarmed
//    - control_abort() force-stops profiles, disables PWM
//    - Profile finished by default → no motion at boot
// ═══════════════════════════════════════════════════════════════════════════
#ifndef CONTROL_TASK_H
#define CONTROL_TASK_H

#include <Arduino.h>
#include "config.h"
#include "profile.h"
#include "hal_encoder.h"
#include "hal_imu.h"
#include "hal_battery.h"
#include "motors_controller.h"

// ─────────────────────────────────────────────────────────────────────────────
// Global profiles — exposed for direct access where needed
// ─────────────────────────────────────────────────────────────────────────────
inline Profile forward_profile  (PROFILE_FWD_THRESHOLD_MM);
inline Profile rotation_profile (PROFILE_ROT_THRESHOLD_DEG);

// ─────────────────────────────────────────────────────────────────────────────
// Internal state
// ─────────────────────────────────────────────────────────────────────────────
inline TaskHandle_t _ctl_task_handle = nullptr;
inline volatile bool     _ctl_armed     = false;
inline volatile uint32_t _ctl_tick_count = 0;
inline volatile uint32_t _ctl_overrun_count = 0;

// Previous encoder reading for delta computation
inline float _ctl_prev_fwd_mm = 0;

// Absolute encoder distance captured at arm — telemetry reports forward travel
// RELATIVE to this origin so each run starts at actual=0 (the controller itself
// uses per-tick deltas and is unaffected; this only fixes the displayed value).
inline float _ctl_fwd_origin = 0;

// Encoder glitch counter — ticks where an impossible fwd delta was rejected.
inline volatile uint32_t _ctl_glitch_count = 0;

// Spin-turn startup kick (closed-loop: punch past stiction, release on gyro
// breakaway). pwm sign: + = CCW (left). See control_kick_rotation().
inline volatile uint16_t _ctl_kick_ticks   = 0;
inline volatile int16_t  _ctl_kick_pwm     = 0;
inline volatile float    _ctl_kick_release = 0;

// Steering adjustment from wall follower (test_06_v2 will set this)
inline volatile float _ctl_steering_adj = 0.0f;

// Heading accumulator — integrates gyro Z to track absolute rotation (deg).
// Used by spin-turn test to compare commanded angle vs actual.
inline volatile float _ctl_heading_deg = 0;

// Telemetry snapshot (read by main loop / debug)
inline volatile float _ctl_last_v_setpoint     = 0;
inline volatile float _ctl_last_omega_setpoint = 0;
inline volatile float _ctl_last_fwd_setpoint   = 0;   // forward profile.position()
inline volatile float _ctl_last_rot_setpoint   = 0;   // rotation profile.position()
inline volatile float _ctl_last_fwd_actual     = 0;   // encoder fwd avg
inline volatile float _ctl_last_gyro_dps       = 0;

// ─────────────────────────────────────────────────────────────────────────────
// One control tick — called from task body every 2ms
// ─────────────────────────────────────────────────────────────────────────────
static void _ctl_tick(void) {
    // 1. Update sensors
    encoder_update();
    imu_update();

    // 2. Advance profiles (generate next setpoint)
    forward_profile.update();
    rotation_profile.update();

    // 3. Measure deltas — with encoder-glitch sanity filter.
    // The encoder ISR can occasionally yield an impossible delta (count
    // overflow / missed-edge / pin glitch) → a huge fwd_change spike that makes
    // the PID slam (the "value jumps to thousands of mm" symptom). A real wheel
    // does < ~5mm/tick at 500Hz; 20mm/tick = 10 m/s = generous ceiling. On a
    // glitch: feed 0 (no bogus position jump) but still advance the baseline.
    float fwd_now = 0.5f * (encoder_get_distance_mm(ENCODER_LEFT)
                          + encoder_get_distance_mm(ENCODER_RIGHT));
    float fwd_change_raw = fwd_now - _ctl_prev_fwd_mm;
    float fwd_change;
    if (fabsf(fwd_change_raw) > 20.0f) { fwd_change = 0; _ctl_glitch_count++; }
    else                               { fwd_change = fwd_change_raw; }
    _ctl_prev_fwd_mm = fwd_now;

    float gyro_z_dps = imu_get_gyro_z();
    float rot_change = gyro_z_dps * LOOP_INTERVAL;

    // Integrate heading (absolute rotation since last arm)
    _ctl_heading_deg += rot_change;

    // 4. Battery safety
    if (hal_battery_is_critical()) {
        motors_disable_controllers();
        _ctl_armed = false;
        forward_profile.stop();
        rotation_profile.stop();
    }

    // 5. Drive motor controller (math + PWM output)
    float v     = forward_profile.speed();
    float omega = rotation_profile.speed();
    motors_update(v, omega, fwd_change, rot_change, _ctl_steering_adj);

    // 5b. Spin-turn startup kick — override output with a near-full-PWM pulse so
    //     stiction breaks; release the instant the gyro confirms rotation.
    if (_ctl_kick_ticks > 0 && _ctl_armed) {
        _ctl_kick_ticks--;
        if (fabsf(gyro_z_dps) > _ctl_kick_release) _ctl_kick_ticks = 0;  // breakaway → hand off
        else motor_set_speed(-_ctl_kick_pwm, _ctl_kick_pwm);
    }

    // 6. Snapshot telemetry
    _ctl_last_v_setpoint     = v;
    _ctl_last_omega_setpoint = omega;
    _ctl_last_fwd_setpoint   = forward_profile.position();
    _ctl_last_rot_setpoint   = rotation_profile.position();
    _ctl_last_fwd_actual     = fwd_now - _ctl_fwd_origin;   // travel since arm
    _ctl_last_gyro_dps       = gyro_z_dps;

    _ctl_tick_count++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Task body — periodic loop at 500Hz
// ─────────────────────────────────────────────────────────────────────────────
static void _ctl_task_body(void* arg) {
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(2);  // 2ms = 500Hz (assumes FreeRTOS tick 1000Hz)

    while (true) {
        _ctl_tick();

        // vTaskDelayUntil — wait until next 2ms boundary
        // If tick took longer than 2ms, this returns immediately (no catch-up)
        TickType_t now = xTaskGetTickCount();
        if ((now - last_wake) > period) {
            _ctl_overrun_count++;
            last_wake = now;  // resync after overrun
        }
        vTaskDelayUntil(&last_wake, period);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

// Start the control task. Call ONCE after all HAL inits in setup().
// Motors remain DISARMED until control_arm() is called.
inline void control_task_start(void) {
    // Reset all controllers/profiles to known state
    motors_reset_controllers();
    forward_profile.reset();
    rotation_profile.reset();

    // Capture baseline encoder reading (for delta)
    encoder_update();
    _ctl_prev_fwd_mm = 0.5f * (encoder_get_distance_mm(ENCODER_LEFT)
                              + encoder_get_distance_mm(ENCODER_RIGHT));
    _ctl_fwd_origin  = _ctl_prev_fwd_mm;

    // Spawn task on core 1 with high priority
    xTaskCreatePinnedToCore(
        _ctl_task_body,
        "controlTask",
        4096,             // stack size
        nullptr,          // arg
        20,               // priority (high — Arduino loop is 1)
        &_ctl_task_handle,
        1                 // pinned to core 1
    );
}

// Arm motors: clear errors + capture baseline + enable PWM output
// Call before starting a profile.
inline void control_arm(void) {
    motors_reset_controllers();
    encoder_update();
    _ctl_prev_fwd_mm = 0.5f * (encoder_get_distance_mm(ENCODER_LEFT)
                              + encoder_get_distance_mm(ENCODER_RIGHT));
    _ctl_fwd_origin  = _ctl_prev_fwd_mm;   // zero the per-run actual reference
    forward_profile.reset();
    rotation_profile.reset();
    _ctl_steering_adj = 0;
    _ctl_heading_deg  = 0;     // zero heading reference at arm
    _ctl_kick_ticks = 0; _ctl_kick_pwm = 0;   // clear any stale kick from a prior run
    motors_enable_controllers();
    _ctl_armed = true;
}

// Disarm: stop PWM output (math keeps running). Profiles also stopped.
inline void control_disarm(void) {
    forward_profile.stop();
    rotation_profile.stop();
    motors_disable_controllers();
    _ctl_armed = false;
}

// Force-stop immediately (for emergency abort)
inline void control_abort(void) {
    forward_profile.stop();
    rotation_profile.stop();
    motor_stop();
    motors_disable_controllers();
    _ctl_armed = false;
}

// Start a forward profile (non-blocking — control task tracks it)
inline void control_start_forward(float distance_mm, float top_mmps,
                                  float final_mmps, float accel_mmps2) {
    forward_profile.start(distance_mm, top_mmps, final_mmps, accel_mmps2);
}

// Start a rotation profile (non-blocking)
inline void control_start_rotation(float angle_deg, float top_dps,
                                   float final_dps, float alpha_dps2) {
    rotation_profile.start(angle_deg, top_dps, final_dps, alpha_dps2);
}

// Fire a closed-loop spin-turn startup kick (call right after control_start_rotation):
// punch at `pwm` (sign + = CCW) until |gyro| > release_dps or max_ms elapses.
inline void control_kick_rotation(int16_t pwm, uint16_t max_ms, float release_dps) {
    _ctl_kick_pwm     = pwm;
    _ctl_kick_release = release_dps;
    _ctl_kick_ticks   = max_ms / 2;   // control tick = 2ms
}

// True when NEITHER profile is actively running.
// A profile counts as "not blocking" if IDLE (never started) or FINISHED.
// Only ACCELERATING / BRAKING count as active — this lets a single-axis
// move (e.g. spin only, forward IDLE) report done correctly.
inline bool control_is_motion_done(void) {
    uint8_t fs = forward_profile.state();
    uint8_t rs = rotation_profile.state();
    bool fwd_active = (fs == Profile::PS_ACCELERATING || fs == Profile::PS_BRAKING);
    bool rot_active = (rs == Profile::PS_ACCELERATING || rs == Profile::PS_BRAKING);
    return !fwd_active && !rot_active;
}

// Block until both profiles done or timeout (0 = no timeout)
inline bool control_wait_for_motion(uint32_t timeout_ms = 0) {
    uint32_t start = millis();
    while (!control_is_motion_done()) {
        if (timeout_ms > 0 && (millis() - start > timeout_ms)) return false;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return true;
}

// Set steering correction (called by wall follower in test_06_v2)
inline void control_set_steering(float adj_deg) {
    _ctl_steering_adj = adj_deg;
}

// Status getters
inline bool     control_is_armed()         { return _ctl_armed; }
inline uint32_t control_get_tick_count()   { return _ctl_tick_count; }
inline uint32_t control_get_overrun_count(){ return _ctl_overrun_count; }
inline float    control_get_v_setpoint()   { return _ctl_last_v_setpoint; }
inline float    control_get_omega_setpoint(){ return _ctl_last_omega_setpoint; }
inline float    control_get_fwd_setpoint() { return _ctl_last_fwd_setpoint; }
inline float    control_get_rot_setpoint() { return _ctl_last_rot_setpoint; }
inline float    control_get_fwd_actual()   { return _ctl_last_fwd_actual; }
inline float    control_get_gyro_dps()     { return _ctl_last_gyro_dps; }
inline float    control_get_heading()      { return _ctl_heading_deg; }

// Position error (setpoint - actual) — useful telemetry
inline float    control_get_fwd_error()    {
    return _ctl_last_fwd_setpoint - _ctl_last_fwd_actual;
}

#endif // CONTROL_TASK_H
