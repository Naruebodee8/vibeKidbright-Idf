// ═══════════════════════════════════════════════════════════════════════════
//  config.h  —  test_05_v2 (minimal, grows per module)
//
//  For A2 (profile.h validation), only the constants profile.h needs.
//  Will grow as A3 (hal_battery), A4-5 (motors_controller), A6 (control_task)
//  add their own requirements.
// ═══════════════════════════════════════════════════════════════════════════
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ★ Lab6 (Control/PID) — ค่าที่ "คำนวณ" (gaincalc) แล้วปรับละเอียด (ค้นหาด้านล่าง):
//      FWD_KP / FWD_KD / FWD_KI    คุมระยะเดินหน้า
//      ROT_KP / ROT_KD / ROT_KI    คุมการหมุน
//  → ได้จาก gaincalc <Km> <Tm>  (ค่า Km, Tm มาจาก Lab5 Feedforward)
//  • ใช้ค่า FF จาก Lab5 ด้วย (FF_SPEED/FF_BIAS/FF_ACC) — ต้องทำ Lab5 ก่อน
//  • ค่าอื่น = พิน/ค่าระบบที่ใช้ร่วมทุกแล็บ — ปกติไม่ต้องแก้
// ═══════════════════════════════════════════════════════════════════════════

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
// Conservative starting point — refine in A2 once rotation HW tested
#define PROFILE_ROT_THRESHOLD_DEG   0.25f

// Creep speed during BRAKING when final_speed = 0
// Prevents profile from getting stuck at speed=0 with remaining > threshold.
// At 5 mm/s × 2ms tick = 0.01 mm/tick → covers threshold in ~20 ticks (40 ms).
// Trade-off: ~5 mm/s creep at end of profile. See R4 in design hub.
#define PROFILE_FINISH_CREEP_SPEED  5.0f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 4: BATTERY MONITORING (A3 — added 2026-05-21)
// ─────────────────────────────────────────────────────────────────────────────
// HW: 2S LiPo → voltage divider (RTOP=100k, RBOT=33k) → PIN 10 ADC
// Divider ratio (4.03) verified vs multimeter in Project1 — same HW

#define PIN_BATTERY_ADC         10
// Calibrated against multimeter on actual HW (2026-05-21):
//   theoretical = (100k+33k)/33k = 4.03  ← what resistor labels say
//   measured    = 4.148  ← real ratio (multimeter 7.87V vs ADC 1.897V)
//   Resistor tolerance (~5%) shifts effective ratio. Project1 also used 4.14.
// To recalibrate: run 'c' in test sketch, follow prompts.
#define BATTERY_DIVIDER_RATIO   2.647f
#define BATTERY_DIVIDER_X100    265       // integer form (avoid float in HAL hot path)

//#define ADC_RESOLUTION_BITS     12

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
#define PIN_MOTOR_L_IN1         4
#define PIN_MOTOR_L_IN2         5
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
// Per-wheel BIAS uses START/RUN blend (Project1 pattern):
//   Near v=0 (≤V0):     use BIAS_START (overcome static friction)
//   At v≥V1:            use BIAS_RUN  (regression intercept, lower)
//   Between V0 and V1:  linear blend
// Solves the "stiction vs overshoot" trade-off.

#define FF_BIAS_START_L_V        4.500f    // Project1 Lab2b: 4500 mV stiction breakaway
#define FF_BIAS_START_R_V        4.500f
#define FF_BIAS_RUN_L_V          0.100f    // Project1 Lab3: regression intercept (on floor)
#define FF_BIAS_RUN_R_V          0.133f
#define FF_BIAS_BLEND_V0_MMPS    0.0f
#define FF_BIAS_BLEND_V1_MMPS    120.0f

