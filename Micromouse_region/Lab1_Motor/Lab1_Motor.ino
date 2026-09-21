// ═══════════════════════════════════════════════════════════════════════════
//  test_01_motor.ino
//  Workshop 1: Motor Control Test
//  
//  วัตถุประสงค์:
//    1. ทดสอบการทำงานของมอเตอร์ซ้าย/ขวา
//    2. หาค่า PWM ต่ำสุดที่มอเตอร์เริ่มหมุน (MOTOR_PWM_MIN)
//    3. ทดสอบทิศทางการหมุน (ปรับ MOTOR_L/R_DIRECTION ถ้าผิดทาง)
//    4. ทดสอบโหมด Coast และ Brake
//  
//  การใช้งาน:
//    1. Upload โปรแกรม
//    2. เปิด Serial Monitor (115200 baud)
//    3. ใช้คำสั่งตามที่แสดงใน Help menu
//  
//  คำสั่ง:
//    w/s     = เพิ่ม/ลด PWM ทั้งสองมอเตอร์
//    a/d     = ทดสอบเลี้ยวซ้าย/ขวา
//    1-9     = ตั้งค่า PWM (1=10%, 5=50%, 9=90%)
//    f       = Forward (ไปข้างหน้า)
//    b       = Backward (ถอยหลัง)
//    x       = Stop (coast)
//    z       = Brake (เบรค)
//    l       = ทดสอบมอเตอร์ซ้ายอย่างเดียว
//    r       = ทดสอบมอเตอร์ขวาอย่างเดียว
//    m       = หา Minimum PWM (ค่าต่ำสุดที่มอเตอร์หมุน)
//    t       = ทดสอบ Ramp up/down
//    p       = แสดงสถานะปัจจุบัน
//    h/?     = แสดง Help
//  
//  ผลที่คาดหวัง:
//    - มอเตอร์ทั้งสองหมุนไปในทิศทางที่ถูกต้อง
//    - เมื่อสั่ง forward ล้อทั้งสองหมุนไปข้างหน้า
//    - ค่า PWM เพิ่ม/ลดตามคำสั่ง
//    - หาค่า MOTOR_PWM_MIN ได้
//  
//  ★ ค่าที่ต้องบันทึก:
//    - MOTOR_PWM_MIN = ______ (ค่า PWM ต่ำสุดที่มอเตอร์เริ่มหมุน)
//    - MOTOR_L_DIRECTION = ______ (1 หรือ -1)
//    - MOTOR_R_DIRECTION = ______ (1 หรือ -1)
// ═══════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
// INCLUDES
// ─────────────────────────────────────────────────────────────────────────────
#include "config.h"
#include "hal_motor.h"
#include "ble_debug.h"
#include "lab_link.h"     // ← ท้ายสุด: Serial ส่งได้ทั้ง USB + BLE (วัดตอนหุ่นอยู่บนพื้น ไม่ต้องต่อสาย)

// ─────────────────────────────────────────────────────────────────────────────
// TEST PARAMETERS (นักเรียนปรับค่าตรงนี้ได้)
// ─────────────────────────────────────────────────────────────────────────────
#define TEST_PWM_STEP       10          // ค่าที่เพิ่ม/ลดแต่ละครั้ง
#define TEST_DEFAULT_PWM    590         // ค่าเริ่มต้น
#define TEST_RAMP_DELAY_MS  50          // Delay ระหว่าง ramp step
#define MIN_PWM_TEST_START  50          // เริ่มทดสอบ minimum PWM ที่
#define MIN_PWM_TEST_STEP   10          // Step สำหรับหา minimum PWM

