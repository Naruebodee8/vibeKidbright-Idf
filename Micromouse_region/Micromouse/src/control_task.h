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
// RELATIVE to this origin so each run/cell starts at actual=0 (the controller
// uses per-tick deltas and is unaffected; this only fixes the displayed value).
inline float _ctl_fwd_origin = 0;

// Steering adjustment from wall follower (test_06_v2 will set this)
inline volatile float _ctl_steering_adj = 0.0f;

// Manual PWM override — for closed-loop sensor-feedback corrections (e.g.
// fw_square_heading) where the FF bias step (FF_BIAS_START ≈ 4.5V) launches
// the robot far faster than commanded for tiny profile angles, blowing past
// the gyro's ±250 dps full scale and making fine 1-5° rotations impossible
// through the profile path. When _ctl_manual_on, _ctl_tick writes these PWM
// values to the motors AFTER motors_update has run, suppressing the
// controller's output. Cleared with control_clear_manual_pwm().
inline volatile bool    _ctl_manual_on    = false;
inline volatile int16_t _ctl_manual_pwm_l = 0;
inline volatile int16_t _ctl_manual_pwm_r = 0;

// Spin-turn startup kick — overrides the controller's motor output with a
// near-full-PWM pulse to break static friction. The profile controller can
// sit just under breakaway torque at a standstill, so an in-place spin
// sometimes fails to start.
//
// The kick is CLOSED-LOOP: it punches only until the gyro confirms the robot
// is rotating (|gyro_z| > _ctl_kick_release), then hands straight back to the
// controller. A fixed-length kick is dangerous — punch too long and the spin
// blows past the gyro's ±250 dps full-scale, the gyro rails, and the
// controller goes blind and cannot brake. Releasing on first motion keeps the
// spin well inside the gyro range. _ctl_kick_ticks is only a safety timeout.
// _ctl_kick_pwm sign convention: positive = CCW (left turn).
inline volatile uint16_t _ctl_kick_ticks   = 0;   // remaining timeout ticks
inline volatile int16_t  _ctl_kick_pwm     = 0;   // signed kick PWM
inline volatile float    _ctl_kick_release = 0;   // |gyro| dps that ends kick

// When true, _ctl_tick skips imu_update() — lets the main loop run an IMU
// gyro re-calibration without colliding with this task on the I2C bus.
inline volatile bool _ctl_imu_suspend = false;

// Heading accumulator — integrates gyro Z to track absolute rotation (deg).
// Used by spin-turn test to compare commanded angle vs actual.
inline volatile float _ctl_heading_deg = 0;

// Gyro heading-hold — runs at 500Hz inside _ctl_tick (vs the old 100Hz in
// doForward) so it catches fast startup veer with no update lag. When on, the
// controller adds -(heading - target) * kp * dt to the steering injection
// every control tick. Enable during forward moves; DISABLE during spins (they
// change heading on purpose).
inline volatile bool  _ctl_hold_on     = false;
inline volatile float _ctl_hold_target = 0.0f;
inline volatile float _ctl_hold_kp     = 6.0f;   // deg/s per deg of drift

// Telemetry snapshot (read by main loop / debug)
inline volatile float _ctl_last_v_setpoint     = 0;
inline volatile float _ctl_last_omega_setpoint = 0;
inline volatile float _ctl_last_fwd_setpoint   = 0;   // forward profile.position()
inline volatile float _ctl_last_rot_setpoint   = 0;   // rotation profile.position()
inline volatile float _ctl_last_fwd_actual     = 0;   // encoder fwd avg
inline volatile float _ctl_last_gyro_dps       = 0;

// Encoder glitch counter — incremented each tick the per-tick fwd delta is
// rejected as physically impossible (encoder ISR / pin glitch).
inline volatile uint32_t _ctl_glitch_count = 0;

