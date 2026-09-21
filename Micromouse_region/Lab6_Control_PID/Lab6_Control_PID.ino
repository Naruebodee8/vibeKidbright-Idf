// ═══════════════════════════════════════════════════════════════════════════
//  test_06_pid.ino  —  STEP 6: PID controller (บทเรียนแยก, ทดสอบไร้สายผ่าน BLE)
//
//  เป้าหมายของบทเรียน:
//    STEP 5 สอน "Feedforward" — แปลง "ความเร็วที่ต้องการ" → "แรงดันมอเตอร์"
//    โดยใช้โมเดล (BIAS + SPEED·v + ACC·a). มันเป็น OPEN-LOOP: เดาแรงดันล่วงหน้า
//    แต่ไม่รู้ว่าหุ่นไปถึงไหนจริง ๆ → ถ้าโมเดลเพี้ยน (แรงเสียดทาน, แบตตก, พื้นลื่น)
//    หุ่นจะ "ดริฟต์" ออกจากเป้า แล้ว error สะสมไปเรื่อย ๆ โดยไม่มีอะไรมาแก้ตอนหุ่นยนต์กำลังทำงาน
//
//    STEP 6 เพิ่ม "position PID (PD)" ปิดลูปทับ FF:
//        fwd_pd = Kp·(error) + Kd·(d error/dt)
//    error = ระยะที่ profile สั่ง − ระยะที่ encoder วัดได้จริง
//    PD จะดันแรงดันเพิ่ม/ลด เพื่อ "ลาก" หุ่นกลับเข้าเป้า → CLOSED-LOOP
//    เช่นเดียวกันมี rot_pd ที่ใช้ gyro คุมหัวให้ตรง (error = มุมที่ควรเป็น − มุมจริง)
//
//  วิธีทดสอบ (A/B): รัน move เดียวกัน 2 แบบ แล้วเทียบ
//    - FF-ONLY  (PID OFF): หุ่นวิ่งได้ แต่หยุดเพี้ยนจากเป้า + อาจเป๋ออกข้าง
//    - FF + PD  (PID ON ): หุ่นเกาะ profile, หยุดตรงเป้า ±ไม่กี่ mm, วิ่งตรง
//    ทั้งคู่ FEEDFORWARD เหมือนกันเป๊ะ — ต่างกันแค่ "เปิด PD ทับหรือไม่"
//
//  ★ จุดสำคัญทางวิศวกรรม: PID ในที่นี้ไม่ได้ "ขับ" หุ่นเอง — FF เป็นคนขับหลัก
//    PD แค่ "เก็บกวาด" ส่วนต่างที่ FF เดาพลาด. นี่คือเหตุผลที่ Kp ต้องการแค่น้อย ๆ
//    (Peter Harrison pattern): FF ทำงานหนัก → PD ทำงานเบา → จูนง่าย เสถียร
//
//  ──────────────────────────────────────────────────────────────────────────
//  ★ ต้องทดสอบแบบไร้สาย (เพราะหุ่นต้องวิ่งไม่งั้น สาย USB จะลาก/รั้ง ทำให้ค่าเพี้ยนได้):
//    BLE Nordic UART Service ชื่อ "MM_test06pid"
//    - แอป: nRF Connect / "Serial Bluetooth Terminal" (โหมด BLE) / Web Bluetooth
//    - พิมพ์คำสั่งตัวเดียวกับ USB ส่งไป → ผลลัพธ์ + CSV สตรีมกลับมาทาง BLE
//    - output ออกทั้ง USB และ BLE พร้อมกัน (ต่อทางไหนก็เห็น)
//    - 'x' abort ระหว่าง countdown ใช้ได้ทั้ง USB และ BLE
//  ──────────────────────────────────────────────────────────────────────────
//
//  คำสั่ง (USB serial @115200 newline  หรือ  BLE UART):
//    f = รัน move แบบ FF + PD   (closed-loop, ปกติ)
//    g = รัน move แบบ FF-only   (PID OFF — ดู error สะสม)
//    c = A/B compare: รัน FF-only แล้วตามด้วย FF+PD, สรุปเทียบกัน หุ่นยนต์ทำงานเอง 2 ครั้งตามลำดับ หลังวิ่งครั้งแรกมีเวลาตั้งหุ่นยนต์ให้ตรงภายใน 3 วินาที
//    d = dump CSV ของ run ล่าสุด (error เทียบเวลา → เอาไป plot)
//    p / o = เพิ่ม / ลด FWD_KP ทีละ 0.5   (จูน gain สด ๆ)
//    k / m = เพิ่ม / ลด FWD_KD ทีละ 0.02
//    s = แสดง gain ปัจจุบัน + สถานะ
//    x = abort (ระหว่าง countdown)
//    ? = ช่วยเหลือ
//  ปุ่ม START บนบอร์ด = รัน move แบบ FF + PD (เหมือน 'f')
//
//  ⚠️ หุ่นจะวิ่งจริงบนพื้น — วางบนพื้นโล่ง ควรมีระยะทางวิ่ง ≥ 500mm ข้างหน้า
// ═══════════════════════════════════════════════════════════════════════════

