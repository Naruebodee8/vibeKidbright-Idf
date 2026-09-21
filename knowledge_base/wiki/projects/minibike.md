# KidBright Minibike — Workshop Guide

> **Raw source**: `raw/minibike_workshop.md` (อ้างอิง: KBControl-Workshop-7Jul25.pdf)
> **Related**: [[kidbright32_pinout]] · [[sensor_guide]] · [[formula_kid]]

---

## ภาพรวม Workshop

| กิจกรรม | หัวข้อ |
|---------|--------|
| **1.1** | มินิไบค์ทรงตัว (Self-Balancing + Reaction Wheel + PID) |
| **2.1** | เทียบวัดจอยสติ๊ก (Calibration) |
| **2.2** | วิทยุบังคับ (ESP-NOW: ภาคส่ง + ภาครับ) |
| **3.1** | อ่านเซนเซอร์ IR TCRT5000 |
| **3.2** | Line Follower PID |
| **3.3** | โหมดรวม (ทรงตัว + วิทยุ + เดินเส้น) |

---

## Hardware

| ชิ้นส่วน | รายละเอียด |
|---------|-----------|
| บอร์ด | KidBright32 V1.5 iA + Extension |
| มอเตอร์ขับ | BDC Motor + L298N |
| Reaction Wheel | BDC Motor (ทรงตัว) |
| Servo | GPIO15 (Servo1) ควบคุมแกนเลี้ยว |
| IR Sensor | TCRT5000 × 2 → I3 (GPIO34) + I4 (GPIO35) |
| IMU | MPU-6050 / KXTJ3 (ในตัวบอร์ด) |
| แบตเตอรี่ | 9V × 2 + Power Bank |

### GPIO Mapping

| ฟังก์ชัน | GPIO |
|---------|------|
| Servo1 (แกนเลี้ยว) | GPIO15 |
| L298N IN1 (motor direction) | GPIO34 (I3) |
| L298N IN2 (motor direction) | GPIO35 (I4) |
| IR Sensor ซ้าย | GPIO34 (I3) — Analog |
| IR Sensor ขวา | GPIO35 (I4) — Analog |
| SW2 (เริ่มทรงตัว/calibrate) | GPIO14 |

---

## ⚠️ กฎเหล็กก่อนเขียนโค้ด (AI MANDATORY RULE)

**AI ต้องถามค่า configuration ก่อนเสมอ — ห้ามเดาค่าหรือใช้ placeholder เด็ดขาด**

| กิจกรรม | ต้องถาม? |
|---------|---------|
| 1.1 Self-Balancing | ✅ ถาม (Kp/Ki/Kd, GPIO, setpoint, ทิศมอเตอร์) |
| 2.1 Joystick Calibration | ❌ ไม่ต้อง (ใช้วัดค่าเท่านั้น) |
| 2.2 ภาคส่ง | ✅ ถาม (MAC Address ขาดไม่ได้ + ค่า Joystick 6 ค่า) |
| 2.2 ภาครับ | ✅ ถาม (Protocol, Servo center/range, motor speed max) |
| 3.1 อ่าน IR | ❌ ไม่ต้อง (ใช้วัดค่าเท่านั้น) |
| 3.2 Line Follower | ✅ ถาม (threshold, offset, Kp/Ki/Kd, base speed) |
| 3.3 โหมดรวม | ✅ ถาม (ค่าทั้งหมดจาก 2.2 + 3.2 รวมกัน) |

---

## ⚠️ ESP-IDF Gotchas

### 1. ADC Struct Name
```c
// ✅ ถูกต้อง
adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
// ❌ ผิด — คอมไพล์ไม่ผ่าน
adc_oneshot_unit_init_config_t init_config = ...;
```

### 2. Crystal Frequency 26 MHz (KidBright-specific!)
บอร์ด KidBright ใช้ XTAL **26 MHz** (ต่างจาก ESP32 DevKit ทั่วไป 40 MHz!)
ถ้าไม่ตั้ง → Serial Monitor แสดงขยะที่ 115200 baud
```sdkconfig
CONFIG_XTAL_FREQ=26
CONFIG_XTAL_FREQ_26=y
```
หรือ menuconfig → ESP System Settings → Main XTAL frequency → **26 MHz**

