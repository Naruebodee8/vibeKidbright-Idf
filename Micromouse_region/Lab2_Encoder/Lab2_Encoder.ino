// ═══════════════════════════════════════════════════════════════════════════
//  test_02_encoder.ino
//  Workshop 2: Encoder Test
//  
//  วัตถุประสงค์:
//    1. ทดสอบการอ่านค่า Encoder (Quadrature X4)
//    2. หาค่า COUNTS_PER_REV (จำนวน counts ต่อรอบ)
//    3. ตรวจสอบทิศทางการนับ (บวก=ไปหน้า, ลบ=ถอยหลัง)
//    4. คำนวณระยะทางจากค่า Encoder
//  
//  การใช้งาน:
//    1. Upload โปรแกรม
//    2. เปิด Serial Monitor (115200 baud)
//    3. ใช้คำสั่งทดสอบตามที่แสดง
//  
//  คำสั่ง:
//  STEP 1 — หาค่า CPR (หมุนล้อด้วยมือ):
//    r          = Reset counters เป็น 0
//    s          = Live counts ON/OFF (หมุนล้อ 1 รอบ → ค่า count = CPR)
//    c          = แสดง Count ครั้งเดียว
//  STEP 2 — ทดสอบว่า CPR ถูก (วัดด้วยไม้บรรทัด):
//    drive <mm> = ขับระยะตาม config CPR → วัดจริงว่าตรงไหม (ไม่ตรง = CPR ผิด)
//    cpr <v>    = ขับ 1 รอบ (v counts) ตาม CPR ที่กรอก → ควรได้ ~เส้นรอบวงล้อ
//  อื่น ๆ:  v = Velocity · d = Distance · t = motor test · p = status · g = plot · h/? = help
//
//  วิธีหา COUNTS_PER_REV:
//    1. กด 'r' reset → 's' เปิด live counts
//    2. หมุนล้อด้วยมือ 1 รอบเต็ม (ทำเครื่องหมายจุดเริ่มต้น)
//    3. อ่านค่า count = CPR (หมุนหลายรอบแล้วหารจำนวนรอบก็ได้)
//    4. ใส่ใน config.h (COUNTS_PER_REV) แล้ว re-flash → ทดสอบด้วย STEP 2
//  
//  ผลที่คาดหวัง:
//    - หมุนล้อไปข้างหน้า count เพิ่มขึ้น (บวก)
//    - หมุนล้อถอยหลัง count ลดลง (ลบ)
//    - Count per rev ประมาณ 815 (ตามที่ทดสอบก่อนหน้า ขึ้นอยู่กับอัตราทดเกียร์ ค่านี้อาจจะแตกต่างกัน แต่มอเตอร์ 2 ตัวที่นำมาใช้ควรจะได้ใกล้เคียงกัน)
//  
//  ★ ค่าที่ต้องบันทึก:
//    - COUNTS_PER_REV = ______ (counts ต่อรอบ)
//    - ตรวจสอบว่าทิศทางถูกต้อง ค่า + ล้อหมุนไปหน้า ค่า - ล้อหมุนถอยหลัง
// ═══════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
// INCLUDES
// ─────────────────────────────────────────────────────────────────────────────
#include "config.h"
#include "hal_motor.h"
#include "hal_encoder.h"
#include "ble_debug.h"
#include "lab_link.h"     // ← ต้องอยู่ท้ายสุด: ทำให้ Serial พูดได้ทั้ง USB + BLE

// ─────────────────────────────────────────────────────────────────────────────
// TEST PARAMETERS
// ─────────────────────────────────────────────────────────────────────────────
#define TEST_MOTOR_PWM      700         // PWM สำหรับทดสอบมอเตอร์
#define TEST_DURATION_MS    3000        // ระยะเวลาทดสอบ (ms)
#define DISPLAY_INTERVAL_MS 100         // Interval สำหรับ continuous display

// Distance-verification drive test ('x')
#define DIST_DRIVE_PWM      650         // PWM ขณะขับทดสอบระยะ (ช้ากว่า = coast น้อย)
#define DIST_DRIVE_TIMEOUT  8000        // safety timeout (ms) กันมอเตอร์ค้าง
#define DIST_BRAKE_MS       200         // เบรกสั้น ๆ กันหุ่นไหล (0 = coast ล้วน)

