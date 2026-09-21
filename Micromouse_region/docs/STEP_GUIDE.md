# คู่มือการสอนทีละ STEP

> ทุก Lab: เปิด `.ino` ในโฟลเดอร์ → Upload → Serial Monitor **115200 baud, line ending = Newline**
> ค่าที่จด ให้ไปกรอกใน [`CALIBRATION_WORKSHEET.md`](CALIBRATION_WORKSHEET.md) แล้วนำไปแก้ `config.h` ของหุ่นตัวนั้น

> **ทดสอบไร้สาย (ไม่ต้องต่อสาย):** ทุกแล็บคุมผ่าน BLE ด้วยเครื่องมือเดียว — รัน `python mm_ui.py`
> (ไม่ต้องพิมพ์ชื่อ) → กด **Scan** → เลือกหุ่นของตัวเอง → **Connect** → เลือกแท็บของแล็บ
> ชื่อหุ่นไม่ซ้ำกัน (มีรหัสชิปต่อท้าย เช่น `MM_Lab4_IR_3F2A9C`) — **หุ่นปริ้นชื่อตัวเองบน Serial Monitor ตอนเปิดเครื่อง** ให้จดไว้เลือกตอนสแกน
> ฐานชื่อต่อแล็บ: Lab1=`MM_Lab1_Motor` · Lab2=`MM_Lab2_Enc` · Lab3=`MM_Lab3_IMU` · Lab4=`MM_Lab4_IR` · Lab5=`MM_Lab5_FF` · Lab6=`MM_Lab6_PID` · Integration=`MM_robot`
> รายละเอียดดู [`BLE_TESTING.md`](BLE_TESTING.md)  *(Lab4 ใช้ BLE เท่านั้น — WiFi กวน IR/ADC2)*

---

## ⚙️ ตั้งค่า Arduino IDE (ทำก่อนเริ่ม)

1. ลง **ESP32 by Espressif** ใน Boards Manager
2. Tools ▸ Board ▸ **ESP32S3 Dev Module**
3. Tools ▸ Partition Scheme ▸ **Huge App (3MB No OTA)**
4. เลือกพอร์ต (COM) ให้ถูก แล้ว Upload

**เช็ค ESP32 ด้วย BlinkRGB:** ลองตัวอย่าง BlinkRGB — ถ้าไฟ RGB **ไม่ติด** ให้ตรวจการบัดกรีขา RGB LED (**GPIO48**) บนบอร์ด เพราะบางบอร์ดมาแบบยังไม่บัดกรีขานี้

---

## STEP 2 + 4 — เช็ค + จูนทีละโมดูล

### Lab1_Motor  (มอเตอร์)
**เป้า:** มอเตอร์หมุนถูกข้าง/ถูกทิศ + หา PWM ต่ำสุดที่เริ่มหมุน
คำสั่ง: `w/s` เพิ่ม-ลด PWM · `f/b` เดินหน้า-ถอย · `a/d` เลี้ยวซ้าย-ขวา · `l/r` ทดสอบมอเตอร์ทีละข้าง · `m` หา PWM_MIN · `x` หยุด · `p` สถานะ
**จด:** `MOTOR_L_PWM_MIN`, `MOTOR_R_PWM_MIN`, ทิศหมุนถูกไหม (ถ้าผิด สลับสายหรือแก้ flag ใน config), Wheel diameter, Wheelbase
> ทีมขอ "ฟังก์ชันปรับมอเตอร์ซ้าย-ขวาให้เท่ากัน" → ทำได้ใน Feedforward (per-wheel) ภายหลัง

### Lab2_Encoder  (encoder)
**เป้า:** encoder นับถูกทิศ + หา CPR + **ตรวจว่าระยะที่หุ่นวิ่งตรงกับที่คำนวณ**
คำสั่ง: `r` รีเซ็ตนับ · หมุนล้อ 1 รอบด้วยมือ อ่าน counts = CPR · `x` ขับไปข้างหน้าระยะที่ตั้ง · `e` กรอก CPR → บอกว่าควรวิ่งได้กี่ mm · `k` กรอกระยะจริงที่วัด → คำนวณ CPR ที่ควรเป็น
**จด:** `COUNTS_PER_REV`, ทิศ (เดินหน้า=บวก), ความคลาดของระยะ (ปรับ CPR ให้ระยะจริงตรง)
> เพราะ RPM มอเตอร์แต่ละตัวไม่เท่ากัน CPR จะต่างกัน — ใช้โหมดตรวจระยะนี้หาค่าจริง

### Lab3_IMU  (ไจโร)
**เป้า:** หา gyro offset (หุ่นต้องวางนิ่ง)
คำสั่ง: `c` calibrate gyro (วางนิ่ง ~2 วิ) · `r` reset heading · `s` แสดงค่าสด · `p` สถานะ
**จด:** `GYRO_OFFSET_X/Y/Z` (ใช้แค่แกน Z สำหรับ heading)