// ─────────────────────────────────────────────────────────────────────────────
// VARIABLES
// ─────────────────────────────────────────────────────────────────────────────
int16_t currentPWM = 0;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 500;  // ms

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════
void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) delay(10);
    delay(1000);  // Wait for Serial Monitor
    ble_debug_init("MM_Lab1_Motor");   // คุมไร้สายผ่าน BLE

    // ปุ่ม START + สวิตช์ MODE — สำหรับเช็ค HW หลังบัดกรี (คำสั่ง 'i')
    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_BUTTON_MODE,  INPUT_PULLUP);

    // Print header
    printHeader();
    
    // Initialize motor
    Serial.println("[INIT] Initializing motors...");
    
    if (!motor_init()) {
        Serial.println("════════════════════════════════════════");
        Serial.println("  *** ERROR: Motor initialization failed! ***");
        Serial.println("════════════════════════════════════════");
        Serial.println("Check:");
        Serial.println("  - Wiring connections");
        Serial.println("  - Pin definitions in config.h");
        Serial.println("  - DRV8833 power supply");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("[INIT] Motors initialized successfully!");
    Serial.println();
    
    // Print pin configuration
    printPinConfig();
    
    // Print help
    printHelp();
    
    Serial.println();
    Serial.println("Ready! Enter command...");
    Serial.println("════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════
void loop() {
    // Handle Serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        handleCommand(cmd);
    }
    
    // Periodic status print (if motor is running)
    if (currentPWM != 0 && millis() - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = millis();
        printCurrentStatus();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMAND HANDLER
// ═══════════════════════════════════════════════════════════════════════════
// เช็คปุ่ม START + สวิตช์ MODE (หลังบัดกรี) — กดปุ่มจริงแล้วดูค่าเปลี่ยน
void testButtons() {
    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_BUTTON_MODE,  INPUT_PULLUP);
    Serial.println();
    Serial.println("=== BUTTON / MODE TEST (เช็ค HW หลังบัดกรี) ===");
    Serial.println("กดปุ่ม START และสลับสวิตช์ MODE — ดูค่าด้านล่างเปลี่ยน");
    Serial.println("ถ้ากดแล้วค่าไม่เปลี่ยน = บัดกรี/สายมีปัญหา   (ส่ง 'x' เพื่อออก)");
    int lastS = -1, lastM = -1;
    uint32_t t0 = millis();
    while (millis() - t0 < 60000) {              // ออกเองใน 60 วิ
        int s = digitalRead(PIN_BUTTON_START);
        int m = digitalRead(PIN_BUTTON_MODE);
        if (s != lastS || m != lastM) {
            Serial.printf("  START = %-8s   MODE = %-4s\n",
                          s == LOW ? "PRESSED" : "released",
                          m == LOW ? "ON" : "off");
            lastS = s; lastM = m;
        }
        if (Serial.available()) { char c = Serial.read(); if (c == 'x' || c == 'X') break; }
        delay(20);
    }
    Serial.println("=== จบ button test ===");
}

void handleCommand(char cmd) {
    switch (cmd) {
        // ─────────────────────────────────────────────────────────────────
        // Speed control
        // ─────────────────────────────────────────────────────────────────
        case 'w':
        case 'W':
            currentPWM += TEST_PWM_STEP;
            currentPWM = CONSTRAIN(currentPWM, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
            motor_set_speed(currentPWM, currentPWM);
            Serial.printf("[CMD] Increase PWM → %d\n", currentPWM);
            break;
            
        case 's':
        case 'S':
            currentPWM -= TEST_PWM_STEP;
            currentPWM = CONSTRAIN(currentPWM, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
            motor_set_speed(currentPWM, currentPWM);
            Serial.printf("[CMD] Decrease PWM → %d\n", currentPWM);
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Preset speeds (1-9 = 10%-90%)
        // ─────────────────────────────────────────────────────────────────
        case '0':
            currentPWM = 0;
            motor_stop();
            Serial.println("[CMD] PWM = 0 (Stop)");
            break;
            
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            currentPWM = (cmd - '0') * (MOTOR_PWM_MAX / 10);
            motor_set_speed(currentPWM, currentPWM);
            Serial.printf("[CMD] PWM = %d (%d%%)\n", currentPWM, (cmd - '0') * 10);
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Direction control
        // ─────────────────────────────────────────────────────────────────
        case 'f':
        case 'F':
            currentPWM = ABS(currentPWM);
            if (currentPWM == 0) currentPWM = TEST_DEFAULT_PWM;
            motor_forward(currentPWM);
            Serial.printf("[CMD] Forward @ PWM=%d\n", currentPWM);
            break;
            
        case 'b':
        case 'B':
            currentPWM = -ABS(currentPWM);
            if (currentPWM == 0) currentPWM = -TEST_DEFAULT_PWM;
            motor_backward(ABS(currentPWM));
            Serial.printf("[CMD] Backward @ PWM=%d\n", ABS(currentPWM));
            break;
            
        case 'a':
        case 'A':
            motor_turn_left(TEST_DEFAULT_PWM);
            Serial.printf("[CMD] Turn Left @ PWM=%d\n", TEST_DEFAULT_PWM);
            break;
            
        case 'd':
        case 'D':
            motor_turn_right(TEST_DEFAULT_PWM);
            Serial.printf("[CMD] Turn Right @ PWM=%d\n", TEST_DEFAULT_PWM);
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Stop modes
        // ─────────────────────────────────────────────────────────────────
        case 'x':
        case 'X':
            currentPWM = 0;
            motor_stop();
            Serial.println("[CMD] Stop (Coast)");
            break;
            
        case 'z':
        case 'Z':
            currentPWM = 0;
            motor_brake();
            Serial.println("[CMD] Brake");
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Single motor test
        // ─────────────────────────────────────────────────────────────────
        case 'l':
        case 'L':
            testSingleMotor(MOTOR_LEFT);
            break;
            
        case 'r':
        case 'R':
            testSingleMotor(MOTOR_RIGHT);
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Special tests
        // ─────────────────────────────────────────────────────────────────
        case 'm':
        case 'M':
            testMinimumPWM();
            break;

        case 'v':
        case 'V':
            showBattery();
            break;

        case 'c':
        case 'C':
            calibrateBattery();
            break;

        case 'i':
        case 'I':
            testButtons();
            break;

        case 't':
        case 'T':
            testRamp();
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Status & Help
        // ─────────────────────────────────────────────────────────────────
        case 'p':
        case 'P':
            motor_print_status();
            break;
            
        case 'h':
        case 'H':
        case '?':
            printHelp();
            break;
            
        case '\n':
        case '\r':
            // Ignore newline
            break;
            
        default:
            Serial.printf("[CMD] Unknown command: '%c' (0x%02X)\n", cmd, cmd);
            Serial.println("     Press 'h' for help");
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief ทดสอบมอเตอร์ตัวเดียว
 */
void testSingleMotor(MotorID_t motor) {
    const char* motorName = (motor == MOTOR_LEFT) ? "LEFT" : "RIGHT";
    
    Serial.println();
    Serial.println("════════════════════════════════════════");
    Serial.printf("  SINGLE MOTOR TEST: %s\n", motorName);
    Serial.println("════════════════════════════════════════");
    
    // Stop first
    motor_stop();
    delay(500);
    
    // Forward test
    Serial.printf("[TEST] %s motor FORWARD @ PWM=%d...\n", motorName, TEST_DEFAULT_PWM);
    motor_set_single(motor, TEST_DEFAULT_PWM);
    delay(2000);
    
    // Stop
    motor_stop();
    delay(500);
    
    // Backward test
    Serial.printf("[TEST] %s motor BACKWARD @ PWM=%d...\n", motorName, TEST_DEFAULT_PWM);
    motor_set_single(motor, -TEST_DEFAULT_PWM);
    delay(2000);
    
    // Stop
    motor_stop();
    
    Serial.println("[TEST] Single motor test complete");
    Serial.println();
    Serial.println("★ สังเกต:");
    Serial.println("  - มอเตอร์หมุนถูกทิศทางหรือไม่?");
    Serial.println("  - ถ้าหมุนผิดทิศ ให้แก้ไข MOTOR_L/R_DIRECTION ใน config.h");
    Serial.println("════════════════════════════════════════");
}

/**
 * @brief หาค่า PWM ต่ำสุดที่มอเตอร์เริ่มหมุน
 */
// ─────────────────────────────────────────────────────────────────────────────
// BATTERY — อ่าน Vbat + คาลิเบรต divider ratio  (ใช้คู่กับโมเดลคุมเป็น "โวลต์"
// ของแลปรวมร่าง: PWM = volts / Vbat)
// ─────────────────────────────────────────────────────────────────────────────
float readVbatVolts() {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_BATTERY_ADC);
    float pin_mV = sum / 16.0f;
    return pin_mV * (float)BATTERY_DIVIDER_RATIO / 1000.0f;
}

void showBattery() {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_BATTERY_ADC);
    float pin_mV = sum / 16.0f;
    float vbat = pin_mV * (float)BATTERY_DIVIDER_RATIO / 1000.0f;
    Serial.println();
    Serial.printf("BATTERY: %.2f V   (pin %.0f mV x ratio %.2f)\n",
                  vbat, pin_mV, (float)BATTERY_DIVIDER_RATIO);
    if (vbat < 6.6f) Serial.println("  [!] ต่ำ — ควรชาร์จก่อนทดสอบ");
}

// หา divider ratio ให้ค่าที่อ่าน ตรงกับมัลติมิเตอร์ (ใช้ได้ทั้ง USB และ BLE)
void calibrateBattery() {
    Serial.println();
    Serial.println("=== BATTERY CALIBRATION (หา divider ratio) ===");
    uint32_t sum = 0;
    for (int i = 0; i < 50; i++) { sum += analogReadMilliVolts(PIN_BATTERY_ADC); delay(10); }
    float adc_mV = sum / 50.0f;
    Serial.printf("ADC pin (เฉลี่ย 50 ครั้ง): %.0f mV\n", adc_mV);
    Serial.println("วัดแรงดันแบตจริงด้วยมัลติมิเตอร์ แล้วพิมพ์เป็น mV (เช่น 7870) กด Enter:");

    delay(20);
    while (Serial.available()) Serial.read();          // flush
    uint32_t t0 = millis();
    String in = "";
    while (millis() - t0 < 30000) {                    // รอ 30 วิ
        if (Serial.available()) {
            char ch = Serial.read();
            if (ch == '\n' || ch == '\r') { if (in.length()) break; }
            else in += ch;
        }
        delay(2);
    }
    long batt_mV = in.toInt();
    if (batt_mV < 3000 || batt_mV > 10000) {
        Serial.printf("ค่าไม่ถูกต้อง ('%s') — ต้องอยู่ 3000-10000 mV\n", in.c_str());
        return;
    }
    float ratio = (float)batt_mV / adc_mV;
    Serial.println("──────────────────────────────────────────");
    Serial.printf("Multimeter %ld mV / ADC %.0f mV  ->  ratio = %.3f\n", batt_mV, adc_mV, ratio);
    Serial.printf("ปัจจุบัน BATTERY_DIVIDER_RATIO = %.2f\n", (float)BATTERY_DIVIDER_RATIO);
    Serial.println("★ ใส่ใน config.h:");
    Serial.printf("   #define BATTERY_DIVIDER_RATIO   %.2ff\n", ratio);
    Serial.println("แล้ว re-flash → กด 'v' ตรวจว่าตรงมัลติมิเตอร์");
}

void testMinimumPWM() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  MINIMUM PWM TEST  (+ deadband เป็นโวลต์)");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("กำลังเพิ่ม PWM ทีละขั้น — สังเกตว่ามอเตอร์เริ่มหมุนที่แถวไหน");
    Serial.println("คอลัมน์ Volts = PWM/1023 x Vbat = deadband_V ที่ PWM นั้น");

    float vbat = readVbatVolts();
    Serial.printf("Vbat = %.2f V\n", vbat);
    Serial.println();
    Serial.println("PWM    Volts(V)");
    Serial.println("───────────────");

    motor_stop();
    delay(500);

    for (int pwm = MIN_PWM_TEST_START; pwm <= 600; pwm += MIN_PWM_TEST_STEP) {
        motor_set_speed(pwm, pwm);
        Serial.printf("%3d    %.2f\n", pwm, pwm * vbat / MOTOR_PWM_MAX);
        delay(500);
    }

    motor_stop();

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("★ บันทึกค่า:");
    Serial.println("  - MOTOR_PWM_MIN = PWM ต่ำสุดที่มอเตอร์เริ่มหมุน (ปกติ ~60-120)");
    Serial.println("  - deadband_V    = ค่า Volts แถวเดียวกัน (ใช้ในโมเดลคุมเป็นโวลต์)");
    Serial.println("  ทดสอบบนพื้นจริง (ผ่าน BLE) เพื่อได้ค่าตรงกับตอนใช้งาน");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println();
}

/**
 * @brief ทดสอบ Ramp up/down
 */
void testRamp() {
    Serial.println();
    Serial.println("════════════════════════════════════════");
    Serial.println("  RAMP TEST");
    Serial.println("════════════════════════════════════════");
    
    motor_stop();
    delay(500);
    
    // Ramp up
    Serial.println("[TEST] Ramping UP (0 → 100%)...");
    for (int pwm = 0; pwm <= MOTOR_PWM_MAX; pwm += 50) {
        motor_set_speed(pwm, pwm);
        Serial.printf("  PWM: %4d (%3d%%)\n", pwm, pwm * 100 / MOTOR_PWM_MAX);
        delay(TEST_RAMP_DELAY_MS);
    }
    
    delay(1000);
    
    // Ramp down
    Serial.println("[TEST] Ramping DOWN (100% → 0)...");
    for (int pwm = MOTOR_PWM_MAX; pwm >= 0; pwm -= 50) {
        motor_set_speed(pwm, pwm);
        Serial.printf("  PWM: %4d (%3d%%)\n", pwm, pwm * 100 / MOTOR_PWM_MAX);
        delay(TEST_RAMP_DELAY_MS);
    }
    
    motor_stop();
    
    Serial.println();
    Serial.println("[TEST] Ramp test complete");
    Serial.println("════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printHeader() {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                            ║");
    Serial.println("║          MICROMOUSE ESP32-S3                               ║");
    Serial.println("║          Workshop 1: Motor Control Test                    ║");
    Serial.println("║                                                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printPinConfig() {
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ PIN CONFIGURATION                                          │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.printf(" │  Motor L IN1: GPIO %2d                                     │\n", PIN_MOTOR_L_IN1);
    Serial.printf(" │  Motor L IN2: GPIO %2d                                     │\n", PIN_MOTOR_L_IN2);
    Serial.printf(" │  Motor R IN1: GPIO %2d                                     │\n", PIN_MOTOR_R_IN1);
    Serial.printf(" │  Motor R IN2: GPIO %2d                                     │\n", PIN_MOTOR_R_IN2);
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.printf(" │  PWM Frequency:  %5d Hz                                    │\n", MOTOR_PWM_FREQ);
    Serial.printf(" │  PWM Resolution: %2d-bit (0-%d)                            │\n", MOTOR_PWM_RESOLUTION, MOTOR_PWM_MAX);
    Serial.println("└────────────────────────────────────────────────────────────┘");
    Serial.println();
}

void printHelp() {
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ COMMANDS                                                   │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│ SPEED CONTROL:                                             │");
    Serial.println("│   w/s     = เพิ่ม/ลด PWM (±50)                               │");
    Serial.println("│   0-9     = ตั้ง PWM (0=0%, 1=10%, ..., 9=90%)               │");
    Serial.println("│                                                            │");
    Serial.println("│ DIRECTION:                                                 │");
    Serial.println("│   f       = Forward (ไปข้างหน้า)                             │");
    Serial.println("│   b       = Backward (ถอยหลัง)                              │");
    Serial.println("│   a       = Turn Left (เลี้ยวซ้าย)                            │");
    Serial.println("│   d       = Turn Right (เลี้ยวขวา)                           │");
    Serial.println("│                                                            │");
    Serial.println("│ STOP:                                                      │");
    Serial.println("│   x       = Stop (Coast - ปล่อยหมุนอิสระ)                    │");
    Serial.println("│   z       = Brake (เบรคทันที)                                │");
    Serial.println("│                                                            │");
    Serial.println("│ TESTS:                                                     │");
    Serial.println("│   l       = ทดสอบมอเตอร์ซ้ายอย่างเดียว                         │");
    Serial.println("│   r       = ทดสอบมอเตอร์ขวาอย่างเดียว                        │");
    Serial.println("│   m       = หาค่า Minimum PWM                              │");
    Serial.println("│   t       = ทดสอบ Ramp up/down                             │");
    Serial.println("│                                                            │");
    Serial.println("│ STATUS:                                                    │");
    Serial.println("│   p       = แสดงสถานะปัจจุบัน                                │");
    Serial.println("│   h/?     = แสดง Help                                      │");
    Serial.println("└────────────────────────────────────────────────────────────┘");
}

void printCurrentStatus() {
    int16_t left, right;
    motor_get_pwm(&left, &right);
    
    Serial.printf("[STATUS] PWM L:%4d R:%4d | Target:%4d\n", left, right, currentPWM);
}
