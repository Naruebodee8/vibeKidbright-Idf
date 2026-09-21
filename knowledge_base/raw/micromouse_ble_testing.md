# คู่มือทดสอบไร้สายผ่าน BLE (ทุกแล็บ)

ทุกแล็บคุมได้แบบ **ไม่ต้องต่อสาย USB** ผ่าน BLE — ใช้เครื่องมือเดียว สั่งคำสั่งตัวเดิมกับที่ใช้บน Serial Monitor และเห็นผลกลับมาบนหน้าจอ

> ทำไมต้องไร้สาย: หลายแล็บหุ่นต้องเคลื่อนที่ (Lab1 วัดบนพื้น, Lab2 ขับวัดระยะ, Lab5 วิ่ง/หมุน, Integration วิ่งจริง) — สาย USB จะลาก/รั้งให้ผลเพี้ยน. และ **Lab4 (IR) ใช้ WiFi ไม่ได้** เพราะ IR อยู่บน ADC2 ที่ WiFi ล็อก — จึงใช้ BLE

---

## เชื่อมต่อด้วยอะไรได้บ้าง (เลือกอย่างใดอย่างหนึ่ง)

**ก) คอมพิวเตอร์ — `mm_ui.py` (แนะนำ)**
```
cd Micromouse/tools
pip install -r requirements.txt          # ครั้งแรกครั้งเดียว
python mm_ui.py                           # ← ตัวเดียวใช้ได้ทุกแล็บ ไม่ต้องพิมพ์ชื่อ
```
แล้วในหน้าต่าง:
1. กด **Scan** → จะขึ้นรายชื่อหุ่นที่เจอ (เช่น `MM_Lab4_IR_3F2A9C`)
2. เลือก **ชื่อหุ่นของตัวเอง** (ดูชื่อได้จาก Serial Monitor ตอนเปิดหุ่น) → กด **Connect**
3. ดูจุดสถานะ: 🟢 เขียว=ต่ออยู่ · 🔴 แดง=หลุด/ยังไม่ต่อ (รู้ทันทีถ้าหลุด)
4. เลือก **แท็บของแล็บ** ที่ทำอยู่ → กดปุ่มคำสั่ง
5. ผลตอบกลับ + telemetry + ช่องพิมพ์คำสั่งเอง อยู่ที่ **แถบล่างถาวร** (เห็นได้ทุกแท็บ ไม่ต้องสลับ)
- ทุกหุ่นชื่อไม่ซ้ำกัน (มีรหัสชิปต่อท้าย) → ในห้องที่มี 20 ตัว เลือกของตัวเองได้ตรง
- ปุ่ม **save** เก็บ Log เป็นไฟล์

**ข) มือถือ — แอป BLE UART**
- nRF Connect หรือ "Serial Bluetooth Terminal" (โหมด BLE)
- สแกนหาอุปกรณ์ตามชื่อ → เชื่อม → service Nordic UART (NUS) → พิมพ์คำสั่ง + ดูผล

> เปิด Serial Monitor (USB) คู่กันได้ — output ออกทั้งสองทางพร้อมกัน

---

## ทำทีละแล็บ

> ทุกแล็บ: `python mm_ui.py` → **Scan** → เลือกหุ่นตัวเอง → **Connect** → เปิด **แท็บของแล็บ** → กดคำสั่ง / ดูผลในแถบ Log ด้านล่าง

### Lab1 — Motor  ·  BLE: `MM_Lab1_Motor`
ทดสอบมอเตอร์ + แบต + ปุ่ม **ตอนหุ่นอยู่บนพื้นจริง** (ค่าตรงกับตอนใช้งาน)
1. ทิศ/ขับ: `f`/`b`/`a`/`d` · `w`/`s` เพิ่มลด PWM · `x` หยุด · `l`/`r` ทีละล้อ
2. `m` = หา `MOTOR_PWM_MIN` + คอลัมน์ Volts (= `deadband_V`)
3. `v` = อ่าน Vbat · `c` = calibrate battery ratio (กรอกค่ามัลติมิเตอร์ → ได้ `BATTERY_DIVIDER_RATIO`)
4. **`i` = เช็คปุ่ม START/MODE (หลังบัดกรี)** — กดปุ่มจริงแล้วค่าต้องเปลี่ยน