#include <stdarg.h>
#include "config.h"
#include "profile.h"
#include "hal_battery.h"
#include "hal_motor.h"
#include "hal_encoder.h"
#include "hal_imu.h"
#include "hal_led.h"
#include "motors_controller.h"
#include "control_task.h"
#include "ble_debug.h"

// ─────────────────────────────────────────────────────────────────────────────
// Move under test — same trapezoid for both FF-only and FF+PD (fair comparison).
// Matches the A7 known-good profile (200mm @ 200mm/s, 1000mm/s²).
// ─────────────────────────────────────────────────────────────────────────────
#define MOVE_DIST_MM    200.0f
#define MOVE_TOP_MMPS   200.0f
#define MOVE_FINAL_MMPS   0.0f
#define MOVE_ACCEL      1000.0f

#define BLE_NAME        "MM_Lab6_PID"

// ระยะวิ่งทดสอบ (ปรับสดได้ผ่านคำสั่ง "dist <mm>" — ไม่ต้อง recompile)
float g_move_dist = MOVE_DIST_MM;

// พารามิเตอร์การหมุน (ทดสอบ ROT PID)
#define TURN_TOP_DPS      150.0f
#define TURN_ALPHA        720.0f
#define SPIN_KICK_PWM     1000      // kick ฆ่า stiction ตอนเริ่มหมุน
#define SPIN_KICK_MAX_MS  200
#define SPIN_KICK_REL_DPS 25.0f     // ปล่อย kick เมื่อ gyro ยืนยันว่าหมุนแล้ว

// ─────────────────────────────────────────────────────────────────────────────
// out() — unified output: mirror every line to USB Serial AND BLE.
// Include '\n' in the format string where you want a line break.
// ─────────────────────────────────────────────────────────────────────────────
char _outbuf[200];
void out(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_outbuf, sizeof(_outbuf), fmt, ap);
    va_end(ap);
    Serial.print(_outbuf);
    ble_debug_send(_outbuf);
}

// ─────────────────────────────────────────────────────────────────────────────
// RAM log — robot runs untethered (no chatty I/O during motion), dump after.
//   500 samples × 4ms = 2.0 s coverage.
// ─────────────────────────────────────────────────────────────────────────────
#define LOG_SIZE 500
struct Sample {
    uint16_t time_ms;
    int16_t  setpoint_x10;   // profile position (mm × 10)
    int16_t  actual_x10;     // encoder distance (mm × 10)
    int16_t  error_x10;      // (setpoint − actual) (mm × 10)
    int16_t  heading_x10;    // gyro heading (deg × 10) — shows veer
    int16_t  L_volts_x100;   // V × 100
    int16_t  R_volts_x100;   // V × 100
};
Sample   logBuf[LOG_SIZE];
int      logCount   = 0;
bool     logValid   = false;
bool     logWasPID  = true;          // mode of the run currently in the buffer

// Final result snapshots (captured at END, before user handles robot)
float    lastFinalError = 0;
float    lastPeakError  = 0;
float    lastFinalHdg   = 0;

// A/B compare results
bool     cmpValid     = false;
float    cmpErrFF     = 0;   // FF-only final error
float    cmpErrPID    = 0;   // FF+PD final error
float    cmpHdgFF     = 0;
float    cmpHdgPID    = 0;

