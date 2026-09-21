# Micromouse — ภาพรวมและคู่มือสอน

> **Raw sources**: `raw/micromouse_overview.md` · `raw/micromouse_step_guide.md` · `raw/micromouse_calibration_worksheet.md` · `raw/micromouse_library_readme.md`
> **Related**: [[micromouse_architecture]] · [[micromouse_ble]]

---

## คืออะไร

ชุดโค้ด + เอกสารสำหรับสอนนักเรียนสร้างและจูนหุ่น Micromouse จนวิ่งถึงจุดเป้าหมายโดยไม่ชนกำแพง (รอบคัดเลือก)

**Hardware:**
- MCU: ESP32-S3
- Motor: N20 + Maker Drive (H-Bridge)
- Encoder: X4
- IMU: MPU6050
- IR Sensor: 4 ตัว

> ค่าใน `config.h` เป็นค่าเริ่มต้นจากหุ่นต้นแบบ — **หุ่นแต่ละตัวต้องจูนเองหลังประกอบ**

---

## โครงสร้างชั้น (4-Layer Architecture)

```
L3 — WallFollower.ino (นักเรียนแก้)
     ↓
L2 — Micromouse facade library (moveCell, turn90, wallFront...)
     ↓
L1 — config.h (ค่าจูน + pins)
     ↓
L0 — HAL + control loop 500Hz (ซ่อนใน library)
```

ดูรายละเอียด flowchart → [[micromouse_architecture]]

---

## เส้นทางการเรียน (STEPs)

| STEP | หัวข้อ | Lab | สิ่งที่ได้ |
|---|---|---|---|
| 1 | บัดกรี | — | (Hardware) |
| 2 | เช็คอุปกรณ์ | Lab1–Lab4 | ยืนยัน OK/NG ทุกโมดูล |
| 3 | ประกอบหุ่น | — | (Hardware) |
| 4 | จูนหุ่น | Lab1–Lab4 | ค่า PWM_MIN, CPR, gyro offset, IR threshold |
| 5 | Feedforward | Lab5_Feedforward | วัด Km/Tm → FF ต่อล้อ |
| 6 | PID | Lab6_Control_PID | คำนวณ Kp/Kd จาก Km/Tm |
| 7 | รวมร่าง | Micromouse/ + WallFollower | หุ่นเดินตามกำแพงถึงเป้า |

---

## Lab Guide ทีละ STEP

### Arduino IDE Setup
1. ลง **ESP32 by Espressif** ใน Boards Manager
2. Tools → Board → **ESP32S3 Dev Module**
3. Tools → Partition Scheme → **Huge App (3MB No OTA)**

> เช็ค ESP32: ลอง BlinkRGB — ถ้าไฟ RGB ไม่ติด ตรวจการบัดกรีขา RGB LED (**GPIO48**)

### การใช้ BLE ไร้สาย (ทุก Lab)
```bash
cd Micromouse/tools
pip install -r requirements.txt
python mm_ui.py    # auto-scan หา MM_robot
```

| Lab | BLE Name |
|---|---|
| Lab1 | `MM_Lab1_Motor` |
| Lab2 | `MM_Lab2_Enc` |
| Lab3 | `MM_Lab3_IMU` |
| Lab4 | `MM_Lab4_IR` ⚠️ BLE เท่านั้น (WiFi กวน IR/ADC2) |
| Lab5 | `MM_Lab5_FF` |
| Lab6 | `MM_Lab6_PID` |
| Integration | `MM_robot` |

---

### Lab1: Motor
**เป้า:** มอเตอร์หมุนถูกทิศ + หา PWM ต่ำสุด
**คำสั่ง:** `w/s` เพิ่ม-ลด PWM · `f/b` เดินหน้า-ถอย · `a/d` เลี้ยว · `m` หา PWM_MIN
**จด:** `MOTOR_L_PWM_MIN`, `MOTOR_R_PWM_MIN`, Wheel diameter, Wheelbase

### Lab2: Encoder
**เป้า:** encoder นับถูกทิศ + หา CPR + ตรวจว่าระยะตรง
**คำสั่ง:** `r` รีเซ็ต · หมุนล้อ 1 รอบด้วยมือ อ่าน counts · `k` กรอกระยะจริง → คำนวณ CPR
**จด:** `COUNTS_PER_REV`, ทิศ (เดินหน้า = บวก)

### Lab3: IMU
**เป้า:** หา gyro offset (วางหุ่นนิ่ง)
**คำสั่ง:** `c` calibrate (~2 วิ) · `r` reset heading
**จด:** `GYRO_OFFSET_X/Y/Z`