---

## กิจกรรม 1.1 — Self-Balancing

**หลักการ**: IMU วัดมุมเอียง → PID → ควบคุม Reaction Wheel Motor

### ขั้นตอนทดสอบ (ลำดับสำคัญมาก!)
1. ตั้งมินิไบค์ให้สมดุลมากที่สุด
2. กด **Reset** บน KidBright
3. รอ **2-3 วินาที** → กด **SW2 (GPIO14)** → calibrate IMU + เริ่มทรงตัว
4. Reaction Wheel เริ่มหมุน → ปล่อยมือ (เตรียมประคองกรณีล้ม)

### การแก้ปัญหา
| อาการ | วิธีแก้ |
|-------|--------|
| รถสั่น | ตรวจแกนเลี้ยวล้อหน้าหลวมไหม |
| ทรงตัวแต่เอียงๆ | เลื่อนตำแหน่งศูนย์ล้อ |

---

## กิจกรรม 2.1 — Joystick Calibration

โค้ดนี้ใช้ **วัดและแสดงค่า raw** จาก ADC เท่านั้น ไม่มี parameter ที่ต้องกรอก
ค่าที่ได้จะนำไปใส่ใน 2.2 Sender

---

## กิจกรรม 2.2 — ESP-NOW Radio Control

### ภาคส่ง (Sender) — คำถามที่ต้องถาม
| # | ค่าที่ต้องการ | หมายเหตุ |
|---|---|---|
| 1 | ⭐ **MAC Address บอร์ดรับ** | รูปแบบ `AA:BB:CC:DD:EE:FF` — ขาดไม่ได้! |
| 2 | js_y_neutral | ค่า Y แกนขณะไม่โยก |
| 3 | js_y_max | Y โยกบนสุด (เดินหน้า) |
| 4 | js_y_min | Y โยกล่างสุด (ถอย) |
| 5 | js_x_neutral | ค่า X ขณะไม่โยก |
| 6 | js_x_max | X โยกขวาสุด |
| 7 | js_x_min | X โยกซ้ายสุด |

> ⭐ ถ้ายังไม่รู้ MAC → Upload โค้ดภาครับก่อน แล้วดูใน Serial Monitor

### ภาครับ (Receiver) — คำถามที่ต้องถาม
| # | ค่าที่ต้องการ |
|---|---|
| 1 | Protocol encoding จากภาคส่ง (แกน X/Y ส่งค่าเป็นอะไร) |
| 2 | Servo center angle (default 90°) |
| 3 | Servo range ± กี่องศา |
| 4 | Motor speed สูงสุด (PWM 0-255) |

---

## กิจกรรม 3.1 — อ่านเซนเซอร์ IR

อ่านค่า TCRT5000 แล้วแสดงบน **Serial Plotter** เท่านั้น
ค่าที่ได้ (threshold ขาว/ดำ) นำไปใส่ใน 3.2

---

## กิจกรรม 3.2 — Line Follower PID

### คำถามที่ต้องถาม
| # | ค่าที่ต้องการ |
|---|---|
| 1 | ADC เมื่อ IR อยู่บนพื้นขาว |
| 2 | ADC เมื่อ IR อยู่บนเส้นดำ |
| 3 | Sensor offset (ซ้าย-ขวาต่างกันไหม?) |
| 4 | Kp, Ki, Kd (default: 0.05, 0.001, 0.02) |
| 5 | MOTOR_BASE_SPEED (PWM 0-255) |
| 6 | SERVO_CENTER (°) |

---

## ⚡ ข้อควรระวังด้านไฟ

- **ห้าม**เปิดสวิตช์บอร์ด Extension ทิ้งไว้จนไฟดับ
- แรงดันขณะใช้งาน: **≥ 17.5V**
- แรงดันขึ้นลง > 2V → หยุด นำชาร์จ
- สายเซนเซอร์ IR: **สายขาว = VCC ต้องอยู่ด้านใกล้บอร์ด**

---

## See Also

- [[sensor_guide]] — ADC API และ I2C init order
- [[kidbright32_pinout]] — GPIO ทั้งหมด
- [[formula_kid]] — โปรเจกต์สนามแข่งอีกรูปแบบ