### Lab2 — Encoder  ·  BLE: `MM_Lab2_Enc`
หุ่นต้อง **ขับเคลื่อนวัดระยะ** → ไร้สายช่วยให้ระยะไม่เพี้ยนจากสายลาก
1. วางบนพื้น มี runway ข้างหน้า → `x` → กรอกระยะเป้าหมาย (เช่น 200) → หุ่นขับไปแล้วหยุด
2. วัดระยะจริงด้วยไม้บรรทัด → `k` → กรอกระยะจริง → ดูค่า CPR ที่ควรเป็น (+%error)
3. (หา CPR เบื้องต้น) `r` reset → หมุนล้อมือ 1 รอบ → `c` อ่าน counts
- อื่น ๆ: `e` (กรอก CPR ดูระยะ) · `d` ระยะ · `p` สถานะ · `h`

### Lab3 — IMU  ·  BLE: `MM_Lab3_IMU`
1. วางหุ่น **นิ่งสนิท** บนพื้นราบ → `c` (calibrate gyro ~2 วิ ห้ามขยับ)
2. จด `GYRO_OFFSET_X/Y/Z` (สำคัญคือแกน Z สำหรับ heading)
   - `ACCEL_OFFSET_X/Y/Z` แสดงด้วย — **optional ไม่บังคับ** (ไม่ใช้ขับ ไว้ต่อยอด)
- อื่น ๆ: `r` reset heading · `s` ค่าสด (หมุนหุ่นดู heading เปลี่ยน) · `p`

### Lab4 — IR  ·  BLE: `MM_Lab4_IR`   (WiFi ปิดถาวร)
1. `s` อ่านต่อเนื่อง → เลื่อนหุ่นเข้า/ออกจากกำแพง ดูค่า mm เปลี่ยน
2. ตรวจแต่ละตัว: กำแพงใกล้ ~30 mm = ค่าน้อยและนิ่ง / เปิดโล่ง = ค่ามาก
3. `t` ตั้ง threshold → พิมพ์ **`60 80`** (front เว้นวรรค side)
4. ตั้ง `WALL_THRESHOLD_*` และ `WALL_OFFSET_*` → จดลงใบจูน
- อื่น ๆ: `r` อ่านครั้งเดียว · `v` raw mV · `l` ดู LUT

### Lab5 — Feedforward (step response)  ·  BLE: `MM_Lab5_FF`   (★ บนพื้นจริง)
**ทำก่อน Lab6** — ต้องรู้คุณลักษณะมอเตอร์ (Km, Tm) ก่อนจึงคำนวณ PID ได้
วิธี UKMARS: ป้อนแรงดันแบบ step วัดการตอบสนองความเร็ว → ได้ค่าระบบ **Km** (gain) + **Tm** (time constant)
**★ ทำบนพื้นจริง** (โหลดมีผลต่อ FF) พื้นที่โล่ง ~0.5m — หุ่นวิ่งสลับหน้า-หลังเพื่ออยู่กับที่
1. `r` = sweep อัตโนมัติ (2→5V สลับทิศ) → วัด speedL/R + 63% rise time ต่อล้อ
2. โปรแกรมคำนวณให้เลย + ขึ้น **กราฟ 2 รูป** ในแท็บ (เหมือนสไลด์ UKMARS):
   - Step response (speed-vs-time)  ·  Motor-Volts-vs-Speed (slope=FF_SPEED, intercept=FF_BIAS)
3. คัดลอกค่าใส่ config: `FF_SPEED_L/R_V_PER_MMPS`, `FF_BIAS_RUN_L/R_V`, `FF_ACC_V_PER_MMPS2`
4. จดค่า **Km, Tm** ท้าย output → เอาไปคำนวณ Kp/Kd ใน Lab6 (`gaincalc`)
- `ff <V>` = step เดียว (ดูเส้นโค้ง+Tm ของแรงดันนั้น) · `clear plot` ล้างกราฟ · `v` Vbat · `x` หยุด

