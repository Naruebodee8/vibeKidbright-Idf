// ═══════════════════════════════════════════════════════════════════════════
//  config.h
//  Micromouse ESP32-S3 Configuration
//  
//  Layer: 0 (Configuration)
//  Dependencies: None
//  
//  ★★★ นักเรียน: กรอกค่าที่ได้จากการทดสอบในแต่ละ Workshop ★★★
// ═══════════════════════════════════════════════════════════════════════════
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ★ Lab5 (Feedforward) — ค่าที่ "วัดได้" (คำสั่ง r) แล้วนำมาใส่ (ค้นหาด้านล่าง):
//      FF_SPEED_L_V_PER_MMPS / FF_SPEED_R_V_PER_MMPS   slope (V ต่อ mm/s)
//      FF_BIAS_RUN_L_V / FF_BIAS_RUN_R_V               intercept (static FF)
//      FF_BIAS_START_L_V / FF_BIAS_START_R_V           deadband (จาก Lab1)
//      FF_ACC_V_PER_MMPS2                              = FF_SPEED × Tm
//  → จด Km, Tm จากผลรัน ไปใช้ Lab6 (gaincalc) คำนวณ Kp/Kd ต่อ
//  • ค่าอื่น = พิน/ค่าระบบที่ใช้ร่วมทุกแล็บ — ปกติไม่ต้องแก้
// ═══════════════════════════════════════════════════════════════════════════

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: PIN DEFINITIONS
// │ (ไม่ต้องแก้ไข ยกเว้นต้องการเปลี่ยน hardware)
// └───────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// I2C Bus 0: IMU (MPU6050)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_I2C0_SCL            9
#define PIN_I2C0_SDA            8
#define I2C0_FREQ               400000      // 400 kHz Fast Mode

// ─────────────────────────────────────────────────────────────────────────────
// I2C Bus 1: Wall Sensors (VL6180X)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_I2C1_SCL            1
#define PIN_I2C1_SDA            2
#define I2C1_FREQ               400000      // 400 kHz Fast Mode

// ─────────────────────────────────────────────────────────────────────────────
// VL6180X XSHUT Pins (สำหรับ I2C re-addressing)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_VL_XSHUT_RIGHT      37          // Sensor 0: Right
#define PIN_VL_XSHUT_FR         38          // Sensor 1: Front-Right
#define PIN_VL_XSHUT_FL         39          // Sensor 2: Front-Left
#define PIN_VL_XSHUT_LEFT       40          // Sensor 3: Left

// VL6180X I2C Addresses (หลัง re-address)
#define VL6180X_ADDR_RIGHT      0x30
#define VL6180X_ADDR_FR         0x31
#define VL6180X_ADDR_FL         0x32
#define VL6180X_ADDR_LEFT       0x33
#define VL6180X_ADDR_DEFAULT    0x29        // Default address ก่อน re-address

// ─────────────────────────────────────────────────────────────────────────────
// Motors (DRV8833)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_MOTOR_L_IN1         4
#define PIN_MOTOR_L_IN2         5
#define PIN_MOTOR_R_IN1         6
#define PIN_MOTOR_R_IN2         7

// ─────────────────────────────────────────────────────────────────────────────
// Encoders (N20 with Quadrature Encoder)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_ENC_L_A             15
#define PIN_ENC_L_B             16
#define PIN_ENC_R_A             17
#define PIN_ENC_R_B             18

// ─────────────────────────────────────────────────────────────────────────────
// Battery Monitoring (ADC)
// Voltage divider: RTOP=100k, RBOTTOM=33k (with 0.1uF cap)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BATTERY_ADC         10

// ─────────────────────────────────────────────────────────────────────────────
// User Interface
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_RGB_LED             48          // WS2812 Onboard

#define PIN_BUTTON_START        21          // Start/Stop Button
#define PIN_BUTTON_MODE         41          // Mode Select Button

