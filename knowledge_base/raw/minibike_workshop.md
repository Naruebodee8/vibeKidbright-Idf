# 🏍️ KidBright Minibike — Workshop Knowledge Base
> อ้างอิงจาก: **KBControl-Workshop-7Jul25.pdf** · ทีมวิจัยเทคโนโลยีเพื่อการศึกษา (EDT)
> ครอบคลุม: **การประกอบ · กิจกรรม 1.1 (Self-Balancing) · กิจกรรม 2.x (วิทยุบังคับ) · กิจกรรม 3.x (เดินตามเส้น)**

---

## ⚠️ ESP-IDF Code Generation Rules & Gotchas (ESP-IDF v5.x)
เมื่อมีการเขียนหรือแนะนำโค้ด C/C++ สำหรับ ESP-IDF v5.x (เช่น โครงการ Micromouse หรือโปรแกรมบน ESP32) ให้ปฏิบัติตามกฎนี้อย่างเคร่งครัด:
* **ADC Oneshot Driver (`esp_adc/adc_oneshot.h`):** ต้องใช้ชื่อโครงสร้าง **`adc_oneshot_unit_init_cfg_t`** เสมอ (ห้ามใช้ `adc_oneshot_unit_init_config_t` เด็ดขาด เพราะทำให้คอมไพล์ไม่ผ่าน)
  ```c
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ```
* **Crystal Frequency Config (26 MHz XTAL):** บอร์ด KidBright ส่วนใหญ่ใช้ Crystal oscillator ความถี่ **26 MHz** (ต่างจาก ESP32 Dev Kit ทั่วไปที่เป็น 40 MHz) หากไม่ได้เปลี่ยนค่านี้ใน config หน้าจอ Serial Monitor จะอ่านข้อมูลเป็นตัวอักษรขยะ (`xxxx`) ที่ความเร็ว 115200 baud.
  ให้ตั้งค่าใน `sdkconfig` ดังนี้:
  ```sdkconfig
  CONFIG_XTAL_FREQ=26
  CONFIG_XTAL_FREQ_26=y
  ```
  หรือตั้งค่าผ่าน `menuconfig` -> `Component config` -> `ESP System Settings` -> `Main XTAL frequency` -> เลือก `26 MHz`


---

## 🤖 AI MANDATORY PRE-CODING RULE — อ่านก่อนเขียนโค้ดทุกครั้ง

> **⚠️ CRITICAL INSTRUCTION FOR AI:**
> ก่อนเขียนหรืออัพเดทโค้ด Minibike กิจกรรมใดๆ **AI ต้องถามค่า configuration จากผู้ใช้ก่อนเสมอ**
> ห้ามเดาค่าหรือใช้ค่า placeholder เด็ดขาด — ค่าทุกตัวต้องมาจากผู้ใช้

### 📝 คำถามที่ต้องถาม แยกตามกิจกรรม

---

#### กิจกรรม 1.1 — Self-Balancing (ถามก่อนเขียนโค้ดเสมอ)

ถามคำถามต่อไปนี้ **ทั้งหมด** ก่อนเขียนโค้ด:

| # | คำถาม | ทำไมต้องถาม |
|---|-------|------------|
| 1 | **มุมเอียง setpoint คือเท่าไหร่?** (ปกติ 0° แต่ถ้ารถเอียงเป็นธรรมชาติอาจต้องปรับ) | กำหนด balance point |
| 2 | **ค่า Kp, Ki, Kd ที่ต้องการเริ่มต้น** (หรือให้ใช้ค่า default: Kp=15, Ki=0.5, Kd=0.8?) | ค่า PID เริ่มต้น |
| 3 | **ทิศทางมอเตอร์ Reaction Wheel**: หมุน forward ควรทำให้รถเอียงไปทิศใด? | ป้องกัน PID feedback กลับทาง |
| 4 | **GPIO ของมอเตอร์**: IN1, IN2, ENA ต่อที่ GPIO อะไร? | ถ้าต่างจาก default |