### Lab4_IR  (เซนเซอร์กำแพง)
**เป้า:** ตั้ง threshold มี/ไม่มีกำแพง + offset ต่อตัว (กันติดตั้งเบี้ยว)
คำสั่ง: `r` อ่านทุกตัว (mm) · `v` อ่าน raw mV (ใช้ตอน cal LUT) · `s` อ่านต่อเนื่อง · `t` ตั้ง threshold · `l` ดู LUT
**จด:** `WALL_THRESHOLD_FRONT`, `WALL_THRESHOLD_SIDE`, `WALL_OFFSET_L/FL/FR/R`
> วิธีตรวจให้แน่ใจ: วางกำแพงใกล้ (~30 mm) → ค่าควรน้อยและนิ่ง; เปิดโล่ง → ค่าควรมาก (ไกล/นอกพิสัย). ตั้ง threshold ให้แยกสองสภาพนี้ได้ชัด และใช้ offset ปรับให้ซ้าย/ขวาสมมาตร

---

## STEP 5 — Feedforward: วัดคุณลักษณะมอเตอร์ (วัด → คำนวณ)

### Lab5_Feedforward  ·  BLE `MM_Lab5_FF`
**เป้า:** วัด step response **บนพื้นจริง** → ได้ค่าระบบ **Km** (gain), **Tm** (time constant) ต่อล้อ → ค่า Feedforward
คำสั่ง: `r` sweep อัตโนมัติ (ขึ้นกราฟ 2 รูปเหมือนสไลด์ UKMARS) · `ff <V>` step เดียว · `v` Vbat · `x` หยุด
**ได้:** `FF_SPEED`, `FF_BIAS`, `FF_ACC` ต่อล้อ + **Km, Tm** (ส่งต่อให้ Lab6 คำนวณ PID)
> ★ ต้องวัดบนพื้นจริง (โหลด/แรงเสียดทานมีผลต่อ FF) — ไม่ใช่ยกล้อลอย

---

## STEP 6 — PID: คำนวณ + จูนการคุมปิดลูป

### Lab6_Control_PID  ·  BLE `MM_Lab6_PID`
**เป้า:** เข้าใจ **Feedforward** (เดาแรงดัน) vs **PID** (แก้ error จาก encoder/gyro) + **คำนวณ** gain แทนการลองมั่ว
คำสั่ง: `gaincalc <Km> <Tm>` คำนวณ Kp/Kd จากค่า Lab5 · `f` FF+PD · `g` FF · `c` A/B · `d` CSV · `turn 90/180` ทดสอบหมุน · `s` ดูค่า
**ได้เข้าใจ:** FF พาไป ~ถึง, PD เก็บกวาดให้ตรง; **คำนวณ Kp/Kd ได้จาก Km/Tm** (UKMARS) แล้วปรับละเอียดอีกนิด
> ลำดับสำคัญ: รู้คุณลักษณะมอเตอร์ (Lab5) ก่อน จึงคำนวณ PID (Lab6) ได้ — ไม่ใช่จูนก่อนแล้วค่อยหา FF

---

## STEP 7 — รวมร่าง: หุ่นวิ่งถึงเป้า (Wall-following)

ใช้ library `Micromouse` + ตัวอย่าง `WallFollower` (ดูภาพการทำงานใน [`ARCHITECTURE.md`](ARCHITECTURE.md))

**สิ่งที่นักเรียนแก้ — ไฟล์เดียว** `WallFollower.ino`:
```cpp
const bool USE_LEFT_HAND = true;     // มือซ้าย/ขวาแตะกำแพง
const int  GOAL_X = 3, GOAL_Y = 2;   // ตั้งเป้าหมาย (เปลี่ยนตามสนาม)
// แก้ decideMove() = หัวใจอัลกอริทึม (ทีหลังเปลี่ยนเป็น flood-fill ได้)
```

**API ที่เรียกได้** (ที่เหลือซ่อนใน library):
```cpp
mouse.begin();  mouse.waitForStart();
mouse.moveCell(1);  mouse.turn90(LEFT/RIGHT);  mouse.turnBack();
mouse.wallFront();  mouse.wallLeft();  mouse.wallRight();   // → true/false
mouse.wallDistances(fl,fr,l,r);                            // → mm
```
ทุก primitive เป็น **blocking + คืน bool** (false = ชน/หยุด) → อัปเดต pose เฉพาะตอนคืน true

**ขั้นวิ่ง:** วางหุ่นที่จุดเริ่ม → กดปุ่ม START (หรือส่ง `g` ทาง BLE) → ดูผลทาง UI/Serial
**เงื่อนไขจบ:** `GOAL reached!` (ถึงเป้า) / `returned to START` / `MAX_STEPS` / `aborted/stalled`

> รอบนี้ wall-following เป็นตัวอย่างที่เราให้ — ใครทำ flood-fill เองได้ในแข่งจริงถือเป็นความสามารถพิเศษ
