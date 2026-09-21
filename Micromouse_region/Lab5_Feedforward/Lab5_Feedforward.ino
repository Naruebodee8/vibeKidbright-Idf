// ═══════════════════════════════════════════════════════════════════════════
//  Lab5_Feedforward.ino  —  Characterise the drive system (UKMARS method)
//
//  อ้างอิง: Peter Harrison / UKMARS "Feeding Your Robot" + "Easier PD Controller"
//
//  หลักการ: ป้อนแรงดันแบบ STEP แล้วดูการตอบสนองความเร็ว (1st-order):
//        v(t) = Vmax · (1 − e^(−t/Tm))
//  จาก step response บนพื้นจริง (มีโหลด) วัดได้ 2 ค่าคุณลักษณะต่อล้อ:
//        • Km = system gain  (mm/s ต่อ Volt)   = ความชัน speed-vs-voltage
//        • Tm = time constant (วินาที)          = เวลาขึ้นถึง 63% ของ Vmax
//  แล้วได้พารามิเตอร์ feedforward:
//        FF_SPEED = 1/Km   ·   FF_BIAS = จุดตัดแกน V (static FF)   ·   FF_ACC = FF_SPEED·Tm
//  และส่ง Km, Tm ไปคำนวณ Kp/Kd ใน Lab6 ("gaincalc") ได้เลย — ไม่ต้องลองผิดลองถูก
//
//  ★ ทำบนพื้นจริง (ค่าจึงตรงกับตอนใช้งาน) — ต้องการพื้นที่โล่ง ~0.5 m
//    หุ่นจะวิ่งสลับ "หน้า-หลัง" ทีละ step เพื่ออยู่กับที่ ไม่วิ่งหนี
//
//  คุมไร้สาย BLE "MM_Lab5_FF"  (ผ่าน mm_ui.py แท็บ Lab5 — มีกราฟให้ดู)
//  คำสั่ง:  r = รัน step-response sweep (อัตโนมัติ) → สรุป FF + Km/Tm  ★
//          ff <V> = step เดียว (สตรีมเส้นโค้ง speed-vs-time ให้กราฟ)
//          v = อ่าน Vbat · x = หยุด · h = help
// ═══════════════════════════════════════════════════════════════════════════
#include "config.h"
#include "hal_motor.h"
#include "hal_encoder.h"
#include "ble_debug.h"
#include "lab_link.h"     // ← ท้ายสุด: Serial = USB + BLE

// ─────────────────────────────────────────────────────────────────────────────
// Test parameters
// ─────────────────────────────────────────────────────────────────────────────
const float FF_TEST_VOLTS[] = { 2.0f, 3.0f, 4.0f, 5.0f };  // straddle most robots
const int   FF_N            = sizeof(FF_TEST_VOLTS) / sizeof(FF_TEST_VOLTS[0]);
#define STEP_MS        450      // step duration (≈4·Tm → reaches plateau)
#define SAMPLE_MS      10       // speed sample interval (100 Hz)
#define MAXSAMP        50       // STEP_MS / SAMPLE_MS + margin
#define SETTLE_MS      700      // pause between steps (wheels fully stop)
#define MOVE_MIN_MMPS  40.0f    // below this = wheel didn't move → skip in fit

// step-response sample buffers (one step at a time)
float g_t[MAXSAMP];     // ms since step start
float g_sL[MAXSAMP];    // |left speed|  mm/s
float g_sR[MAXSAMP];    // |right speed| mm/s
int   g_n = 0;

// ─────────────────────────────────────────────────────────────────────────────
float readVbat() {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_BATTERY_ADC);
    return (sum / 16.0f) * (float)BATTERY_DIVIDER_RATIO / 1000.0f;
}

int voltsToPwm(float volts, float vbat) {
    if (vbat < 1.0f) vbat = 7.4f;
    int pwm = (int)(volts / vbat * MOTOR_PWM_MAX + 0.5f);
    if (pwm >  MOTOR_PWM_MAX) pwm =  MOTOR_PWM_MAX;
    if (pwm < -MOTOR_PWM_MAX) pwm = -MOTOR_PWM_MAX;
    return pwm;
}

