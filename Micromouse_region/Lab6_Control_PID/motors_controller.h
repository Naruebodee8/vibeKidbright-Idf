// ═══════════════════════════════════════════════════════════════════════════
//  motors_controller.h — Position PD + per-wheel FF + battery comp
//
//  Layer: 2 (Control)
//  Dependencies: hal_motor, hal_battery, config.h
//  (NOTE: no hal_encoder dep — caller supplies fwd_change/rot_change as inputs)
//
//  Spec: Section 12 (motors_controller.h) of Micromouse_2027/index.html
//
//  Architecture (REFACTORED A6):
//    motors_update(v, omega, fwd_change, rot_change, steering) does pure math:
//      1. integrate position errors:
//         fwd_error += velocity·dt - fwd_change
//         rot_error += omega·dt    - rot_change + steering
//      2. compute PD output (volts) on each error
//      3. compute per-wheel FF (volts) with omega tangent + signed BIAS
//      4. mix: left = fwd_pd - rot_pd + left_ff, right = fwd_pd + rot_pd + right_ff
//      5. clamp to MAX_MOTOR_VOLTS, battery-compensate → PWM
//      6. output to hal_motor (only if enabled)
//
//  Why caller supplies deltas:
//    - Motors_controller is pure math, no HAL coupling (testable in isolation)
//    - Caller decides rotation source: encoder differential OR gyro (D15)
//    - control_task supplies gyro-based rot_change (per-tick, no slip artifacts)
//
//  Threading:
//    - motors_update() is called from controlTask only (single writer)
//    - getters are safe to read from any task (volatile + 32-bit atomic on ESP32)
//
//  Values used (from Project1 HW calibration — D14 signed BIAS):
//    BIAS START 4.50V, RUN 3.80V (blend at 0-120 mm/s)
//    SPEED L=5.07, R=5.39 mV/(mm/s)
//    ACC = 0.30 mV/(mm/s²)
//    ROT_KP=8.0 (Project1), FWD_KP=2.0 (Peter — tune)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef MOTORS_CONTROLLER_H
#define MOTORS_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "hal_motor.h"
#include "hal_battery.h"
// NOTE: no hal_encoder include — caller supplies deltas

// ─────────────────────────────────────────────────────────────────────────────
// State (inline — header-only)
// ─────────────────────────────────────────────────────────────────────────────

inline bool     _mc_enabled        = false;

// ── STEP 6 lesson hooks ──────────────────────────────────────────────────────
// Runtime-tunable PD gains (seeded from config #defines). The FEEDFORWARD path
// is untouched — only the position-PD correction is gated/tuned here, so a
// student sees exactly what the PID layer adds on top of STEP 5 feedforward.
//   _mc_pid_enabled = false → fwd_pd and rot_pd are forced to 0 (FF-only).
//                             Position error still INTEGRATES (telemetry shows
//                             the drift the open-loop FF model leaves behind).
inline volatile bool  _mc_pid_enabled = true;
inline volatile float _mc_fwd_kp = FWD_KP;
inline volatile float _mc_fwd_kd = FWD_KD;
inline volatile float _mc_rot_kp = ROT_KP;
inline volatile float _mc_rot_kd = ROT_KD;

// Integral gains — DEFAULT 0 (system runs as FF + PD, behaviour unchanged).
// Raising Ki accumulates the position error → eventually overcomes stiction and
// removes the small steady-state residual that pure PD leaves behind. Too much
// Ki = windup / overshoot — that tradeoff is the lesson. Anti-windup clamps the
// integral so its volt-contribution never exceeds the PD voltage ceiling.
inline volatile float _mc_fwd_ki = 0.0f;
inline volatile float _mc_rot_ki = 0.0f;
inline float _mc_fwd_integral = 0;   // accumulated position error (mm·s)
inline float _mc_rot_integral = 0;   // accumulated heading error  (deg·s)

// Position PD errors (volts after multiplication by KP/KD)
inline float    _mc_fwd_error      = 0;
inline float    _mc_rot_error      = 0;
inline float    _mc_prev_fwd_error = 0;
inline float    _mc_prev_rot_error = 0;

// Previous per-wheel speed (for FF acceleration term)
inline float    _mc_prev_left_speed  = 0;
inline float    _mc_prev_right_speed = 0;

