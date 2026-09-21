// ═══════════════════════════════════════════════════════════════════════════
//  config.h  —  test_06_v2 (Sharp IR wall sensor + steering CTE correction)
//
//  Carried over from test_05_v2 (profile / motors_controller / control_task),
//  plus SECTION 9 (Sharp IR sensors) and SECTION 10 (Steering / CTE) added
//  for the wall-following work.
// ═══════════════════════════════════════════════════════════════════════════
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 1: CONTROL LOOP TIMING
// ─────────────────────────────────────────────────────────────────────────────
// Decision D3: 500Hz FreeRTOS task pinned core 1
// Profile uses LOOP_INTERVAL to integrate position from speed each tick.

#define LOOP_FREQUENCY    500.0f
#define LOOP_INTERVAL     (1.0f / LOOP_FREQUENCY)   // 0.002 s = 2 ms

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 2: ROBOT PHYSICAL (for threshold derivation)
// ─────────────────────────────────────────────────────────────────────────────
// Measured Lab 2a (encoder calibration). Used to derive profile termination
// threshold (must be >= 1 encoder count to avoid "stuck just short of target").

#define WHEEL_DIAMETER_MM     33.5f
#define WHEEL_BASE_MM         78.0f       // for rotation threshold (future)
#define COUNTS_PER_REV        815

#define WHEEL_CIRCUMFERENCE_MM  (WHEEL_DIAMETER_MM * PI)                   // ≈ 105.24 mm
#define MM_PER_COUNT            (WHEEL_CIRCUMFERENCE_MM / COUNTS_PER_REV)  // ≈ 0.1291 mm/count

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 3: PROFILE CONFIG (overridable per Profile instance)
// ─────────────────────────────────────────────────────────────────────────────

// Default termination threshold for forward profiles
// 1.5 × 1 encoder count ≈ 0.19 mm — gives ~1.5 counts of slack before
// declaring FINISHED. Empirically tune in A2 if "stuck short of target" occurs.
#define PROFILE_FWD_THRESHOLD_MM    (MM_PER_COUNT * 1.5f)

// Default termination threshold for rotation profiles
#define PROFILE_ROT_THRESHOLD_DEG   0.25f

// Creep speed during BRAKING when final_speed = 0
#define PROFILE_FINISH_CREEP_SPEED  5.0f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 4: BATTERY MONITORING (A3 — added 2026-05-21)
// ─────────────────────────────────────────────────────────────────────────────
// HW: 2S LiPo → voltage divider (RTOP=100k, RBOT=33k) → PIN 10 ADC

#define PIN_BATTERY_ADC         10
#define BATTERY_DIVIDER_RATIO   4.15f
#define BATTERY_DIVIDER_X100    415       // integer form (avoid float in HAL hot path)

#define ADC_RESOLUTION_BITS     12

// 2S LiPo voltage thresholds (mV)
#define BATT_FULL_MV            8400      // 4.20V × 2 (fresh charge)
#define BATT_NOMINAL_MV         7400      // 3.70V × 2 (info — between full & low)
#define BATT_LOW_MV             7000      // 3.50V × 2 (warn — LED yellow)
#define BATT_CRITICAL_MV        6400      // 3.20V × 2 (stop motors, LED red)

// EMA filter strength: 26/256 ≈ 0.10 (smooth)
#define BATT_EMA_ALPHA_X256     26

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 5: MOTOR & ENCODER HW (A4/A5)
// ─────────────────────────────────────────────────────────────────────────────

// Motor driver pins (DRV8833)
#define PIN_MOTOR_L_IN1         5
#define PIN_MOTOR_L_IN2         4
#define PIN_MOTOR_R_IN1         6
#define PIN_MOTOR_R_IN2         7

// PWM
#define MOTOR_PWM_FREQ          20000     // 20 kHz (ultrasonic)
#define MOTOR_PWM_RESOLUTION    10        // 10-bit (0-1023)
#define MOTOR_PWM_MAX           1023
#define MOTOR_PWM_MIN           590       // not used in v2 (FF handles bias)

// Direction polarity (1=normal, -1=flip)
#define MOTOR_L_DIRECTION       1
#define MOTOR_R_DIRECTION       1

// Encoder pins
#define PIN_ENC_L_A             15
#define PIN_ENC_L_B             16
#define PIN_ENC_R_A             17
#define PIN_ENC_R_B             18

// User interface pins (A7 — used by Lab 1 button trigger)
#define PIN_BUTTON_START        21
#define PIN_BUTTON_MODE         41
#define PIN_DIP_SW_0            37
#define PIN_DIP_SW_1            38
#define PIN_DIP_SW_2            39
#define PIN_DIP_SW_3            40
#define PIN_RGB_LED             48

// Geometry derivations (used by motors_controller)
#define MM_PER_DEG_DIFFERENCE   (PI * WHEEL_BASE_MM / 360.0f)    // ≈ 0.681 mm per 1° rotation
#define DEG_PER_MM_DIFFERENCE   (360.0f / (PI * WHEEL_BASE_MM))  // ≈ 1.469 °/mm differential

