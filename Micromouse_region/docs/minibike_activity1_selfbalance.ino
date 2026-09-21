// ═══════════════════════════════════════════════════════════════════
//  minibike_activity1_selfbalance.ino
//  กิจกรรม 1.1 — มินิไบค์ทรงตัว (Self-Balancing with Reaction Wheel)
//
//  บอร์ด: KidBright V1.5 iA (ESP32)
//  IDE:   KidBright IDE (Arduino Framework)
//
//  หลักการ:
//    IMU (MPU-6050/KXTJ3) อ่านมุมเอียง → PID → ควบคุม Reaction Wheel Motor
//
//  ลำดับขั้นตอนก่อนทดสอบ:
//    1. ตั้งรถให้สมดุลที่สุด
//    2. กด Reset บนบอร์ด KidBright
//    3. รอ 2-3 วินาที
//    4. กด SW2 (GPIO14) → calibrate IMU + เริ่มทรงตัว
// ═══════════════════════════════════════════════════════════════════

#include <Wire.h>

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
#define PIN_SW2           14    // ปุ่ม SW2 (Active LOW) → กดเพื่อ Start
#define PIN_MOTOR_IN1     26    // OUT1 → L298N IN1 (Reaction Wheel)
#define PIN_MOTOR_IN2     27    // OUT2 → L298N IN2 (Reaction Wheel)
#define PIN_MOTOR_ENA     25    // Enable (PWM) ← ถ้า L298N ต่อ ENA ไว้ที่ GPIO25

// ─── IMU (MPU-6050) ───────────────────────────────────────────────
#define MPU6050_ADDR      0x68
#define GYRO_SCALE        (250.0f / 32768.0f)    // ±250°/s range

// ─── PID PARAMETERS ───────────────────────────────────────────────
// ★ ปรับค่าเหล่านี้จากการทดสอบ
float Kp = 15.0f;     // ค่าเริ่มต้น — ปรับจนรถทรงตัวได้
float Ki = 0.5f;      // เพิ่มถ้ายังเอียงค้างอยู่ (steady-state error)
float Kd = 0.8f;      // เพิ่มถ้ารถสั่น (damping)

// ─── STATE VARIABLES ──────────────────────────────────────────────
float angle        = 0;       // มุมเอียงปัจจุบัน (องศา)
float pid_error    = 0;
float pid_prev     = 0;
float pid_integral = 0;
float gyro_offset  = 0;       // offset ของ gyro (calibrate ตอนเริ่ม)

bool  system_armed = false;   // false = รอกด SW2
unsigned long last_time = 0;

// ─── MOTOR CONTROL ─────────────────────────────────────────────────
void motor_set(int pwm) {
    // pwm: -255 ถึง +255
    // บวก = หมุนทิศหนึ่ง, ลบ = อีกทิศ
    pwm = constrain(pwm, -255, 255);
    if (pwm > 0) {
        analogWrite(PIN_MOTOR_ENA, pwm);
        digitalWrite(PIN_MOTOR_IN1, HIGH);
        digitalWrite(PIN_MOTOR_IN2, LOW);
    } else if (pwm < 0) {
        analogWrite(PIN_MOTOR_ENA, -pwm);
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
    } else {
        analogWrite(PIN_MOTOR_ENA, 0);
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
    }
}

// ─── MPU-6050 READ ─────────────────────────────────────────────────
int16_t imu_read_accel_y() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3D);   // ACCEL_YOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 2);
    int16_t raw = (Wire.read() << 8) | Wire.read();
    return raw;
}

float imu_read_gyro_z() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x47);   // GYRO_ZOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 2);
    int16_t raw = (Wire.read() << 8) | Wire.read();
    return (float)raw * GYRO_SCALE - gyro_offset;
}

// ─── IMU INIT ──────────────────────────────────────────────────────
void imu_init() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B);   // PWR_MGMT_1
    Wire.write(0x00);   // Wake up
    Wire.endTransmission();

    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x1B);   // GYRO_CONFIG
    Wire.write(0x00);   // ±250°/s
    Wire.endTransmission();
}

