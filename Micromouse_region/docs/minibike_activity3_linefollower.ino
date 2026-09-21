// ═══════════════════════════════════════════════════════════════════
//  minibike_activity3_linefollower.ino
//  กิจกรรม 3.1 + 3.2 — อ่านเซนเซอร์ IR และ Line Follower PID
//
//  บอร์ด: KidBright V1.5 iA (ESP32) บนตัวรถ
//  IDE:   KidBright IDE (Arduino Framework)
//
//  เซนเซอร์: TCRT5000 × 2 ตัว
//    - ซ้าย  → I3 (GPIO34) — ADC Input-only
//    - ขวา   → I4 (GPIO35) — ADC Input-only
//
//  Servo (แกนเลี้ยว):
//    - Servo1 → GPIO15
//
//  Motor (ขับเคลื่อน):
//    - OUT1 (GPIO26) → L298N IN1
//    - OUT2 (GPIO27) → L298N IN2
//    - GPIO25 → L298N ENA (PWM)
//
//  หลักการ PID:
//    Error    = IR_ขวา - IR_ซ้าย  (กำหนด setpoint = 0)
//    Output   → ปรับมุม Servo เลี้ยว
//
//  ขั้นตอนทดสอบกิจกรรม 3.2:
//    1. หมุนขาตั้งทั้งสองข้างให้ยาวออกมาเสมอพื้น (รถไม่ล้ม)
//    2. วางรถบนเส้น → กด SW2 เพื่อเริ่ม
//    3. ปรับ Kp, Ki, Kd จนรถวิ่งตามเส้นได้ดี
// ═══════════════════════════════════════════════════════════════════

#include <ESP32Servo.h>   // ติดตั้งผ่าน Library Manager

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
#define PIN_IR_LEFT       34    // I3 (GPIO34) → IR ซ้าย
#define PIN_IR_RIGHT      35    // I4 (GPIO35) → IR ขวา
#define PIN_SERVO         15    // Servo1 บนบอร์ด KidBright iA
#define PIN_MOTOR_IN1     26    // OUT1 → L298N IN1
#define PIN_MOTOR_IN2     27    // OUT2 → L298N IN2
#define PIN_MOTOR_ENA     25    // L298N ENA (PWM speed)
#define PIN_SW2           14    // SW2 → Start/Stop

// ─── SERVO SETTINGS ───────────────────────────────────────────────
#define SERVO_CENTER      90    // มุมเซอร์โวตรงกลาง (ปรับตาม hardware)
#define SERVO_MAX_TURN    30    // ± สูงสุดที่เลี้ยวได้ (องศา)

// ─── MOTOR SETTINGS ───────────────────────────────────────────────
#define MOTOR_BASE_SPEED  150   // ความเร็วพื้นฐาน (0-255)
#define MOTOR_MAX_SPEED   220   // ความเร็วสูงสุด

// ─── PID PARAMETERS ───────────────────────────────────────────────
// ★ ปรับค่าเหล่านี้จากการทดสอบ
float Kp = 0.05f;    // เริ่มต้น — ค่อยๆ เพิ่มจนรถไม่สั่าย
float Ki = 0.001f;   // เพิ่มถ้ารถยังเบนไม่กลับ
float Kd = 0.02f;    // เพิ่มถ้ารถสั่ายมาก

// ─── IR FILTER (ค่าเฉลี่ยเคลื่อนที่) ─────────────────────────────
#define FILTER_SIZE       5     // ยิ่งเยอะยิ่งกรองดีแต่หน่วงมาก
int ir_left_buf[FILTER_SIZE]  = {0};
int ir_right_buf[FILTER_SIZE] = {0};
int filter_idx = 0;

// ─── THRESHOLD ────────────────────────────────────────────────────
// ★ ปรับตามสภาพแสงและสีของเส้น
#define LINE_THRESHOLD    2000  // ค่า ADC ที่ถือว่าพบเส้นสีดำ (< 2000 = ดำ)

// ─── STATE ────────────────────────────────────────────────────────
bool   running     = false;
float  pid_integral = 0;
float  pid_prev_err = 0;

Servo servo_front;

// ─── MOTOR CONTROL ─────────────────────────────────────────────────
void motor_set(int pwm) {
    pwm = constrain(pwm, -MOTOR_MAX_SPEED, MOTOR_MAX_SPEED);
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

// ─── IR READ WITH FILTER ───────────────────────────────────────────
int read_ir_filtered(int pin, int* buf) {
    buf[filter_idx % FILTER_SIZE] = analogRead(pin);
    long sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) sum += buf[i];
    return (int)(sum / FILTER_SIZE);
}

