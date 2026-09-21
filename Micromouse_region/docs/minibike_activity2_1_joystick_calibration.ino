// ═══════════════════════════════════════════════════════════════════
//  minibike_activity2_1_joystick_calibration.ino
//  กิจกรรม 2.1 — เทียบวัดจอยสติ๊ก (Joystick Calibration)
//
//  บอร์ด: KidBright V1.5 iA (ESP32) — ฝั่ง Remote Controller
//  IDE:   KidBright IDE (Arduino Framework)
//
//  วิธีใช้:
//    1. Upload โค้ดนี้ไปยังบอร์ด Remote
//    2. เปิด Serial Monitor (115200 baud) หรือ Serial Plotter
//    3. จดค่าตามขั้นตอนด้านล่าง
//
//  ขั้นตอน Calibration:
//    Step 1 → ไม่โยกจอยสติ๊กใดเลย → จดค่า X_neutral, Y_neutral
//    Step 2 → โยก JS แกน X ไปขวาสุด → จดค่า X_max
//    Step 3 → โยก JS แกน X ไปซ้ายสุด → จดค่า X_min
//    Step 4 → โยก JS แกน Y ไปบนสุด  → จดค่า Y_max
//    Step 5 → โยก JS แกน Y ไปล่างสุด → จดค่า Y_min
//
//  ผลที่ได้นำไปใส่ในโค้ดกิจกรรม 2.2 (Sender)
// ═══════════════════════════════════════════════════════════════════

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
// ★ ปรับ GPIO ให้ตรงกับ hardware จริงของรีโมต
#define PIN_JS_X    33    // IN2 (GPIO33) → Joystick แกน X (ซ้าย/ขวา)
#define PIN_JS_Y    32    // IN1 (GPIO32) → Joystick แกน Y (บน/ล่าง)

// ─── SETTINGS ─────────────────────────────────────────────────────
#define FILTER_SIZE   10    // เฉลี่ย 10 ค่า (เพิ่มถ้าค่ากระโดด)
#define PRINT_MS      100   // พิมพ์ทุก 100ms

// ─── GLOBAL ───────────────────────────────────────────────────────
int x_buf[FILTER_SIZE] = {0};
int y_buf[FILTER_SIZE] = {0};
int buf_idx = 0;

int x_min = 9999,  x_max = -9999,  x_neutral_sum = 0;
int y_min = 9999,  y_max = -9999,  y_neutral_sum = 0;
int neutral_count = 0;

unsigned long last_print = 0;

// ─── FILTER ───────────────────────────────────────────────────────
int filtered(int* buf, int new_val) {
    buf[buf_idx % FILTER_SIZE] = new_val;
    long sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) sum += buf[i];
    return (int)(sum / FILTER_SIZE);
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);   // ESP32: ADC 12-bit (0–4095)

    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║   กิจกรรม 2.1 — Joystick Calibration    ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println();
    Serial.println("📋 ขั้นตอน:");
    Serial.println("  1. ไม่โยกจอยสติ๊กใดเลย → จด X_neutral, Y_neutral");
    Serial.println("  2. โยก JS แกน X ไปขวาสุด → จด X_max");
    Serial.println("  3. โยก JS แกน X ไปซ้ายสุด → จด X_min");
    Serial.println("  4. โยก JS แกน Y ไปบนสุด   → จด Y_max");
    Serial.println("  5. โยก JS แกน Y ไปล่างสุด  → จด Y_min");
    Serial.println();
    Serial.println("กด Enter ใน Serial Monitor เพื่อแสดงสรุปค่า");
    Serial.println("────────────────────────────────────────────");
    Serial.println("JS_X_raw\tJS_Y_raw\t| X_min\tX_max\tY_min\tY_max");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {
    // อ่านและกรองค่า
    int x_raw = filtered(x_buf, analogRead(PIN_JS_X));
    int y_raw = filtered(y_buf, analogRead(PIN_JS_Y));
    buf_idx++;

    // Track min/max อัตโนมัติ
    if (x_raw < x_min) x_min = x_raw;
    if (x_raw > x_max) x_max = x_raw;
    if (y_raw < y_min) y_min = y_raw;
    if (y_raw > y_max) y_max = y_raw;

    // พิมพ์ค่าทุก 100ms
    if (millis() - last_print >= PRINT_MS) {
        last_print = millis();

        // Format สำหรับ Serial Plotter (ดูกราฟ) และ Serial Monitor (ดูตัวเลข)
        Serial.printf("%d\t%d\t| %d\t%d\t%d\t%d\n",
                      x_raw, y_raw,
                      x_min, x_max, y_min, y_max);
    }

    // รับคำสั่งจาก Serial Monitor
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r' || c == 's') {
            print_summary(x_raw, y_raw);
        }
        // ล้าง buffer ที่เหลือ
        while (Serial.available()) Serial.read();
    }

    delay(20);
}

// ─── PRINT SUMMARY ─────────────────────────────────────────────────
void print_summary(int x_now, int y_now) {
    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║           สรุปค่า Calibration            ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("  JS แกน X (ซ้าย/ขวา):\n");
    Serial.printf("    ค่าปัจจุบัน  = %d\n", x_now);
    Serial.printf("    X_min (ซ้ายสุด)  = %d\n", x_min);
    Serial.printf("    X_max (ขวาสุด)  = %d\n", x_max);
    Serial.println();
    Serial.printf("  JS แกน Y (บน/ล่าง):\n");
    Serial.printf("    ค่าปัจจุบัน  = %d\n", y_now);
    Serial.printf("    Y_min (ล่างสุด) = %d\n", y_min);
    Serial.printf("    Y_max (บนสุด)   = %d\n", y_max);
    Serial.println();
    Serial.println("──────────────────────────────────────────");
    Serial.println("📋 นำค่าเหล่านี้ไปใส่ในโค้ดกิจกรรม 2.2 (Sender):");
    Serial.println();
    Serial.printf("  int js_x_neutral = %d;  // ← ค่า X ขณะไม่โยก (อ่านตอนนี้)\n", x_now);
    Serial.printf("  int js_x_min     = %d;\n", x_min);
    Serial.printf("  int js_x_max     = %d;\n", x_max);
    Serial.println();
    Serial.printf("  int js_y_neutral = %d;  // ← ค่า Y ขณะไม่โยก (อ่านตอนนี้)\n", y_now);
    Serial.printf("  int js_y_min     = %d;\n", y_min);
    Serial.printf("  int js_y_max     = %d;\n", y_max);
    Serial.println("──────────────────────────────────────────");
    Serial.println();
}