---

#### กิจกรรม 2.1 — Joystick Calibration (ไม่ต้องถาม — โค้ดนี้ใช้วัดค่า)

> โค้ดกิจกรรม 2.1 ใช้สำหรับ **อ่านและแสดงค่า raw** เท่านั้น ไม่มี parameter ที่ต้องกรอก
> ผู้ใช้จะได้ค่าจากกิจกรรมนี้ไปใช้ใน 2.2

---

#### กิจกรรม 2.2 ภาครับ (Receiver) — ถามก่อนเขียนโค้ดเสมอ

ถามคำถามต่อไปนี้ **ทั้งหมด**:

| # | คำถาม | ทำไมต้องถาม |
|---|-------|------------|
| 1 | **Protocol encoding จากภาคส่ง**: แกน X ส่งค่าเป็นอะไร? แกน Y ส่งค่าเป็นอะไร? ค่าหยุดคือ? | กำหนด decode logic |
| 2 | **มุม Servo center คือเท่าไหร่?** (ค่า default = 90°) | เซ็ตตำแหน่งเริ่มต้นเซอร์โว |
| 3 | **Servo range ± กี่องศา?** จากกลางไปซ้าย/ขวาสุดได้เท่าไหร่? | จำกัดการเลี้ยว |
| 4 | **ความเร็วมอเตอร์สูงสุด** (PWM 0-255) ที่ต้องการ? | ป้องกันมอเตอร์หมุนเร็วเกิน |

---

#### กิจกรรม 2.2 ภาคส่ง (Sender/Remote) — ถามก่อนเขียนโค้ดเสมอ

ถามคำถามต่อไปนี้ **ทั้งหมด** — **ขาดคำถามใดคำถามหนึ่งไม่ได้**:

| # | คำถาม | ทำไมต้องถาม |
|---|-------|------------|
| 1 | ⭐ **MAC Address ของบอร์ดรับ** คืออะไร? (รูปแบบ `AA:BB:CC:DD:EE:FF`) | ใส่ใน `receiver_mac[]` ไม่ได้เดา |
| 2 | **ค่า Joystick แกน Y ขณะไม่โยก** (neutral) คือเท่าไหร่? | js_y_neutral |
| 3 | **ค่า Joystick แกน Y โยกบนสุด** (เดินหน้า max) คือเท่าไหร่? | js_y_max |
| 4 | **ค่า Joystick แกน Y โยกล่างสุด** (ถอยหลัง max) คือเท่าไหร่? | js_y_min |
| 5 | **ค่า Joystick แกน X ขณะไม่โยก** (neutral) คือเท่าไหร่? | js_x_neutral |
| 6 | **ค่า Joystick แกน X โยกขวาสุด** คือเท่าไหร่? | js_x_max |
| 7 | **ค่า Joystick แกน X โยกซ้ายสุด** คือเท่าไหร่? | js_x_min |

> ⭐ **MAC Address ขาดไม่ได้เด็ดขาด** ถ้าผู้ใช้ยังไม่รู้ MAC → แนะนำให้ Upload โค้ดภาครับก่อน แล้วดูใน Serial Monitor

---

#### กิจกรรม 3.1 — อ่านเซนเซอร์ IR (ไม่ต้องถาม — โค้ดนี้ใช้วัดค่า)

> โค้ดกิจกรรม 3.1 ใช้สำหรับ **อ่านและแสดงค่า IR ใน Serial Plotter** เท่านั้น
> ผู้ใช้จะได้ค่า threshold จากกิจกรรมนี้ไปใช้ใน 3.2

---

#### กิจกรรม 3.2 — Line Follower PID — ถามก่อนเขียนโค้ดเสมอ

ถามคำถามต่อไปนี้ **ทั้งหมด**:

| # | คำถาม | ทำไมต้องถาม |
|---|-------|------------|
| 1 | **ค่า ADC ของ IR เมื่ออยู่บนพื้นสีขาว** (จาก Serial Plotter กิจกรรม 3.1) คือเท่าไหร่? | กำหนด LINE_THRESHOLD |
| 2 | **ค่า ADC ของ IR เมื่ออยู่บนเส้นสีดำ** คือเท่าไหร่? | ยืนยัน threshold สมเหตุสมผล |
| 3 | **ค่า sensor offset**: IR ซ้ายและขวาอ่านค่าต่างกันขณะอยู่บนพื้นสีขาวเหมือนกันไหม? ต่างกันเท่าไหร่? | ชดเชย sensor mismatch |
| 4 | **ค่า Kp, Ki, Kd** ที่ต้องการเริ่มต้น (หรือใช้ default: Kp=0.05, Ki=0.001, Kd=0.02?) | ค่า PID |
| 5 | **ความเร็วมอเตอร์พื้นฐาน** ที่ต้องการ (PWM 0-255)? | MOTOR_BASE_SPEED |
| 6 | **มุม Servo center** คือเท่าไหร่? (ต้องตรงกับกิจกรรม 2.2) | SERVO_CENTER |

---

#### กิจกรรม 3.3 — โหมดรวม — ถามก่อนเขียนโค้ดเสมอ

> กิจกรรม 3.3 รวมทุกอย่าง ดังนั้นต้องถาม **ค่าทั้งหมดจากกิจกรรม 2.2 และ 3.2** รวมกัน
> ดูรายการคำถามจาก 2.2 Sender + 2.2 Receiver + 3.2 ด้านบน

---

### ⚡ วิธีถาม (ตัวอย่าง template)

เมื่อผู้ใช้ขอโค้ดกิจกรรมใด ให้ตอบว่า:

```
ก่อนเขียนโค้ดกิจกรรม [X.X] ขอทราบค่าต่อไปนี้ก่อนนะครับ:

1. [คำถาม 1]
2. [คำถาม 2]
...

(ถ้าค่าไหนยังไม่มี/ไม่แน่ใจ ให้บอกด้วย จะใช้ค่า default แทน)
```

---

## 📋 ภาพรวมกิจกรรม (Workshop Outline)

| กิจกรรม | หัวข้อ | สิ่งที่ต้องทำ |
|---------|--------|--------------|
| **1.1** | มินิไบค์ทรงตัว | Self-Balancing ด้วย Reaction Wheel + PID |
| **2.1** | เทียบวัดจอยสติ๊ก | Calibration ค่า X/Y ของ Remote |
| **2.2** | วิทยุบังคับ | ภาคส่ง (ESP-NOW) + ภาครับ (Servo + Motor) |
| **3.1** | อ่านเซนเซอร์ IR | อ่านค่า TCRT5000 ด้วย Analog Filter |
| **3.2** | Line Follower | PID ควบคุมการเลี้ยวตามเส้นสีดำ |
| **3.3** | ทรงตัว + วิทยุ + เดินตามเส้น | รวมทุก mode ใน 1 โปรแกรม |

---

## 🔧 ฮาร์ดแวร์ Minibike

### ชิ้นส่วนหลัก
| ชิ้นส่วน | รายละเอียด |
|---------|-----------|
| **บอร์ด KidBright** | V1.5 iA (ESP32) + บอร์ด Extension |
| **มอเตอร์ขับเคลื่อน** | BDC Motor + บอร์ดขับ **L298N** |
| **Reaction Wheel Motor** | BDC Motor ขับ Reaction Wheel (ทรงตัว) |
| **เซอร์โวมอเตอร์** | ต่อที่ **Servo1** → GPIO15 (ควบคุมแกนเลี้ยว) |
| **แบตเตอรี่** | 9V × 2 ก้อน (ชาร์จด้วยสาย USB-C) + Power Bank (Micro-USB) |
| **เซนเซอร์ IR** | TCRT5000 × 2 ตัว (ซ้าย/ขวา) → ต่อที่ **I3 (GPIO34)** และ **I4 (GPIO35)** |
| **IMU** | MPU-6050 หรือ KXTJ3 ในตัวบอร์ด (ทรงตัว) |
| **โวลต์มิเตอร์** | แสดงแรงดันไฟ ≥ 17.5V ขณะใช้งาน |