bool is_black(int ir_val) {
    return ir_val < LINE_THRESHOLD;   // ค่าต่ำ = สีดำ (สะท้อนน้อย)
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);   // 12-bit ADC

    pinMode(PIN_MOTOR_IN1, OUTPUT);
    pinMode(PIN_MOTOR_IN2, OUTPUT);
    pinMode(PIN_MOTOR_ENA, OUTPUT);
    pinMode(PIN_SW2, INPUT_PULLUP);
    motor_set(0);

    servo_front.attach(PIN_SERVO, 500, 2400);
    servo_front.write(SERVO_CENTER);

    Serial.println("Minibike Line Follower");
    Serial.println("กิจกรรม 3.1: เปิด Serial Plotter เพื่อดูค่า IR");
    Serial.println("กด SW2 เพื่อเริ่ม/หยุด Line Follower");
    Serial.println("Format: IR_LEFT,IR_RIGHT");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {

    // ─── Toggle Start/Stop ด้วย SW2 ─────────────────────────────
    if (digitalRead(PIN_SW2) == LOW) {
        delay(50);
        if (digitalRead(PIN_SW2) == LOW) {
            running = !running;
            if (!running) {
                motor_set(0);
                servo_front.write(SERVO_CENTER);
                pid_integral = 0;
                pid_prev_err = 0;
                Serial.println("[STOP]");
            } else {
                Serial.println("[START] Line Follower running...");
            }
            while (digitalRead(PIN_SW2) == LOW) delay(10);
        }
    }

    // ─── อ่าน IR (ทำทุก loop เพื่อ Serial Plotter ใน 3.1) ───────
    int ir_l = read_ir_filtered(PIN_IR_LEFT,  ir_left_buf);
    int ir_r = read_ir_filtered(PIN_IR_RIGHT, ir_right_buf);
    filter_idx++;

    // กิจกรรม 3.1: ดูค่าใน Serial Plotter
    Serial.printf("%d,%d\n", ir_l, ir_r);

    if (!running) {
        delay(20);
        return;
    }

    // ─── Line Detection Logic ─────────────────────────────────────
    bool left_on_line  = is_black(ir_l);
    bool right_on_line = is_black(ir_r);

    if (left_on_line && right_on_line) {
        // ─── ทั้งสองเจอเส้น → หยุด ──────────────────────────────
        motor_set(0);
        servo_front.write(SERVO_CENTER);
        Serial.println("  [STOP] Both sensors on line");

    } else {
        // ─── PID Control ─────────────────────────────────────────
        // Error = IR_ขวา - IR_ซ้าย
        // ถ้า Error > 0 → ขวาเจอเส้นมากกว่า → เลี้ยวขวา
        // ถ้า Error < 0 → ซ้ายเจอเส้นมากกว่า → เลี้ยวซ้าย
        float setpoint = 0;
        float error    = (float)(ir_r - ir_l);   // ส่วนต่าง

        // ชดเชยค่าความแตกต่างของเซนเซอร์ (offset compensation)
        // ★ ถ้าเซนเซอร์สองตัวค่าต่างกันตอนอยู่บนสีขาวเหมือนกัน → หักออก
        // float sensor_offset = 0;  // ปรับค่านี้ถ้าจำเป็น
        // error -= sensor_offset;

        pid_integral += error * 0.02f;   // 20ms loop
        pid_integral  = constrain(pid_integral, -500.0f, 500.0f);
        float derivative = (error - pid_prev_err) / 0.02f;
        pid_prev_err = error;

        float pid_out = Kp * error + Ki * pid_integral + Kd * derivative;
        pid_out = constrain(pid_out, -(float)SERVO_MAX_TURN, (float)SERVO_MAX_TURN);

        // ─── ปรับมุมเซอร์โว ──────────────────────────────────────
        int servo_angle = SERVO_CENTER + (int)pid_out;
        servo_angle = constrain(servo_angle,
                                SERVO_CENTER - SERVO_MAX_TURN,
                                SERVO_CENTER + SERVO_MAX_TURN);
        servo_front.write(servo_angle);

        // ─── ขับมอเตอร์ (ลดความเร็วเมื่อเลี้ยว) ─────────────────
        int turn_reduce = (int)(abs(pid_out) * 0.5f);   // ลดความเร็วขณะเลี้ยว
        int speed = MOTOR_BASE_SPEED - turn_reduce;
        speed = constrain(speed, 80, MOTOR_MAX_SPEED);
        motor_set(speed);

        Serial.printf("  err=%.0f pid=%.1f servo=%d spd=%d\n",
                      error, pid_out, servo_angle, speed);
    }

    delay(20);   // 20ms loop (ตาม PDF กิจกรรม 3.2)
}
