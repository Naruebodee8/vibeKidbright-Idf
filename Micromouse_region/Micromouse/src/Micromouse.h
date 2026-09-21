// ═══════════════════════════════════════════════════════════════════════════
//  Micromouse.h — L2 facade (the ONE class students call)
//
//  This header bundles layers L0 (HAL) + L1 (config) + L2 (motion/sensing) of
//  the EE-BME Micromouse teaching platform behind a single, simple class.
//  Students write only their .ino (layer L3 = maze logic) and call:
//
//      Micromouse mouse;
//      mouse.begin();
//      mouse.waitForStart();
//      mouse.moveCell();  mouse.turn90(LEFT);  mouse.wallFront();  ...
//
//  Everything below this API — the 500 Hz FF+PD control loop, trapezoidal
//  profiles, gyro spins, wall-centering steering, front-wall correction, BLE —
//  is hidden in the proven modules this header includes. Students can't break
//  it, but the source stays open for the curious.
//
//  ── Design contract (matches SDD §3) ────────────────────────────────────────
//    * Motion primitives are BLOCKING and return bool: false = aborted (a 'x'
//      came in over USB/BLE) or the move failed (e.g. a spin stalled).
//      → L3 must only update its (x,y,heading) pose when the call returns true.
//    * Sensing is binary (wall present?) + analog (mm). NOTHING above this line
//      ever sees raw ADC/mV — so swapping Sharp IR for pulsed-IR later is purely
//      an L0 change, this API does not move.
//
//  ── Comms line protocol (for the Python BLE UI) ─────────────────────────────
//    robot → PC (text lines, also mirrored to USB Serial):
//        T,<fwd_mm>,<heading_deg>,<cte_mm>,<left_mm>,<right_mm>,<vbat_V>,<armed>
//        P,<x>,<y>,<heading>            pose (sent by L3 via reportPose())
//        W,<x>,<y>,<mask>               wall bitmask (L3 via reportWall())
//        anything else                  free-text log
//    PC → robot (NUS writes / USB chars):
//        x  abort      g  start (= press START)     t  toggle telemetry stream
//        "kp <v>" / "kd <v>" / "nom <v>"  tune steering (live, no recompile)
//        "fw 0|1"  enable/disable front-wall correction
// ═══════════════════════════════════════════════════════════════════════════
#ifndef MICROMOUSE_H
#define MICROMOUSE_H

#include <Arduino.h>
#include <stdarg.h>

// ── Bundled platform layers (L0–L2) ─────────────────────────────────────────
#include "config.h"
#include "profile.h"
#include "hal_battery.h"
#include "hal_motor.h"
#include "hal_encoder.h"
#include "hal_imu.h"
#include "hal_led.h"
#include "hal_wall_sensor.h"
#include "motors_controller.h"
#include "control_task.h"
#include "steering.h"
#include "front_wall.h"
#include "ble_debug.h"

// ── Public enums (SDD §3) ────────────────────────────────────────────────────
enum Dir     { LEFT, RIGHT };
enum RunMode { EXPLORE, SPEEDRUN };

// ── Motion tuning (Phase A: compile-time; Phase B moves these to NVS/L1) ─────
#ifndef MM_MOVE_V
  #define MM_MOVE_V            200.0f   // forward cruise (mm/s)
  #define MM_MOVE_A            1000.0f  // forward accel  (mm/s^2)
  #define MM_TURN_TOP_DPS      150.0f   // spin speed (deg/s)
  #define MM_TURN_ALPHA        720.0f   // spin accel (deg/s^2)
  #define MM_SPIN_KICK_PWM     1000     // stiction-break kick PWM
  #define MM_SPIN_KICK_REL_DPS 25.0f    // release kick once spinning this fast
  #define MM_SPIN_KICK_MAX_MS  200      // kick safety timeout (ms)
#endif
#ifndef MM_BLE_NAME
  #define MM_BLE_NAME          "MM_robot"
#endif