### GPIO Mapping — Minibike
| ฟังก์ชัน | GPIO | หมายเหตุ |
|---------|------|---------|
| Servo1 (แกนเลี้ยว) | GPIO15 | PWM/LEDC ช่อง Servo1 |
| BDC Motor (L298N IN1) | I3 → GPIO34 | ควบคุมทิศทางมอเตอร์ขับ |
| BDC Motor (L298N IN2) | I4 → GPIO35 | ควบคุมทิศทางมอเตอร์ขับ |
| IR Sensor ซ้าย | I3 (GPIO34) | Analog Input |
| IR Sensor ขวา | I4 (GPIO35) | Analog Input |
| SW2 (ปุ่มเริ่มทรงตัว) | GPIO14 | กดหลัง Reset เพื่อ calibrate IMU |

> ⚠️ **สายเซนเซอร์ IR:** สายสีขาว = VCC ต้องอยู่ด้านใกล้บอร์ด KidBright ตามรูปใน PDF

---

## ⚡ ข้อควรระวังด้านไฟ

- **ห้าม** เสียบแบตเตอรี่แล้วเปิดสวิตช์บอร์ด Extension ทิ้งไว้จนไฟดับ → แบตเตอรี่เสียและอาจอันตราย
- ขณะใช้งาน แรงดันต้องอยู่ที่ **≥ 17.5V**
- หากแรงดันขึ้นลงมากกว่า 2V → หยุดใช้งาน นำไปชาร์จ

---

## 🏍️ กิจกรรม 1.1 — มินิไบค์ทรงตัว (Self-Balancing)

### หลักการ
ใช้ **Reaction Wheel** (ล้อหมุน) สร้าง angular momentum เพื่อชดเชยการเอียงตัว
ข้อมูล IMU (มุมเอียง) ป้อนเข้า **PID Controller** → ควบคุมความเร็ว Reaction Wheel Motor

### ลำดับขั้นตอนทดสอบ (CRITICAL)
1. ตั้งมินิไบค์ให้อยู่ในตำแหน่ง **สมดุล** มากที่สุด
2. กดปุ่ม **Reset** บนบอร์ด KidBright
3. รอ **2-3 วินาที** แล้วกดปุ่ม **SW2 (GPIO14)** → โปรแกรม calibrate IMU และเริ่มระบบทรงตัวอัตโนมัติ
4. เมื่อ Reaction Wheel เริ่มหมุน → ปล่อยมือ (แต่เตรียมประคองกรณีล้ม)
5. จ่ายไฟด้วย **Adapter 12V** แล้วทำการทดสอบ (เหมือน Mini-Control)

### การแก้ปัญหาเบื้องต้น
| อาการ | วิธีแก้ |
|-------|--------|
| รถสั่น | ตรวจสอบแกนเลี้ยวล้อหน้าว่าหลวมหรือไม่ |
| รถทรงตัวแบบเอียงๆ | เลื่อนตำแหน่งศูนย์ล้อหลังและหน้า |

---

## 📡 กิจกรรม 2.x — วิทยุบังคับ (ESP-NOW Remote Control)

### หลักการ
ใช้ **ESP-NOW** (Espressif peer-to-peer wireless) สื่อสารระหว่าง:
- **ภาคส่ง (Remote):** บอร์ด KidBright พร้อม Joystick × 2
- **ภาครับ (Minibike):** บอร์ด KidBright บนรถ