#define PIN_DIP_SW_0            11          // DIP Switch Bit 0 (LSB)
#define PIN_DIP_SW_1            12          // DIP Switch Bit 1
#define PIN_DIP_SW_2            13          // DIP Switch Bit 2
#define PIN_DIP_SW_3            14          // DIP Switch Bit 3 (MSB)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: MOTOR PARAMETERS
// │ ★ Workshop 1: Motor - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define MOTOR_PWM_FREQ          20000       // 20 kHz (ลดเสียง audible)
#define MOTOR_PWM_RESOLUTION    10          // 10-bit resolution (0-1023)
#define MOTOR_PWM_MAX           1023        // Maximum PWM value
#define MOTOR_PWM_MIN           590          // ค่าต่ำสุดที่มอเตอร์เริ่มหมุน ← ทดสอบใน Workshop 1 (L=560, R=590)

// Motor direction compensation (1 หรือ -1 เพื่อปรับทิศทาง)
#define MOTOR_L_DIRECTION       1           // 1 = ปกติ, -1 = กลับทิศ
#define MOTOR_R_DIRECTION       1           // 1 = ปกติ, -1 = กลับทิศ

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: ROBOT PHYSICAL PARAMETERS
// │ ★ Workshop 2: Encoder - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define WHEEL_DIAMETER_MM       33.0        // เส้นผ่านศูนย์กลางล้อ (mm)
#define WHEEL_BASE_MM           78.0        // ระยะห่างระหว่างล้อ (mm) ← วัดจากหุ่นยนต์จริง
#define COUNTS_PER_REV          2774         // Encoder counts ต่อรอบ (X4 quadrature) ← ทดสอบใน Workshop 2
#define GEAR_RATIO              1.0         // Gear ratio (ถ้ามี gearbox)

// Calculated parameters (อย่าแก้ไข)
#define WHEEL_CIRCUMFERENCE_MM  (WHEEL_DIAMETER_MM * PI)                    // = 105.24 mm
#define MM_PER_COUNT            (WHEEL_CIRCUMFERENCE_MM / COUNTS_PER_REV)   // = 0.1291 mm/count

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: IMU CALIBRATION
// │ ★ Workshop 3: IMU - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define IMU_I2C_ADDR            0x68        // MPU6050 default address

// Gyroscope offset (raw values เมื่อหุ่นยนต์อยู่นิ่ง)
#define GYRO_OFFSET_X           0.0         // ← จาก Workshop 3 calibration
#define GYRO_OFFSET_Y           0.0         // ← จาก Workshop 3 calibration
#define GYRO_OFFSET_Z           0.0         // ← จาก Workshop 3 calibration

// Accelerometer offset
#define ACCEL_OFFSET_X          0.0         // ← จาก Workshop 3 calibration
#define ACCEL_OFFSET_Y          0.0         // ← จาก Workshop 3 calibration
#define ACCEL_OFFSET_Z          0.0         // ← จาก Workshop 3 calibration

// Gyroscope scale (degrees per second per LSB)
#define GYRO_SCALE              (250.0 / 32768.0)   // ±250°/s range

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: WALL SENSOR PARAMETERS
// │ ★ Workshop 4: Wall Sensors - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define WALL_SENSOR_COUNT       4

// Wall detection threshold (mm)
#define WALL_THRESHOLD_FRONT    100         // ระยะที่ถือว่ามีกำแพงด้านหน้า ← ทดสอบใน Workshop 4
#define WALL_THRESHOLD_SIDE     80          // ระยะที่ถือว่ามีกำแพงด้านข้าง ← ทดสอบใน Workshop 4

// Sensor offset (ระยะจาก sensor ถึงผนังเมื่ออยู่กลาง cell)
#define WALL_OFFSET_FRONT       50          // ← ทดสอบใน Workshop 4
#define WALL_OFFSET_SIDE        40          // ← ทดสอบใน Workshop 4

// VL6180X measurement parameters
#define VL6180X_RANGE_MAX       200         // Maximum reliable range (mm)
#define VL6180X_RANGE_MIN       5           // Minimum reliable range (mm)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PID PARAMETERS
// │ ★ Workshop 5: PID - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

// Velocity PID (ควบคุมความเร็วล้อแต่ละข้าง)
#define PID_VEL_KP              1.0         // ← ทดสอบใน Workshop 5
#define PID_VEL_KI              0.1         // ← ทดสอบใน Workshop 5
#define PID_VEL_KD              0.01        // ← ทดสอบใน Workshop 5
#define PID_VEL_MAX_OUTPUT      MOTOR_PWM_MAX
#define PID_VEL_MIN_OUTPUT      (-MOTOR_PWM_MAX)

