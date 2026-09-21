// ═══════════════════════════════════════════════════════════════════════════
//  test_04_ir_sensor.ino
//  Workshop 4: Sharp IR Sensor Test
//
//  วัตถุประสงค์:
//    1. ตรวจสอบการต่อวงจร Sharp GP2Y0A51SK0F (อ่านค่า mV และ mm ได้)
//    2. ทำความเข้าใจ LUT calibration (mV → mm) และปรับให้ตรงกับเซนเซอร์จริง
//    3. ตั้งค่า threshold สำหรับ wall detection
//    4. ดู real-time data บน Dashboard (BLE)
//
//  Hardware Setup:
//    - Sharp GP2Y0A51SK0F × 4 ต่อกับ ADC GPIO:
//        Right:       GPIO 14
//        Front-Right: GPIO 13
//        Front-Left:  GPIO 12
//        Left:        GPIO 11
//    - ไม่ต้องใช้ I2C (ไม่ต้องต่อ SCL/SDA สำหรับ sensor นี้)
//
//  คำสั่ง Serial:
//    r = อ่านทุกเซนเซอร์ทันที (แสดง mV + mm)
//    v = แสดง raw mV ทุกตัว (ใช้ตอน calibrate LUT)
//    s = Toggle continuous reading on/off
//    t = ตั้งค่า threshold
//    l = แสดง SHARP_LUT ที่ใช้อยู่
//    n = Re-initialize sensors
//    p = แสดง full status
//    h = Help
//
//  ★ ค่าที่ต้องบันทึก (ใส่ใน config.h):
//    - WALL_THRESHOLD_FRONT = ______ mm
//    - WALL_THRESHOLD_SIDE  = ______ mm
//    - WALL_OFFSET_L/FL/FR/R = ______  (ถ้าต้องการปรับ offset)
// ═══════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
// INCLUDES
// ─────────────────────────────────────────────────────────────────────────────
#include "config.h"
#include "hal_wall_sensor.h"
#include "telemetry.h"
#include "ble_debug.h"
#include "lab_link.h"     // ← ท้ายสุด: Serial ส่งได้ทั้ง USB + BLE

// ─────────────────────────────────────────────────────────────────────────────
// TEST PARAMETERS
// ─────────────────────────────────────────────────────────────────────────────
#define SENSOR_READ_INTERVAL_MS  25     // 40 Hz (round-robin → ~10 Hz per sensor)
#define DISPLAY_INTERVAL_MS      200    // 5 Hz display

// Display modes — เลือกด้วยคำสั่ง 's'
#define DISPLAY_OFF   0    // ปิด continuous display (ใช้ cmd 'r'/'v' แทน)
#define DISPLAY_MM    1    // แสดงระยะ mm + wall detection เท่านั้น
#define DISPLAY_MV    2    // แสดง raw ADC mV เท่านั้น  ← เหมาะตอน calibrate LUT
#define DISPLAY_BOTH  3    // แสดงทั้ง mm และ mV (2 บรรทัด)

// ─────────────────────────────────────────────────────────────────────────────
// VARIABLES
// ─────────────────────────────────────────────────────────────────────────────
uint8_t displayMode   = DISPLAY_OFF;  // เริ่มด้วยโหมดแสดงครบ
unsigned long lastSensorRead  = 0;
unsigned long lastDisplayTime = 0;
uint32_t loopTimeUs = 0;