// Mouse radius for FF omega tangent
#define MOUSE_RADIUS_MM         (WHEEL_BASE_MM / 2.0f)           // 39 mm
#ifndef RADIANS_PER_DEGREE
#define RADIANS_PER_DEGREE      (PI / 180.0f)
#endif

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 6: FEEDFORWARD (volts-based, from Project1 HW calibration)
// ─────────────────────────────────────────────────────────────────────────────

#define FF_BIAS_START_L_V        4.500f    // Project1 Lab2b: 4500 mV stiction breakaway
#define FF_BIAS_START_R_V        4.500f
#define FF_BIAS_RUN_L_V          3.800f    // Project1 Lab3: regression intercept (on floor)
#define FF_BIAS_RUN_R_V          3.800f
#define FF_BIAS_BLEND_V0_MMPS    0.0f
#define FF_BIAS_BLEND_V1_MMPS    120.0f

// Speed FF: voltage needed per (mm/s) of velocity
#define FF_SPEED_L_V_PER_MMPS    0.00507f  // Project1: 5.07 mV/(mm/s)
#define FF_SPEED_R_V_PER_MMPS    0.00539f  // Project1: 5.39 mV/(mm/s) — R needs more
#define FF_ACC_V_PER_MMPS2       0.00030f  // Project1: 0.30 mV/(mm/s²)

#define FF_MOTOR_TAU_MS          150       // motor time constant (Project1)

// Rotation FF (separate from linear — for in-place spin)
#define FF_ROT_BIAS_V            4.550f    // Project1
#define FF_ROT_SPEED_V_PER_DPS   0.00310f
#define FF_ROT_ACC_V_PER_DPS2    0.00048f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 7: PD CONTROLLERS (position-based, volts output) — A4
// ─────────────────────────────────────────────────────────────────────────────
#define FWD_KP                   2.0f      // Peter default — tune in HW
#define FWD_KD                   0.1f      // reduced from 1.1 — was amplifying encoder noise at standstill
#define FWD_MAX_VOLTS            5.0f      // raised from 2.0 — allow PD to brake harder

#define ROT_KP                   1.0f      // heading-error gain (V per degree)
#define ROT_KD                   0.03f     // yaw-rate damping — kills the weave
#define ROT_MAX_VOLTS            2.5f      // Project1: 2500 mV

// Final motor voltage clamp (safety — should be below typical battery full 8.4V)
#define MAX_MOTOR_VOLTS          8.0f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 7B: IMU (A6 — MPU6050 via I2C bus 0)
// ─────────────────────────────────────────────────────────────────────────────

#define PIN_I2C0_SCL             9
#define PIN_I2C0_SDA             8
#define I2C0_FREQ                400000        // 400 kHz Fast Mode

#define IMU_I2C_ADDR             0x68          // MPU6050 default

#define GYRO_OFFSET_X            -313.6f
#define GYRO_OFFSET_Y            88.7f
#define GYRO_OFFSET_Z            -32.1f

#define ACCEL_OFFSET_X           -828.4f
#define ACCEL_OFFSET_Y           -127.8f
#define ACCEL_OFFSET_Z           1264.5f

// Gyro scale (for ±250°/s full-scale range)
#define GYRO_SCALE               (250.0f / 32768.0f)

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 8: Macros + back-compat aliases (for copied HAL headers)
// ─────────────────────────────────────────────────────────────────────────────

#ifndef CONSTRAIN
#define CONSTRAIN(x, mn, mx)    ((x) < (mn) ? (mn) : ((x) > (mx) ? (mx) : (x)))
#endif
#ifndef ABS
#define ABS(x)                  ((x) < 0 ? -(x) : (x))
#endif
#ifndef SIGN
#define SIGN(x)                 ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#endif

// Battery constant aliases — old hal_motor.h uses BATTERY_*_MV, new style uses BATT_*_MV
#define BATTERY_FULL_MV         BATT_FULL_MV
#define BATTERY_NOMINAL_MV      BATT_NOMINAL_MV
#define BATTERY_LOW_MV          BATT_LOW_MV
#define BATTERY_CRITICAL_MV     BATT_CRITICAL_MV

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 9: SHARP IR WALL SENSORS (test_06_v2 — hal_wall_sensor.h)
// ─────────────────────────────────────────────────────────────────────────────
// Sharp GP2Y0A51SK0F × 4, analog ADC. No I2C — independent of the IMU bus.
//
// Layout (looking down on the robot):
//        FRONT
//    ┌─────────┐
//    │ FL   FR │   FL = Front-Left,  FR = Front-Right
//  L │         │ R L  = Left side,   R  = Right side
//    └─────────┘
//
// Pins 11-14 are free in the v2 pin map (DIP switches moved to 37-40).