// Last output (telemetry)
inline volatile float _mc_last_left_volts  = 0;
inline volatile float _mc_last_right_volts = 0;
inline volatile float _mc_last_fwd_pd      = 0;
inline volatile float _mc_last_rot_pd      = 0;
inline volatile float _mc_last_left_ff     = 0;
inline volatile float _mc_last_right_ff    = 0;
inline volatile int16_t _mc_last_left_pwm  = 0;
inline volatile int16_t _mc_last_right_pwm = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

// Blended BIAS: START near v=0, RUN at higher speeds, linear blend in between.
// Returns positive bias magnitude — caller applies sign based on speed direction.
inline float _mc_bias_blended(float bias_start_v, float bias_run_v, float speed_mmps) {
    float abs_v = fabsf(speed_mmps);
    if (abs_v <= FF_BIAS_BLEND_V0_MMPS) return bias_start_v;
    if (abs_v >= FF_BIAS_BLEND_V1_MMPS) return bias_run_v;
    float t = (abs_v - FF_BIAS_BLEND_V0_MMPS)
            / (FF_BIAS_BLEND_V1_MMPS - FF_BIAS_BLEND_V0_MMPS);
    return bias_start_v + t * (bias_run_v - bias_start_v);
}

// Per-wheel FF — signed BIAS pattern (D14)
//   if speed > 0 → +bias
//   if speed < 0 → -bias
//   if speed == 0 → no bias (motor coasts at 0)
inline float _mc_ff_left(float left_speed_mmps, float left_accel_mmps2) {
    float ff = FF_SPEED_L_V_PER_MMPS * left_speed_mmps
             + FF_ACC_V_PER_MMPS2    * left_accel_mmps2;
    float bias = _mc_bias_blended(FF_BIAS_START_L_V, FF_BIAS_RUN_L_V, left_speed_mmps);
    if      (left_speed_mmps > 0.0f) ff += bias;
    else if (left_speed_mmps < 0.0f) ff -= bias;
    return ff;
}

inline float _mc_ff_right(float right_speed_mmps, float right_accel_mmps2) {
    float ff = FF_SPEED_R_V_PER_MMPS * right_speed_mmps
             + FF_ACC_V_PER_MMPS2    * right_accel_mmps2;
    float bias = _mc_bias_blended(FF_BIAS_START_R_V, FF_BIAS_RUN_R_V, right_speed_mmps);
    if      (right_speed_mmps > 0.0f) ff += bias;
    else if (right_speed_mmps < 0.0f) ff -= bias;
    return ff;
}

