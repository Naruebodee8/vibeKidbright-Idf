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
//  ★ Lab4 (IR) — ค่าที่ "ต้องจูน/บันทึก" ในแล็บนี้ (ค้นหาชื่อด้านล่าง):
//      WALL_THRESHOLD_FRONT / WALL_THRESHOLD_SIDE   เกณฑ์ว่ามีกำแพง (คำสั่ง t)
//      WALL_OFFSET_L / _FL / _FR / _R               ชดเชยระยะต่อตัว
//  • ค่าอื่น = พิน/ค่าระบบที่ใช้ร่วมทุกแล็บ — ปกติไม่ต้องแก้
//  • Lab4 ใช้ BLE เท่านั้น (WiFi กวน IR บน ADC2)
// ═══════════════════════════════════════════════════════════════════════════

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: PIN DEFINITIONS
// │ (ไม่ต้องแก้ไข ยกเว้นต้องการเปลี่ยน hardware)
// └───────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// I2C Bus 0: IMU (MPU6050)  — ใช้ใน test_03, test_05, test_06
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_I2C0_SCL            9
#define PIN_I2C0_SDA            8
#define I2C0_FREQ               400000      // 400 kHz Fast Mode

// ─────────────────────────────────────────────────────────────────────────────
// Sharp GP2Y0A51SK0F — ADC Pins (ใช้ใน test_04_ir_sensor)
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_IR_RIGHT            14          // ADC GPIO 14 (Right sensor)
#define PIN_IR_FRONT_R          13          // ADC GPIO 13 (Front-Right sensor)
#define PIN_IR_FRONT_L          12          // ADC GPIO 12 (Front-Left sensor)
#define PIN_IR_LEFT             11          // ADC GPIO 11 (Left sensor)

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

#define PIN_BUTTON_START        21
#define PIN_BUTTON_MODE         41

#define PIN_DIP_SW_0            11
#define PIN_DIP_SW_1            12
#define PIN_DIP_SW_2            13
#define PIN_DIP_SW_3            14

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: MOTOR PARAMETERS
// │ ★ Workshop 1: Motor - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define MOTOR_PWM_FREQ          20000
#define MOTOR_PWM_RESOLUTION    10
#define MOTOR_PWM_MAX           1023
#define MOTOR_PWM_MIN           590         // ← ทดสอบใน Workshop 1

#define MOTOR_L_DIRECTION       1
#define MOTOR_R_DIRECTION       1

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: ROBOT PHYSICAL PARAMETERS
// │ ★ Workshop 2: Encoder - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define WHEEL_DIAMETER_MM       33.0
#define WHEEL_BASE_MM           79.0
#define COUNTS_PER_REV          2774         // ← ทดสอบใน Workshop 2
#define GEAR_RATIO              1.0

#define WHEEL_CIRCUMFERENCE_MM  (WHEEL_DIAMETER_MM * PI)
#define MM_PER_COUNT            (WHEEL_CIRCUMFERENCE_MM / COUNTS_PER_REV)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: IMU CALIBRATION
// │ ★ Workshop 3: IMU - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define IMU_I2C_ADDR            0x68

#define GYRO_OFFSET_X           -313.6      // ← จาก Workshop 3
#define GYRO_OFFSET_Y            88.7
#define GYRO_OFFSET_Z           -32.1
#define ACCEL_OFFSET_X          -828.4
#define ACCEL_OFFSET_Y          -127.8
#define ACCEL_OFFSET_Z          1264.5
#define GYRO_SCALE              (250.0 / 32768.0)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: SHARP IR SENSOR PARAMETERS
// │ ★ Workshop 4: IR Sensor - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define WALL_SENSORS_NUM        4

// Sharp sensor supply voltage (จริงๆ ต้องการ 4.5-5.5V แต่ใช้ 3.3V)
// *** ค่า LUT ใน hal_wall_sensor.h อาจต้องปรับ ถ้า curve ต่างจาก typical ***
#define SHARP_SUPPLY_MV         3300

// จำนวน ADC samples ต่อการอ่าน 1 ครั้ง (เพิ่มเพื่อลด noise)
#define SHARP_ADC_SAMPLES       4

// Wall detection threshold (mm)
// ★ ค่าเริ่มต้น — ปรับหลังจากทดสอบจริงในสนาม
#define WALL_THRESHOLD_FRONT    60          // ← ทดสอบใน Workshop 4
#define WALL_THRESHOLD_SIDE     70          // ← ทดสอบใน Workshop 4

