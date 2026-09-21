// ═══════════════════════════════════════════════════════════════════════════
//  front_wall.h  —  Front-wall correction  (Task C2)
//
//  Layer: 2 (Control glue)
//  Dependencies: hal_wall_sensor, control_task, profile
//
//  PURPOSE
//    A forward move ends with two accumulated errors: encoder-based position
//    drifts (slip, wheel-diameter mismatch) and gyro-integrated heading
//    drifts (~1.7°/spin in RRRR tests). The front wall is an ABSOLUTE
//    reference for both:
//      • avg(FL, FR)  → robot's true distance to the wall
//      • FL − FR      → robot's heading squareness (== 0 when perpendicular)
//    Calling fw_correct() before each spin turn (when a wall is ahead)
//    re-zeros both, so the drift does not accumulate across long paths.
//
//  TWO PHASES (run in this order — squaring first)
//    1) Square heading: rotate so |FL − FR| ≤ FW_SQUARE_TOL_MM
//         correction_deg = -(FL − FR) * FW_SQUARE_GAIN
//         (sign per geometry below — verify on first 'c' test)
//    2) Fix distance:   drive ±err so |avg − FW_NOMINAL_MM| ≤ FW_DIST_TOL_MM
//         correction_mm = front − nominal   (signs straightforward)
//
//    Both phases are ITERATIVE — apply a small corrective motion, re-read
//    sensors, repeat up to FW_MAX_ITERATIONS. A single open-loop correction
//    would leave residual error from gain mismatch; the loop converges.
//
//  SIGN CONVENTION (square heading)
//    Layout (hal_wall_sensor.h):     FL is at robot's FRONT-LEFT
//                                    FR is at robot's FRONT-RIGHT
//                                    both look forward
//    Robot rotated CCW (heading +)   → nose swings LEFT
//                                    → FL recedes from wall, FR approaches
//                                    → FL > FR → asym = (FL−FR) > 0
//    To square: rotate CW (negative angle).  ⇒  correction = −asym * gain
//
//    If the FIRST 'c' test makes asym GROW instead of shrink, flip the sign
//    of FW_SQUARE_GAIN (set negative) — sensor orientation is the only
//    thing that can invert this.
//
//  CALIBRATION (do once on hardware before flipping the auto-integration on)
//    1) Place robot at cell centre, manually squared against a front wall.
//       The wall should be one cell-pitch ahead in the maze frame.
//    2) Send 'm' → read steady FL & FR. Set FW_NOMINAL_MM = avg(FL, FR).
//    3) Rotate the robot by hand ~+5° CCW. Note asym = FL−FR (will be
//       positive). Derive FW_SQUARE_GAIN ≈ 5.0 / asym  (deg per mm).
//    4) Send 'c' → robot should converge to asym≈0 and front≈nominal.
//       If it diverges → sign of FW_SQUARE_GAIN is wrong.
//
//  PUBLIC API
//    fw_wall_present()       both FL & FR see a wall within FW_PRESENT_MM
//    fw_asymmetry_mm()       (FL − FR) signed, 0 if either invalid
//    fw_front_distance_mm()  avg(FL, FR); 255 if neither valid
//    fw_square_heading()     one-shot iterative square; returns true if OK
//    fw_fix_distance()       one-shot iterative distance fix; ditto
//    fw_correct()            square then distance; true if both OK
//
//    fw_last_*               stats from the most recent fw_correct() call
//                            (initial / final asym & dist, iters used)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef FRONT_WALL_H
#define FRONT_WALL_H

#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "hal_wall_sensor.h"
#include "control_task.h"