#define PIN_IR_RIGHT            14        // ADC — Right side sensor
#define PIN_IR_FRONT_R          13        // ADC — Front-Right sensor
#define PIN_IR_FRONT_L          12        // ADC — Front-Left sensor
#define PIN_IR_LEFT             11        // ADC — Left side sensor

#define WALL_SENSORS_NUM        4

// Sharp supply voltage. Datasheet wants 4.5-5.5V; we run at 3.3V (out of spec)
// → the whole mV curve sits lower → SHARP_LUT MUST be HW-calibrated ('k' cmd).
#define SHARP_SUPPLY_MV         3300

// ADC samples averaged per single read (more = less noise, ~1/√N)
#define SHARP_ADC_SAMPLES       4

// Wall-detection thresholds (mm) — used only by wall_sensor_is_wall(); the
// steering loop uses STEER_WALL_PRESENT_MM (Section 10), not these.
#define WALL_THRESHOLD_FRONT    60
#define WALL_THRESHOLD_SIDE     80

// Per-sensor offset (mm) subtracted from the LUT distance to compensate the
// sensor's mounting inset. Start at 0; after 'k' calibration, trim L vs R in
// the 'v' monitor so both side sensors read equal when the robot is centred.
#define WALL_OFFSET_L           0
#define WALL_OFFSET_FL          0
#define WALL_OFFSET_FR          0
#define WALL_OFFSET_R           0

// Sharp effective range
#define SHARP_RANGE_MIN_MM      20
#define SHARP_RANGE_MAX_MM      150

// hal_wall_sensor.h prints status under this flag
#define DEBUG_SERIAL            1

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 10: STEERING / CROSS-TRACK ERROR (test_06_v2 — steering.h)
// ─────────────────────────────────────────────────────────────────────────────
// Peter Harrison pattern (D2): the side-wall cross-track error (CTE) is turned
// into a small per-tick rotation-error increment and injected into the rotation
// PD inside motors_update(). It nudges the heading to keep the mouse centred.
//
// CTE sign convention (see steering.h):
//   cte > 0  →  mouse is closer to the RIGHT wall  →  steer LEFT  (+rot_error)
//   cte < 0  →  mouse is closer to the LEFT  wall  →  steer RIGHT (-rot_error)
//
// All gains below are STARTING values — they MUST be tuned on hardware.
// They are loaded into runtime variables in steering.h so they can be adjusted
// over Serial ('+'/'-' commands) without reflashing.

#define CELL_SIZE_MM            180.0f    // maze cell pitch
#define STEER_RUN_DIST_MM       360.0f    // straight-run default length (mm)
                                          // runtime-adjustable with '8' / '9'

// PD gains. KP: yaw-rate (°/s) per mm of CTE. KD: yaw-rate (°/s) per (mm/s) of
// CTE rate — pure P steering is an undamped oscillator, KD supplies the damping.
#define STEERING_KP             0.70f     // °/s per mm
#define STEERING_KD             0.10f     // °/s per (mm/s)

// Clamp on the commanded steering yaw rate (°/s)
#define STEERING_MAX_DPS        45.0f

// HW sign flip — if the mouse steers AWAY from centre (diverges), set to -1.
// Same idea as the FF L/R signs: verified on hardware, not from theory.
#define STEERING_SIGN           1

// CTE-filter EMA alphas. The side sensors report integer mm, so CTE is
// quantised to 0.5mm; differentiating that raw at 100Hz buries the real signal
// under ±45°/s spikes. CTE and its rate are EMA-filtered before the PD.
// Smaller alpha = more smoothing + more lag.
#define STEER_CTE_EMA_ALPHA     0.15f     // filter on CTE itself
#define STEER_RATE_EMA_ALPHA    0.35f     // filter on the CTE rate (KD input)

// A side reading is "wall present" when valid AND <= this (mm). Above it
// (or 255) → that side is an opening. 75 keeps a receding wall at a cell
// junction from injecting a big false CTE.
#define STEER_WALL_PRESENT_MM   75

// Side-sensor reading (mm) when the mouse sits centred — target for the
// single-wall fallback. Measured 2026-05-22: L≈29, R≈33 → ~31.
#define STEER_NOMINAL_SIDE_MM   31.0f

// CTE magnitude clamp (mm) — rejects glitches / cell openings.
#define STEER_CTE_MAX_MM        60.0f

// Steering update cadence (Hz) — informational; the .ino run loop sets it.
#define STEER_UPDATE_HZ         100

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 11: BLE DEBUG (test_06_v2 — ble_debug.h)
// ─────────────────────────────────────────────────────────────────────────────
// Wireless debug over BLE (Nordic UART Service). Project 1 proved BLE coexists
// with the Sharp IR sensors on ADC2 on this hardware — unlike WiFi, which hard-
// locks ADC2. This first increment streams telemetry only; the 'm' ADC-noise
// test lets us confirm BLE adds no meaningful ADC noise before expanding.

#define BLE_DEBUG_NAME          "MM_test07"   // advertised name
#define BLE_TELEMETRY_MS        100           // telemetry push interval (10 Hz)

#endif // CONFIG_H