// ─────────────────────────────────────────────────────────────────────────────
// One control tick — called from task body every 2ms
// ─────────────────────────────────────────────────────────────────────────────
static void _ctl_tick(void) {
    // 1. Update sensors  (IMU read skipped while a re-calibration is running)
    encoder_update();
    if (!_ctl_imu_suspend) imu_update();

    // 2. Advance profiles (generate next setpoint)
    forward_profile.update();
    rotation_profile.update();

    // 3. Measure deltas — with encoder-glitch sanity filter
    //
    // The encoder ISR occasionally yields an impossible delta (~1300mm in
    // a single 2ms tick observed in CSV logs) — likely count-overflow,
    // missed-edge, or pin-glitch artefact. At 500Hz a real wheel maxes out
    // around ~5mm/tick (motors ~150 RPM × 36mm wheel ≈ 280 mm/s tangent).
    // 20mm/tick = 10 m/s = generous physical ceiling.
    //
    // When a glitch fires:
    //   • Controller sees fwd_change = 0 (no position update this tick), so
    //     the position PD does NOT command a huge corrective REVERSE — that
    //     was the "robot walks backward then stalls" symptom in the logs.
    //   • Baseline IS advanced to the new reading so subsequent ticks
    //     compute deltas from where we are now (we don't get stuck rejecting
    //     every tick forever).
    float fwd_now = 0.5f * (encoder_get_distance_mm(ENCODER_LEFT)
                          + encoder_get_distance_mm(ENCODER_RIGHT));
    float fwd_change_raw = fwd_now - _ctl_prev_fwd_mm;
    float fwd_change;
    if (fabsf(fwd_change_raw) > 20.0f) {
        fwd_change = 0;
        _ctl_glitch_count++;
    } else {
        fwd_change = fwd_change_raw;
    }
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

    // Steering injection = wall-CTE term (set at ~100Hz from doForward) PLUS
    // a gyro heading-hold term computed HERE at the full 500Hz. The heading
    // hold runs in the control loop (not the 100Hz motion loop) so it reacts
    // to a fast startup veer with no update lag — at 188 dps a 10ms lag let
    // ~2°/sample of veer accumulate before correction. _ctl_steering_adj is
    // the wall part; we add the gyro part on top per-tick.
    float steer_adj = _ctl_steering_adj;
    if (_ctl_hold_on) {
        float hdrift = _ctl_heading_deg - _ctl_hold_target;
        steer_adj += -hdrift * _ctl_hold_kp * LOOP_INTERVAL;
    }
    motors_update(v, omega, fwd_change, rot_change, steer_adj);

    // 5b. Spin-turn startup kick — override the controller's output with a
    //     near-full-PWM pulse so static friction breaks before the profile
    //     controller (which can sit just under breakaway torque at a
    //     standstill) takes over. Closed-loop: the instant the gyro confirms
    //     the robot is rotating, release back to the controller — punching
    //     any longer would overshoot the gyro's full scale. The timeout is
    //     only a fallback for the case where breakaway is never detected.
    //     The rotation profile and rot_error keep integrating normally
    //     underneath, so the controller resumes consistent on release.
    if (_ctl_kick_ticks > 0 && _ctl_armed) {
        _ctl_kick_ticks--;
        if (fabsf(gyro_z_dps) > _ctl_kick_release) {
            _ctl_kick_ticks = 0;        // breakaway confirmed — hand off now
        } else {
            motor_set_speed(-_ctl_kick_pwm, _ctl_kick_pwm);
        }
    }

    // 5c. Manual PWM override — bypass the controller entirely for closed-loop
    //     sensor-feedback corrections that the FF bias would over-drive.
    //     Runs LAST so it wins over both motors_update and the kick.
    if (_ctl_manual_on && _ctl_armed) {
        motor_set_speed(_ctl_manual_pwm_l, _ctl_manual_pwm_r);
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

    // DEFENSIVE RESETS — these mechanism-state variables don't get cleared
    // anywhere else and only decrement/clear while _ctl_armed is true. If a
    // previous run ended (or aborted) mid-kick or with manual override still
    // engaged, the values get frozen at their last state. The very next arm
    // then immediately resurrects the old kick/override and stomps on the new
    // command — that's the "rerun after a crash → everything is wrong, must
    // reset firmware" symptom. Reset them here so every arm starts CLEAN.
    _ctl_kick_ticks   = 0;
    _ctl_kick_pwm     = 0;
    _ctl_manual_on    = false;
    _ctl_manual_pwm_l = 0;
    _ctl_manual_pwm_r = 0;
    _ctl_imu_suspend  = false;   // ensure IMU reads resume
    _ctl_hold_on      = false;   // heading hold off until a forward enables it

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

// Fire a closed-loop spin-turn startup kick. Overrides motor output with a
// `pwm`-magnitude pulse (sign: + = CCW) until the gyro reads |rate| above
// `release_dps`, or until `max_ms` elapses — whichever comes first. Call
// immediately after control_start_rotation() to break stiction at the
// marginal-torque start of an in-place spin.
inline void control_kick_rotation(int16_t pwm, uint16_t max_ms,
                                  float release_dps) {
    _ctl_kick_pwm     = pwm;
    _ctl_kick_release = release_dps;
    _ctl_kick_ticks   = max_ms / 2;   // control tick = 2ms
}

// Manual PWM override — set per-wheel PWM directly, bypassing motors_update
// AND the FF bias step. Stays active until control_clear_manual_pwm().
// Caller MUST clear before resuming profile-based motion, otherwise the
// override will keep stomping on the controller. Used by fw_square_heading
// for fine sensor-feedback rotation that the FF bias would over-drive.
inline void control_set_manual_pwm(int16_t left_pwm, int16_t right_pwm) {
    _ctl_manual_pwm_l = left_pwm;
    _ctl_manual_pwm_r = right_pwm;
    _ctl_manual_on    = true;
}

inline void control_clear_manual_pwm(void) {
    _ctl_manual_on = false;
}

// Gyro heading-hold control. Enable with the absolute target heading (deg);
// the control loop then holds _ctl_heading_deg to it at 500Hz. Disable during
// spins. kp is in deg/s of correction per deg of drift.
inline void control_set_heading_hold(bool on, float target_deg) {
    _ctl_hold_target = target_deg;
    _ctl_hold_on     = on;
}
inline void control_set_hold_kp(float kp) { _ctl_hold_kp = kp; }
inline float control_get_hold_kp(void)    { return _ctl_hold_kp; }

// Suspend/resume this task's IMU reads — so the main loop can safely run an
// imu_calibrate_gyro() without two tasks hitting the I2C bus at once.
inline void control_suspend_imu(bool suspend) {
    _ctl_imu_suspend = suspend;
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
inline uint32_t control_get_glitch_count() { return _ctl_glitch_count; }

// Position error (setpoint - actual) — useful telemetry
inline float    control_get_fwd_error()    {
    return _ctl_last_fwd_setpoint - _ctl_last_fwd_actual;
}

#endif // CONTROL_TASK_H