// ── Tunables (start values — calibrate on hardware then update here) ────────
#ifndef FW_NOMINAL_MM
#define FW_NOMINAL_MM        22.0f   // front reading at correct cell centre
#endif
#ifndef FW_PRESENT_MM
#define FW_PRESENT_MM        80      // both FL & FR < this counts as wall ahead
#endif
#ifndef FW_SQUARE_TOL_MM
#define FW_SQUARE_TOL_MM     2.0f    // |FL−FR| accepted as squared
#endif
#ifndef FW_DIST_TOL_MM
#define FW_DIST_TOL_MM       3.0f    // |front − nominal| accepted as positioned
#endif
#ifndef FW_MAX_ITERATIONS
#define FW_MAX_ITERATIONS    3       // safety cap for distance fix
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Square-heading uses DIRECT PWM (control_set_manual_pwm), NOT the rotation
// profile — the FF bias step (FF_BIAS_START ≈ 4.5V) launches the wheels far
// faster than commanded for tiny profile angles (e.g. a 2° rotation peaks at
// ~16 mm/s wheel speed, FF_speed term contributes only 0.08V, but the bias
// dumps 4.5V on the wheel the instant speed crosses 0). The robot blew past
// the gyro's ±250 dps full scale on the very first iter, gyro saturated, the
// controller went blind, and the spin compounded across iterations into a
// full revolution. Direct PWM gives the wheel exactly the voltage we want,
// closed-loop on asym → stop when |asym| ≤ tol or asym crosses zero.
// ─────────────────────────────────────────────────────────────────────────────
// Adaptive PWM: PWM = MIN + |asym|*K, clamped to MAX. Self-throttling — high
// at large asym (breaks stiction + rotates fast), low near zero (gentle
// landing). Replaces the fixed-PWM design that either failed to break
// stiction at large initial tilt (PWM 550 ≈ 4V < FF_BIAS_START 4.5V) or
// overshot when close to zero (PWM 650 was "too much" empirically).
#define FW_SQUARE_PWM_MIN     520    // kinetic floor — sustains rotation once
                                     // moving (≈3.8V); below stiction so used
                                     // only after motion has already started
#define FW_SQUARE_PWM_MAX     800    // breakaway ceiling — guarantees stiction
                                     // breaks even at far/large-tilt cases
#define FW_SQUARE_PWM_K       30     // PWM units added per mm of |asym|
                                     // asym=10 → PWM = 520+300 = 800 (max)
                                     // asym=5  → PWM = 520+150 = 670
                                     // asym=3  → PWM = 520+90  = 610
#define FW_SQUARE_TIMEOUT_MS  1200   // hard cap if asym never reaches tol
                                     // (large-tilt + far case needs ≥1s)

#define FW_FWD_TOP_MMPS       80.0f
#define FW_FWD_ALPHA          800.0f

// ── Last-run statistics (read by the 'c' command in the sketch) ─────────────
inline float   fw_last_asym_init   = 0;
inline float   fw_last_asym_final  = 0;
inline uint8_t fw_last_dist_init   = 0;
inline uint8_t fw_last_dist_final  = 0;
inline int     fw_last_sq_iters    = 0;
inline int     fw_last_dist_iters  = 0;
inline bool    fw_last_sq_ok       = false;
inline bool    fw_last_dist_ok     = false;

// ── Sensor accessors (read NOW, return derived values) ──────────────────────
inline bool fw_wall_present(void) {
    wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
    wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
    uint8_t fl = wall_sensor_get_distance(WALL_SENSOR_FRONT_LEFT);
    uint8_t fr = wall_sensor_get_distance(WALL_SENSOR_FRONT_RIGHT);
    return fl != 255 && fr != 255 && fl < FW_PRESENT_MM && fr < FW_PRESENT_MM;
}

inline float fw_asymmetry_mm(void) {
    uint8_t fl = wall_sensor_get_distance(WALL_SENSOR_FRONT_LEFT);
    uint8_t fr = wall_sensor_get_distance(WALL_SENSOR_FRONT_RIGHT);
    if (fl == 255 || fr == 255) return 0.0f;
    return (float)fl - (float)fr;
}

inline uint8_t fw_front_distance_mm(void) {
    return wall_sensor_get_front_distance();
}