// ─────────────────────────────────────────────────────────────────────────────
// TELEMETRY COMMAND HANDLER
// ─────────────────────────────────────────────────────────────────────────────
void onTelemetryCommand(TelemetryCommand_t cmd) {
    String command = String(cmd.command);

    if (command == "calibrate") {
        Serial.println("[CMD] Reading all sensors...");
        telemetry_log("Reading sensors...");
        wall_sensor_read_all();
        wall_sensor_print_status();
    } else if (command == "reset") {
        Serial.println("[CMD] Re-initializing sensors...");
        telemetry_log("Re-init sensors...");
        wall_sensor_init();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// BATTERY
// ─────────────────────────────────────────────────────────────────────────────
uint16_t readBatteryVoltage() {
    uint16_t raw_adc    = analogRead(PIN_BATTERY_ADC);
    float    pin_voltage = (raw_adc * (float)ADC_REFERENCE_MV) / (float)ADC_RESOLUTION;
    return (uint16_t)(pin_voltage * BATTERY_DIVIDER_RATIO);
}

// ─────────────────────────────────────────────────────────────────────────────
// TELEMETRY UPDATE
// ─────────────────────────────────────────────────────────────────────────────
void updateTelemetryData() {
    // Wall sensors — col "Raw" = LUT mm ก่อนหักลบ offset (ดู dashboard)
    //               col "Dist" = mm หลังหักลบ offset (ค่าที่ใช้งานจริง)
    //               raw_mv (ADC mV) แสดงแยกใน Serial คำสั่ง 'v' / 'r'
    telemetry_set_wall_sensors_full(
        _wall_state.sensor[WALL_SENSOR_LEFT].raw_distance_mm,
        _wall_state.sensor[WALL_SENSOR_FRONT_LEFT].raw_distance_mm,
        _wall_state.sensor[WALL_SENSOR_FRONT_RIGHT].raw_distance_mm,
        _wall_state.sensor[WALL_SENSOR_RIGHT].raw_distance_mm,
        _wall_state.sensor[WALL_SENSOR_LEFT].distance_mm,
        _wall_state.sensor[WALL_SENSOR_FRONT_LEFT].distance_mm,
        _wall_state.sensor[WALL_SENSOR_FRONT_RIGHT].distance_mm,
        _wall_state.sensor[WALL_SENSOR_RIGHT].distance_mm,
        wall_sensor_wall_left(),
        wall_sensor_wall_front(),
        wall_sensor_wall_right()
    );

    telemetry_set_battery(readBatteryVoltage());
    telemetry_set_loop_time(loopTimeUs);
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) delay(10);
    delay(1000);
    ble_debug_init("MM_Lab4_IR");      // คุมไร้สายผ่าน BLE (WiFi ปิดแล้ว เพราะกวน IR/ADC2)

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                            ║");
    Serial.println("║          MICROMOUSE ESP32-S3                               ║");
    Serial.println("║          Workshop 4: Sharp IR Sensor Test                  ║");
    Serial.println("║                                                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    // แสดง pin configuration
    Serial.println("[INIT] ADC Pin Configuration:");
    Serial.printf("  Left:        GPIO %d\n", PIN_IR_LEFT);
    Serial.printf("  Front-Left:  GPIO %d\n", PIN_IR_FRONT_L);
    Serial.printf("  Front-Right: GPIO %d\n", PIN_IR_FRONT_R);
    Serial.printf("  Right:       GPIO %d\n", PIN_IR_RIGHT);
    Serial.printf("  ADC samples: %d  |  Supply: %dmV\n", SHARP_ADC_SAMPLES, SHARP_SUPPLY_MV);
    Serial.println();

    // Initialize wall sensors
    Serial.println("[INIT] Initializing IR sensors...");
    uint8_t sensors_ok = wall_sensor_init();

    if (sensors_ok == 0) {
        Serial.println("════════════════════════════════════════");
        Serial.println("  *** ERROR: No IR sensors configured ***");
        Serial.println("════════════════════════════════════════");
        Serial.println("ตรวจสอบ:");
        Serial.println("  - PIN_IR_* ใน config.h ตรงกับที่ต่อจริงไหม");
        Serial.println("  - ต่อสาย VCC (3.3V), GND, และ Signal ครบไหม");
        Serial.println("  - ค่า ADC pin ต้องไม่เป็น -1");
    } else {
        Serial.printf("[INIT] %d/%d sensors ready\n", sensors_ok, WALL_SENSOR_COUNT);
    }

    // Initialize Telemetry (WiFi Dashboard)
    Serial.println("[INIT] Initializing telemetry...");
    if (!telemetry_init()) {
        Serial.println("[WARN] Telemetry init failed - Serial only mode");
    } else {
        telemetry_set_command_callback(onTelemetryCommand);
        telemetry_print_info();
    }

    // อ่านค่าครั้งแรกก่อนแสดง status
    wall_sensor_read_all();
    wall_sensor_print_status();
    printHelp();

    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("Ready! วางวัตถุใกล้เซนเซอร์เพื่อทดสอบ");
    Serial.println("════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════
void loop() {
    uint32_t loopStart = micros();

    // อ่าน sensor แบบ round-robin — ทำงานตลอดเวลาเพื่อให้ telemetry ได้ข้อมูล
    // (ไม่ขึ้นกับ displayMode)
    if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
        lastSensorRead = millis();
        wall_sensor_read_next();
    }

    // ส่ง telemetry (WiFi dashboard)
    updateTelemetryData();
    telemetry_update();

    // Serial commands — reset lastDisplayTime ด้วยเพื่อกัน output ทับกัน
    if (Serial.available()) {
        handleSerialCommand(Serial.read());
    }

    // Continuous Serial display — แสดงตาม displayMode เท่านั้น
    if (displayMode != DISPLAY_OFF && millis() - lastDisplayTime >= DISPLAY_INTERVAL_MS) {
        lastDisplayTime = millis();
        printQuickStatus();
    }

    loopTimeUs = micros() - loopStart;
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMAND HANDLERS
// ═══════════════════════════════════════════════════════════════════════════

void handleSerialCommand(char cmd) {
    // รีเซ็ต timer ก่อนทุก cmd ที่มี output เพื่อกัน printQuickStatus() ยิงทับ
    if (cmd != '\n' && cmd != '\r') {
        lastDisplayTime = millis();
    }

    switch (cmd) {
        case 'r': case 'R':
            Serial.println("[CMD] Reading all sensors...");
            wall_sensor_read_all();
            printAllDistances();
            break;

        case 'v': case 'V':
            wall_sensor_read_all();
            printRawMv();
            break;

        case 's': case 'S': {
            displayMode = (displayMode + 1) % 4;
            const char* modeNames[] = {
                "OFF       (หยุดแสดงผลต่อเนื่อง)",
                "MM only   (ระยะ mm + wall status)",
                "mV only   (raw ADC mV  ← calibrate)",
                "MM + mV   (ทั้ง mm และ mV)"
            };
            Serial.printf("[MODE] Display: %s\n", modeNames[displayMode]);
            break;
        }

        case 't': case 'T':
            setThreshold();
            break;

        case 'l': case 'L':
            printLutTable();
            break;

        case 'n': case 'N':
            Serial.println("[CMD] Re-initializing sensors...");
            wall_sensor_init();
            break;

        case 'p': case 'P':
            wall_sensor_read_all();
            wall_sensor_print_status();
            break;

        case 'i': case 'I':
            telemetry_print_info();
            break;

        case 'h': case 'H': case '?':
            printHelp();
            break;

        case '\n': case '\r':
            break;

        default:
            Serial.printf("[CMD] Unknown: '%c' (h = help)\n", cmd);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SET THRESHOLD
// ─────────────────────────────────────────────────────────────────────────────
void setThreshold() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  SET WALL DETECTION THRESHOLD");
    Serial.println("════════════════════════════════════════════════════════════");

    uint8_t front, side;
    wall_sensor_get_threshold(&front, &side);
    Serial.printf("Current: Front=%d mm, Side=%d mm\n", front, side);
    Serial.printf("Sharp range: %d – %d mm\n", SHARP_RANGE_MIN_MM, SHARP_RANGE_MAX_MM);
    Serial.println();
    Serial.println("Enter: front_mm side_mm  (e.g. \"60 80\")");
    Serial.println("(กด Enter เพื่อเก็บค่าเดิม)");

    // flush \n ที่ค้างมาจากการกด Enter ส่งคำสั่ง 't'
    delay(20);
    while (Serial.available()) Serial.read();

    while (!Serial.available()) delay(10);
    delay(50);

    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
        int space = input.indexOf(' ');
        if (space > 0) {
            int new_front = input.substring(0, space).toInt();
            int new_side  = input.substring(space + 1).toInt();

            if (new_front >= SHARP_RANGE_MIN_MM && new_front <= SHARP_RANGE_MAX_MM &&
                new_side  >= SHARP_RANGE_MIN_MM && new_side  <= SHARP_RANGE_MAX_MM) {
                wall_sensor_set_threshold((uint8_t)new_front, (uint8_t)new_side);
                Serial.printf("Threshold set: Front=%d mm, Side=%d mm\n", new_front, new_side);
                Serial.println();
                Serial.println("★ ใส่ค่านี้ใน config.h:");
                Serial.println("────────────────────────────────────────");
                Serial.printf("#define WALL_THRESHOLD_FRONT  %d\n", new_front);
                Serial.printf("#define WALL_THRESHOLD_SIDE   %d\n", new_side);
                Serial.println("────────────────────────────────────────");
            } else {
                Serial.printf("ค่าต้องอยู่ใน %d–%d mm\n",
                              SHARP_RANGE_MIN_MM, SHARP_RANGE_MAX_MM);
            }
        } else {
            Serial.println("Format ผิด ต้องการ: front_mm side_mm");
        }
    } else {
        Serial.println("เก็บค่าเดิม");
    }
    Serial.println();
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printHelp() {
    const char* modeNames[] = {"OFF", "MM only", "mV only", "MM + mV"};
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ COMMANDS                                                   │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│   r = อ่านทุก sensor ทันที (mm + mV + wall)               │");
    Serial.println("│   v = อ่าน raw mV ทันที  ← ใช้ตอน calibrate LUT          │");
    Serial.println("│   s = เปลี่ยน continuous display mode (กด s วนซ้ำ):       │");
    Serial.println("│         OFF → MM only → mV only → MM+mV → OFF ...         │");
    Serial.println("│   t = ตั้งค่า threshold (mm)                               │");
    Serial.println("│   l = แสดง SHARP_LUT ที่ใช้อยู่                            │");
    Serial.println("│   n = Re-initialize sensors                                │");
    Serial.println("│   p = Full status                                          │");
    Serial.println("│   i = WiFi/telemetry info                                  │");
    Serial.println("│   h = Help                                                 │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.printf( "│ Display mode ตอนนี้: %-38s│\n", modeNames[displayMode]);
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│ SENSOR LAYOUT:              FRONT                          │");
    Serial.println("│                          ┌─────────┐                       │");
    Serial.println("│                          │ FL   FR │                       │");
    Serial.println("│                       L  │         │  R                    │");
    Serial.println("│                          └─────────┘                       │");
    Serial.println("└────────────────────────────────────────────────────────────┘");
}

void printQuickStatus() {
    uint8_t r, fr, fl, l;
    wall_sensor_get_distances(&r, &fr, &fl, &l);

    if (displayMode == DISPLAY_MM || displayMode == DISPLAY_BOTH) {
        Serial.printf("[IR ] L:%3dmm%s FL:%3dmm%s FR:%3dmm%s R:%3dmm%s\n",
                      l,  wall_sensor_wall_left()  ? "*" : " ",
                      fl, wall_sensor_is_wall(WALL_SENSOR_FRONT_LEFT)  ? "*" : " ",
                      fr, wall_sensor_is_wall(WALL_SENSOR_FRONT_RIGHT) ? "*" : " ",
                      r,  wall_sensor_wall_right() ? "*" : " ");
    }

    if (displayMode == DISPLAY_MV || displayMode == DISPLAY_BOTH) {
        Serial.printf("[mV] L:%4d   FL:%4d   FR:%4d   R:%4d\n",
                      wall_sensor_get_raw_mv(WALL_SENSOR_LEFT),
                      wall_sensor_get_raw_mv(WALL_SENSOR_FRONT_LEFT),
                      wall_sensor_get_raw_mv(WALL_SENSOR_FRONT_RIGHT),
                      wall_sensor_get_raw_mv(WALL_SENSOR_RIGHT));
    }
}

void printAllDistances() {
    Serial.println();
    Serial.println("┌─────────────────────────────────────────────────┐");
    Serial.println("│           SHARP IR SENSOR READINGS               │");
    Serial.println("├──────────────┬──────┬──────┬────────┬───────────┤");
    Serial.println("│ Sensor       │  mV  │ Raw  │  Dist  │ Wall      │");
    Serial.println("├──────────────┼──────┼──────┼────────┼───────────┤");

    const char* names[] = {"Left        ", "Front-Left  ", "Front-Right ", "Right       "};
    const WallSensorID_t ids[] = {
        WALL_SENSOR_LEFT,
        WALL_SENSOR_FRONT_LEFT,
        WALL_SENSOR_FRONT_RIGHT,
        WALL_SENSOR_RIGHT
    };

    for (int i = 0; i < 4; i++) {
        WallSensorReading_t rd = wall_sensor_get_reading(ids[i]);
        if (rd.status == IR_STATUS_NO_PIN) {
            Serial.printf("│ %s │  --  │  --  │   --   │ NO PIN    │\n", names[i]);
        } else {
            Serial.printf("│ %s │ %4d │ %3d  │ %3dmm  │ %s │\n",
                          names[i],
                          rd.raw_mv,
                          rd.raw_distance_mm,
                          rd.distance_mm,
                          rd.wall_detected ? "YES [WALL]" : "no        ");
        }
    }

    Serial.println("├──────────────┴──────┴──────┴────────┴───────────┤");
    Serial.printf("│ Front avg: %3d mm  %s                       │\n",
                  wall_sensor_get_front_distance(),
                  wall_sensor_wall_front() ? "[WALL]" : "      ");
    Serial.println("└─────────────────────────────────────────────────┘");
    Serial.println();
}

void printRawMv() {
    wall_sensor_read_all();

    Serial.println();
    Serial.println("┌──────────────────────────────────────────────────────┐");
    Serial.println("│  RAW ADC millivolts  (ใช้สำหรับ calibrate LUT)       │");
    Serial.println("├──────────────┬──────────┬──────────────────────────  │");
    Serial.println("│ Sensor       │  mV      │  Note                       │");
    Serial.println("├──────────────┼──────────┼──────────────────────────  │");

    const char* names[] = {"Left        ", "Front-Left  ", "Front-Right ", "Right       "};
    const WallSensorID_t ids[] = {
        WALL_SENSOR_LEFT, WALL_SENSOR_FRONT_LEFT,
        WALL_SENSOR_FRONT_RIGHT, WALL_SENSOR_RIGHT
    };

    for (int i = 0; i < 4; i++) {
        uint16_t mv = wall_sensor_get_raw_mv(ids[i]);
        uint8_t  mm = wall_sensor_get_distance(ids[i]);
        const char* note = (mm == 255) ? "out of range" : "";
        Serial.printf("│ %s │  %4d    │  → %3s mm %s\n",
                      names[i], mv, mm == 255 ? "---" : String(mm).c_str(), note);
    }

    Serial.println("└──────────────────────────────────────────────────────┘");
    Serial.println();
    Serial.println("วิธี calibrate LUT:");
    Serial.println("  1. วางวัตถุที่ระยะรู้ค่า (เช่น 30, 50, 80, 100, 120 mm)");
    Serial.println("  2. บันทึก mV ที่อ่านได้");
    Serial.println("  3. แก้ไข SHARP_LUT ใน hal_wall_sensor.h ให้ตรง");
    Serial.println();
}

void printLutTable() {
    Serial.println();
    Serial.println("┌──────────────────────────────────────────┐");
    Serial.println("│  SHARP_LUT  (mV → mm)                    │");
    Serial.println("│  *** Typical curve at 3.3V supply ***     │");
    Serial.println("│  *** Calibrate for your sensor!    ***    │");
    Serial.println("├────────┬────────┤");
    Serial.println("│   mV   │   mm   │");
    Serial.println("├────────┼────────┤");
    for (int i = 0; i < SHARP_LUT_SIZE; i++) {
        Serial.printf("│  %4d  │   %3d  │\n", SHARP_LUT[i].mv, SHARP_LUT[i].mm);
    }
    Serial.println("└────────┴────────┘");
    Serial.printf("Supply: %d mV  |  Samples per read: %d\n",
                  SHARP_SUPPLY_MV, SHARP_ADC_SAMPLES);
    Serial.println();
}