// ─── CALIBRATE GYRO ────────────────────────────────────────────────
void calibrate_gyro() {
    Serial.println("Calibrating gyro — keep robot stationary...");
    float sum = 0;
    for (int i = 0; i < 500; i++) {
        Wire.beginTransmission(MPU6050_ADDR);
        Wire.write(0x47);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU6050_ADDR, 2);
        int16_t raw = (Wire.read() << 8) | Wire.read();
        sum += (float)raw * GYRO_SCALE;
        delay(2);
    }
    gyro_offset = sum / 500.0f;
    Serial.printf("Gyro offset Z = %.3f deg/s\n", gyro_offset);
}

// ─── COMPUTE ANGLE FROM ACCEL ──────────────────────────────────────
float get_angle_from_accel() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B);   // ACCEL_XOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 6);
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();
    // คำนวณมุมเอียงจาก accelerometer
    return atan2f((float)ay, (float)az) * 180.0f / PI;
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);   // SDA=GPIO21, SCL=GPIO22 (I2C_NUM_0)

    pinMode(PIN_SW2, INPUT_PULLUP);
    pinMode(PIN_MOTOR_IN1, OUTPUT);
    pinMode(PIN_MOTOR_IN2, OUTPUT);
    pinMode(PIN_MOTOR_ENA, OUTPUT);

    motor_set(0);     // มอเตอร์หยุด
    imu_init();

    Serial.println("Minibike Self-Balance");
    Serial.println(">> ตั้งรถให้สมดุล แล้วกด SW2 เพื่อเริ่ม");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {

    // ─── รอกด SW2 เพื่อ calibrate และ arm ────────────────────────
    if (!system_armed) {
        if (digitalRead(PIN_SW2) == LOW) {
            delay(50);   // debounce
            if (digitalRead(PIN_SW2) == LOW) {
                Serial.println("SW2 pressed — calibrating IMU...");
                calibrate_gyro();
                // เก็บ angle เริ่มต้นจาก accelerometer เป็น reference
                angle = get_angle_from_accel();
                pid_integral = 0;
                pid_prev     = 0;
                last_time    = millis();
                system_armed = true;
                Serial.printf("Armed! Initial angle = %.2f deg\n", angle);
                // รอปล่อยปุ่ม
                while (digitalRead(PIN_SW2) == LOW) delay(10);
            }
        }
        return;
    }

    // ─── Control Loop ────────────────────────────────────────────
    unsigned long now = millis();
    float dt = (now - last_time) / 1000.0f;
    last_time = now;
    if (dt <= 0 || dt > 0.1f) { dt = 0.01f; }   // guard

    // อ่านค่าจาก gyro และ integrate → angle
    float gyro_z = imu_read_gyro_z();   // deg/s
    float accel_angle = get_angle_from_accel();

    // Complementary filter: ผสม gyro (ไว) + accel (ช้าแต่ไม่ drift)
    angle = 0.98f * (angle + gyro_z * dt) + 0.02f * accel_angle;

    // ─── PID ─────────────────────────────────────────────────────
    float setpoint = 0.0f;   // ต้องการมุม = 0 (ตั้งตรง)
    pid_error    = setpoint - angle;
    pid_integral = constrain(pid_integral + pid_error * dt, -50.0f, 50.0f);
    float pid_derivative = (pid_error - pid_prev) / dt;
    pid_prev = pid_error;

    float output = Kp * pid_error + Ki * pid_integral + Kd * pid_derivative;

    // ─── ส่งออกมอเตอร์ ───────────────────────────────────────────
    int pwm = (int)constrain(output, -255.0f, 255.0f);
    motor_set(pwm);

    // ─── Debug ───────────────────────────────────────────────────
    Serial.printf("angle=%.2f  err=%.2f  out=%d\n", angle, pid_error, pwm);

    delay(10);   // ~100Hz control loop
}
