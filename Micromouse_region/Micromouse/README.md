# Micromouse — teaching platform library (Phase A)

A single Arduino library that hides layers **L0 (HAL)**, **L1 (config)** and
**L2 (motion / sensing primitives)** behind one class, so students write only
their **L3** maze logic in a single `.ino`.

```
Micromouse mouse;
void setup(){ mouse.begin(); }
void loop(){
  mouse.waitForStart();
  // ... call mouse.moveCell(), mouse.turn90(LEFT), mouse.wallFront() ...
}
```

## What's inside (hidden from students)
- 500 Hz FreeRTOS FF + position-PD control loop (`control_task.h`, `motors_controller.h`)
- Trapezoidal motion profiles (`profile.h`)
- Gyro in-place spins with stiction-break kick
- Wall-centering steering (CTE) + 500 Hz gyro heading-hold (`steering.h`)
- Front-wall squaring + distance re-zero, kills grid drift (`front_wall.h`)
- Sharp-IR wall sensing exposed as **binary + mm only** (`hal_wall_sensor.h`)
- BLE Nordic-UART console for wireless run/abort/tune (`ble_debug.h`)

Because sensing is exposed only as `wallFront()/wallLeft()/wallRight()` (bool)
and `wallDistances()` (mm), swapping Sharp IR for **pulsed-IR** later is an
**L0-only** change — this API does not move.

## API (L2 contract)
```cpp
void  begin();
void  waitForStart();              // blocks until START button or BLE 'g'
RunMode mode();                    // EXPLORE / SPEEDRUN (mode switch)
bool  moveCell(uint8_t n = 1);     // forward n cells, centered+straight; false=abort/stall
bool  turn90(Dir d);               // LEFT / RIGHT in-place 90°
bool  turnBack();                  // 180°
bool  wallFront(); wallLeft(); wallRight();
void  wallDistances(int& fl,int& fr,int& l,int& r);   // mm (255=out of range)
void  led(uint8_t r,uint8_t g,uint8_t b);  void beep(uint16_t ms);
// helpers so the UI can draw the run:
void  poll();                      // service BLE + stream telemetry (idle/between moves)
void  reportPose(int x,int y,int heading);
void  reportWall(int x,int y,int mask);
void  log(const char* s);
```
All motion primitives are **blocking** and return `bool` (false = aborted/stalled).
**Only update your pose when a call returns true.**

## Install
Copy the `Micromouse/` folder into your Arduino `libraries/` directory
(or `arduino-cli lib install --zip-path Micromouse.zip`). Then open
**File → Examples → Micromouse → WallFollower**.

Board: `esp32:esp32:esp32s3` (PartitionScheme = huge_app).

## Wireless UI
```
cd tools
pip install -r requirements.txt
python mm_ui.py                # auto-connects to BLE name "MM_robot"
```
Shows live telemetry + a dead-reckoned maze view, START/ABORT buttons, and
live steering tuning (`kp/kd/nom`). Same line protocol works in any BLE
terminal app (nRF Connect, Serial Bluetooth Terminal).

## Roadmap
- **Phase A (this):** facade + wall-follower example, tuning over BLE (RAM).
- **Phase B:** `MouseConfig` + NVS persistence + guided calibration routines.
- **Phase C:** port to competition PCB = swap L0 HAL + `config.h` pins only.
- **L3 next:** replace `decideMove()` in the example with flood-fill.
