// ═══════════════════════════════════════════════════════════════════
//  minibike_activity2_remote_sender.ino
//  กิจกรรม 2.2 — ภาคส่งวิทยุบังคับ (ESP-NOW Sender / Remote)
//
//  บอร์ด: KidBright V1.5 iA (ESP32) ฝั่ง Remote Controller
//  IDE:   KidBright IDE (Arduino Framework)
//
//  Joystick: ต่อผ่าน I/O ports ของบอร์ด KidBright
//    - แกน Y (JS ซ้าย): ADC บน IN3/IN4 หรือ analog reads
//    - แกน X (JS ขวา): ควบคุมเซอร์โว
//
//  NOTE: ในกิจกรรมนี้ใช้ KidBright IDE Block → ดูภาพใน PDF หน้า 69
//  โค้ดนี้เป็น Arduino equivalent ของบล็อกใน IDE
//
//  ★ สำคัญ: ต้องกรอก MAC Address ของบอร์ดรับก่อน Upload
// ═══════════════════════════════════════════════════════════════════

#include <esp_now.h>
#include <WiFi.h>

// ─── ★ กรอก MAC Address ของบอร์ดรับ (ดูจาก Serial Monitor ของภาครับ) ─
uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//                         ↑ แก้ค่านี้ให้ตรงกับบอร์ดรับ

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
// ★ ปรับตาม hardware จริงของรีโมต
#define PIN_JS_Y_AXIS     33    // IN2/GPIO33 → Joystick Y (เดินหน้า/ถอย)
#define PIN_JS_X_AXIS     32    // IN1/GPIO32 → Joystick X (เลี้ยว)
#define PIN_SW2           14    // SW2 → สลับ mode (optional)

// ─── CALIBRATION VALUES (จาก กิจกรรม 2.1) ────────────────────────
// ★ แก้ค่าเหล่านี้ตามผลการ calibrate ของ Joystick ตัวคุณ
int js_y_neutral = 2048;    // ค่าขณะไม่โยก (ADC ~2048 = กลาง)
int js_y_min     = 0;       // ค่าเมื่อโยกลงสุด (ถอยหลัง)
int js_y_max     = 4095;    // ค่าเมื่อโยกขึ้นสุด (เดินหน้า)

int js_x_neutral = 2048;    // ค่าขณะไม่โยก
int js_x_min     = 0;       // ค่าเมื่อโยกซ้ายสุด
int js_x_max     = 4095;    // ค่าเมื่อโยกขวาสุด

// ─── PROTOCOL SETTINGS ────────────────────────────────────────────
#define DEADZONE          10    // Dead zone ±10%
#define OFFSET_X          1000  // offset สำหรับแกน X (เพื่อแยกจากแกน Y)
#define SEND_INTERVAL_MS  100   // ส่งทุก 100ms

// ─── ESP-NOW SEND CALLBACK ─────────────────────────────────────────
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[WARN] ESP-NOW send failed");
    }
}

// ─── READ JOYSTICK (แปลงค่า ADC → -100 ถึง +100) ─────────────────
int read_joystick(int pin, int neutral, int val_min, int val_max) {
    int raw = analogRead(pin);    // 0-4095

    int out;
    if (raw < neutral) {
        // ด้านลบ
        out = map(raw, val_min, neutral, -100, 0);
    } else {
        // ด้านบวก
        out = map(raw, neutral, val_max, 0, 100);
    }
    out = constrain(out, -100, 100);

    // Dead zone
    if (abs(out) < DEADZONE) out = 0;
    return out;
}

// ─── SEND VALUE ────────────────────────────────────────────────────
void espnow_send(int32_t value) {
    esp_now_send(receiver_mac, (uint8_t*)&value, sizeof(value));
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);   // ESP32 ADC 12-bit

    pinMode(PIN_SW2, INPUT_PULLUP);

    // ESP-NOW init
    WiFi.mode(WIFI_STA);
    Serial.print("Sender MAC: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW init failed");
        return;
    }
    esp_now_register_send_cb(on_data_sent);

    // เพิ่ม peer (บอร์ดรับ)
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, receiver_mac, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;
    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        Serial.println("[ERROR] Failed to add peer");
        return;
    }

    Serial.println("Minibike Remote Sender — Ready");
    Serial.println("กิจกรรม 2.1: โยกจอยสติ๊กและดูค่าใน Serial Monitor");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {
    // อ่านค่า Joystick
    int js_y = read_joystick(PIN_JS_Y_AXIS, js_y_neutral, js_y_min, js_y_max);
    int js_x = read_joystick(PIN_JS_X_AXIS, js_x_neutral, js_x_min, js_x_max);

    Serial.printf("JS_Y=%+4d  JS_X=%+4d\n", js_y, js_x);

    // ─── ตัดสินใจว่าจะส่งอะไร ──────────────────────────────────
    int32_t send_val = 999;   // default = หยุด

    if (js_y != 0) {
        // แกน Y (เดินหน้า/ถอย) → ส่งค่าตรงๆ -100 ถึง +100
        send_val = (int32_t)js_y;
        Serial.printf("  >> Send Y: %d\n", send_val);

    } else if (js_x != 0) {
        // แกน X (เลี้ยว) → บวก OFFSET_X เพื่อแยกจากแกน Y
        send_val = (int32_t)(js_x + OFFSET_X);   // 900-1100
        Serial.printf("  >> Send X: %d (raw=%d + offset=%d)\n",
                      send_val, js_x, OFFSET_X);

    } else {
        // ทั้งคู่ dead zone → หยุด
        send_val = 999;
    }

    espnow_send(send_val);

    delay(SEND_INTERVAL_MS);
}