// ═══════════════════════════════════════════════════════════════════════════
class Micromouse {
public:
    // ── Lifecycle ────────────────────────────────────────────────────────────
    void begin() {
        Serial.begin(115200);
        delay(300);
        _emit("\n=== Micromouse (EE-BME teaching platform) ===\n");

        hal_battery_init();
        if (!motors_init())  _emit("[!] motors_init failed\n");
        if (!encoder_init()) _emit("[!] encoder_init failed\n");
        motors_disable_controllers();                 // safe: no motion at boot

        pinMode(PIN_BUTTON_START, INPUT_PULLUP);
        pinMode(PIN_BUTTON_MODE,  INPUT_PULLUP);

        hal_led_init();
        hal_led_set(LED_BOOT);

        if (!imu_init_default()) _emit("[!] imu_init failed\n");
        else { _emit("calibrating gyro (hold still)...\n"); imu_calibrate_gyro(500); }

        if (!wall_sensor_is_initialized()) wall_sensor_init();

        control_task_start();                          // 500 Hz FF+PD loop, disarmed
        delay(50);

        ble_debug_init(MM_BLE_NAME);
        _emit("BLE \"" MM_BLE_NAME "\" advertising. Ready.\n");
        hal_led_set(LED_READY_SMOOTH);                 // purple = ready
    }

    // Block until START button OR a 'g' command. Streams telemetry + accepts
    // tuning while waiting. On release: zero pose reference and arm the motors.
    void waitForStart() {
        _disarmIdle();
        hal_led_set(LED_READY_SMOOTH);
        _abort = false; _startReq = false;
        _emit("waitForStart — press START or send 'g'\n");
        while (!_startReq) {
            _poll();
            if (_buttonPressed(PIN_BUTTON_START)) _startReq = true;
            _streamTelemetry();
            delay(20);
        }
        _emit("START\n");
        encoder_reset(-1);
        control_arm();                 // resets errors + heading to 0
        _targetHeading = 0.0f;         // arm-time heading = absolute maze reference
        _armed = true;
        delay(50);
        hal_led_set(LED_RUNNING);      // red = moving
    }

    RunMode mode() {
        return (digitalRead(PIN_BUTTON_MODE) == LOW) ? SPEEDRUN : EXPLORE;
    }

    // ── Motion primitives (blocking, return false on abort/stall) ─────────────
    bool moveCell(uint8_t n = 1) {
        if (_abort) return false;
        if (!_armed) _armNow();
        _forward(n);
        // Re-square + re-zero distance against a front wall (kills grid drift).
        if (!_abort && _useFrontWall && fw_wall_present()) {
            _emit("front wall -> correct\n");
            fw_correct();
        }
        return !_abort;
    }

    bool turn90(Dir d) {
        if (_abort) return false;
        if (!_armed) _armNow();
        return _spin(d == LEFT ? 0 : 1);
    }

    bool turnBack() {
        if (_abort) return false;
        if (!_armed) _armNow();
        return _spin(2);
    }

    // ── Sensing ───────────────────────────────────────────────────────────────
    bool wallFront() {
        wall_sensor_read(WALL_SENSOR_FRONT_LEFT);
        wall_sensor_read(WALL_SENSOR_FRONT_RIGHT);
        return wall_sensor_wall_front();
    }
    bool wallLeft()  { wall_sensor_read(WALL_SENSOR_LEFT);  return wall_sensor_wall_left();  }
    bool wallRight() { wall_sensor_read(WALL_SENSOR_RIGHT); return wall_sensor_wall_right(); }

    // Processed distances in mm (255 = out of range). Order: front-L, front-R, left, right.
    void wallDistances(int& fl, int& fr, int& l, int& r) {
        wall_sensor_read_all();
        fl = wall_sensor_get_distance(WALL_SENSOR_FRONT_LEFT);
        fr = wall_sensor_get_distance(WALL_SENSOR_FRONT_RIGHT);
        l  = wall_sensor_get_distance(WALL_SENSOR_LEFT);
        r  = wall_sensor_get_distance(WALL_SENSOR_RIGHT);
    }