// Heading PID (ควบคุมทิศทาง/มุม)
#define PID_HEAD_KP             2.0         // ← ทดสอบใน Workshop 5
#define PID_HEAD_KI             0.0         // ← ทดสอบใน Workshop 5
#define PID_HEAD_KD             0.1         // ← ทดสอบใน Workshop 5
#define PID_HEAD_MAX_OUTPUT     500.0
#define PID_HEAD_MIN_OUTPUT     (-500.0)

// Wall Follow PID (เกาะกำแพง)
#define PID_WALL_KP             0.5         // ← ทดสอบใน Workshop 5
#define PID_WALL_KI             0.0         // ← ทดสอบใน Workshop 5
#define PID_WALL_KD             0.05        // ← ทดสอบใน Workshop 5
#define PID_WALL_MAX_OUTPUT     200.0
#define PID_WALL_MIN_OUTPUT     (-200.0)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: MOTION PARAMETERS
// │ ★ Workshop 6: Motion Control - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

// Maze cell size (ตามกติกาการแข่งขัน)
#define CELL_SIZE_MM            180.0       // ขนาด cell (mm)

// Search run parameters (ความเร็วต่ำ ปลอดภัย)
#define SEARCH_VELOCITY         200.0       // mm/s ← ทดสอบใน Workshop 6
#define SEARCH_ACCELERATION     500.0       // mm/s² ← ทดสอบใน Workshop 6
#define SEARCH_DECELERATION     500.0       // mm/s² ← ทดสอบใน Workshop 6

// Speed run parameters (ความเร็วสูง)
#define SPEED_RUN_VELOCITY      500.0       // mm/s ← ทดสอบใน Workshop 6
#define SPEED_RUN_ACCELERATION  1000.0      // mm/s² ← ทดสอบใน Workshop 6
#define SPEED_RUN_DECELERATION  1000.0      // mm/s² ← ทดสอบใน Workshop 6

// Turn parameters
#define TURN_VELOCITY           150.0       // mm/s ขณะเลี้ยว ← ทดสอบใน Workshop 6
#define TURN_ACCELERATION       300.0       // mm/s² ← ทดสอบใน Workshop 6

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: BATTERY PARAMETERS
// └───────────────────────────────────────────────────────────────────────────

// Voltage divider ratio: (RTOP + RBOTTOM) / RBOTTOM = (100 + 33) / 33 = 4.03
#define BATTERY_DIVIDER_RATIO   4.03

// ESP32 ADC reference
#define ADC_REFERENCE_MV        3300        // 3.3V reference
#define ADC_RESOLUTION          4095        // 12-bit ADC

// 2S LiPo voltage thresholds (mV)
#define BATTERY_FULL_MV         8400        // Full charge (4.2V × 2)
#define BATTERY_NOMINAL_MV      7400        // Nominal (3.7V × 2)
#define BATTERY_LOW_MV          7000        // Low warning (3.5V × 2)
#define BATTERY_CRITICAL_MV     6400        // Critical shutdown (3.2V × 2)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 9: SYSTEM PARAMETERS
// │ (ไม่ต้องแก้ไข ยกเว้นต้องการปรับ performance)
// └───────────────────────────────────────────────────────────────────────────

// Control loop timing
#define CONTROL_LOOP_FREQ_HZ    1000        // 1 kHz control loop
#define CONTROL_LOOP_PERIOD_US  1000        // 1000 µs = 1 ms
#define CONTROL_LOOP_PERIOD_MS  1           // 1 ms

// Sensor task timing
#define SENSOR_UPDATE_FREQ_HZ   40          // ~40 Hz wall sensor update

// FreeRTOS task priorities (สูงกว่า = สำคัญกว่า)
#define CONTROL_TASK_PRIORITY   10          // Highest - real-time control
#define SENSOR_TASK_PRIORITY    8           // High - sensor reading
#define APP_TASK_PRIORITY       5           // Medium - application logic
#define TELEMETRY_TASK_PRIORITY 3           // Low - debug/telemetry
#define UI_TASK_PRIORITY        2           // Lowest - user interface