### โครงสร้าง Joystick Remote
| แกน | หน้าที่ | ทิศทาง |
|-----|---------|--------|
| **แกน Y (JS ซ้าย)** | ความเร็วมอเตอร์ขับ | บน = เดินหน้า, ล่าง = ถอยหลัง |
| **แกน X (JS ขวา)** | เลี้ยว (Servo) | ซ้าย/ขวา |

### กิจกรรม 2.1 — เทียบวัด Joystick (Calibration)
1. สร้างโปรแกรมอ่านค่า Raw จอยสติ๊ก
2. จดค่าแกน X และ Y **ขณะไม่โยก** (ค่า neutral)
3. โยกแกน X → ขวาสุด (จด), ซ้ายสุด (จด)
4. โยกแกน Y → บนสุด (จด), ล่างสุด (จด)

#### 💻 โค้ดโปรแกรม Calibrate จอยสติ๊ก (ESP-IDF v5.x + หน้าจอ OLED SH1106)
โค้ดด้านล่างนี้ใช้เพื่ออ่านค่าดิบ (Raw values) ของแกน Y (จอยสติ๊กฝั่งซ้าย) ผ่าน **GPIO32** และแกน X (จอยสติ๊กฝั่งขวา) ผ่าน **GPIO33** นำมาแสดงผลปัจจุบัน ค่า Min และ Max ที่วัดได้บนจอ OLED SH1106 และสามารถสั่งพิมพ์สรุปผลการ Calibrate ออกมาทาง Serial Monitor ได้เมื่อเคาะปุ่ม Enter (หรือส่ง newline)

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// กำหนดพอร์ตและขาสำหรับ OLED บนบอร์ด KidBright
#define I2C_PORT I2C_NUM_0
#define SDA_PIN  GPIO_NUM_21
#define SCL_PIN  GPIO_NUM_22
#define OLED_ADDR 0x3C

static const char *TAG = "JOYSTICK_CAL";

// คำสั่งเริ่มต้นการทำงานของ OLED SH1106
static const uint8_t sh1106_init_cmds[] = {
    0xAE,       // ปิดการแสดงผล
    0xA8, 0x3F, // Set multiplex ratio (1/64)
    0xD3, 0x00, // Set display offset
    0x40,       // Set display start line
    0xA1,       // Set segment re-map (กลับตำแหน่งหน้าจอซ้าย-ขวา)
    0xC8,       // Set COM output scan direction
    0xDA, 0x12, // Set COM pins hardware configuration
    0x81, 0x7F, // ปรับความสว่างหน้าจอ (Contrast)
    0xA4,       // แสดงผลตามข้อมูลใน RAM
    0xA6,       // โหมดหน้าจอปกติ (ไม่กลับสี)
    0xD5, 0x80, // Set oscillator frequency
    0xD9, 0x22, // Set pre-charge period
    0xDB, 0x35, // Set VCOMH deselect level
    0xAD, 0x8B, // เปิดวงจรทวีแรงดันภายใน (DC-DC Charge Pump ON)
    0xAF        // เปิดการแสดงผล (Display ON)
};

// ข้อมูล Font ขนาด 5x7 สำหรับแสดงผลอักขระ ASCII ตั้งแต่เว้นวรรค (Space) ถึง z
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x24, 0x24, 0x24, 0x24, 0x24}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // V
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // W
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0c, 0x50, 0x50, 0x50, 0x3c}, // y
    {0x44, 0x64, 0x54, 0x4c, 0x44}  // z
};

static volatile bool g_print_summary = false;

static esp_err_t sh1106_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_PORT, OLED_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t sh1106_write_data(const uint8_t *data, size_t len) {
    uint8_t *buf = malloc(len + 1);
    if (!buf) {
        return ESP_ERR_NO_MEM;
    }
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    esp_err_t ret = i2c_master_write_to_device(I2C_PORT, OLED_ADDR, buf, len + 1, pdMS_TO_TICKS(100));
    free(buf);
    return ret;
}