    // ── Feedback ──────────────────────────────────────────────────────────────
    void led(uint8_t r, uint8_t g, uint8_t b) { neopixelWrite(PIN_RGB_LED, r, g, b); }
    void beep(uint16_t ms) {                       // no buzzer on this rig → LED blink
        neopixelWrite(PIN_RGB_LED, 80, 80, 80);
        delay(ms > 200 ? 200 : ms);
        hal_led_set(_armed ? LED_RUNNING : LED_READY_SMOOTH);
    }

    // ── Comms helpers for L3 (so the UI can draw pose / maze) ─────────────────
    void poll()        { _poll(); _streamTelemetry(); }   // call between moves/while idle
    void reportPose(int x, int y, int heading) { _emitf("P,%d,%d,%d\n", x, y, heading); }
    void reportWall(int x, int y, int mask)    { _emitf("W,%d,%d,%d\n", x, y, mask); }
    void log(const char* s)                    { _emit(s); _emit("\n"); }
    void logf(const char* fmt, ...) {                       // printf-style log (USB+BLE)
        char buf[160];
        va_list ap; va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        _emit(buf);
    }
    bool aborted()     { return _abort; }
    void clearAbort()  { _abort = false; }

private:
    float         _targetHeading = 0.0f;   // expected heading in maze frame (deg)
    bool          _armed         = false;
    volatile bool _abort         = false;
    bool          _startReq      = false;
    bool          _telem         = true;   // stream telemetry by default (UI wants it)
    bool          _useFrontWall  = true;
    bool          _settleTurns   = true;   // null residual heading after each spin

    // Spin tuning (BLE-tunable — turns over-rotate erratically; let the bench
    // dial these in live). kickPwm=0 disables the stiction-break kick entirely.
    int16_t       _kickPwm       = MM_SPIN_KICK_PWM;
    int16_t       _kickMs        = MM_SPIN_KICK_MAX_MS;
    float         _kickRel       = MM_SPIN_KICK_REL_DPS;  // higher = kick holds full PWM longer (beats stall)
    float         _turnDps       = MM_TURN_TOP_DPS;
    uint32_t      _lastTelem     = 0;
    uint32_t      _lastBtn       = 0;

    // ── output: mirror to USB + BLE ───────────────────────────────────────────
    void _emit(const char* s) { Serial.print(s); ble_debug_send(s); }
    void _emitf(const char* fmt, ...) {
        char buf[160];
        va_list ap; va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        Serial.print(buf);
        ble_debug_send(buf);
    }

    // ── arm without re-zeroing heading (used if a move runs before waitForStart)
    void _armNow() {
        encoder_reset(-1);
        control_arm();
        _targetHeading = 0.0f;
        _armed = true;
    }
    void _disarmIdle() {
        steering_enable(false);
        control_set_steering(0.0f);
        control_disarm();
        _armed = false;
    }

    bool _buttonPressed(int pin) {
        if (digitalRead(pin) != LOW) return false;
        if (millis() - _lastBtn < 250) return false;   // debounce
        _lastBtn = millis();
        delay(20);
        return digitalRead(pin) == LOW;
    }

