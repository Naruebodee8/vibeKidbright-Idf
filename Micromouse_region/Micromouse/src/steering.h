// ═══════════════════════════════════════════════════════════════════════════
//  steering.h — Side-wall cross-track-error (CTE) correction
//
//  Layer: 2 (Control — glue)
//  Dependencies: config.h, hal_wall_sensor.h, control_task.h
//
//  Spec: Decision D2 (Peter Harrison steering pattern)
//
//  WHAT IT DOES
//    During a straight run the mouse drifts (motor mismatch, gyro bias). The
//    side IR sensors see how far each wall is; the difference is the
//    cross-track error (CTE) — the lateral offset from the corridor centre.
//    This module turns CTE into a small per-tick rotation-error increment and
//    hands it to the control task via control_set_steering(). motors_update()
//    adds it straight into the rotation PD error — so a CTE bends the heading
//    just enough to walk the mouse back to the middle.
//
//  WHY IT IS A PER-TICK VALUE
//    control_task adds _ctl_steering_adj to _mc_rot_error EVERY control tick
//    (500 Hz). steering_update() is called more slowly (~100 Hz) from the main
//    loop — the value it sets is held and re-applied for the 5 ticks until the
//    next update. So the value must be "degrees of yaw error to inject per
//    tick": yaw_rate(°/s) × LOOP_INTERVAL. Applying it every tick then yields
//    exactly the intended yaw rate, regardless of the update cadence.
//
//  CTE SIGN CONVENTION
//    cte > 0  →  closer to the RIGHT wall  →  steer LEFT  (+rot_error)
//    cte < 0  →  closer to the LEFT  wall  →  steer RIGHT (-rot_error)
//    +rot_error turns the mouse LEFT/CCW (verified: smooth-L gave +90°).
//    If on hardware the mouse diverges, flip STEERING_SIGN in config.h.
//
//  CONTROL MODEL (why KD is mandatory)
//    Pure-P steering (yaw rate ∝ offset) is an undamped oscillator:
//      dθ/dt = -Kp·x ,  dx/dt = v·θ   →   d²x/dt² + v·Kp·x = 0   (SHM)
//    The KD term acts on the CTE rate (≈ v·θ) and supplies the damping:
//      d²x/dt² + (Kd·v)·dx/dt + (v·Kp)·x = 0   → damped, settles.
// ═══════════════════════════════════════════════════════════════════════════
#ifndef STEERING_H
#define STEERING_H

#include <Arduino.h>
#include "config.h"
#include "hal_wall_sensor.h"
#include "control_task.h"

// ─────────────────────────────────────────────────────────────────────────────
// Runtime-tunable gains (seeded from config.h — adjustable over Serial)
// ─────────────────────────────────────────────────────────────────────────────
inline float steering_kp      = STEERING_KP;            // °/s per mm
inline float steering_kd      = STEERING_KD;            // °/s per (mm/s)
inline float steering_nominal = STEER_NOMINAL_SIDE_MM;  // centred side reading

// ─────────────────────────────────────────────────────────────────────────────
// Internal state
// ─────────────────────────────────────────────────────────────────────────────
inline bool  _st_enabled    = false;
inline float _st_cte        = 0.0f;   // raw cross-track error (mm) — telemetry
inline float _st_cte_filt   = 0.0f;   // EMA-filtered CTE — controller input
inline float _st_last_cte_filt = 0.0f;
inline bool  _st_have_last  = false;  // false until first valid sample
inline float _st_cte_rate   = 0.0f;   // EMA-filtered CTE rate (mm/s)
inline float _st_yaw_cmd    = 0.0f;   // commanded steering yaw rate (°/s)
inline float _st_adj        = 0.0f;   // per-tick rotation-error increment (°)
inline bool  _st_left_wall  = false;
inline bool  _st_right_wall = false;
inline uint32_t _st_updates = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

// Clear all state and zero the injected steering. Call right before a run.
inline void steering_reset(void) {
    _st_cte = _st_cte_filt = _st_last_cte_filt = _st_cte_rate = 0.0f;
    _st_yaw_cmd = _st_adj = 0.0f;
    _st_have_last  = false;
    _st_left_wall  = false;
    _st_right_wall = false;
    _st_updates    = 0;
    control_set_steering(0.0f);
}

// Enable/disable injection. When disabled, CTE is still computed (for logging
// the bare drift) but nothing is pushed into the rotation PD.
inline void steering_enable(bool on) {
    _st_enabled = on;
    if (!on) {
        control_set_steering(0.0f);
        _st_have_last = false;     // derivative restarts cleanly when re-enabled
    }
}

inline bool steering_is_enabled(void) { return _st_enabled; }

inline void steering_set_gains(float kp, float kd) {
    steering_kp = kp;
    steering_kd = kd;
}

inline void steering_set_nominal(float side_mm) {
    steering_nominal = side_mm;
}