static void sh1106_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    for (size_t i = 0; i < sizeof(sh1106_init_cmds); i++) {
        sh1106_write_cmd(sh1106_init_cmds[i]);
    }
}

static void sh1106_clear(void) {
    uint8_t clear_buf[132] = {0};
    for (int page = 0; page < 8; page++) {
        sh1106_write_cmd(0xB0 | page);
        sh1106_write_cmd(0x02); // starting column lower nibble (offset 2 สำหรับ SH1106)
        sh1106_write_cmd(0x10); // starting column higher nibble
        sh1106_write_data(clear_buf, 128);
    }
}

static void sh1106_draw_string(const char *str, int page, int col) {
    if (page < 0 || page >= 8) {
        return;
    }
    
    sh1106_write_cmd(0xB0 | page);
    
    int real_col = col + 2; // ชดเชย Column offset 2 พิกเซลสำหรับจอภาพ SH1106
    sh1106_write_cmd(0x00 | (real_col & 0x0F));
    sh1106_write_cmd(0x10 | ((real_col >> 4) & 0x0F));
    
    while (*str) {
        char c = *str++;
        if (c >= ' ' && c <= 'z') {
            int index = c - ' ';
            sh1106_write_data(font5x7[index], 5);
            uint8_t space = 0x00;
            sh1106_write_data(&space, 1);
        }
    }
}

