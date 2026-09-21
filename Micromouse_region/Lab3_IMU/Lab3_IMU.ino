// ═══════════════════════════════════════════════════════════════════════════
//  test_03_imu.ino
//  Workshop 3: IMU (MPU6050) Test with Telemetry
//  
//  วัตถุประสงค์:
//    1. ทดสอบการอ่านค่า Gyroscope และ Accelerometer
//    2. Calibrate gyroscope offset
//    3. ทดสอบ heading integration (การหมุน)
//    4. ดู real-time data บน Dashboard
//  
//  การใช้งาน:
//    1. Upload โปรแกรม
//    2. เปิด Serial Monitor หรือต่อ WiFi Dashboard
//    3. ใช้คำสั่งทดสอบ
//  
//  คำสั่ง Serial:
//    c = Calibrate gyroscope (วางหุ่นยนต์นิ่งๆ)
//    r = Reset heading เป็น 0
//    s = Toggle continuous display
//    p = Print status
//    h = Help
//  
//  ★ ค่าที่ต้องบันทึก:
//    - GYRO_OFFSET_X = ______
//    - GYRO_OFFSET_Y = ______
//    - GYRO_OFFSET_Z = ______
// ═══════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
// INCLUDES
// ─────────────────────────────────────────────────────────────────────────────
#include "config.h"
#include "hal_imu.h"
#include "telemetry.h"
#include "ble_debug.h"
#include "lab_link.h"     // ← ท้ายสุด: Serial ส่งค่าได้ทั้ง USB + BLE

// ─────────────────────────────────────────────────────────────────────────────
// TEST PARAMETERS
// ─────────────────────────────────────────────────────────────────────────────
#define IMU_UPDATE_INTERVAL_MS  10          // 100 Hz update
#define DISPLAY_INTERVAL_MS     100         // 10 Hz display

// ─────────────────────────────────────────────────────────────────────────────
// VARIABLES
// ─────────────────────────────────────────────────────────────────────────────
bool continuousDisplay = false;
unsigned long lastIMUUpdate = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastLoopTime = 0;
uint32_t loopTimeUs = 0;