// Task stack sizes (bytes)
#define CONTROL_TASK_STACK      4096
#define SENSOR_TASK_STACK       4096
#define APP_TASK_STACK          8192
#define TELEMETRY_TASK_STACK    4096
#define UI_TASK_STACK           2048

// Core assignment
#define CONTROL_TASK_CORE       1           // Core 1 - dedicated for real-time
#define SENSOR_TASK_CORE        1           // Core 1 - with control
#define APP_TASK_CORE           0           // Core 0 - shared with WiFi/BLE
#define TELEMETRY_TASK_CORE     0           // Core 0
#define UI_TASK_CORE            0           // Core 0

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 10: MAZE PARAMETERS
// └───────────────────────────────────────────────────────────────────────────

#define MAZE_SIZE               16          // 16×16 cells

// Start position (มุมล่างซ้าย)
#define START_X                 0
#define START_Y                 0
#define START_HEADING           90          // หันไปทาง +Y (North)

// Goal position (กลาง maze)
#define GOAL_X_MIN              7
#define GOAL_Y_MIN              7
#define GOAL_X_MAX              8
#define GOAL_Y_MAX              8

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 11: DIP SWITCH MODES
// └───────────────────────────────────────────────────────────────────────────

typedef enum {
    MODE_IDLE               = 0b0000,   // 0: Wait for button
    MODE_SEARCH_RUN         = 0b0001,   // 1: Search run (explore maze)
    MODE_SPEED_RUN          = 0b0010,   // 2: Speed run (fast)
    MODE_SPEED_RUN_DIAG     = 0b0011,   // 3: Speed run with diagonals
    MODE_CALIBRATE_SENSORS  = 0b0100,   // 4: Calibrate wall sensors
    MODE_CALIBRATE_GYRO     = 0b0101,   // 5: Calibrate gyroscope
    MODE_CALIBRATE_MOTORS   = 0b0110,   // 6: Motor/Encoder test
    MODE_DEBUG_TELEMETRY    = 0b0111,   // 7: Enable WiFi debug
    MODE_TEST_FORWARD       = 0b1000,   // 8: Move forward test
    MODE_TEST_TURN          = 0b1001,   // 9: Turn test
    MODE_TEST_WALL_FOLLOW   = 0b1010,   // 10: Wall following test
    MODE_RESERVED_11        = 0b1011,   // 11: Reserved
    MODE_RESERVED_12        = 0b1100,   // 12: Reserved
    MODE_RESERVED_13        = 0b1101,   // 13: Reserved
    MODE_RESERVED_14        = 0b1110,   // 14: Reserved
    MODE_RESERVED_15        = 0b1111    // 15: Reserved
} RobotMode_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 12: DEBUG FLAGS
// └───────────────────────────────────────────────────────────────────────────

#define DEBUG_SERIAL            1           // Enable Serial debug output
#define DEBUG_MOTORS            0           // Debug motor control
#define DEBUG_ENCODERS          0           // Debug encoder readings
#define DEBUG_IMU               0           // Debug IMU readings
#define DEBUG_WALL_SENSORS      0           // Debug wall sensor readings
#define DEBUG_PID               0           // Debug PID controllers
#define DEBUG_MOTION            0           // Debug motion control
#define DEBUG_MAZE              0           // Debug maze solver

// Serial baud rate
#define SERIAL_BAUD_RATE        115200

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 13: UTILITY MACROS
// └───────────────────────────────────────────────────────────────────────────

// Constrain value between min and max
#define CONSTRAIN(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// Map value from one range to another
#define MAP(x, in_min, in_max, out_min, out_max) \
    (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

// Absolute value
#define ABS(x)                  ((x) < 0 ? -(x) : (x))

// Sign of value (-1, 0, +1)
#define SIGN(x)                 ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

// Convert degrees to radians
#define DEG_TO_RAD(deg)         ((deg) * PI / 180.0)

// Convert radians to degrees
#define RAD_TO_DEG(rad)         ((rad) * 180.0 / PI)

// Square of value
#define SQ(x)                   ((x) * (x))

#endif // CONFIG_H
