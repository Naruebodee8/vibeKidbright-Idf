// ═══════════════════════════════════════════════════════════════════
//  joystick_calibration_oled.ino
//  Joystick Calibration Test Program with SSD1306 OLED Display
// ═══════════════════════════════════════════════════════════════════

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── OLED CONFIGURATION ───────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1     // Share reset pin or none
#define OLED_ADDR     0x3C   // Standard I2C address for SSD1306

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── PIN DEFINITIONS ──────────────────────────────────────────────
#define PIN_JS_X      33     // Joystick X-axis (Analog Pin)
#define PIN_JS_Y      32     // Joystick Y-axis (Analog Pin)

// ─── SETTINGS ─────────────────────────────────────────────────────
#define FILTER_SIZE   10     // Number of readings for moving average
#define PRINT_MS      100    // Serial print interval in milliseconds
#define OLED_MS       150    // OLED screen update interval in milliseconds

// ─── GLOBAL VARIABLES ─────────────────────────────────────────────
int x_buf[FILTER_SIZE] = {0};
int y_buf[FILTER_SIZE] = {0};
int buf_idx = 0;

int x_min = 4095, x_max = 0;
int y_min = 4095, y_max = 0;

unsigned long last_print = 0;
unsigned long last_oled = 0;

// ─── LOW PASS / MOVING AVERAGE FILTER ──────────────────────────────
int filtered(int* buf, int new_val) {
    buf[buf_idx % FILTER_SIZE] = new_val;
    long sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += buf[i];
    }
    return (int)(sum / FILTER_SIZE);
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);   // ESP32 ADC: 12-bit resolution (0–4095)

    // Initialize OLED Display
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    delay(1000);

    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║        Joystick Calibration Test         ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println("Instructions:");
    Serial.println("  1. Leave Joystick idle (Neutral) -> Press Enter to log neutral");
    Serial.println("  2. Move Joystick to max/min limits of X and Y axes");
    Serial.println("  3. Check the screen and Serial Monitor for calibrated limits");
    Serial.println("────────────────────────────────────────────");
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {
    // Read and filter analog values
    int x_raw = filtered(x_buf, analogRead(PIN_JS_X));
    int y_raw = filtered(y_buf, analogRead(PIN_JS_Y));
    buf_idx++;

    // Track minimum and maximum values automatically
    if (x_raw < x_min) x_min = x_raw;
    if (x_raw > x_max) x_max = x_raw;
    if (y_raw < y_min) y_min = y_raw;
    if (y_raw > y_max) y_max = y_raw;

    unsigned long current_time = millis();

    // Print to Serial Monitor for logging/plotting
    if (current_time - last_print >= PRINT_MS) {
        last_print = current_time;
        Serial.printf("X_raw: %d, Y_raw: %d | X_min: %d, X_max: %d | Y_min: %d, Y_max: %d\n",
                      x_raw, y_raw, x_min, x_max, y_min, y_max);
    }

    // Update OLED Display
    if (current_time - last_oled >= OLED_MS) {
        last_oled = current_time;
        
        display.clearDisplay();
        
        // Draw Header
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 0);
        display.println("JOYSTICK CALIBRATION");
        display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

        // Display current live values
        display.setCursor(0, 15);
        display.printf("X: %4d   Y: %4d", x_raw, y_raw);

        // Display measured min/max
        display.setCursor(0, 28);
        display.printf("X Min: %4d Max: %4d", x_min, x_max);
        
        display.setCursor(0, 38);
        display.printf("Y Min: %4d Max: %4d", y_min, y_max);

        // Visual Joystick Position Box (right side / helper)
        display.drawRect(95, 15, 30, 30, SSD1306_WHITE);
        display.drawFastHLine(95, 30, 30, SSD1306_WHITE);
        display.drawFastVLine(110, 15, 30, SSD1306_WHITE);

        // Map analog values to coordinates in the joystick visualizer box
        // Box is from x=95 to 125 (width 30, center 110), y=15 to 45 (height 30, center 30)
        int dot_x = map(x_raw, x_min, x_max, 96, 124);
        int dot_y = map(y_raw, y_min, y_max, 44, 16); // Invert Y for screen coordinates (up is smaller Y)
        
        // Handle initial out-of-bounds or zero-range map issues
        if(x_max - x_min == 0) dot_x = 110;
        if(y_max - y_min == 0) dot_y = 30;
        dot_x = constrain(dot_x, 96, 124);
        dot_y = constrain(dot_y, 16, 44);

        display.fillCircle(dot_x, dot_y, 2, SSD1306_WHITE);

        // Bottom status / instructions
        display.setTextSize(1);
        display.setCursor(0, 54);
        display.print("Hold 'S' on Serial");
        
        display.display();
    }

    // Check Serial input to print final summary
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r' || c == 's' || c == 'S') {
            print_summary(x_raw, y_raw);
        }
        // Flush remaining buffer
        while (Serial.available()) Serial.read();
    }

    delay(10);
}

// ─── PRINT SUMMARY TO SERIAL ───────────────────────────────────────
void print_summary(int x_now, int y_now) {
    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║          CALIBRATION SUMMARY             ║");
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.printf("  X-Axis (Left/Right):\n");
    Serial.printf("    Current (Neutral) = %d\n", x_now);
    Serial.printf("    Min (Leftmost)    = %d\n", x_min);
    Serial.printf("    Max (Rightmost)   = %d\n", x_max);
    Serial.println();
    Serial.printf("  Y-Axis (Up/Down):\n");
    Serial.printf("    Current (Neutral) = %d\n", y_now);
    Serial.printf("    Min (Downmost)    = %d\n", y_min);
    Serial.printf("    Max (Upmost)      = %d\n", y_max);
    Serial.println("──────────────────────────────────────────");
    Serial.println("Copy and paste these values into your main controller code:");
    Serial.printf("  int js_x_neutral = %d;\n", x_now);
    Serial.printf("  int js_x_min     = %d;\n", x_min);
    Serial.printf("  int js_x_max     = %d;\n", x_max);
    Serial.printf("  int js_y_neutral = %d;\n", y_now);
    Serial.printf("  int js_y_min     = %d;\n", y_min);
    Serial.printf("  int js_y_max     = %d;\n", y_max);
    Serial.println("──────────────────────────────────────────");
    Serial.println();
}