// ── Phase 1: square the heading ─────────────────────────────────────────────
//  Direct-PWM closed-loop rotation. Bypasses motors_update (and the FF bias
//  step that overdrives small profile angles). Drives the wheels at a fixed
//  PWM in the correction direction, polls FL/FR every 10ms, and stops when
//  |asym| ≤ tol OR when asym crosses zero (= small overshoot, accept it).
//
//  Direction (per geometry in the file header — verified against hardware
//  data: tilt CCW 10° → asym +7, tilt CW 10° → asym -7):
//    a0 > 0  → CCW-tilted → rotate CW  → motor_set_speed(+pwm, -pwm)
//    a0 < 0  → CW-tilted  → rotate CCW → motor_set_speed(-pwm, +pwm)
//
//  On exit: motors stopped (coast), motors_reset_controllers() called so the
//  controller's accumulated rot_error from the manual rotation doesn't make
//  it try to "undo" what we just did when the next phase resumes.
//
//  Caller MUST have called control_arm() before this.
inline bool fw_square_heading(void) {
    // Initial reading
    wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
    wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
    float a0 = fw_asymmetry_mm();
    fw_last_asym_init = a0;

    // Already squared? exit immediately
    if (fabsf(a0) <= FW_SQUARE_TOL_MM) {
        fw_last_asym_final = a0;
        fw_last_sq_iters   = 0;
        fw_last_sq_ok      = true;
        return true;
    }

    // Adaptive direct-PWM rotation: PWM scales with |asym| each tick.
    // a0 > 0 (CCW tilt) → rotate CW : left forward (+), right backward (-)
    // a0 < 0 (CW tilt)  → rotate CCW: left backward (-), right forward (+)
    int8_t a0_sign   = (a0 > 0) ? +1 : -1;
    int8_t left_dir  = +a0_sign;
    int8_t right_dir = -a0_sign;

    // Poll asym at 10ms — recompute PWM each tick, bail on first stop cond
    uint32_t t0 = millis();
    while (millis() - t0 < FW_SQUARE_TIMEOUT_MS) {
        wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
        wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
        float a = fw_asymmetry_mm();
        if (fabsf(a) <= FW_SQUARE_TOL_MM) break;
        // Sign flip ⇒ already crossed zero — coast (don't drive past)
        if (((a > 0) ? +1 : -1) != a0_sign) break;

        // Adaptive PWM: large |asym| → high PWM (breaks stiction + fast spin)
        //                small |asym| → low PWM  (gentle landing near zero)
        int16_t pwm = (int16_t)(FW_SQUARE_PWM_MIN + fabsf(a) * FW_SQUARE_PWM_K);
        if (pwm > FW_SQUARE_PWM_MAX) pwm = FW_SQUARE_PWM_MAX;
        control_set_manual_pwm(left_dir * pwm, right_dir * pwm);

        delay(10);
    }

    // Coast: hold manual at 0/0 (PWM=0 = coast in hal_motor); friction stops it
    control_set_manual_pwm(0, 0);
    delay(200);

    // Capture final reading for stats
    wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
    wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
    fw_last_asym_final = fw_asymmetry_mm();
    fw_last_sq_iters   = 1;
    fw_last_sq_ok      = fabsf(fw_last_asym_final) <= FW_SQUARE_TOL_MM;

    // Clear controller state, then hand back to the controller. Order matters
    // — reset BEFORE clear so the moment manual goes off the controller sees
    // zero rot_error (otherwise it would try to "undo" the rotation we just
    // drove manually, since rot_error accumulated while gyro saw the spin).
    motors_reset_controllers();
    control_clear_manual_pwm();
    return fw_last_sq_ok;
}

// ── Phase 2: fix the forward distance ──────────────────────────────────────
//  front > nominal → too far  → drive forward (+err)
//  front < nominal → too close → drive backward (-err)
inline bool fw_fix_distance(void) {
    wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
    wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
    fw_last_dist_init = fw_front_distance_mm();

    int i;
    for (i = 0; i < FW_MAX_ITERATIONS; i++) {
        wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
        wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
        uint8_t front = fw_front_distance_mm();
        if (front == 255) {           // wall lost mid-fix — abort
            fw_last_dist_final = 255;
            fw_last_dist_iters = i;
            fw_last_dist_ok    = false;
            return false;
        }
        float err = (float)front - FW_NOMINAL_MM;
        if (fabsf(err) <= FW_DIST_TOL_MM) {
            fw_last_dist_final = front;
            fw_last_dist_iters = i;
            fw_last_dist_ok    = true;
            return true;
        }
        control_start_forward(err, FW_FWD_TOP_MMPS, 0.0f, FW_FWD_ALPHA);
        control_wait_for_motion(1500);
        delay(80);
    }
    // didn't converge
    wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
    wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
    fw_last_dist_final = fw_front_distance_mm();
    fw_last_dist_iters = i;
    fw_last_dist_ok    = false;
    return false;
}

// ── Combined: square first, then distance ───────────────────────────────────
//  Returns true only if BOTH phases converged. Caller (sketch) is responsible
//  for arming/disarming the control task around this call.
inline bool fw_correct(void) {
    if (!fw_wall_present()) {
        fw_last_sq_ok      = false;
        fw_last_dist_ok    = false;
        return false;
    }
    bool sq_ok   = fw_square_heading();
    bool dist_ok = fw_fix_distance();
    return sq_ok && dist_ok;
}

#endif // FRONT_WALL_H