inline float _mc_clamp(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

inline bool motors_init() {
    return motor_init();   // delegate to hal_motor
}

inline void motors_enable_controllers() { _mc_enabled = true; }
inline void motors_disable_controllers() { _mc_enabled = false; }

// ── STEP 6 lesson API — toggle / tune the position PD at runtime ─────────────
inline void  motors_set_pid_enabled(bool on) {
    // Turning PID ON: clear the error/integral that accumulated while it was OFF,
    // so the next tick doesn't apply a sudden Kp*(huge error) kick (robot lurch).
    if (on && !_mc_pid_enabled) {
        _mc_fwd_error = 0; _mc_rot_error = 0;
        _mc_prev_fwd_error = 0; _mc_prev_rot_error = 0;
        _mc_fwd_integral = 0; _mc_rot_integral = 0;
    }
    _mc_pid_enabled = on;
}
inline bool  motors_is_pid_enabled()         { return _mc_pid_enabled; }
inline void  motors_set_fwd_gains(float kp, float kd) { _mc_fwd_kp = kp; _mc_fwd_kd = kd; }
inline void  motors_set_rot_gains(float kp, float kd) { _mc_rot_kp = kp; _mc_rot_kd = kd; }
inline void  motors_set_fwd_ki(float ki) { _mc_fwd_ki = ki; _mc_fwd_integral = 0; }  // reset accum on change
inline void  motors_set_rot_ki(float ki) { _mc_rot_ki = ki; _mc_rot_integral = 0; }
inline float motors_get_fwd_kp() { return _mc_fwd_kp; }
inline float motors_get_fwd_kd() { return _mc_fwd_kd; }
inline float motors_get_fwd_ki() { return _mc_fwd_ki; }
inline float motors_get_rot_kp() { return _mc_rot_kp; }
inline float motors_get_rot_kd() { return _mc_rot_kd; }
inline float motors_get_rot_ki() { return _mc_rot_ki; }

inline void motors_reset_controllers() {
    _mc_fwd_error = 0;
    _mc_rot_error = 0;
    _mc_prev_fwd_error = 0;
    _mc_prev_rot_error = 0;
    _mc_fwd_integral = 0;
    _mc_rot_integral = 0;
    _mc_prev_left_speed = 0;
    _mc_prev_right_speed = 0;
    // No HAL reads — caller tracks its own prev_fwd/prev_rot for delta computation
}

inline void motors_stop() {
    _mc_enabled = false;
    motor_stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// motors_update — main entry, called every control tick
//   velocity_mmps:        forward setpoint (from profile.speed)
//   omega_dps:            angular setpoint (from rotation profile.speed)
//   fwd_change_mm:        measured forward movement since last tick (encoder)
//   rot_change_deg:       measured rotation since last tick (gyro × dt, D15)
//   steering_adjustment:  CTE adjustment (deg) — injects to rot_error
// ─────────────────────────────────────────────────────────────────────────────
inline void motors_update(float velocity_mmps, float omega_dps,
                          float fwd_change_mm, float rot_change_deg,
                          float steering_adjustment) {
    // ── 1. integrate position errors ─────────────────────────────────────────
    _mc_fwd_error += (velocity_mmps * LOOP_INTERVAL) - fwd_change_mm;
    _mc_rot_error += (omega_dps    * LOOP_INTERVAL) - rot_change_deg;

    // Steering CTE injected as angular error (Peter pattern, D2)
    _mc_rot_error += steering_adjustment;

    // ── 2. Position PID outputs (volts) = P + I + D ──────────────────────────
    // Integral: accumulate position error only while PID is active (no windup
    // during FF-only). Anti-windup: clamp the I volt-contribution to the ceiling
    // and back-calculate the accumulator so it can't grow past what saturates.
    float fwd_i = 0.0f, rot_i = 0.0f;
    if (_mc_pid_enabled && _mc_fwd_ki != 0.0f) {
        _mc_fwd_integral += _mc_fwd_error * LOOP_INTERVAL;
        fwd_i = _mc_fwd_ki * _mc_fwd_integral;
        if      (fwd_i >  FWD_MAX_VOLTS) { fwd_i =  FWD_MAX_VOLTS; _mc_fwd_integral = fwd_i / _mc_fwd_ki; }
        else if (fwd_i < -FWD_MAX_VOLTS) { fwd_i = -FWD_MAX_VOLTS; _mc_fwd_integral = fwd_i / _mc_fwd_ki; }
    }
    if (_mc_pid_enabled && _mc_rot_ki != 0.0f) {
        _mc_rot_integral += _mc_rot_error * LOOP_INTERVAL;
        rot_i = _mc_rot_ki * _mc_rot_integral;
        if      (rot_i >  ROT_MAX_VOLTS) { rot_i =  ROT_MAX_VOLTS; _mc_rot_integral = rot_i / _mc_rot_ki; }
        else if (rot_i < -ROT_MAX_VOLTS) { rot_i = -ROT_MAX_VOLTS; _mc_rot_integral = rot_i / _mc_rot_ki; }
    }

    float fwd_diff = _mc_fwd_error - _mc_prev_fwd_error;
    _mc_prev_fwd_error = _mc_fwd_error;
    float fwd_pd = _mc_fwd_kp * _mc_fwd_error + fwd_i + _mc_fwd_kd * fwd_diff * LOOP_FREQUENCY;
    fwd_pd = _mc_clamp(fwd_pd, -FWD_MAX_VOLTS, FWD_MAX_VOLTS);

    float rot_diff_err = _mc_rot_error - _mc_prev_rot_error;
    _mc_prev_rot_error = _mc_rot_error;
    float rot_pd = _mc_rot_kp * _mc_rot_error + rot_i + _mc_rot_kd * rot_diff_err * LOOP_FREQUENCY;
    rot_pd = _mc_clamp(rot_pd, -ROT_MAX_VOLTS, ROT_MAX_VOLTS);

    // STEP 6 gate: drop the PID correction entirely → pure feedforward.
    // (Errors above keep integrating so telemetry still reports the drift.)
    if (!_mc_pid_enabled) { fwd_pd = 0.0f; rot_pd = 0.0f; }

    // ── 3. Per-wheel speeds (for FF — handles smooth turn) ───────────────────
    float tangent_mmps    = omega_dps * MOUSE_RADIUS_MM * RADIANS_PER_DEGREE;
    float left_speed_mmps  = velocity_mmps - tangent_mmps;
    float right_speed_mmps = velocity_mmps + tangent_mmps;

    // Per-wheel acceleration estimate (delta speed / dt)
    float left_accel  = (left_speed_mmps  - _mc_prev_left_speed)  * LOOP_FREQUENCY;
    float right_accel = (right_speed_mmps - _mc_prev_right_speed) * LOOP_FREQUENCY;
    _mc_prev_left_speed  = left_speed_mmps;
    _mc_prev_right_speed = right_speed_mmps;

    // ── 4. Per-wheel FF (volts) ──────────────────────────────────────────────
    float left_ff  = _mc_ff_left(left_speed_mmps,   left_accel);
    float right_ff = _mc_ff_right(right_speed_mmps, right_accel);

    // ── 5. Mix: combine PD + FF for each wheel ───────────────────────────────
    float left_volts  = fwd_pd - rot_pd + left_ff;
    float right_volts = fwd_pd + rot_pd + right_ff;

    left_volts  = _mc_clamp(left_volts,  -MAX_MOTOR_VOLTS, MAX_MOTOR_VOLTS);
    right_volts = _mc_clamp(right_volts, -MAX_MOTOR_VOLTS, MAX_MOTOR_VOLTS);

    // ── 6. Battery compensation → PWM ────────────────────────────────────────
    float vbat = hal_battery_volts();
    if (vbat < 4.0f) vbat = 7.4f;   // safety: bad reading → assume nominal

    int16_t left_pwm  = (int16_t)(MOTOR_PWM_MAX * left_volts  / vbat);
    int16_t right_pwm = (int16_t)(MOTOR_PWM_MAX * right_volts / vbat);

    // Final PWM clamp
    if (left_pwm  >  MOTOR_PWM_MAX) left_pwm  =  MOTOR_PWM_MAX;
    if (left_pwm  < -MOTOR_PWM_MAX) left_pwm  = -MOTOR_PWM_MAX;
    if (right_pwm >  MOTOR_PWM_MAX) right_pwm =  MOTOR_PWM_MAX;
    if (right_pwm < -MOTOR_PWM_MAX) right_pwm = -MOTOR_PWM_MAX;

    // Telemetry
    _mc_last_left_volts  = left_volts;
    _mc_last_right_volts = right_volts;
    _mc_last_fwd_pd      = fwd_pd;
    _mc_last_rot_pd      = rot_pd;
    _mc_last_left_ff     = left_ff;
    _mc_last_right_ff    = right_ff;
    _mc_last_left_pwm    = left_pwm;
    _mc_last_right_pwm   = right_pwm;

    // ── 7. Output (only if enabled) ──────────────────────────────────────────
    if (_mc_enabled) {
        motor_set_speed(left_pwm, right_pwm);
    } else {
        motor_set_speed(0, 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry getters
// ─────────────────────────────────────────────────────────────────────────────
inline float   motors_get_left_volts()  { return _mc_last_left_volts; }
inline float   motors_get_right_volts() { return _mc_last_right_volts; }
inline float   motors_get_fwd_pd()      { return _mc_last_fwd_pd; }
inline float   motors_get_rot_pd()      { return _mc_last_rot_pd; }
inline float   motors_get_left_ff()     { return _mc_last_left_ff; }
inline float   motors_get_right_ff()    { return _mc_last_right_ff; }
inline int16_t motors_get_left_pwm()    { return _mc_last_left_pwm; }
inline int16_t motors_get_right_pwm()   { return _mc_last_right_pwm; }
inline float   motors_get_fwd_error()   { return _mc_fwd_error; }
inline float   motors_get_rot_error()   { return _mc_rot_error; }
inline bool    motors_is_enabled()      { return _mc_enabled; }

#endif // MOTORS_CONTROLLER_H