bool batteryOk() {
    float v = readVbat();
    if (v < 7.0f) { Serial.printf("[!] Battery %.2fV ต่ำ — ชาร์จก่อน\n", v); return false; }
    return true;
}

void showBattery() { Serial.printf("\nBATTERY: %.2f V\n", readVbat()); }

bool abortNow() {
    if (Serial.available()) { char c = Serial.read(); if (c == 'x' || c == 'X') return true; }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// sampleStep — apply a voltage STEP (signed) and log per-wheel speed vs time
//   sign: +1 forward, -1 reverse (alternate to stay in place)
// ─────────────────────────────────────────────────────────────────────────────
void sampleStep(float volts, int sign) {
    float vbat = readVbat();
    int pwm = sign * voltsToPwm(volts, vbat);

    encoder_update();
    float lp = encoder_get_distance_mm(ENCODER_LEFT);
    float rp = encoder_get_distance_mm(ENCODER_RIGHT);
    uint32_t t0 = millis(), tp = t0;
    g_n = 0;

    motor_set_speed(pwm, pwm);
    while ((millis() - t0) < STEP_MS && g_n < MAXSAMP) {
        if ((millis() - tp) >= SAMPLE_MS) {
            uint32_t now = millis();
            float dt = (now - tp) / 1000.0f; if (dt < 0.001f) dt = 0.001f;
            encoder_update();
            float ln = encoder_get_distance_mm(ENCODER_LEFT);
            float rn = encoder_get_distance_mm(ENCODER_RIGHT);
            g_t[g_n]  = (float)(now - t0);
            g_sL[g_n] = fabsf((ln - lp) / dt);
            g_sR[g_n] = fabsf((rn - rp) / dt);
            lp = ln; rp = rn; tp = now;
            g_n++;
        }
    }
    motor_stop();
}

// extract Vmax (plateau) and Tm (63% rise, ms) per wheel from the last step
void analyzeStep(float* vmaxL, float* vmaxR, float* tmL, float* tmR) {
    int k = (g_n < 8) ? g_n : 8;            // average last k samples = plateau
    float sL = 0, sR = 0;
    for (int i = g_n - k; i < g_n; i++) { sL += g_sL[i]; sR += g_sR[i]; }
    *vmaxL = (k > 0) ? sL / k : 0;
    *vmaxR = (k > 0) ? sR / k : 0;

    *tmL = (g_n > 0) ? g_t[g_n - 1] : 0;    // default = end if never crosses
    *tmR = *tmL;
    float tgtL = 0.63f * (*vmaxL), tgtR = 0.63f * (*vmaxR);
    for (int i = 0; i < g_n; i++) if (g_sL[i] >= tgtL) { *tmL = g_t[i]; break; }
    for (int i = 0; i < g_n; i++) if (g_sR[i] >= tgtR) { *tmR = g_t[i]; break; }
}

// least-squares fit y = slope*x + intercept  (returns false if <2 points)
bool linreg(const float* x, const float* y, int n, float* slope, float* intercept) {
    if (n < 2) return false;
    float sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; i++) { sx += x[i]; sy += y[i]; sxx += x[i]*x[i]; sxy += x[i]*y[i]; }
    float d = n * sxx - sx * sx;
    if (fabsf(d) < 1e-6f) return false;
    *slope     = (n * sxy - sx * sy) / d;
    *intercept = (sy - (*slope) * sx) / n;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// runFFRamp — full automated sweep across FF_TEST_VOLTS, alternating direction
//   Streams to UI:  S,<V>,<t>,<sL>,<sR>   (step curve)   ·   FP,<V>,<sL>,<sR> (point)
//                   FIT,<ffsL>,<ffbL>,<ffsR>,<ffbR>      (fitted FF lines)
// ─────────────────────────────────────────────────────────────────────────────
void runFFRamp() {
    if (!batteryOk()) return;
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("  FEEDFORWARD STEP-RESPONSE SWEEP  (★ บนพื้นจริง, ~0.5m)");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("หุ่นจะวิ่งสลับหน้า-หลัง — เริ่มใน 2 วิ (ส่ง 'x' ยกเลิก)");
    delay(20); while (Serial.available()) Serial.read();
    for (int s = 2; s > 0; s--) {
        Serial.printf("  %d...\n", s);
        uint32_t t = millis();
        while (millis() - t < 1000) { if (abortNow()) { Serial.println("ยกเลิก"); return; } delay(10); }
    }

    float Vpt[8], spL[8], spR[8], tmLs[8], tmRs[8];
    int np = 0;

    for (int i = 0; i < FF_N; i++) {
        float V = FF_TEST_VOLTS[i];
        int sign = (i % 2 == 0) ? +1 : -1;       // alternate → stay near start
        sampleStep(V, sign);

        float vmL, vmR, tmL, tmR;
        analyzeStep(&vmL, &vmR, &tmL, &tmR);

        // stream the curve (speed vs time) for THIS voltage → UI overlays them
        for (int j = 0; j < g_n; j++)
            Serial.printf("S,%.1f,%.0f,%.0f,%.0f\n", V, g_t[j], g_sL[j], g_sR[j]);
        Serial.printf("FP,%.2f,%.0f,%.0f\n", V, vmL, vmR);  // V-vs-speed point (FP ≠ maze P)

        Serial.printf("  V=%.1f  speedL=%4.0f  speedR=%4.0f  TmL=%3.0fms  TmR=%3.0fms\n",
                      V, vmL, vmR, tmL, tmR);

        Vpt[np] = V; spL[np] = vmL; spR[np] = vmR; tmLs[np] = tmL; tmRs[np] = tmR; np++;

        uint32_t t = millis();
        while (millis() - t < SETTLE_MS) { if (abortNow()) break; delay(10); }
    }

    // ---- fit only the points where the wheel actually moved ----
    float xL[8], yL[8], xR[8], yR[8]; int nL = 0, nR = 0;
    float tmL_sum = 0, tmR_sum = 0; int tmL_n = 0, tmR_n = 0;
    for (int i = 0; i < np; i++) {
        if (spL[i] > MOVE_MIN_MMPS) { xL[nL] = spL[i]; yL[nL] = Vpt[i]; nL++; tmL_sum += tmLs[i]; tmL_n++; }
        if (spR[i] > MOVE_MIN_MMPS) { xR[nR] = spR[i]; yR[nR] = Vpt[i]; nR++; tmR_sum += tmRs[i]; tmR_n++; }
    }

    float ffsL, ffbL, ffsR, ffbR;
    bool okL = linreg(xL, yL, nL, &ffsL, &ffbL);   // V = FF_SPEED·speed + FF_BIAS
    bool okR = linreg(xR, yR, nR, &ffsR, &ffbR);
    if (!okL || !okR) {
        Serial.println("[!] จุดที่ล้อขยับไม่พอ (อาจ BIAS สูง) — ลองเพิ่มแรงดันด้วย 'ff <V>' ที่ V สูงขึ้น");
        return;
    }

    float tmL_s = (tmL_n ? tmL_sum / tmL_n : 0) / 1000.0f;   // sec
    float tmR_s = (tmR_n ? tmR_sum / tmR_n : 0) / 1000.0f;
    float KmL = 1.0f / ffsL, KmR = 1.0f / ffsR;
    float accL = ffsL * tmL_s, accR = ffsR * tmR_s;          // FF_ACC = FF_SPEED·Tm
    float Km_avg = 0.5f * (KmL + KmR), Tm_avg = 0.5f * (tmL_s + tmR_s);

    Serial.printf("FIT,%.5f,%.4f,%.5f,%.4f\n", ffsL, ffbL, ffsR, ffbR);   // → UI fit lines

    Serial.println("──────────────────────────────────────────────");
    Serial.println("★ ผลลัพธ์ → ใส่ใน config.h:");
    Serial.printf("   #define FF_SPEED_L_V_PER_MMPS   %.5ff\n", ffsL);
    Serial.printf("   #define FF_SPEED_R_V_PER_MMPS   %.5ff\n", ffsR);
    Serial.printf("   #define FF_BIAS_RUN_L_V         %.3ff\n", ffbL);
    Serial.printf("   #define FF_BIAS_RUN_R_V         %.3ff\n", ffbR);
    Serial.printf("   #define FF_ACC_V_PER_MMPS2      %.5ff   (เฉลี่ย L/R = %.5f)\n",
                  0.5f*(accL+accR), 0.5f*(accL+accR));
    Serial.println("──────────────────────────────────────────────");
    Serial.printf("   ค่าระบบ:  Km(L=%.0f R=%.0f mm/s/V)   Tm(L=%.0f R=%.0f ms)\n",
                  KmL, KmR, tmL_s*1000, tmR_s*1000);
    Serial.printf("   → ไป Lab5 คำนวณ Kp/Kd:  gaincalc %.0f %.3f 1.0 0.12\n", Km_avg, Tm_avg);
    Serial.println("     (Km Tm zeta TD ; zeta=1 ไม่ overshoot, TD=settling time วินาที)");
    Serial.println("══════════════════════════════════════════════");
}

// ─────────────────────────────────────────────────────────────────────────────
// stepVoltageOne — single step at V, stream the curve (detailed time-response plot)
// ─────────────────────────────────────────────────────────────────────────────
void stepVoltageOne(float V) {
    if (!batteryOk()) return;
    Serial.printf("\nFF step %.1fV (บนพื้น) — เริ่มใน 1 วิ\n", V);
    delay(1000);
    sampleStep(V, +1);
    float vmL, vmR, tmL, tmR;
    analyzeStep(&vmL, &vmR, &tmL, &tmR);
    for (int j = 0; j < g_n; j++)
        Serial.printf("S,%.1f,%.0f,%.0f,%.0f\n", V, g_t[j], g_sL[j], g_sR[j]);
    Serial.printf("  Vmax  L=%.0f  R=%.0f mm/s   Tm  L=%.0f  R=%.0f ms\n", vmL, vmR, tmL, tmR);
    if (vmL > MOVE_MIN_MMPS && vmR > MOVE_MIN_MMPS)
        Serial.printf("  Km L=%.0f R=%.0f mm/s/V\n", vmL / V, vmR / V);
}

// ─────────────────────────────────────────────────────────────────────────────
void printHelp() {
    Serial.println();
    Serial.println("──────── Lab6 Feedforward (step response) ────────");
    Serial.println("  r       = sweep อัตโนมัติ → FF_SPEED/BIAS/ACC + Km/Tm  ★");
    Serial.println("  ff <V>  = step เดียว ที่แรงดัน V (ดูเส้นโค้ง + Tm)");
    Serial.println("  v       = อ่าน Vbat");
    Serial.println("  x       = หยุดมอเตอร์ / ยกเลิก");
    Serial.println("  h/?     = help");
    Serial.println("  *** ทำบนพื้นจริง พื้นที่โล่ง ~0.5m (หุ่นวิ่งสลับหน้า-หลัง) ***");
    Serial.println("──────────────────────────────────────────────────");
}

void handleCommand(char c) {
    switch (c) {
        case 'r': case 'R': runFFRamp();   break;
        case 'v': case 'V': showBattery(); break;
        case 'x': case 'X': motor_stop(); Serial.println("STOP"); break;
        case 'h': case 'H': case '?': printHelp(); break;
        case '\n': case '\r': case ' ': break;
        default: Serial.printf("ไม่รู้จัก '%c' — 'h' ดูคำสั่ง\n", c); break;
    }
}

void handleLine(char* s) {
    char* sp = strchr(s, ' ');
    if (!sp) { if (s[0]) handleCommand(s[0]); return; }
    *sp = 0;
    float v = atof(sp + 1);
    if (!strcmp(s, "ff")) {
        if (v >= 0.5f && v <= 7.0f) stepVoltageOne(v);
        else Serial.println("ff range 0.5-7.0 V");
    } else {
        Serial.printf("? unknown '%s'\n", s);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial) delay(10);
    delay(1000);
    ble_debug_init("MM_Lab5_FF");

    Serial.println("\n=== Lab5: Feedforward characterisation (step response) ===");
    if (!motor_init())   Serial.println("[!] motor_init failed");
    if (!encoder_init()) Serial.println("[!] encoder_init failed");
    motor_stop();
    printHelp();
}

// Serial (=LL tee) carries BOTH USB and BLE. Buffer a full line then dispatch.
void loop() {
    static char buf[24];
    static int  n = 0;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') { if (n) { buf[n] = 0; handleLine(buf); n = 0; } }
        else if (n < (int)sizeof(buf) - 1) buf[n++] = c;
    }
    delay(2);
}