// ─────────────────────────────────────────────────────────────────────────────
// TELEMETRY COMMAND HANDLER
// ─────────────────────────────────────────────────────────────────────────────
void onTelemetryCommand(TelemetryCommand_t cmd) {
    String command = String(cmd.command);
    
    if (command == "calibrate") {
        Serial.println("[CMD] Calibrating gyroscope...");
        telemetry_log("Calibrating... Keep still!");
        
        if (imu_calibrate_gyro(500)) {
            float ox, oy, oz;
            imu_get_gyro_offset(&ox, &oy, &oz);
            
            char msg[64];
            snprintf(msg, sizeof(msg), "Cal OK: %.1f, %.1f, %.1f", ox, oy, oz);
            telemetry_log(msg);
            
            float ax, ay, az;
            imu_get_accel_offset(&ax, &ay, &az);
            Serial.println("[CAL] ════════════════════════════════════");
            Serial.println("[CAL] CALIBRATION COMPLETE");
            Serial.printf("[CAL] GYRO_OFFSET_X = %.1f   ← ใช้แกน Z สำหรับ heading\n", ox);
            Serial.printf("[CAL] GYRO_OFFSET_Y = %.1f\n", oy);
            Serial.printf("[CAL] GYRO_OFFSET_Z = %.1f\n", oz);
            Serial.printf("[CAL] ACCEL_OFFSET_X = %.3f  (optional — ไม่ใช้ขับ ใช้ต่อยอดได้)\n", ax);
            Serial.printf("[CAL] ACCEL_OFFSET_Y = %.3f\n", ay);
            Serial.printf("[CAL] ACCEL_OFFSET_Z = %.3f\n", az);
            Serial.println("[CAL] ════════════════════════════════════");
        } else {
            telemetry_log("Calibration failed!");
        }
    }
    else if (command == "reset") {
        imu_reset_heading();
        Serial.println("[CMD] Heading reset to 0");
        telemetry_log("Heading reset");
    }
    else if (command == "stop") {
        // Nothing to stop for IMU
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UPDATE TELEMETRY DATA
// ─────────────────────────────────────────────────────────────────────────────
void updateTelemetryData() {
    IMUData_t imuData = imu_get_data();
    
    // Set IMU data
    telemetry_set_imu(imuData.gyro_z, imuData.accel_x, imuData.accel_y);
    
    // Use custom values for more IMU data
    telemetry_set_custom(
        imuData.heading,                    // Custom 1: Heading (degrees)
        imuData.gyro_z,                     // Custom 2: Gyro Z (°/s)
        imuData.accel_z,                    // Custom 3: Accel Z (g)
        imuData.temperature                 // Custom 4: Temperature (°C)
    );
    
    // Loop time
    telemetry_set_loop_time(loopTimeUs);
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) delay(10);
    delay(1000);
    ble_debug_init("MM_Lab3_IMU");      // คุมไร้สายผ่าน BLE
    
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                            ║");
    Serial.println("║          MICROMOUSE ESP32-S3                               ║");
    Serial.println("║          Workshop 3: IMU Test (MPU6050)                    ║");
    Serial.println("║                                                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    
    // Initialize IMU
    Serial.println("[INIT] Initializing IMU (MPU6050)...");
    Serial.printf("[INIT] I2C: SDA=%d, SCL=%d, Freq=%d Hz\n", 
                  PIN_I2C0_SDA, PIN_I2C0_SCL, I2C0_FREQ);
    
    if (!imu_init_default()) {
        Serial.println("════════════════════════════════════════");
        Serial.println("  *** ERROR: IMU initialization failed! ***");
        Serial.println("════════════════════════════════════════");
        Serial.println("Check:");
        Serial.println("  - GY-521 wiring (SDA, SCL, VCC, GND)");
        Serial.println("  - I2C address (0x68 default, 0x69 if AD0=VCC)");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("[INIT] IMU OK");
    
    // Initialize Telemetry
    Serial.println("[INIT] Initializing telemetry...");
    if (!telemetry_init()) {
        Serial.println("[WARN] Telemetry init failed - Serial only mode");
    } else {
        telemetry_set_command_callback(onTelemetryCommand);
        telemetry_print_info();
    }
    
    // Print instructions
    printHelp();
    
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("Ready! Try rotating the robot to see heading change.");
    Serial.println("════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════
void loop() {
    uint32_t loopStart = micros();
    
    // Update IMU
    if (millis() - lastIMUUpdate >= IMU_UPDATE_INTERVAL_MS) {
        lastIMUUpdate = millis();
        imu_update();
    }
    
    // Update telemetry data
    updateTelemetryData();
    
    // Send telemetry
    telemetry_update();
    
    // Handle Serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        handleSerialCommand(cmd);
    }
    
    // Continuous display
    if (continuousDisplay && millis() - lastDisplayTime >= DISPLAY_INTERVAL_MS) {
        lastDisplayTime = millis();
        printQuickStatus();
    }
    
    // Calculate loop time
    loopTimeUs = micros() - loopStart;
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMAND HANDLERS
// ═══════════════════════════════════════════════════════════════════════════

void handleSerialCommand(char cmd) {
    switch (cmd) {
        case 'a':
        case 'A':
            if (imu_calibrate_accel(1000)) {
                float ax, ay, az;
                imu_get_accel_offset(&ax, &ay, &az); // อย่าลืมเพิ่ม Getter ใน hal_imu.h ด้วยนะครับ
                Serial.println("★ ค่าที่ต้องใส่ใน config.h:");
                Serial.printf("#define ACCEL_OFFSET_X  %.1f\n", ax);
                Serial.printf("#define ACCEL_OFFSET_Y  %.1f\n", ay);
                Serial.printf("#define ACCEL_OFFSET_Z  %.1f\n", az);
            }
            break;
        case 'c':
        case 'C':
            runCalibration();
            break;
            
        case 'r':
        case 'R':
            imu_reset_heading();
            Serial.println("[CMD] Heading reset to 0°");
            break;
            
        case 's':
        case 'S':
            continuousDisplay = !continuousDisplay;
            Serial.printf("[CMD] Continuous display: %s\n", 
                          continuousDisplay ? "ON" : "OFF");
            break;
            
        case 'p':
        case 'P':
            imu_print_status();
            break;
            
        case 'i':
        case 'I':
            telemetry_print_info();
            break;
            
        case 'g':
        case 'G':
            // Plot mode for Serial Plotter
            printPlotData();
            break;
            
        case 't':
        case 'T':
            runTurnTest();
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

void runCalibration() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  GYROSCOPE CALIBRATION");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("⚠️  Keep robot COMPLETELY STILL for 2 seconds...");
    Serial.println();
    
    delay(1000);  // Give time to stabilize
    
    if (imu_calibrate_gyro(1000)) {  // 1000 samples ≈ 1 second
        float ox, oy, oz;
        imu_get_gyro_offset(&ox, &oy, &oz);
        
        Serial.println();
        Serial.println("✓ CALIBRATION COMPLETE");
        Serial.println();
        Serial.println("★ ค่าที่ต้องใส่ใน config.h:");
        Serial.println("────────────────────────────────────────");
        Serial.printf("#define GYRO_OFFSET_X  %.1f\n", ox);
        Serial.printf("#define GYRO_OFFSET_Y  %.1f\n", oy);
        Serial.printf("#define GYRO_OFFSET_Z  %.1f\n", oz);
        Serial.println("────────────────────────────────────────");
    } else {
        Serial.println("✗ CALIBRATION FAILED");
        Serial.println("  Check I2C connection and try again");
    }
    Serial.println();
}

void runTurnTest() {
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  TURN TEST - Rotate robot 90° and check heading");
    Serial.println("════════════════════════════════════════════════════════════");
    
    imu_reset_heading();
    Serial.println("Heading reset to 0°");
    Serial.println("Rotate robot 90° clockwise, then press 't' again to check");
    Serial.printf("Current heading: %.1f°\n", imu_get_heading());
    Serial.println();
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printHelp() {
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────────┐");
    Serial.println("│ COMMANDS                                                   │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│   c = Calibrate gyroscope (keep robot still)               │");
    Serial.println("│   r = Reset heading to 0°                                  │");
    Serial.println("│   t = Turn test (check 90° rotation)                       │");
    Serial.println("│   s = Toggle continuous display                            │");
    Serial.println("│   p = Print full status                                    │");
    Serial.println("│   i = Print WiFi info                                      │");
    Serial.println("│   g = Print data for Serial Plotter                        │");
    Serial.println("│   h = Help                                                 │");
    Serial.println("├────────────────────────────────────────────────────────────┤");
    Serial.println("│ TELEMETRY: Connect to WiFi and open dashboard              │");
    Serial.println("│   - Custom1 = Heading (°)                                  │");
    Serial.println("│   - Custom2 = Gyro Z (°/s)                                 │");
    Serial.println("│   - Custom3 = Accel Z (g)                                  │");
    Serial.println("│   - Custom4 = Temperature (°C)                             │");
    Serial.println("└────────────────────────────────────────────────────────────┘");
}

void printQuickStatus() {
    IMUData_t data = imu_get_data();
    
    Serial.printf("[IMU] Heading: %7.1f° | Gyro Z: %7.2f°/s | Accel: %.2f, %.2f, %.2f g\n",
                  data.heading,
                  data.gyro_z,
                  data.accel_x, data.accel_y, data.accel_z);
}

void printPlotData() {
    // Format for Arduino Serial Plotter
    // Labels: Heading,GyroZ,AccelX,AccelY,AccelZ
    IMUData_t data = imu_get_data();
    
    Serial.printf("%.1f,%.2f,%.3f,%.3f,%.3f\n",
                  data.heading,
                  data.gyro_z,
                  data.accel_x, data.accel_y, data.accel_z);
}
