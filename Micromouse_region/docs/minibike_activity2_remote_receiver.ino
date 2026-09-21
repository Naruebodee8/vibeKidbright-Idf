// ═══════════════════════════════════════════════════════════════════
//  minibike_activity2_remote_receiver.ino
//  กิจกรรม 2.2 — ภาครับวิทยุบังคับ (ESP-NOW Receiver)
//
//  บอร์ด: KidBright V1.5 iA (ESP32) บนตัวรถ
//  IDE:   KidBright IDE (Arduino Framework)
//
//  รับคำสั่ง:
//    - แกน X (Joystick ขวา) → ควบคุม Servo1 (GPIO15) เลี้ยว
//    - แกน Y (Joystick ซ้าย) → ควบคุม BDC Motor ขับเคลื่อน
//
//  โปรโตคอลค่าที่รับ (ตกลงกับภาคส่ง):
//    ค่า X ส่งตรงๆ: -100 ถึง +100 แต่บวก offset (เช่น +1000) เพื่อแยก
//    ค่า Y ส่งตรงๆ: -100 ถึง +100
//    999 = หยุด
// ═══════════════════════════════════════════════════════════════════

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>   // ติดตั้งผ่าน Library Manager

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
#define PIN_SERVO         15    // Servo1 บนบอร์ด KidBright iA
#define PIN_MOTOR_IN1     26    // OUT1 → L298N IN1
#define PIN_MOTOR_IN2     27    // OUT2 → L298N IN2
#define PIN_MOTOR_ENA     25    // L298N ENA (PWM speed)

// ─── SERVO SETTINGS ───────────────────────────────────────────────
#define SERVO_CENTER      90    // มุมเซอร์โวตรงกลาง (องศา)
#define SERVO_RANGE       45    // ± องศาจากกลาง (ปรับได้)

// ─── MOTOR SETTINGS ───────────────────────────────────────────────
#define MOTOR_MAX_PWM     200   // ความเร็วสูงสุด (0-255)
#define MOTOR_DEADZONE    10    // ค่าต่ำกว่านี้ถือว่าหยุด

// ─── PROTOCOL: ค่าที่ส่งมาจากภาคส่ง ──────────────────────────────
// ★ ตกลงกับโปรแกรมภาคส่งว่าใช้ offset อะไร
// ตัวอย่าง: แกน X บวก offset 1000 → ค่า 900-1100
// ตัวอย่าง: แกน Y ส่งตรงๆ → ค่า -100 ถึง +100
// 999 = stop ทุกอย่าง
#define OFFSET_X          1000  // offset สำหรับแยกแกน X จากแกน Y

Servo servo_front;

// ─── STATE ────────────────────────────────────────────────────────
volatile int32_t received_value = 999;
volatile bool    data_received  = false;
unsigned long    last_receive   = 0;
#define TIMEOUT_MS 500          // ถ้าไม่ได้รับข้อมูลนาน 500ms → หยุด

// ─── MOTOR CONTROL ─────────────────────────────────────────────────
void motor_set(int pwm) {
    pwm = constrain(pwm, -MOTOR_MAX_PWM, MOTOR_MAX_PWM);
    if (abs(pwm) < MOTOR_DEADZONE) {
        analogWrite(PIN_MOTOR_ENA, 0);
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
    } else if (pwm > 0) {
        analogWrite(PIN_MOTOR_ENA, pwm);
        digitalWrite(PIN_MOTOR_IN1, HIGH);
        digitalWrite(PIN_MOTOR_IN2, LOW);
    } else {
        analogWrite(PIN_MOTOR_ENA, -pwm);
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
    }
}

// ─── ESP-NOW RECEIVE CALLBACK ──────────────────────────────────────
void on_data_recv(const esp_now_recv_info_t *recv_info,
                  const uint8_t *data, int len) {
    if (len == sizeof(int32_t)) {
        memcpy((void*)&received_value, data, sizeof(int32_t));
        data_received = true;
        last_receive  = millis();
    }
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Motor pins
    pinMode(PIN_MOTOR_IN1, OUTPUT);
    pinMode(PIN_MOTOR_IN2, OUTPUT);
    pinMode(PIN_MOTOR_ENA, OUTPUT);
    motor_set(0);

    // Servo init → ตำแหน่งกลาง
    servo_front.attach(PIN_SERVO, 500, 2400);
    servo_front.write(SERVO_CENTER);
    delay(500);

    // ESP-NOW init
    WiFi.mode(WIFI_STA);
    Serial.print("Receiver MAC: ");
    Serial.println(WiFi.macAddress());   // ★ จดค่านี้ไปใส่ในภาคส่ง

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(on_data_recv);

    Serial.println("Minibike Receiver — Ready");
    Serial.println("Waiting for commands...");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {
    // Timeout: ถ้าไม่ได้รับสัญญาณ → หยุด
    if (millis() - last_receive > TIMEOUT_MS && last_receive != 0) {
        motor_set(0);
        servo_front.write(SERVO_CENTER);
        return;
    }

    if (!data_received) return;
    data_received = false;

    int32_t val = received_value;
    Serial.printf("Received: %d\n", val);

    if (val == 999) {
        // ─── หยุด ───────────────────────────────────────────────
        motor_set(0);
        servo_front.write(SERVO_CENTER);

    } else if (val >= OFFSET_X - 100 && val <= OFFSET_X + 100) {
        // ─── แกน X (เลี้ยว Servo) ───────────────────────────────
        int x_val  = val - OFFSET_X;   // -100 ถึง +100
        int angle  = SERVO_CENTER + (x_val * SERVO_RANGE / 100);
        angle = constrain(angle, SERVO_CENTER - SERVO_RANGE, SERVO_CENTER + SERVO_RANGE);
        servo_front.write(angle);
        Serial.printf("  Servo -> %d deg\n", angle);

    } else if (val >= -100 && val <= 100) {
        // ─── แกน Y (ความเร็วมอเตอร์) ────────────────────────────
        int y_val = val;   // -100 ถึง +100
        // ลบ offset ออก (ถ้าภาคส่งบวก offset มา)
        if (abs(y_val) < MOTOR_DEADZONE) {
            motor_set(0);
        } else {
            int pwm = (y_val * MOTOR_MAX_PWM) / 100;
            motor_set(pwm);
            Serial.printf("  Motor -> PWM %d\n", pwm);
        }
    }

    delay(30);   // 30ms loop
}