// Sensor offsets (mm) — ลบออกจากค่าที่อ่านได้ เพื่อชดเชยตำแหน่งติดตั้ง
// เริ่มที่ 0 ทั้งหมด แล้วปรับหลัง calibrate
#define WALL_OFFSET_L           70           // ← ปรับใน Workshop 4
#define WALL_OFFSET_FL          60
#define WALL_OFFSET_FR          60
#define WALL_OFFSET_R           80

// Sharp effective range
#define SHARP_RANGE_MIN_MM      25          // ใกล้สุดที่อ่านได้ (~2 cm)
#define SHARP_RANGE_MAX_MM      255         // ไกลสุดที่อ่านได้ (~15 cm)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PID PARAMETERS
// │ ★ Workshop 5: PID - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define PID_VEL_KP              1.0
#define PID_VEL_KI              0.1
#define PID_VEL_KD              0.01
#define PID_VEL_MAX_OUTPUT      MOTOR_PWM_MAX
#define PID_VEL_MIN_OUTPUT      (-MOTOR_PWM_MAX)

#define PID_HEAD_KP             2.0
#define PID_HEAD_KI             0.0
#define PID_HEAD_KD             0.1
#define PID_HEAD_MAX_OUTPUT     500.0
#define PID_HEAD_MIN_OUTPUT     (-500.0)

#define PID_WALL_KP             0.5
#define PID_WALL_KI             0.0
#define PID_WALL_KD             0.05
#define PID_WALL_MAX_OUTPUT     200.0
#define PID_WALL_MIN_OUTPUT     (-200.0)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: MOTION PARAMETERS
// │ ★ Workshop 6: Motion Control - กรอกค่าที่ทดสอบได้ ★
// └───────────────────────────────────────────────────────────────────────────

#define CELL_SIZE_MM            180.0

#define SEARCH_VELOCITY         200.0
#define SEARCH_ACCELERATION     500.0
#define SEARCH_DECELERATION     500.0

#define SPEED_RUN_VELOCITY      500.0
#define SPEED_RUN_ACCELERATION  1000.0
#define SPEED_RUN_DECELERATION  1000.0

#define TURN_VELOCITY           150.0
#define TURN_ACCELERATION       300.0

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: BATTERY PARAMETERS
// └───────────────────────────────────────────────────────────────────────────

#define BATTERY_DIVIDER_RATIO   4.03
#define ADC_REFERENCE_MV        3300
#define ADC_RESOLUTION          4095

#define BATTERY_FULL_MV         8400
#define BATTERY_NOMINAL_MV      7400
#define BATTERY_LOW_MV          7000
#define BATTERY_CRITICAL_MV     6400

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 9: SYSTEM PARAMETERS
// └───────────────────────────────────────────────────────────────────────────

#define CONTROL_LOOP_FREQ_HZ    1000
#define CONTROL_LOOP_PERIOD_US  1000
#define CONTROL_LOOP_PERIOD_MS  1

#define SENSOR_UPDATE_FREQ_HZ   40

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 10: TELEMETRY CONFIGURATION
// └───────────────────────────────────────────────────────────────────────────

#define TELEMETRY_WIFI_ENABLED   0   // ปิด WiFi — IR อยู่บน ADC2 ที่ WiFi ล็อกทำให้ค่ามั่ว; ใช้ BLE แทน
#define TELEMETRY_SERIAL_ENABLED 1

#define WIFI_AP_SSID             "Micromouse"
#define WIFI_AP_PASSWORD         "12345678"
#define WIFI_AP_CHANNEL          1
#define WIFI_AP_MAX_CONNECTIONS  2

#define WEBSOCKET_PORT           80
#define TELEMETRY_UPDATE_RATE_HZ 10
#define TELEMETRY_BUFFER_SIZE    1024

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 11: DEBUG FLAGS
// └───────────────────────────────────────────────────────────────────────────

#define DEBUG_SERIAL            1
#define DEBUG_WALL_SENSORS      0

#define SERIAL_BAUD_RATE        115200

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 12: UTILITY MACROS
// └───────────────────────────────────────────────────────────────────────────

#define CONSTRAIN(x, mn, mx)    ((x) < (mn) ? (mn) : ((x) > (mx) ? (mx) : (x)))
#define ABS(x)                  ((x) < 0 ? -(x) : (x))
#define SIGN(x)                 ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define DEG_TO_RAD(deg)         ((deg) * PI / 180.0)
#define RAD_TO_DEG(rad)         ((rad) * 180.0 / PI)
#define SQ(x)                   ((x) * (x))

#endif // CONFIG_H