// Speed FF: voltage needed per (mm/s) of velocity
#define FF_SPEED_L_V_PER_MMPS    0.03861f  // Project1: 5.07 mV/(mm/s)
#define FF_SPEED_R_V_PER_MMPS    0.03677f  // Project1: 5.39 mV/(mm/s) — R needs more
#define FF_ACC_V_PER_MMPS2       0.00046f  // Project1: 0.30 mV/(mm/s²)

#define FF_MOTOR_TAU_MS          150       // motor time constant (Project1)

// Rotation FF (separate from linear — for in-place spin)
#define FF_ROT_BIAS_V            4.550f    // Project1
#define FF_ROT_SPEED_V_PER_DPS   0.00310f
#define FF_ROT_ACC_V_PER_DPS2    0.00174f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 7: PD CONTROLLERS (position-based, volts output) — A4
// ─────────────────────────────────────────────────────────────────────────────
// Forward position PD: error in mm, output in V
// Rotation position PD: error in °, output in V
// CTE injection enters rotation PD (D2 Peter pattern)

// A7 Lab 1 iterative tuning (2026-05-22):
//   Iter 1: ROT_KP=8 → severe weave (gyro ±130°/s). Lowered to 1.0 → ±30°/s.
//   Iter 2: FWD_KD=1.1 → after motion, encoder noise × Kd × 500Hz saturated
//           output ±5V alternating tick (motor hums, no movement).
//           Lowered FWD_KD to 0.1 (10× reduction).
//   Iter 2: FWD_MAX=2 → too tight; raised to 5.0V for full PD authority.
#define FWD_KP                   0.011f      // Peter default — tune in HW
#define FWD_KD                   0.0004f      // reduced from 1.1 — was amplifying encoder noise at standstill
#define FWD_MAX_VOLTS            5.0f      // raised from 2.0 — allow PD to brake harder

// A7 result: ROT_KD=0 → P-only heading hold = underdamped, ~5Hz weave.
// In our formula, D-term math reduces to -ROT_KD × gyro_dps (direct yaw-rate
// damping). 0.03 gives ~1.8V damping at 60°/s weave amplitude.
#define ROT_KP                   1.0f      // heading-error gain (V per degree)
#define ROT_KD                   0.03f     // yaw-rate damping — kills the weave
#define ROT_MAX_VOLTS            2.5f      // Project1: 2500 mV

// Final motor voltage clamp (safety — should be below typical battery full 8.4V)
#define MAX_MOTOR_VOLTS          8.0f

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 7B: IMU (A6 — MPU6050 via I2C bus 0)
// ─────────────────────────────────────────────────────────────────────────────
// Used for gyro Z (rotation rate) — feeds rot_change to motors_controller (D15)

#define PIN_I2C0_SCL             9
#define PIN_I2C0_SDA             8
#define I2C0_FREQ                400000        // 400 kHz Fast Mode

#define IMU_I2C_ADDR             0x68          // MPU6050 default

// Gyro/accel offsets — measured Workshop 3 calibration (test_05_feedforward HW)
// Will be re-calibrated at every boot via imu_calibrate_gyro(500) — these are
// fallback values used if calibration skipped.
#define GYRO_OFFSET_X           -478.7f         // ← จาก Workshop 3 calibration
#define GYRO_OFFSET_Y            47.1f      // ←f จาก Workshop 3 calibration
#define GYRO_OFFSET_Z           -30.0f        // ← จาก Workshop 3 calibration 

#define ACCEL_OFFSET_X          -828.4f         
#define ACCEL_OFFSET_Y          -127.8f       
#define ACCEL_OFFSET_Z          1264.5f        

// Gyro scale (for ±250°/s full-scale range)
//   raw_reading × GYRO_SCALE = degrees per second
#define GYRO_SCALE               (250.0f / 32768.0f)

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 8: Macros + back-compat aliases (for copied HAL headers)
// ─────────────────────────────────────────────────────────────────────────────
// hal_motor.h (copied from test_05) uses these macros/names from the old config.
// Define here so we don't need to modify the proven HAL.

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

#endif // CONFIG_H