static void serial_input_task(void *pvParameters) {
    while (1) {
        int c = getchar();
        if (c == '\n' || c == '\r') {
            g_print_summary = true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing OLED SH1106...");
    sh1106_init();
    sh1106_clear();

    sh1106_draw_string("   JOYSTICK CAL   ", 0, 0);
    sh1106_draw_string("====================", 1, 0);

    ESP_LOGI(TAG, "Initializing ADC...");
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    // กำหนดขาและแชนเนลสำหรับอ่านจอยสติ๊ก
    // JS1 แกน Y (จอยสติ๊กฝั่งซ้าย): GPIO32 (ADC1 Channel 4)
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &config));
    // JS2 แกน X (จอยสติ๊กฝั่งขวา): GPIO33 (ADC1 Channel 5)
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_5, &config));

    int min1 = 4095, max1 = 0;
    int min2 = 4095, max2 = 0;

    xTaskCreate(serial_input_task, "serial_input_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Calibration started. Move Joystick to extremes.");
    printf("\n--- Joystick Calibration Started ---\n");
    printf("Move both axes to their MIN and MAX limits.\n");
    printf("Press ENTER in Serial Monitor to print the summary.\n\n");

    char buf1[24];
    char buf2[24];
    char buf3[24];
    char buf4[24];

    while (1) {
        int val1 = 0;
        int val2 = 0;
        
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &val1));
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_5, &val2));

        if (val1 < min1) {
            min1 = val1;
        }
        if (val1 > max1) {
            max1 = val1;
        }
        if (val2 < min2) {
            min2 = val2;
        }
        if (val2 > max2) {
            max2 = val2;
        }

        snprintf(buf1, sizeof(buf1), "JS1 Y (GPIO32): %4d", val1);
        snprintf(buf2, sizeof(buf2), "  Min:%4d Max:%4d", min1, max1);
        snprintf(buf3, sizeof(buf3), "JS2 X (GPIO33): %4d", val2);
        snprintf(buf4, sizeof(buf4), "  Min:%4d Max:%4d", min2, max2);

        sh1106_draw_string(buf1, 3, 0);
        sh1106_draw_string(buf2, 4, 0);
        sh1106_draw_string(buf3, 5, 0);
        sh1106_draw_string(buf4, 6, 0);

        if (g_print_summary) {
            g_print_summary = false;
            printf("\n--- Joystick Calibration Summary ---\n");
            printf("JS1 Y-Axis (GPIO32): Min = %d, Max = %d, Current = %d\n", min1, max1, val1);
            printf("JS2 X-Axis (GPIO33): Min = %d, Max = %d, Current = %d\n", min2, max2, val2);
            printf("------------------------------------\n\n");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```


### กิจกรรม 2.2 — โปรแกรมภาครับ
```
< เซ็ตเซอร์โวมอเตอร์ให้อยู่ในมุม 90 องศา (ตรง)
< อ่านข้อมูลที่บอร์ดรับได้จากภาคส่ง
< ตรวจสอบว่าเป็น Joystick แกน X หรือไม่
  → ถ้าใช่: เซอร์โวหมุนตามมุมจากจอยสติ๊กแกน X
< ตรวจสอบว่าเป็น Joystick แกน Y หรือไม่
  → ลบค่า offset ออก
  → ถ้าค่า > 0: มอเตอร์หมุนตามเข็มด้วยความเร็วที่อ่านได้
  → ถ้าค่า < 0: มอเตอร์หมุนทวนเข็มด้วยความเร็วที่อ่านได้
  → ถ้าค่าไม่อยู่ในช่วง: หยุดการหมุนของมอเตอร์
< หน่วงเวลา 30ms
```

### กิจกรรม 2.2 — โปรแกรมภาคส่ง
```
< กรอกค่าจาก calibration:
  - ค่า output เมื่อโยกซ้ายสุด
  - ค่า output เมื่อโยกขวาสุด
  - ค่า output เมื่อโยกบนสุด / ล่างสุด
< ระบุ MAC Address ของบอร์ดรับ (ดูจากตอนโปรแกรม)
< อ่านค่าแกน X → ส่งผ่าน ESP-NOW
< อ่านค่าแกน Y → ลดความเร็วเมื่อมีการเลี้ยว
  → บวก offset เพื่อแยกจากค่าแกน X
  → ส่ง MAC Address ของบอร์ดรับพร้อมค่าแกน Y
< หน่วงเวลา 100ms
```

---

## 🖊️ กิจกรรม 3.x — เดินตามเส้น (Line Follower)

### เซนเซอร์ TCRT5000
- ต่อที่ **I3 (GPIO34)** และ **I4 (GPIO35)** ของบอร์ด KidBright
- ต่อ GND-I3-3V3 และ GND-I4-3V3 ตามที่ระบุใน PDF หน้า 70
- อ่านค่า **Analog** (ยิ่งเยอะ = กรองดีแต่หน่วงมากขึ้น)
- ค่าสะท้อน: สีขาว = ค่าสูง, สีดำ = ค่าต่ำ

### หลักการ Line Following
```
เส้นสีดำบนพื้นสีขาว:
• เซนเซอร์ทั้งสองอ่านสีขาว → วิ่งตรง
• เซนเซอร์ซ้ายอ่านสีดำ → รถทับเส้นด้านซ้าย → เลี้ยวซ้ายออก
• เซนเซอร์ขวาอ่านสีดำ → รถทับเส้นด้านขวา → เลี้ยวขวาออก  
• เซนเซอร์ทั้งสองอ่านสีดำ → หยุด
```

### PID สำหรับ Line Following
```
Error    = IR_ขวา - IR_ซ้าย
Setpoint = 0  (ต้องการให้ทั้งสองค่าเท่ากัน)
Output   → ปรับมุมเซอร์โว (เลี้ยวซ้าย/ขวา)
```

### กิจกรรม 3.1 — อ่านเซนเซอร์ IR
- เปิด Serial Plotter ดูค่าที่อ่านได้
- ปรับค่า Filter ตามความต้องการ (ยิ่งมาก = กรองดี แต่หน่วงมาก)

### กิจกรรม 3.2 — Line Follower (PID)
```
< กำหนดค่า setpoint
< เซ็ตความเร็วมอเตอร์ขับเคลื่อน
< อ่านเซนเซอร์ซ้ายและกรอง
< อ่านเซนเซอร์ขวาและกรอง
< ชดเชยค่าความแตกต่าง (offset compensation)
< คำนวณความแตกต่างของค่าเซนเซอร์ทั้งสอง
< ถ้าเซนเซอร์ทั้งสองพบเส้นทึบ:
  → หยุดการเลี้ยวและการเคลื่อนที่
< ถ้าไม่ใช่:
  → คำนวณ PID
  → ปรับมุมเลี้ยวตามค่า PID
  → ขับมอเตอร์ตามความเร็วที่กำหนด
< หน่วงเวลา 20ms
```

### ทำไมต้องใช้ PID สำหรับ Line Following
- ลดการสั่าย
- เลี้ยวได้ละเอียดตามเส้นโค้ง
- เขียนโปรแกรมได้ง่ายโดยใช้บล็อกคำสั่ง
- มีประสิทธิภาพกว่าแบบ On-Off
- ข้อเสีย: ต้องหาค่า Kp, Ki, Kd ก่อน

---

## 🎮 กิจกรรม 3.3 — โหมดรวม (ทรงตัว + วิทยุ + เดินตามเส้น)

### สองโหมดการทำงาน
| โหมด | การเปิดใช้ | คำอธิบาย |
|------|-----------|---------|
| **โหมด 0** | ค่าเริ่มต้น | วิทยุบังคับ |
| **โหมด 1** | กดสวิตช์ S2 บนรีโมต | เดินตามเส้น |

> สวิตช์ S2 บนรีโมต = **S2** บนบอร์ด KidBright ฝั่ง Remote

### โครงสร้างโปรแกรมรวม
```
[ภาครับ]
- โค้ดส่วนทรงตัว: ใช้จากกิจกรรม 2.2
- โค้ดส่วนเดินตามเส้น: ใช้จากกิจกรรม 3.2
- โค้ดรับและถอดข้อมูล: เลือก mode ตาม switch

[ภาคส่ง รีโมต]
- สวิตช์ 1 บนรีโมต (S2) ใช้เลือก mode
- โหมด 0 = วิทยุบังคับ, โหมด 1 = เดินตามเส้น
```

---

## 🔌 การใช้งาน KidBright IDE สำหรับ Minibike

### บล็อกคำสั่ง Joystick (ฝั่งรีโมต)
| บล็อก | หน้าที่ |
|-------|--------|
| `อ่านค่าจริงแกน X/Y` | ใช้อ่านค่า Raw (ก่อน calibration) |
| `อ่านค่าแกน X/Y (calibrated)` | ใช้หลัง calibration แล้ว |

### บล็อกคำสั่ง ESP-NOW
| บล็อก | หน้าที่ |
|-------|--------|
| `เริ่มต้น ESP-NOW` | Init ระบบไร้สาย |
| `ส่งข้อมูล ESP-NOW` | ส่งไปยัง MAC Address ที่ระบุ |
| `รับข้อมูล ESP-NOW` | callback เมื่อมีข้อมูลเข้ามา |

### Config IDE สำหรับ Remote (KidBright IDE)
ตำแหน่งไฟล์:
```
C:\Users\ADMIN\AppData\Local\KidBright\app-1.3.2\resources\app\kbide\app\config.json
```
ต้องมี VID/PID ของ USB ให้ตรงกับบอร์ด:
```json
"port_vid_pid": [
  "04b4:0003",   // Cypress
  "0403:6015",   // FTDI
  "10c4:ea60"    // CP2102
]
```

---

## 💡 สรุปบทเรียน

| บทเรียน | เนื้อหา |
|---------|--------|
| Coding พื้นฐาน | บล็อกคำสั่ง KidBright IDE |
| เซนเซอร์และมอเตอร์ | IMU, Servo, BDC, IR |
| ระบบควบคุม | P, PD, PID |
| PID Applications | ทรงตัว Reaction Wheel, เดินตามเส้น, ควบคุมความเร็ว |
| ไร้สาย | ESP-NOW peer-to-peer |