### Lab6 — Control / PID  ·  BLE: `MM_Lab6_PID`
วางหุ่นบนพื้นโล่ง (มี runway) หรือยกล้อสำหรับลองหมุน
- **เดินหน้า (FWD PID):** `c` = เทียบ A/B (FF อย่างเดียว vs FF+PD) · `f` FF+PD · `g` FF · `d` dump CSV
  - จูน: `p`/`o` Kp · `k`/`m` Kd · `i`/`j` Ki · **`dist <mm>`** ตั้งระยะวิ่ง (เช่น `dist 400`)
- **หมุน (ROT PID):** **`turn 90`** / `turn -90` / `turn 180` → หุ่นหมุน + รายงาน turn error
  - จูน: **`rkp <v>` `rkd <v>` `rki <v>`**
- `s` = แสดง gain ทั้งหมด (FWD + ROT) · `x` abort
- **`gaincalc <Km> <Tm> [zeta] [TD]`** = คำนวณ FWD_KP/KD จากค่าระบบของ Lab5 FF (ไม่ต้องลองผิดลองถูก)
  - zeta=1.0 ไม่ overshoot · TD = settling time (วินาที, เช่น 0.12) · ใช้ค่า Km/Tm ที่ Lab5 บอก
  - หรือกดในแท็บ Lab6: ช่อง Km/Tm/zeta/TD → "calc + apply"
- ดู telemetry สด (fwd/heading) ในแถบล่างขณะวิ่ง

### Integration — Wall-following  ·  BLE: `MM_robot`
1. ตั้งเป้าหมายใน `WallFollower.ino` (`GOAL_X/Y`) → วางหุ่นที่จุดเริ่ม
2. กดปุ่ม **START** บนบอร์ด หรือพิมพ์ `g` → หุ่นเดินตามกำแพง; พิมพ์ `x` = หยุด
3. ดูตำแหน่งหุ่นบนแผง Maze + Log; กด save log เก็บผล
- จูนสด (คำสั่งมี space ใช้ได้แล้ว): `kp <v>` · `kd <v>` · `nom <v>` · `kick <pwm>` ฯลฯ

---

## ลำดับการจูนที่แนะนำ (ก่อนทำ wall-follower)
1. **Lab1** → direction, `MOTOR_PWM_MIN`, `deadband_V`, `BATTERY_DIVIDER_RATIO`, เช็คปุ่ม
2. **Lab2** → `COUNTS_PER_REV` (ระยะตรง)
3. **Lab3** → `GYRO_OFFSET_Z` (มุมตรง)
4. **Lab4** → `WALL_THRESHOLD_*`, `WALL_OFFSET_*` (เห็นกำแพง)
5. **Lab5 (FF)** → step response: `FF_SPEED`/`FF_BIAS`/`FF_ACC` ต่อล้อ **+ ค่าระบบ Km, Tm**
6. **Lab6 (PID)** → `gaincalc <Km> <Tm>` **คำนวณ** `FWD_KP/KD` (ยืนยัน/ปรับนิดเดียว) → ตามด้วย `ROT_KP/KD/KI`
7. **Integration** → steering/kick/settle จูนสด แล้วปล่อย wall-follower

---

## ปัญหาที่พบบ่อย
- **ต่อ BLE ไม่ติด:** ตรวจว่า flash แล้ว, เปิดหุ่นอยู่, เลือกชื่อหุ่นถูกตัว (ดูชื่อจาก Serial Monitor ตอนบูต), Bluetooth เครื่องเปิด
- **กด START แล้วไม่ทำงาน (UI):** ใช้ `mm_ui.py` เวอร์ชันล่าสุด (เขียน BLE แบบมี response + auto-reconnect)
- **คำสั่งที่มีช่องว่างไม่เข้า (เช่น `60 80`, `turn 90`):** ใช้ firmware เวอร์ชันล่าสุด (เก็บตัว space ใน BLE แล้ว)
- **ค่า IR มั่ว:** ต้องเป็น Lab4 ที่ `TELEMETRY_WIFI_ENABLED=0` (BLE) — ห้ามเปิด WiFi