// ─────────────────────────────────────────────────────────────────────────────
// VARIABLES
// ─────────────────────────────────────────────────────────────────────────────
bool continuousDisplay = false;
bool plotMode = false;
unsigned long lastDisplayTime = 0;
bool motorsAvailable = false;


// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) delay(10);
    delay(1000);
    ble_debug_init("MM_Lab2_Enc");     // คุมไร้สายผ่าน BLE (ตอนหุ่นวิ่งวัดระยะ ไม่ต้องต่อสาย)

    printHeader();
    
    // Initialize encoder
    Serial.println("[INIT] Initializing encoders...");
    
    if (!encoder_init()) {
        Serial.println("════════════════════════════════════════");
        Serial.println("  *** ERROR: Encoder initialization failed! ***");
        Serial.println("════════════════════════════════════════");
        while (1) delay(1000);
    }
    
    Serial.println("[INIT] Encoders initialized successfully!");
    
    // Try to initialize motor (optional for this test)
    Serial.println("[INIT] Initializing motors (optional)...");
    motorsAvailable = motor_init();
    
    if (motorsAvailable) {
        Serial.println("[INIT] Motors available for testing");
    } else {
        Serial.println("[INIT] Motors not available - manual test only");
    }
    
    Serial.println();
    printPinConfig();
    printEncoderConfig();
    printHelp();
    
    Serial.println();
    Serial.println("Ready! Enter command...");
    Serial.println("════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════
void loop() {
    encoder_update();

    // Line-buffered input (USB + BLE via tee). Single char = command;
    // "key value" (e.g. "drive 100", "cpr 815") = line command.
    static char buf[32];
    static int  n = 0;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') { if (n) { buf[n] = 0; handleLine(buf); n = 0; } }
        else if (n < (int)sizeof(buf) - 1) buf[n++] = c;
    }

    // Continuous display → stream live counts for the UI (Step 1: spin & read)
    if (continuousDisplay && millis() - lastDisplayTime >= DISPLAY_INTERVAL_MS) {
        lastDisplayTime = millis();
        if (plotMode) printPlotData(); else printQuickStatus();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMAND HANDLER
// ═══════════════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
// driveDistance — ขับมอเตอร์จนกว่า encoder จะนับได้ครบ target_mm ตามค่า cpr ที่ระบุ
//   trial=false : ใช้ COUNTS_PER_REV ใน config  (Step 2 วิธี 2: กรอกระยะ → วัดจริง)
//   trial=true  : ใช้ cpr ที่กรอก (ไม่ต้อง reflash) (Step 2 วิธี 1: กรอก CPR → วัดจริง)
// ─────────────────────────────────────────────────────────────────────────────
void driveDistance(float target_mm, float cpr, bool trial) {
    if (!motorsAvailable) { Serial.println("[ERROR] Motors not available"); return; }
    if (target_mm <= 0.0f || cpr <= 0.0f) { Serial.println("[DRIVE] ค่าต้องมากกว่า 0"); return; }

    float circ = WHEEL_CIRCUMFERENCE_MM;
    long  target_counts = (long)(target_mm * cpr / circ + 0.5f);

    Serial.println("════════════════════════════════════════════════════════════");
    if (trial)
        Serial.printf("  DRIVE (ลอง CPR=%.0f): เป้า %.0f mm = %ld counts\n", cpr, target_mm, target_counts);
    else
        Serial.printf("  DRIVE %.0f mm (config CPR=%d)\n", target_mm, COUNTS_PER_REV);

    encoder_reset(-1); delay(50);
    motor_forward(DIST_DRIVE_PWM);
    uint32_t t0 = millis();
    while (true) {
        encoder_update();
        int32_t l, r; encoder_get_counts(&l, &r);
        long avg = (labs(l) + labs(r)) / 2;
        if (avg >= target_counts) break;
        if (millis() - t0 > DIST_DRIVE_TIMEOUT) { Serial.println("  [TIMEOUT]"); break; }
        delay(2);
    }
    if (DIST_BRAKE_MS > 0) { motor_brake(); delay(DIST_BRAKE_MS); }
    motor_stop();
    delay(250);
    encoder_update();
    int32_t l, r; encoder_get_counts(&l, &r);

    Serial.println("  ─────────────────────────────────────────");
    Serial.printf("  ➜ หุ่นควรวิ่งได้ ~%.0f mm — วัดด้วยไม้บรรทัดเทียบ\n", target_mm);
    Serial.printf("     counts จริง: L=%d R=%d\n", l, r);
    if (trial) Serial.println("     ระยะจริง ≈ ค่านี้  → CPR ที่กรอกถูกต้อง");
    else       Serial.println("     ระยะจริง ≠ ค่านี้  → CPR ใน config ยังผิด กลับไป Step 1");
    Serial.println("════════════════════════════════════════════════════════════");
}

// ─────────────────────────────────────────────────────────────────────────────
// handleLine — คำสั่งบรรทัดเดียว (BLE/UI ส่งได้ทั้งบรรทัด); ไม่มี space = คำสั่งตัวเดียว
//   drive <mm>      → ขับระยะตาม config CPR (วิธี 1)
//   cpr <v>         → ขับจนครบ v counts (= 1 รอบล้อ) ตาม CPR ที่กรอก (วิธี 2)
//                     ระยะที่ควรได้ = เส้นรอบวงล้อ; กรอกเลขที่ 2 = override ระยะได้
// ─────────────────────────────────────────────────────────────────────────────
void handleCommand(char cmd);
void handleLine(char* s) {
    char* sp = strchr(s, ' ');
    if (!sp) { if (s[0]) handleCommand(s[0]); return; }
    *sp = 0;
    float v = atof(sp + 1);
    if (!strcmp(s, "drive")) {
        driveDistance(v, (float)COUNTS_PER_REV, false);
    } else if (!strcmp(s, "cpr")) {
        // default = 1 รอบ: driveDistance(circ, v) → target_counts = circ·v/circ = v counts
        char* sp2 = strchr(sp + 1, ' ');
        float dist = sp2 ? atof(sp2 + 1) : (float)WHEEL_CIRCUMFERENCE_MM;
        driveDistance(dist, v, true);
    } else {
        Serial.printf("? unknown '%s'\n", s);
    }
}

void handleCommand(char cmd) {
    switch (cmd) {
        // ─────────────────────────────────────────────────────────────────
        // Basic commands
        // ─────────────────────────────────────────────────────────────────
        case 'r':
        case 'R':
            encoder_reset(-1);  // Reset both
            Serial.println("[CMD] Counters reset to 0");
            break;

        case 'c':
        case 'C':
            printCounts();
            break;
            
        case 'v':
        case 'V':
            printVelocity();
            break;
            
        case 'd':
        case 'D':
            printDistance();
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Step 2 verify = คำสั่งบรรทัดเดียว "drive <mm>" / "cpr <v>" (ดู handleLine)
        // ─────────────────────────────────────────────────────────────────

        // ─────────────────────────────────────────────────────────────────
        // Motor test
        // ─────────────────────────────────────────────────────────────────
        case 't':
        case 'T':
            if (motorsAvailable) {
                testWithMotor();
            } else {
                Serial.println("[ERROR] Motors not available");
            }
            break;
            
        // ─────────────────────────────────────────────────────────────────
        // Display modes
        // ─────────────────────────────────────────────────────────────────
        case 's':
        case 'S':
            continuousDisplay = !continuousDisplay;
            plotMode = false;
            Serial.printf("[CMD] Continuous display: %s\n", continuousDisplay ? "ON" : "OFF");
            break;
            
        case 'g':
        case 'G':
            plotMode = !plotMode;
            continuousDisplay = plotMode;
            if (plotMode) {
                Serial.println("CountL,CountR,VelL,VelR,DistL,DistR");
            } else {
                Serial.println("[CMD] Plot mode: OFF");
            }
            break;
            
        case 'p':
        case 'P':
            encoder_print_status();
            break;
            
        case 'h':
        case 'H':
        case '?':
            printHelp();
            break;
            
        case '\n':
        case '\r':
            break;
            
        default:
            Serial.printf("[CMD] Unknown: '%c'\n", cmd);
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════
/**
 * @brief ทดสอบด้วยมอเตอร์
 */
void testWithMotor() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  MOTOR + ENCODER TEST");
    Serial.println("════════════════════════════════════════════════════════════");
    
    // Reset encoders
    encoder_reset(-1);
    delay(100);
    
    // Forward test
    Serial.printf("[TEST] Forward @ PWM=%d for %dms...\n", TEST_MOTOR_PWM, TEST_DURATION_MS);
    
    motor_forward(TEST_MOTOR_PWM);
    
    unsigned long startTime = millis();
    while (millis() - startTime < TEST_DURATION_MS) {
        encoder_update();
        
        if (millis() % 200 < 10) {
            int32_t count_L, count_R;
            encoder_get_counts(&count_L, &count_R);
            float vel_L = encoder_get_velocity_mm(ENCODER_LEFT, true);
            float vel_R = encoder_get_velocity_mm(ENCODER_RIGHT, true);
            
            Serial.printf("  Count L:%6d R:%6d | Vel L:%6.1f R:%6.1f mm/s\n",
                          count_L, count_R, vel_L, vel_R);
        }
        delay(10);
    }
    
    motor_stop();
    encoder_update();
    
    // Show results
    int32_t final_L, final_R;
    encoder_get_counts(&final_L, &final_R);
    float dist_L = encoder_get_distance_mm(ENCODER_LEFT);
    float dist_R = encoder_get_distance_mm(ENCODER_RIGHT);
    
    Serial.println();
    Serial.println("────────────────────────────────────────────────────────────");
    Serial.println("FORWARD TEST RESULTS:");
    Serial.printf("  Left:  %d counts = %.1f mm\n", final_L, dist_L);
    Serial.printf("  Right: %d counts = %.1f mm\n", final_R, dist_R);
    Serial.printf("  Average: %.1f mm\n", (dist_L + dist_R) / 2.0f);
    Serial.println();
    
    delay(1000);
    
    // Backward test
    encoder_reset(-1);
    delay(100);
    
    Serial.printf("[TEST] Backward @ PWM=%d for %dms...\n", TEST_MOTOR_PWM, TEST_DURATION_MS);
    
    motor_backward(TEST_MOTOR_PWM);
    
    startTime = millis();
    while (millis() - startTime < TEST_DURATION_MS) {
        encoder_update();
        delay(10);
    }
    
    motor_stop();
    encoder_update();
    
    encoder_get_counts(&final_L, &final_R);
    dist_L = encoder_get_distance_mm(ENCODER_LEFT);
    dist_R = encoder_get_distance_mm(ENCODER_RIGHT);
    
    Serial.println();
    Serial.println("────────────────────────────────────────────────────────────");
    Serial.println("BACKWARD TEST RESULTS:");
    Serial.printf("  Left:  %d counts = %.1f mm\n", final_L, dist_L);
    Serial.printf("  Right: %d counts = %.1f mm\n", final_R, dist_R);
    Serial.println();
    
    // Direction check
    Serial.println("────────────────────────────────────────────────────────────");
    Serial.println("★ ตรวจสอบทิศทาง:");
    if (final_L < 0 && final_R < 0) {
        Serial.println("  ✓ ถอยหลัง = count ติดลบ (ถูกต้อง)");
    } else {
        Serial.println("  ✗ ทิศทางผิด - ต้องแก้ไข wiring หรือ config");
    }
    Serial.println("════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printHeader() {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                            ║");
    Serial.println("║          MICROMOUSE ESP32-S3                               ║");
    Serial.println("║          Workshop 2: Encoder Test                          ║");
    Serial.println("║                                                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printPinConfig() {
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ PIN CONFIGURATION                                          │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.printf("│  Encoder L-A: GPIO %2d                                      │\n", PIN_ENC_L_A);
    Serial.printf("│  Encoder L-B: GPIO %2d                                      │\n", PIN_ENC_L_B);
    Serial.printf("│  Encoder R-A: GPIO %2d                                      │\n", PIN_ENC_R_A);
    Serial.printf("│  Encoder R-B: GPIO %2d                                      │\n", PIN_ENC_R_B);
    Serial.println("└────────────────────────────────────────────────────────────┘");
}

void printEncoderConfig() {
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ ENCODER CONFIGURATION                                      │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.printf(" │  Counts per Rev:     %d                                    │\n", COUNTS_PER_REV);
    Serial.printf(" │  Wheel Diameter:     %.1f mm                               │\n", WHEEL_DIAMETER_MM);
    Serial.printf(" │  Wheel Circumference:%.2f mm                               │\n", WHEEL_CIRCUMFERENCE_MM);
    Serial.printf(" │  mm per Count:       %.4f                                  │\n", MM_PER_COUNT);
    Serial.println("│  Mode:               X4 Quadrature                         │");
    Serial.println("└────────────────────────────────────────────────────────────┘");
    Serial.println();
}

void printHelp() {
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ COMMANDS                                                   │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│ STEP 1 — หาค่า CPR (หมุนล้อด้วยมือ):                          │");
    Serial.println("│   r = Reset counters เป็น 0                                 │");
    Serial.println("│   s = Live counts ON/OFF (หมุนล้อ 1 รอบ → count = CPR)       │");
    Serial.println("│   c = Show counts (อ่านครั้งเดียว)                            │");
    Serial.println("│   → เอา CPR + ⌀ล้อ ใส่ config (COUNTS_PER_REV) แล้ว reflash   │");
    Serial.println("│                                                            │");
    Serial.println("│ STEP 2 — ทดสอบว่า CPR ถูก (วัดด้วยไม้บรรทัด):                 │");
    Serial.println("│   drive <mm>   = ขับระยะตาม config CPR → วัดจริง             │");
    Serial.println("│   cpr <v> [mm] = ขับ (เริ่ม 200mm) ตาม CPR ที่กรอก → วัดจริง   │");
    Serial.println("│                                                            │");
    Serial.println("│ อื่น ๆ:  v=velocity  d=distance  t=motor test  p=status  h=help │");
    Serial.println("└────────────────────────────────────────────────────────────┘");
}

void printCounts() {
    int32_t count_L, count_R;
    encoder_get_counts(&count_L, &count_R);
    
    float rev_L = encoder_counts_to_revolutions(count_L);
    float rev_R = encoder_counts_to_revolutions(count_R);
    
    Serial.println();
    Serial.println("┌────────────────────────────────────────┐");
    Serial.println("│         ENCODER COUNTS                 │");
    Serial.println("├────────────────────────────────────────┤");
    Serial.printf(" │  Left:  %8d counts (%.2f rev)          │\n", count_L, rev_L);
    Serial.printf(" │  Right: %8d counts (%.2f rev)          │\n", count_R, rev_R);
    Serial.println("└────────────────────────────────────────┘");
}

void printVelocity() {
    float vel_L = encoder_get_velocity_mm(ENCODER_LEFT, true);
    float vel_R = encoder_get_velocity_mm(ENCODER_RIGHT, true);
    float vel_avg = (vel_L + vel_R) / 2.0f;
    
    Serial.println();
    Serial.println("┌────────────────────────────────────────┐");
    Serial.println("│         VELOCITY                       │");
    Serial.println("├────────────────────────────────────────┤");
    Serial.printf(" │  Left:    %7.1f mm/s                   │\n", vel_L);
    Serial.printf(" │  Right:   %7.1f mm/s                   │\n", vel_R);
    Serial.printf(" │  Average: %7.1f mm/s                   │\n", vel_avg);
    Serial.println("└────────────────────────────────────────┘");
}

void printDistance() {
    float dist_L = encoder_get_distance_mm(ENCODER_LEFT);
    float dist_R = encoder_get_distance_mm(ENCODER_RIGHT);
    float dist_avg = (dist_L + dist_R) / 2.0f;
    
    Serial.println();
    Serial.println("┌────────────────────────────────────────┐");
    Serial.println("│         DISTANCE                       │");
    Serial.println("├────────────────────────────────────────┤");
    Serial.printf(" │  Left:    %8.1f mm                     │\n", dist_L);
    Serial.printf(" │  Right:   %8.1f mm                     │\n", dist_R);
    Serial.printf(" │  Average: %8.1f mm                     │\n", dist_avg);
    Serial.println("└────────────────────────────────────────┘");
}

void printQuickStatus() {
    int32_t count_L, count_R;
    encoder_get_counts(&count_L, &count_R);
    // Compact, parseable line for the UI: live counts (= CPR after 1 hand-spun rev)
    Serial.printf("E,%ld,%ld\n", (long)count_L, (long)count_R);
}

void printPlotData() {
    int32_t count_L, count_R;
    encoder_get_counts(&count_L, &count_R);
    float vel_L = encoder_get_velocity_mm(ENCODER_LEFT, true);
    float vel_R = encoder_get_velocity_mm(ENCODER_RIGHT, true);
    float dist_L = encoder_get_distance_mm(ENCODER_LEFT);
    float dist_R = encoder_get_distance_mm(ENCODER_RIGHT);
    
    // Format for Serial Plotter
    Serial.printf("%d,%d,%.1f,%.1f,%.1f,%.1f\n",
                  count_L, count_R, vel_L, vel_R, dist_L, dist_R);
}