// ─────────────────────────────────────────────────────────────────────────────
// printMenu
// ─────────────────────────────────────────────────────────────────────────────
void printMenu() {
    out("\n");
    out("------------------ test_06_pid - STEP 6 PID ------------------\n");
    out("  f = run FF + PD  (closed-loop, normal)\n");
    out("  g = run FF-only  (PID OFF - watch error build up)\n");
    out("  c = A/B compare  (FF-only then FF+PD, side-by-side)\n");
    out("  d = dump CSV of last run\n");
    out("  p/o = FWD_KP +/- 0.5    k/m = FWD_KD +/- 0.02    i/j = FWD_KI +/- 0.10\n");
    out("  s = show gains + status     x = abort during countdown\n");
    out("  dist <mm> = ตั้งระยะวิ่งทดสอบ (ปัจจุบัน %.0f)\n", g_move_dist);
    out("  turn <deg> = ทดสอบหมุน (turn 90 / turn -90 / turn 180)\n");
    out("  rkp/rkd/rki <v> = จูน ROT PID gain\n");
    out("  gaincalc <Km> <Tm> [zeta] [TD] = คำนวณ FWD_KP/KD จากค่าระบบ (Lab5 FF)\n");
    out("  ? = help     (START button = run FF + PD)\n");
    out("  move: %.0fmm @ %.0fmm/s, accel %.0fmm/s2\n",
        MOVE_DIST_MM, MOVE_TOP_MMPS, MOVE_ACCEL);
    out("  BLE: \"%s\"  (connected: %s)\n",
        BLE_NAME, ble_debug_is_connected() ? "yes" : "no");
    out("--------------------------------------------------------------\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// printGains
// ─────────────────────────────────────────────────────────────────────────────
void printGains() {
    out("\n");
    out("  PID: %s\n", motors_is_pid_enabled() ? "ENABLED" : "DISABLED");
    out("  FWD  Kp=%.2f  Ki=%.3f  Kd=%.3f\n",
        motors_get_fwd_kp(), motors_get_fwd_ki(), motors_get_fwd_kd());
    out("  ROT  Kp=%.2f  Ki=%.3f  Kd=%.3f\n",
        motors_get_rot_kp(), motors_get_rot_ki(), motors_get_rot_kd());
    out("  Battery: %.3f V   BLE: %s\n",
        hal_battery_volts(), ble_debug_is_connected() ? "connected" : "advertising");
}

// ─────────────────────────────────────────────────────────────────────────────
// abortRequested — non-blocking 'x' check on BOTH channels (USB + BLE).
// Used during the countdown so a wireless tester can abort too.
// ─────────────────────────────────────────────────────────────────────────────
bool abortRequested() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == 'x' || c == 'X') return true;
    }
    char line[16];
    if (ble_debug_take_line(line, sizeof(line))) {
        if (line[0] == 'x' || line[0] == 'X') return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// preflight — battery gate + 2s countdown with 'x' abort.  Returns false=abort.
// ─────────────────────────────────────────────────────────────────────────────
bool preflight(const char* label) {
    out("\n");
    out("-----------------------------------------------------\n");
    out("  RUN: %s\n", label);
    out("-----------------------------------------------------\n");
    out("Battery: %.3f V\n", hal_battery_volts());

    if (hal_battery_volts() < 7.0f) {
        out("[!] Battery below 7.0V - charge first. Aborted.\n");
        hal_led_set(LED_ERROR); delay(800); hal_led_set(LED_READY_FWD);
        return false;
    }
    if (hal_battery_is_critical()) {
        out("[!] Battery CRITICAL - charge first. Aborted.\n");
        hal_led_set(LED_ERROR); delay(800); hal_led_set(LED_READY_FWD);
        return false;
    }

    out("Place robot on floor, >=500mm runway ahead.\n");
    out("Starting in 2 sec - 'x' to abort (USB or BLE)\n");

    delay(100);
    while (Serial.available()) Serial.read();
    ble_debug_flush_rx();

    hal_led_set(LED_COUNTDOWN);
    for (int i = 2; i > 0; i--) {
        out("  %d...\n", i);
        uint32_t t = millis();
        while (millis() - t < 1000) {
            if (abortRequested()) {
                out("Aborted by user.\n");
                hal_led_set(LED_ERROR); delay(800); hal_led_set(LED_READY_FWD);
                return false;
            }
            delay(10);
        }
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// runMove — execute one forward move with PID on/off, log to RAM, print summary.
//   Returns final position error (mm).  Sets lastFinalError/lastPeakError/etc.
// ─────────────────────────────────────────────────────────────────────────────
float runMove(bool pid_on, const char* label, bool do_preflight = true) {
    if (do_preflight && !preflight(label)) return NAN;

    motors_set_pid_enabled(pid_on);   // ★ the ONLY difference between the two runs

    hal_led_set(LED_RUNNING);
    out("  running... (PID %s)\n", pid_on ? "ON" : "OFF");
    control_arm();                    // resets errors + heading, enables PWM
    delay(50);

    uint32_t start = millis();
    control_start_forward(g_move_dist, MOVE_TOP_MMPS, MOVE_FINAL_MMPS, MOVE_ACCEL);

    logCount  = 0;
    logValid  = false;
    logWasPID = pid_on;
    float peak_err = 0;

    uint32_t last_sample = 0;
    uint32_t last_telem  = 0;
    while (millis() - start < 4000) {
        if (millis() - last_sample >= 4) {
            last_sample = millis();

            float err = control_get_fwd_error();
            if (fabsf(err) > fabsf(peak_err)) peak_err = err;

            if (logCount < LOG_SIZE) {
                Sample& s = logBuf[logCount++];
                s.time_ms      = (uint16_t)(millis() - start);
                s.setpoint_x10 = (int16_t)(control_get_fwd_setpoint() * 10);
                s.actual_x10   = (int16_t)(control_get_fwd_actual()   * 10);
                s.error_x10    = (int16_t)(err * 10);
                s.heading_x10  = (int16_t)(control_get_heading()      * 10);
                s.L_volts_x100 = (int16_t)(motors_get_left_volts()  * 100);
                s.R_volts_x100 = (int16_t)(motors_get_right_volts() * 100);
            }
        }
        // Stream live telemetry ~20Hz so the UI strip updates while moving.
        if (millis() - last_telem >= 50) {
            last_telem = millis();
            out("T,%.1f,%.1f,%.1f,0,0,%.2f,1\n",
                control_get_fwd_actual(), control_get_heading(),
                control_get_fwd_error(), hal_battery_volts());
        }
        // After motion done, log ~0.7s of settling, then stop.
        if (control_is_motion_done() && (millis() - start > 1900)) break;
        delay(1);
    }

    control_disarm();
    delay(100);

    // Snapshot results NOW (live getters drift if robot is handled)
    lastFinalError = control_get_fwd_error();
    lastPeakError  = peak_err;
    lastFinalHdg   = control_get_heading();
    logValid       = true;

    // Re-enable PID so the system is left in the safe/normal state
    motors_set_pid_enabled(true);

    out("\n");
    out("--- %s - done ---\n", label);
    out("  Final position:  %.2f mm   (target %.0f)\n",
        control_get_fwd_actual(), g_move_dist);
    out("  Final error:     %+.2f mm   %s\n", lastFinalError,
        (fabsf(lastFinalError) < 3.0f) ? "(tight)" :
        (fabsf(lastFinalError) < 10.0f) ? "(loose)" : "(WAY off)");
    out("  Peak |error|:    %.2f mm\n", fabsf(lastPeakError));
    out("  Final heading:   %+.2f deg  (0 = perfectly straight)\n", lastFinalHdg);
    out("  Samples logged:  %d\n", logCount);
    out("  Send 'd' to dump CSV (error vs time).\n");

    hal_led_set(LED_DONE);
    return lastFinalError;
}

// ─────────────────────────────────────────────────────────────────────────────
// runCompare — A/B: FF-only first, then FF+PD, print the contrast.
// ─────────────────────────────────────────────────────────────────────────────
void runCompare() {
    out("\n");
    out("============== A/B COMPARE: FF-only  vs  FF+PD ==============\n");
    out("Same profile, same feedforward. Only the position PD differs.\n");
    out("Reset the robot to the SAME start spot before each run.\n");

    // ── Run 1: FF-only ────────────────────────────────────────────────────────
    float e_ff = runMove(false, "A) FF-only (PID OFF)");
    if (isnan(e_ff)) { out("Compare aborted.\n"); return; }
    cmpErrFF = lastFinalError;  cmpHdgFF = lastFinalHdg;

    out("\n>>> Reposition robot to the start line. Run 2 in 4 sec...\n");
    delay(4000);

    // ── Run 2: FF + PD ────────────────────────────────────────────────────────
    float e_pid = runMove(true, "B) FF + PD (PID ON)");
    if (isnan(e_pid)) { out("Compare aborted after run 1.\n"); return; }
    cmpErrPID = lastFinalError;  cmpHdgPID = lastFinalHdg;
    cmpValid = true;

    // ── Verdict ───────────────────────────────────────────────────────────────
    out("\n");
    out("======================== RESULT ========================\n");
    out("                    FF-only      FF+PD\n");
    out("  final error :    %+7.2f mm   %+7.2f mm\n", cmpErrFF, cmpErrPID);
    out("  final heading:   %+7.2f deg  %+7.2f deg\n", cmpHdgFF, cmpHdgPID);
    out("--------------------------------------------------------\n");
    float improve = fabsf(cmpErrFF) - fabsf(cmpErrPID);
    out("  PD reduced distance error by %.2f mm (%.0f%% of FF-only error)\n",
        improve,
        (fabsf(cmpErrFF) > 0.01f) ? 100.0f * improve / fabsf(cmpErrFF) : 0.0f);
    out("  Expect: FF+PD much closer to 0 on BOTH error and heading.\n");
    out("  Lesson: feedforward gets you ~there; PD cleans up the residual.\n");
    out("\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// dumpLog — print CSV of the last run for plotting (error vs time).
//   Sent over both USB and BLE (one NOTIFY per line).
// ─────────────────────────────────────────────────────────────────────────────
void dumpLog() {
    if (!logValid) {
        out("No run in RAM. Press 'f' / 'g' / START first.\n");
        return;
    }
    out("\n");
    out("# Last run: %s   final_err=%+.2fmm  peak|err|=%.2fmm  final_hdg=%+.2fdeg\n",
        logWasPID ? "FF+PD" : "FF-only",
        lastFinalError, fabsf(lastPeakError), lastFinalHdg);
    out("t_ms,setpoint_mm,actual_mm,error_mm,heading_deg,L_volts,R_volts\n");
    for (int i = 0; i < logCount; i++) {
        Sample& s = logBuf[i];
        out("%u,%.1f,%.1f,%.1f,%.1f,%.2f,%.2f\n",
            s.time_ms,
            s.setpoint_x10 * 0.1f,
            s.actual_x10   * 0.1f,
            s.error_x10    * 0.1f,
            s.heading_x10  * 0.1f,
            s.L_volts_x100 * 0.01f,
            s.R_volts_x100 * 0.01f);
        delay(8);   // pace BLE notifies so the link doesn't drop lines
    }
    out("# end\n");
    out("\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// Gain tuning helpers
// ─────────────────────────────────────────────────────────────────────────────
void bumpFwdKp(float d) {
    float kp = motors_get_fwd_kp() + d;
    if (kp < 0) kp = 0;
    motors_set_fwd_gains(kp, motors_get_fwd_kd());
    out("  FWD_KP -> %.2f\n", motors_get_fwd_kp());
}
void bumpFwdKd(float d) {
    float kd = motors_get_fwd_kd() + d;
    if (kd < 0) kd = 0;
    motors_set_fwd_gains(motors_get_fwd_kp(), kd);
    out("  FWD_KD -> %.3f\n", motors_get_fwd_kd());
}
void bumpFwdKi(float d) {
    float ki = motors_get_fwd_ki() + d;
    if (ki < 0) ki = 0;
    motors_set_fwd_ki(ki);               // resets the integral accumulator
    out("  FWD_KI -> %.3f  (0 = off; raise to kill steady-state residual)\n",
        motors_get_fwd_ki());
}

// ─────────────────────────────────────────────────────────────────────────────
// runTurn — หมุนอยู่กับที่ angle องศา (ทดสอบ ROT PID) + รายงาน error
//   FF + rotation PD ขับการหมุน, kick ฆ่า stiction ตอนเริ่ม
// ─────────────────────────────────────────────────────────────────────────────
void runTurn(float angle, const char* label) {
    if (!preflight(label)) return;
    motors_set_pid_enabled(true);            // ต้องเปิด PID ให้ rot PD ทำงาน
    hal_led_set(LED_RUNNING);
    out("  turning %.0f deg...\n", angle);
    control_arm();
    delay(50);
    float h0 = control_get_heading();
    control_start_rotation(angle, TURN_TOP_DPS, 0.0f, TURN_ALPHA);
    control_kick_rotation(angle > 0 ? +SPIN_KICK_PWM : -SPIN_KICK_PWM,
                          SPIN_KICK_MAX_MS, SPIN_KICK_REL_DPS);
    uint32_t start = millis(), last_t = 0;
    while (millis() - start < 4000) {
        if (millis() - last_t >= 50) {       // live telemetry (heading)
            last_t = millis();
            out("T,0,%.1f,0,0,0,%.2f,1\n", control_get_heading(), hal_battery_volts());
        }
        if (control_is_motion_done() && (millis() - start > 300)) break;
        if (abortRequested()) break;
        delay(2);
    }
    control_disarm();
    delay(150);
    float act = control_get_heading() - h0;
    out("\n--- %s ---\n", label);
    out("  commanded: %+.1f deg\n", angle);
    out("  actual:    %+.1f deg   (err %+.1f)\n", act, act - angle);
    out("  %s\n", fabsf(act - angle) < 5.0f ? "(good)" :
                  fabsf(act - angle) < 15.0f ? "(loose — ปรับ ROT_KD)" :
                                               "(off — ปรับ ROT_KD / kick / tdps)");
    hal_led_set(LED_DONE);
}

// ─────────────────────────────────────────────────────────────────────────────
// handleLine — parse "key value" commands (e.g. "dist 400", "turn 90"); else single char.
// ─────────────────────────────────────────────────────────────────────────────
void handleCommand(char c);
void handleLine(char* s) {
    char* sp = strchr(s, ' ');
    if (!sp) { if (s[0]) handleCommand(s[0]); return; }   // no space → single char
    *sp = 0;
    float v = atof(sp + 1);
    if (!strcmp(s, "dist")) {
        if (v >= 20.0f && v <= 2000.0f) { g_move_dist = v; out("dist -> %.0f mm\n", g_move_dist); }
        else out("dist range 20-2000 mm\n");
    } else if (!strcmp(s, "turn")) {
        runTurn(v, "TURN test");
    } else if (!strcmp(s, "rkp")) {
        motors_set_rot_gains(v, motors_get_rot_kd()); out("ROT_KP -> %.2f\n", v);
    } else if (!strcmp(s, "rkd")) {
        motors_set_rot_gains(motors_get_rot_kp(), v); out("ROT_KD -> %.3f\n", v);
    } else if (!strcmp(s, "rki")) {
        motors_set_rot_ki(v); out("ROT_KI -> %.3f\n", v);
    } else if (!strcmp(s, "gaincalc")) {
        // คำนวณ FWD_KP/KD จากค่าระบบ (UKMARS) — ไม่ต้องลองผิดลองถูก
        //   Kp = (Tm/Km)·16/(zeta^2·TD^2)   ;   Kd = (8·Tm − TD)/(TD·Km)
        float km = 0, tm = 0, zeta = 1.0f, td = 0.12f;
        int got = sscanf(sp + 1, "%f %f %f %f", &km, &tm, &zeta, &td);
        if (got < 2 || km <= 0.0f || tm <= 0.0f) {
            out("use: gaincalc <Km mm/s/V> <Tm s> [zeta=1.0] [TD s=0.12]\n");
        } else {
            if (td >= 8.0f * tm)
                out("[!] TD ควร < 8*Tm (=%.3f) ไม่งั้น Kd ติดลบ\n", 8.0f * tm);
            float kp = (tm / km) * 16.0f / (zeta * zeta * td * td);
            float kd = (8.0f * tm - td) / (td * km);
            motors_set_fwd_gains(kp, kd);
            out("gaincalc: Km=%.0f Tm=%.3f zeta=%.2f TD=%.3f\n", km, tm, zeta, td);
            out("  -> FWD_KP=%.3f  FWD_KD=%.4f  (set + applied)\n", kp, kd);
            out("  config:  #define FWD_KP %.3ff   #define FWD_KD %.4ff\n", kp, kd);
        }
    } else {
        out("? unknown '%s'\n", s);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// handleCommand — single-char dispatch shared by USB serial AND BLE.
// ─────────────────────────────────────────────────────────────────────────────
void handleCommand(char c) {
    switch (c) {
        case 'f': runMove(true,  "FF + PD (PID ON)");  break;
        case 'g': runMove(false, "FF-only (PID OFF)"); break;
        case 'c': runCompare(); break;
        case 'd': dumpLog();    break;
        case 'p': bumpFwdKp(+0.5f);  break;
        case 'o': bumpFwdKp(-0.5f);  break;
        case 'k': bumpFwdKd(+0.02f); break;
        case 'm': bumpFwdKd(-0.02f); break;
        case 'i': bumpFwdKi(+0.10f); break;   // integral gain up (kills residual)
        case 'j': bumpFwdKi(-0.10f); break;   // integral gain down
        case 's': printGains(); break;
        case '?':
        case 'h': printMenu();  break;
        case '\n':
        case '\r':
        case ' ':  break;       // ignore whitespace
        default:
            out("Unknown '%c' - '?' for help\n", c);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// setup
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(800);
    Serial.println();
    Serial.println(F("════════════════════════════════════════════════════"));
    Serial.println(F("  test_06_pid — STEP 6: PID controller (lesson)"));
    Serial.println(F("════════════════════════════════════════════════════"));
    Serial.printf("LOOP_FREQUENCY = %.0f Hz   LOOP_INTERVAL = %.4f s\n",
                  (float)LOOP_FREQUENCY, (float)LOOP_INTERVAL);

    hal_battery_init();
    Serial.printf("Battery: %.3f V\n", hal_battery_volts());

    if (!motors_init())  Serial.println(F("[!] motors_init() failed"));
    if (!encoder_init()) Serial.println(F("[!] encoder_init() failed"));
    motors_disable_controllers();   // explicit safety

    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_BUTTON_MODE,  INPUT_PULLUP);

    hal_led_init();
    hal_led_set(LED_BOOT);

    if (!imu_init_default()) {
        Serial.println(F("[!] imu_init() failed — gyro heading unavailable"));
    } else {
        Serial.println(F("IMU init OK — calibrating gyro (keep robot stationary 1s)..."));
        imu_calibrate_gyro(500);
    }

    control_task_start();
    delay(50);
    Serial.println(F("Control task @ 500Hz started (motors DISARMED until a run)"));

    // Wireless console — robot must move, so a tether would drag/skew the test.
    ble_debug_init(BLE_NAME);
    Serial.printf("BLE advertising as \"%s\" (Nordic UART). Connect to test wirelessly.\n",
                  BLE_NAME);

    hal_led_set(LED_READY_FWD);   // green = ready
    printGains();
    printMenu();
}

// ─────────────────────────────────────────────────────────────────────────────
// loop — command dispatch from USB serial + BLE, plus START button.
// ─────────────────────────────────────────────────────────────────────────────
bool prevStart  = true;     // INPUT_PULLUP: released = HIGH
bool prevBleConn = false;

void loop() {
    // Greet the menu once when a BLE central connects.
    bool bleConn = ble_debug_is_connected();
    if (bleConn && !prevBleConn) {
        delay(200);
        out("\n[BLE connected] test_06_pid ready.\n");
        printMenu();
    }
    prevBleConn = bleConn;

    // START button (active-low) → run FF + PD
    bool start = digitalRead(PIN_BUTTON_START);
    if (prevStart && !start) {     // falling edge
        delay(20);                 // debounce
        if (!digitalRead(PIN_BUTTON_START)) {
            out("[START button] -> FF + PD\n");
            runMove(true, "FF + PD (button)");
        }
    }
    prevStart = start;

    // USB serial — single char commands
    if (Serial.available()) {
        handleCommand((char)Serial.read());
    }

    // BLE — a full line arrived; parse "key value" (e.g. "dist 400", "gaincalc ...") or single char
    char line[48];
    if (ble_debug_take_line(line, sizeof(line))) {
        if (line[0]) handleLine(line);
    }

    delay(5);
}
