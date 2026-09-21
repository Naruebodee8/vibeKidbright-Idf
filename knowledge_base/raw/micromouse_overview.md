# Micromouse — ชุดสอนรอบคัดเลือก (Region)

ชุดโค้ด + เอกสารสำหรับสอนนักเรียนสร้างและจูนหุ่น Micromouse จน **วิ่งไปถึงจุดเป้าหมายโดยไม่ชนกำแพง** (รอบคัดเลือก — ยังไม่ต้องแก้เขาวงกตเต็ม 16×16)

> ฮาร์ดแวร์: ESP32-S3 · มอเตอร์ N20 + Maker Drive (H-Bridge) · encoder X4 · IMU MPU6050 · IR 4 ตัว
> ค่าใน `config.h` เป็นค่าเริ่มต้นจากหุ่นต้นแบบ — **หุ่นแต่ละตัวต้องจูนเองหลังประกอบ** (นั่นคือเนื้อหา Lab)

---

## เดินตามขั้น (STEP)

| STEP | หัวข้อ | โฟลเดอร์ที่ใช้ | ได้อะไร |
|---|---|---|---|
| 1 | บัดกรี | — | (ฮาร์ดแวร์) |
| 2 | เช็คอุปกรณ์ (OK/NG) | `Lab1_Motor` → `Lab4_IR` | ยืนยันมอเตอร์/encoder/IMU/IR ทำงาน |
| 3 | ประกอบหุ่น | — | (ฮาร์ดแวร์) |
| 4 | จูนหุ่น (จดค่า) | `Lab1` → `Lab4` | ได้ค่า PWM_MIN, CPR, gyro offset, IR threshold/offset |
| 5 | Feedforward (motor characterise) | `Lab5_Feedforward` | วัด Km/Tm → ได้ FF ต่อล้อ (วัด→คำนวณ) |
| 6 | PID control | `Lab6_Control_PID` | คำนวณ Kp/Kd จาก Km/Tm + ทดสอบหมุน (FWD/ROT) |
| 7 | **รวมร่าง: วิ่งถึงเป้า** | `Micromouse/` (library) + `WallFollower` | หุ่นเดินตามกำแพงไปถึงเป้า |

> เอกสารละเอียดต่อ STEP อยู่ใน [`docs/STEP_GUIDE.md`](docs/STEP_GUIDE.md)
> ใบจดค่าจูนต่อหุ่น: [`docs/CALIBRATION_WORKSHEET.md`](docs/CALIBRATION_WORKSHEET.md)
> ภาพการทำงาน (flowchart/sequence): [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

---

## โครงสร้างโฟลเดอร์

```
Micromouse_region/
├── Lab1_Motor/          ← เช็ค+จูนมอเตอร์ (PWM_MIN/MAX, ทิศ, เลี้ยว)
├── Lab2_Encoder/        ← เช็ค+จูน encoder (ทิศ, CPR, ตรวจระยะจริง)
├── Lab3_IMU/            ← cal gyro offset
├── Lab4_IR/             ← cal IR (threshold + offset ต่อตัว)
├── Lab5_Feedforward/    ← วัดคุณลักษณะมอเตอร์ (Km/Tm) → ค่า FF ต่อล้อ
├── Lab6_Control_PID/    ← คำนวณ+จูน PID (FWD/ROT), ทดสอบวิ่งตรง/หมุน
├── Micromouse/          ← ★ library รวมร่าง (ซ่อน L0-L2) + ตัวอย่าง WallFollower
│   ├── src/             ← โค้ดควบคุม (นักเรียนไม่ต้องแตะ)
│   ├── examples/WallFollower/  ← ★ ไฟล์ที่นักเรียนเขียน (L3)
│   └── tools/mm_ui.py   ← UI จูน/ดูค่าผ่าน BLE (Python)
└── docs/                ← เอกสารสอน
```

---

## เริ่มต้นใช้งาน (setup)

1. **Arduino IDE** + ESP32 board package; เลือกบอร์ด **ESP32S3 Dev Module**, Partition = **Huge App (3MB)**
2. แต่ละ `Lab*` เปิดไฟล์ `.ino` ในโฟลเดอร์ของมันได้เลย (standalone) → Upload → เปิด Serial Monitor 115200
3. **รวมร่าง:** ติดตั้ง library — copy โฟลเดอร์ `Micromouse/` ไปไว้ใน `Documents/Arduino/libraries/` แล้วเปิด
   `File ▸ Examples ▸ Micromouse ▸ WallFollower`
4. **UI ไร้สาย (ออปชัน):** `cd Micromouse/tools && pip install -r requirements.txt && python mm_ui.py`

> เช็ค ESP32 เบื้องต้นด้วยตัวอย่าง BlinkRGB — ถ้าไฟ RGB ไม่ติด ให้ตรวจการบัดกรีขา RGB LED (GPIO48) บนบอร์ด (บางบอร์ดมาแบบยังไม่บัดกรี)

---

## ปรัชญาการออกแบบ (ทำไมแยกแบบนี้)

- **นักเรียนเขียนไฟล์เดียว** (`WallFollower.ino`) — มีแต่ตรรกะอัลกอริทึม ไม่จมโค้ดควบคุม
- **ลูปคุม 500 Hz (FF+PID, หมุน gyro, จัดกลาง) ซ่อนใน library** — ผ่านการทดสอบแล้ว แก้ไม่ได้/พังไม่ได้
- **ค่าจูนแยกใน `config.h`** — หุ่นแต่ละตัวต่างกัน จูนแล้วไม่ต้องแตะอัลกอริทึม

เป้าหมายรอบนี้: คัดเลือกให้เหลือ ~20 ทีม — เน้น "เข้าใจ + ทำหุ่นถึงเป้าได้" ไม่เน้น flood-fill (สอนต่อภายหลัง)