    // ── comms input ───────────────────────────────────────────────────────────
    void _handleChar(char c) {
        if (c == 'x' || c == 'X') { _abort = true; _emit("** ABORT **\n"); }
        else if (c == 'g' || c == 'G') _startReq = true;
        else if (c == 't' || c == 'T') { _telem = !_telem; _emitf("telem %s\n", _telem ? "ON" : "OFF"); }
    }
    void _handleLine(char* ln) {
        char* sp = strchr(ln, ' ');
        if (!sp) { if (ln[0]) _handleChar(ln[0]); return; }
        *sp = 0;
        float v = atof(sp + 1);
        if      (!strcmp(ln, "kp"))  steering_set_gains(v, steering_get_kd());
        else if (!strcmp(ln, "kd"))  steering_set_gains(steering_get_kp(), v);
        else if (!strcmp(ln, "nom")) steering_set_nominal(v);
        else if (!strcmp(ln, "fw"))  _useFrontWall = (v != 0.0f);
        else if (!strcmp(ln, "settle")) _settleTurns = (v != 0.0f);
        else if (!strcmp(ln, "kick"))   _kickPwm = (int16_t)v;   // 0 = disable kick
        else if (!strcmp(ln, "kickms")) _kickMs  = (int16_t)v;
        else if (!strcmp(ln, "kickrel")) _kickRel = v;
        else if (!strcmp(ln, "tdps"))   _turnDps = v;
        else { _emitf("? unknown '%s'\n", ln); return; }
        _emitf("kp=%.2f kd=%.2f nom=%.1f fw=%d settle=%d kick=%d kickms=%d kickrel=%.0f tdps=%.0f\n",
               steering_get_kp(), steering_get_kd(), steering_get_nominal(),
               _useFrontWall ? 1 : 0, _settleTurns ? 1 : 0,
               _kickPwm, _kickMs, _kickRel, _turnDps);
    }
    void _poll() {
        while (Serial.available()) _handleChar((char)Serial.read());
        char ln[48];
        if (ble_debug_take_line(ln, sizeof(ln))) _handleLine(ln);
    }

    void _streamTelemetry(bool force = false) {
        if (!_telem && !force) return;
        uint32_t now = millis();
        if (!force && now - _lastTelem < 50) return;   // ~20 Hz
        _lastTelem = now;
        wall_sensor_read(WALL_SENSOR_LEFT);
        wall_sensor_read(WALL_SENSOR_RIGHT);
        _emitf("T,%.1f,%.1f,%.1f,%d,%d,%.2f,%d\n",
               control_get_fwd_actual(), control_get_heading(), steering_get_cte(),
               wall_sensor_get_distance(WALL_SENSOR_LEFT),
               wall_sensor_get_distance(WALL_SENSOR_RIGHT),
               hal_battery_volts(), _armed ? 1 : 0);
    }

    // ── forward `cells` cells, wall-centering + 500 Hz gyro heading-hold ──────
    void _forward(int cells) {
        steering_reset();
        steering_enable(true);
        control_set_heading_hold(true, _targetHeading);
        control_start_forward(cells * CELL_SIZE_MM, MM_MOVE_V, 0.0f, MM_MOVE_A);

        uint32_t last = millis(), t0 = last;
        uint16_t n = 0;
        while (true) {
            uint32_t now = millis();
            if (now - last >= 10) {
                last = now;
                wall_sensor_read(WALL_SENSOR_LEFT);
                wall_sensor_read(WALL_SENSOR_RIGHT);
                steering_update(wall_sensor_get_distance(WALL_SENSOR_LEFT),
                                wall_sensor_get_distance(WALL_SENSOR_RIGHT), 0.010f);
                control_set_steering(steering_get_adj());
                if ((n++ & 3) == 0) hal_battery_update();
            }
            _streamTelemetry();
            if (control_is_motion_done() && (millis() - t0 > 150)) break;
            _poll();
            if (_abort) break;
            delay(1);
        }
        control_set_heading_hold(false, 0.0f);
        steering_enable(false);
        control_set_steering(0.0f);
        // Per-move diagnostic: target vs actual heading after a forward. A small
        // err here means the hold is working; a steadily GROWING err move-to-move
        // points at gyro-bias drift (re-zero against a wall more often).
        _emitf("D,F,tgt=%.1f,act=%.1f,err=%+.1f,fwd=%.1f\n",
               _targetHeading, control_get_heading(),
               control_get_heading() - _targetHeading, control_get_fwd_actual());
    }