// ─────────────────────────────────────────────────────────────────────────────
// steering_update — call at a fixed cadence (~100 Hz) with fresh side readings
//
//   left_mm / right_mm : side-sensor distances (255 = invalid / out of range)
//   dt_s               : seconds since the previous call (for the KD term)
//
// Computes CTE, runs the PD, and — if enabled — injects the result via
// control_set_steering(). Safe to call when disabled (compute-only).
// ─────────────────────────────────────────────────────────────────────────────
inline void steering_update(uint8_t left_mm, uint8_t right_mm, float dt_s) {
    _st_updates++;

    // ── 1. wall presence — valid reading AND inside the corridor ─────────────
    bool L = (left_mm  != 255) && (left_mm  <= STEER_WALL_PRESENT_MM);
    bool R = (right_mm != 255) && (right_mm <= STEER_WALL_PRESENT_MM);
    _st_left_wall  = L;
    _st_right_wall = R;

    // ── 2. cross-track error (mm) ────────────────────────────────────────────
    // Saturation override: a side reading pinned at the sensor floor means the
    // mouse is HUGGING that wall. The Sharp cannot measure any closer, so the
    // ordinary CTE under-reports the offset — and once the far wall passes
    // STEER_WALL_PRESENT_MM the one-wall fallback would collapse to a weak
    // constant (near_floor - nominal) and the mouse never recovers. Detect the
    // floor reading directly and force a hard steer AWAY from that wall.
    bool L_hug = (left_mm  != 255) && (left_mm  <= SHARP_RANGE_MIN_MM);
    bool R_hug = (right_mm != 255) && (right_mm <= SHARP_RANGE_MIN_MM);

    float cte;
    if (L_hug && !R_hug) {
        cte = -STEER_CTE_MAX_MM;             // hugging LEFT  → hard steer RIGHT
    } else if (R_hug && !L_hug) {
        cte = +STEER_CTE_MAX_MM;             // hugging RIGHT → hard steer LEFT
    } else if (L && R) {
        // Both walls — centre on the midline.
        //   closer to RIGHT wall → right_mm small, left_mm large → cte > 0
        cte = 0.5f * ((float)left_mm - (float)right_mm);
    } else if (L) {
        // Left wall only — hold the nominal gap.
        //   too close to LEFT → left_mm < nominal → cte < 0   (consistent)
        cte = (float)left_mm - steering_nominal;
    } else if (R) {
        // Right wall only — hold the nominal gap.
        //   too close to RIGHT → right_mm < nominal → cte > 0  (consistent)
        cte = steering_nominal - (float)right_mm;
    } else {
        // No walls in view — cannot steer. Hold straight, drop derivative.
        _st_cte = 0.0f;
        _st_cte_rate = 0.0f;
        _st_yaw_cmd = 0.0f;
        _st_adj = 0.0f;
        _st_have_last = false;
        if (_st_enabled) control_set_steering(0.0f);
        return;
    }

    // Reject glitches / wide cell openings.
    cte = CONSTRAIN(cte, -STEER_CTE_MAX_MM, STEER_CTE_MAX_MM);
    _st_cte = cte;                       // raw CTE — kept for telemetry

    // ── 3. EMA filtering ─────────────────────────────────────────────────────
    // The raw CTE is quantised to 0.5mm (integer-mm sensors). Differentiating
    // it raw at 100Hz buries the real signal under quantisation spikes — so the
    // CTE and its rate are both EMA-filtered before the PD acts on them.
    if (!_st_have_last) _st_cte_filt = cte;            // seed on first sample
    else _st_cte_filt += STEER_CTE_EMA_ALPHA * (cte - _st_cte_filt);

    float raw_rate = 0.0f;
    if (_st_have_last && dt_s > 1e-4f) {
        raw_rate = (_st_cte_filt - _st_last_cte_filt) / dt_s;
    }
    _st_cte_rate += STEER_RATE_EMA_ALPHA * (raw_rate - _st_cte_rate);

    _st_last_cte_filt = _st_cte_filt;
    _st_have_last     = true;

    // ── 4. PD controller → commanded yaw rate (°/s) ──────────────────────────
    float yaw = steering_kp * _st_cte_filt + steering_kd * _st_cte_rate;
    yaw = CONSTRAIN(yaw, -STEERING_MAX_DPS, STEERING_MAX_DPS);
    _st_yaw_cmd = yaw;

    // ── 5. per-tick rotation-error increment ─────────────────────────────────
    // yaw(°/s) × LOOP_INTERVAL(s) = degrees to inject per 500 Hz control tick.
    _st_adj = (float)STEERING_SIGN * yaw * LOOP_INTERVAL;

    // ── 6. hand to the control task (only when enabled) ──────────────────────
    if (_st_enabled) {
        control_set_steering(_st_adj);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry getters
// ─────────────────────────────────────────────────────────────────────────────
inline float    steering_get_cte(void)        { return _st_cte; }
inline float    steering_get_cte_rate(void)   { return _st_cte_rate; }
inline float    steering_get_yaw_cmd(void)    { return _st_yaw_cmd; }
inline float    steering_get_adj(void)        { return _st_adj; }
inline bool     steering_have_left_wall(void) { return _st_left_wall; }
inline bool     steering_have_right_wall(void){ return _st_right_wall; }
inline uint32_t steering_get_updates(void)    { return _st_updates; }
inline float    steering_get_kp(void)         { return steering_kp; }
inline float    steering_get_kd(void)         { return steering_kd; }
inline float    steering_get_nominal(void)    { return steering_nominal; }

#endif // STEERING_H