### Lab4: IR Sensor
**เป้า:** ตั้ง threshold มี/ไม่มีกำแพง + offset ต่อตัว
**คำสั่ง:** `r` อ่าน mm · `t` ตั้ง threshold
**จด:** `WALL_THRESHOLD_FRONT/SIDE`, `WALL_OFFSET_L/FL/FR/R`
> กำแพงใกล้ ~30mm = ค่าน้อยและนิ่ง / โล่ง = ค่ามาก (ตั้ง threshold ให้แยกชัด)

### Lab5: Feedforward
**เป้า:** วัด step response บนพื้นจริง → Km, Tm ต่อล้อ
**คำสั่ง:** `r` sweep อัตโนมัติ
**ได้:** `FF_SPEED`, `FF_BIAS`, `FF_ACC` + **Km, Tm**
> ⚠️ ต้องวัดบนพื้น (โหลด/แรงเสียดทานมีผล) — ไม่ใช่ยกล้อลอย

### Lab6: Control PID
**เป้า:** คำนวณ Kp/Kd จาก Km/Tm (ไม่ต้องลองมั่ว)
**คำสั่ง:** `gaincalc <Km> <Tm>` · `f` FF+PD · `turn 90/180`
> ลำดับสำคัญ: Lab5 (Km/Tm) → Lab6 (PID) — ไม่ใช่กลับกัน

---

## STEP 7: Integration — WallFollower

### ติดตั้ง Library
Copy โฟลเดอร์ `Micromouse/` → `Documents/Arduino/libraries/`
แล้วเปิด File → Examples → Micromouse → WallFollower

### สิ่งที่นักเรียนแก้ (ไฟล์เดียว: WallFollower.ino)
```cpp
const bool USE_LEFT_HAND = true;     // มือซ้าย/ขวาแตะกำแพง
const int  GOAL_X = 3, GOAL_Y = 2;  // เป้าหมาย (เปลี่ยนตามสนาม)
// แก้ decideMove() = หัวใจอัลกอริทึม
```

### Micromouse API (L2)
```cpp
mouse.begin();
mouse.waitForStart();                     // block จน START/'g'
mouse.moveCell(1);                        // เดินหน้า 1 ช่อง (blocking, คืน bool)
mouse.turn90(LEFT/RIGHT);                 // หมุน 90° (blocking, คืน bool)
mouse.turnBack();                         // หมุน 180°
bool wF = mouse.wallFront();              // true = มีกำแพง
bool wL = mouse.wallLeft();
bool wR = mouse.wallRight();
mouse.wallDistances(fl, fr, l, r);       // mm (255 = out of range)
```

> ทุก primitive **blocking + คืน bool** — อัปเดต pose เฉพาะตอนคืน `true`

### เงื่อนไขจบ Run
- `GOAL reached!` — ถึงเป้า (T1)
- `returned to START` — กลับจุดเริ่ม (T2)
- `MAX_STEPS` — เกิน 100 ช่อง (T0)
- `aborted/stalled` — ชน/หยุด

---

## ใบจดค่าจูน (ตัวอย่างจากหุ่นต้นแบบ)

| ค่า | ตัวอย่าง | วัดจาก |
|---|---|---|
| `MOTOR_L_PWM_MIN` | 17 | Lab1 |
| `MOTOR_R_PWM_MIN` | 20 | Lab1 |
| Wheel diameter | 33.35 mm | Lab1 |
| Wheelbase | 93.5 mm | Lab1 |
| `COUNTS_PER_REV` | ~815 (X4 encoder) | Lab2 |
| `GYRO_OFFSET_Z` | -53.0 | Lab3 |
| `WALL_THRESHOLD_FRONT` | 80 mm | Lab4 |
| `WALL_THRESHOLD_SIDE` | 100 mm | Lab4 |

---

## สถานะการ Debug (ล่าสุด)

| ส่วน | สถานะ |
|---|---|
| 500Hz control + FF+PD (เดินตรง) | ✅ ดี (err ~±1°) |
| heading-hold + settle (หลังเทิร์น) | ✅ ทำงาน (set_err ~±5°) |
| **wallRight() sensing** | ❌ อ่านเพี้ยน (37-91mm ที่ช่องเปิด) |
| **turn stall** | ⚠️ ครั้งเว้นครั้ง (แรงบิด/แบต) |
| pose tracking + termination | ✅ ถูกต้อง |

---

## See Also

- [[micromouse_architecture]] — Flowchart ลูปหลัก + Sequence diagram
- [[micromouse_ble]] — BLE wireless testing และ UI