    // ── in-place spin: 0 = left 90, 1 = right 90, 2 = U-turn 180 ──────────────
    bool _spin(int pattern) {
        float angle = (pattern == 0) ? +90.0f : (pattern == 1) ? -90.0f : 180.0f;
        float h0 = control_get_heading();
        _targetHeading += angle;                      // next F holds to THIS target

        steering_enable(false);
        control_set_heading_hold(false, 0.0f);
        control_set_steering(0.0f);
        control_start_rotation(angle, _turnDps, 0.0f, MM_TURN_ALPHA);
        if (_kickPwm > 0) {
            control_kick_rotation((angle > 0) ? +_kickPwm : -_kickPwm,
                                  _kickMs, _kickRel);
        }

        uint32_t t0 = millis();
        while (true) {
            _streamTelemetry();
            if (control_is_motion_done() && (millis() - t0 > 250)) break;
            _poll();
            if (_abort) break;
            delay(1);
        }
        // stall check: rotated < half of commanded ⇒ mechanical failure
        float act = control_get_heading() - h0;
        if (fabsf(act) < fabsf(angle) * 0.5f) {
            _emitf("[STALL] turn rotated %+.1f of %+.1f deg\n", act, angle);
            _abort = true;
        }
        // ★ Null the residual heading error so the NEXT forward starts aligned.
        //   A clean turn here overshoots ~14°; left uncorrected it seeds a
        //   careening cascade (see log). This re-squares to the gyro target
        //   using the proven direct-PWM technique (same as fw_square_heading),
        //   which avoids the FF-bias overdrive that plagues tiny profile spins.
        if (!_abort && _settleTurns) _settleHeading();

        // Per-move diagnostic: how far the spin rotated vs commanded, and the
        // heading AFTER settling vs the ideal target. Watch 'turn_err' (raw
        // overshoot) and 'set_err' (residual after re-null — should be small).
        _emitf("D,%c,cmd=%+.1f,act=%+.1f,turn_err=%+.1f,head=%.1f/tgt%.1f,set_err=%+.1f\n",
               (pattern == 0) ? 'L' : (pattern == 1) ? 'R' : 'U',
               angle, act, act - angle,
               control_get_heading(), _targetHeading,
               control_get_heading() - _targetHeading);
        return !_abort;
    }

    // Rotate in place to bring the gyro heading onto _targetHeading. Adaptive
    // direct PWM (bypasses motors_update → no FF-bias overdrive at small angles),
    // coast, then reset the controller so the next move starts with zero error.
    //   sign: heading too LOW (err>0) → need CCW → left back / right fwd.
    void _settleHeading() {
        // Drive in ONE direction (like fw_square_heading) at a stiction-breaking
        // PWM until the heading crosses the target or we're within tolerance.
        // PWM 500-800 matches fw_square (520-800) — the proven floor that
        // actually rotates the robot in place (the old 110-280 never moved it).
        float err0 = _targetHeading - control_get_heading();
        if (fabsf(err0) > 3.0f && fabsf(err0) <= 80.0f) {   // >80° = real stall, don't fight
            int8_t s = (err0 > 0.0f) ? +1 : -1;             // err>0 → need CCW (heading up)
            uint32_t t0 = millis();
            while (millis() - t0 < 1200) {                  // hard timeout (fw_square-style)
                float err = _targetHeading - control_get_heading();
                if (fabsf(err) <= 3.0f) break;                       // reached target
                if (((err > 0.0f) ? +1 : -1) != s) break;            // crossed target — accept
                if (fabsf(err) > fabsf(err0) + 15.0f) break;         // diverging — bail (wrong dir)
                int16_t pwm = (int16_t)(500.0f + fabsf(err) * 6.0f);
                if (pwm > 800) pwm = 800;
                control_set_manual_pwm(-s * pwm, +s * pwm);          // CCW = left back, right fwd
                delay(8);
            }
        }
        control_set_manual_pwm(0, 0);                  // coast
        delay(120);
        motors_reset_controllers();                    // clear rot_error from manual rotation
        control_clear_manual_pwm();
    }
};

#endif // MICROMOUSE_H
