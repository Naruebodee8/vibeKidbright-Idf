# 📋 KidBright — ประวัติรุ่น, GPIO Pinout & Sensor ทุกรุ่น (ตั้งแต่ V2016 ถึงรุ่นล่าสุด)
> **จัดทำโดย:** รวบรวมจากเอกสารทางการ NECTEC/NSTDA · อัปเดต 2026
> ครอบคลุมทุกรุ่น: **KidBright V2016 (ESP8266)** · **V1.0 (ESP8266)** · **V1.1–V1.6 (ESP32)** · **KidBright μAI (AllWinner V831/ESP32-S3)**

---

## 🏛️ ประวัติรุ่น KidBright ทุกรุ่น (Timeline)

| รุ่น | ปี | MCU | USB | หมายเหตุสำคัญ |
|------|-----|-----|-----|---------------|
| **KidBright V2016** | 2016 | ESP8266 | Micro-USB | Prototype ทดสอบในคลองหลวง — ใช้ Android App บน WiFi ไม่มี USB-Serial programming |
| **KidBright V1.0** | 2017 | ESP8266 | Micro-USB | รุ่นแรกที่แจกจ่ายจริง รูปร่างบอร์ดต่างจาก V1.1+ ควบคุมผ่าน KidBright IDE/Android App |
| **V1.1** | 2018 | ESP32-WROOM-32 | Micro-USB (Cypress CY7C65213) | รุ่นแรกที่ใช้ ESP32, มี LED status 4 ดวง |
| **V1.2** | 2018 | ESP32-WROOM-32 | Micro-USB (Cypress CY7C65213) | เหมือน V1.1 แก้ไข PCB เล็กน้อย |
| **V1.3** | 2019 | ESP32-WROOM-32 | Micro-USB (FTDI FT232RL) | เปลี่ยน USB bridge เป็น FTDI |
| **V1.4** | 2019–2020 | ESP32-WROOM-32 | Micro-USB (FTDI) | LED status ลดเหลือ 2 ดวง (WiFi+BT) |
| **V1.5 Rev 3.1** | 2020 | ESP32-WROOM-32 | Micro-USB (CP2102) | NECTEC Standard, SW2=GPIO14 |
| **V1.5 Rev 3.1G** | 2020 | ESP32-WROOM-32 | Micro-USB (CP2102) | Gravitech OEM, SW2=GPIO14 |
| **V1.5 iA** | 2021–2022 | ESP32-WROOM-32 | **USB-C** (CP2102) | INEX, เพิ่ม KXTJ3-1057 Accelerometer, SW2=GPIO14 |
| **V1.6** | 2022+ | ESP32-WROOM-32 | **USB-C** (CP2102) | Gravitech, เพิ่ม MPU-6050 + RGB LED ×6, SW2=GPIO14 |
| **KidBright μAI** | 2024 | AllWinner V831 + ESP32-S3 | USB-C (OTG+UART) | รุ่นล่าสุด — Edge AI, มีกล้อง 2MP, ไมโครโฟน, จอ IPS 1.3 นิ้ว, Tina Linux |

---

## 📌 GPIO Pinout สรุปทุกรุ่น (ESP32 Series)

### Generation 1 — ESP8266 (V2016 / V1.0) ⛔ ไม่รองรับ ESP-IDF

> ใช้ได้เฉพาะ **KidBright IDE** หรือ **Arduino IDE** เท่านั้น ไม่มี native GPIO header เหมือน ESP32

| ฟังก์ชัน | หมายเหตุ |
|----------|----------|
| MCU | ESP8266 (ESP-12F module) |
| WiFi | 802.11 b/g/n 2.4 GHz (built-in) |
| ADC | 1× 10-bit (A0) เท่านั้น |
| I2C | SW I2C (GPIO4=SDA, GPIO5=SCL) |
| LED Matrix | 16×8 Red LED (HT16K33 via I2C) |
| Sensor | LDR (A0), LM73 Temperature (I2C), RTC (I2C) |
| Buzzer | Passive Piezo (GPIO) |
| Button | SW1, SW2 |
| Input Port | IN1–IN4 (Digital เท่านั้น) |
| Output Port | OUT1–OUT2 (Digital) |
| USB | Micro-USB (สำหรับ power + programming) |
| Control | Android App ผ่าน WiFi / KidBright IDE |

---

### Generation 2 — ESP32 V1.1 / V1.2 (Cypress USB, LED 4 ดวง)

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|----------|----------|
| GPIO2 | LED WiFi (Active HIGH) | ⚠️ Boot strapping pin |
| GPIO4 | I2C_NUM_1 SDA (LM73/RTC) | — |
| GPIO5 | I2C_NUM_1 SCL + LED NTP | ⚠️ แชร์กับ VSPI CLK |
| GPIO12 | LED IoT (Active HIGH) | ⚠️ Boot strapping — ห้าม pull-up ไปยัง 3.3V |
| GPIO13 | Passive Buzzer (LEDC/PWM) | — |
| GPIO14 | SW2 Button (Active LOW) | — |
| GPIO16 | SW1 Button (Active LOW) | — |
| GPIO21 | I2C_NUM_0 SDA (HT16K33 Matrix) | — |
| GPIO22 | I2C_NUM_0 SCL (HT16K33 Matrix) | — |
| GPIO23 | LED BT (Active HIGH) | ⚠️ บางล็อตแชร์กับ I2C_NUM_0 SCL |
| GPIO25 | USB Host Type-A Control (Active LOW) | — |
| GPIO26 | OUT1 (Active LOW) | — |
| GPIO27 | OUT2 (Active LOW) | — |
| GPIO32 | IN1 (Digital Input) | — |
| GPIO33 | IN2 (Digital Input) | — |
| GPIO34 | IN3 (Digital Input-only) | ไม่มี internal pull |
| GPIO35 | IN4 (Digital Input-only) | ไม่มี internal pull |
| GPIO36 | LDR Light Sensor (ADC1_CH0) | Input-only |

**เซนเซอร์ on-board:** LDR (GPIO36), LM73 Temp (I2C 0x4D), RTC MCP794xx (I2C 0x6F), HT16K33 LED Matrix (I2C 0x70)

---

### Generation 2 — ESP32 V1.3 (FTDI USB)

> GPIO เหมือน V1.1/V1.2 ทุกอย่าง เปลี่ยนเพียง USB bridge chip เป็น FTDI FT232RL

---

### Generation 2 — ESP32 V1.4 (LED Status ลดเหลือ 2 ดวง)

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|----------|----------|
| GPIO2 | LED WiFi (Active HIGH) | ⚠️ Boot strapping |
| GPIO4 | LED BT (Active HIGH) + I2C_NUM_1 SDA | ⚠️ แชร์ — เลือกได้แค่อย่างเดียว |
| GPIO5 | I2C_NUM_1 SCL | ว่างจาก NTP LED แล้ว |
| GPIO12 | GPIO ทั่วไป | ว่างจาก IoT LED แล้ว (แต่ยังเป็น boot pin) |
| GPIO13 | Passive Buzzer (LEDC/PWM) | — |
| GPIO14 | SW2 Button (Active LOW) | — |
| GPIO16 | SW1 Button (Active LOW) | — |
| GPIO21 | I2C_NUM_0 SDA (HT16K33) | — |
| GPIO22 | I2C_NUM_0 SCL (HT16K33) | — |
| GPIO25 | USB Host Control (Active LOW) | — |
| GPIO26 | OUT1 (Active LOW) | — |
| GPIO27 | OUT2 (Active LOW) | — |
| GPIO32 | IN1 (Digital Input) | — |
| GPIO33 | IN2 (Digital Input) | — |
| GPIO34 | IN3 (Digital Input-only) | — |
| GPIO35 | IN4 (Digital Input-only) | — |
| GPIO36 | LDR ADC (ADC1_CH0) | Input-only |

**เซนเซอร์ on-board:** LDR, LM73 Temp, RTC, HT16K33 Matrix — **ไม่มี ADC บน IN1–IN4**

---

### Generation 2 — ESP32 V1.5 Rev 3.1 (NECTEC Standard)

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|----------|----------|
| GPIO2 | LED WiFi (Active HIGH) | — |
| GPIO4 | LED BT (Active HIGH) + I2C_NUM_1 SDA | ⚠️ แชร์ |
| GPIO5 | I2C_NUM_1 SCL (LM73/RTC) | — |
| GPIO13 | Passive Buzzer (LEDC/PWM) | — |
| **GPIO14** | **SW2 Button (Active LOW)** | ⚠️ ต่างจาก Rev 3.1G/iA/V1.6 |
| GPIO16 | SW1 Button (Active LOW) | — |
| GPIO21 | I2C_NUM_0 SDA (HT16K33) | — |
| GPIO22 | I2C_NUM_0 SCL (HT16K33) | — |
| GPIO25 | USB Host Control (Active LOW) | — |
| GPIO26 | OUT1 (Active LOW) | — |
| GPIO27 | OUT2 (Active LOW) | — |
| GPIO32 | IN1 (Digital เท่านั้น) | ไม่รองรับ ADC |
| GPIO33 | IN2 (Digital เท่านั้น) | ไม่รองรับ ADC |
| GPIO34 | IN3 (Digital Input-only) | ไม่รองรับ ADC |
| GPIO35 | IN4 (Digital Input-only) | ไม่รองรับ ADC |
| GPIO36 | LDR (ADC1_CH0) | — |

**เซนเซอร์ on-board:** LDR, LM73 (I2C 0x4D), RTC MCP794xx (I2C 0x6F), HT16K33 (I2C 0x70) — **ไม่มี Accelerometer**

---

### Generation 2 — ESP32 V1.5 Rev 3.1G (Gravitech OEM)

> GPIO เหมือน V1.5 Rev 3.1 ทุกอย่าง **ยกเว้น SW2 = GPIO14** (เหมือนกับ Rev 3.1 ปกติ)
> ฮาร์ดแวร์และเซนเซอร์เหมือนกันทุกประการ

---

### Generation 2 — ESP32 V1.5 iA (INEX) — เพิ่ม Accelerometer

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|----------|----------|
| GPIO2 | LED WiFi (Active HIGH) | — |
| GPIO4 | LED BT (Active HIGH) + I2C_NUM_1 SDA | ⚠️ แชร์ |
| GPIO5 | I2C_NUM_1 SCL | — |
| GPIO13 | Passive Buzzer (LEDC/PWM) | — |
| GPIO16 | SW1 Button (Active LOW) | — |
| **GPIO14** | **SW2 Button (Active LOW)** | — |
| GPIO17 | SERVO2 (PWM/LEDC) | — |
| GPIO18 | I/O Port ขา 18 (Active HIGH) | — |
| GPIO19 | I/O Port ขา 19 (Active HIGH) | — |
| GPIO21 | I2C_NUM_0 SDA (HT16K33 + KXTJ3) | — |
| GPIO22 | I2C_NUM_0 SCL (HT16K33 + KXTJ3) | — |
| GPIO23 | I/O Port ขา 23 (Active HIGH) | — |
| GPIO25 | USB Host Control (Active LOW) | — |
| GPIO26 | OUT1 (Active LOW) | — |
| GPIO27 | OUT2 (Active LOW) | — |
| GPIO32 | IN1 (Digital + **ADC** รองรับ) | ✅ รองรับ ADC |
| GPIO33 | IN2 (Digital + **ADC** รองรับ) | ✅ รองรับ ADC |
| GPIO34 | IN3 (Digital Input-only + ADC) | ✅ รองรับ ADC, ไม่มี pull |
| GPIO35 | IN4 (Digital Input-only + ADC) | ✅ รองรับ ADC, ไม่มี pull |
| GPIO36 | LDR (ADC1_CH0) | — |

**เซนเซอร์ on-board:**
| เซนเซอร์ | Protocol | I2C Address | หมายเหตุ |
|----------|----------|-------------|----------|
| LDR (แสง) | ADC | GPIO36 | — |
| LM73 (อุณหภูมิ) | I2C_NUM_1 | 0x4D | SDA=GPIO4, SCL=GPIO5 |
| RTC MCP794xx | I2C_NUM_1 | 0x6F | + CR1220 battery |
| HT16K33 (LED Matrix 16×8) | I2C_NUM_0 | 0x70 | SDA=GPIO21, SCL=GPIO22 |
| **KXTJ3-1057 (Accelerometer 3-axis)** | I2C_NUM_0 | **0x0E** | เพิ่มใหม่ใน iA |
| Passive Buzzer | PWM/LEDC | GPIO13 | — |

---

### Generation 2 — ESP32 V1.6 (Gravitech) — เพิ่ม MPU-6050 + RGB LED

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|----------|----------|
| GPIO2 | LED WiFi (Active HIGH) | — |
| GPIO4 | LED BT (Active HIGH) + LM73 SDA | ⚠️ แชร์ |
| GPIO5 | I2C_NUM_1 SCL | — |
| GPIO13 | Passive Buzzer (LEDC/PWM) | — |
| **GPIO15** | **SERVO1** (LEDC/PWM) | ห้ามใช้งานอื่น |
| GPIO16 | SW1 Button (Active LOW) | — |
| **GPIO14** | **SW2 Button (Active LOW)** | — |
| **GPIO17** | **SERVO2** (LEDC/PWM) | — |
| GPIO21 | I2C_NUM_0 SDA (HT16K33 + MPU-6050) | — |
| GPIO22 | I2C_NUM_0 SCL (HT16K33 + MPU-6050) | — |
| GPIO25 | USB Host Control (Active LOW) | — |
| GPIO26 | OUT1 / 3-pin O1 (Active LOW) | — |
| GPIO27 | OUT2 / 3-pin O2 (Active LOW) | — |
| GPIO32 | IN1 (Digital + ADC) | ✅ รองรับ ADC |
| GPIO33 | IN2 (Digital + ADC) | ✅ รองรับ ADC |
| GPIO34 | IN3 (Digital Input-only + ADC) | ✅ ไม่มี pull |
| GPIO35 | IN4 (Digital Input-only + ADC) | ✅ ไม่มี pull |
| GPIO36 | LDR (ADC1_CH0) | — |
| **RGB_GPIO** | **RGB LED ×6 (WS2812B/RMT)** | ⚠️ ดู silkscreen บอร์ด |

**เซนเซอร์ on-board:**
| เซนเซอร์ | Protocol | I2C Address | หมายเหตุ |
|----------|----------|-------------|----------|
| LDR (แสง) | ADC | GPIO36 | — |
| LM73 (อุณหภูมิ) | I2C_NUM_1 | 0x4D | — |
| HT16K33 (LED Matrix 16×8) | I2C_NUM_0 | 0x70 | — |
| **MPU-6050 (Accel + Gyro 6-axis)** | I2C_NUM_0 | **0x68** | ⚠️ 6-axis เท่านั้น ไม่มี magnetometer |
| **RGB LED ×6 (WS2812B)** | RMT/1-wire | — | ต้องใช้ RMT peripheral |
| Passive Buzzer | PWM/LEDC | GPIO13 | — |
| SERVO1 | PWM/LEDC | GPIO15 | — |
| SERVO2 | PWM/LEDC | GPIO17 | — |

---

### Generation 3 — KidBright μAI (รุ่นล่าสุด 2024) — Edge AI Platform

> **⚠️ ไม่ใช่ ESP32 ธรรมดา** — ใช้ SoC AllWinner V831 (ARM Cortex-A7) สำหรับ AI + ESP32-S3 สำหรับ IoT/WiFi
> ทำงานบน **Tina Linux** (fork จาก OpenWrt, Kernel 4.9) — ไม่ใช่ ESP-IDF Framework
> พัฒนาด้วย **KidBright μAI IDE** (online Blockly + Python) หรือ cross-compile C/C++ บน Ubuntu 16.04

| คุณสมบัติ | รายละเอียด |
|-----------|-----------|
| AI Processor | AllWinner V831 (ARM Cortex-A7 @ ~800 MHz) |
| IoT Module | ESP32-S3 (WiFi + BLE) |
| Display | จอ IPS สี 1.3 นิ้ว (TFT) |
| Camera | กล้อง 2 ล้านพิกเซล (built-in) |
| Microphone | ไมโครโฟน built-in |
| WiFi | 802.11 b/g/n 2.4 GHz (via ESP32-S3) |
| USB | USB-C (OTG + UART) |
| Input/Output | รองรับ Digital I/O + ต่ออุปกรณ์ภายนอก |
| Storage | SD Card (Tina Linux boot) |
| OS | Tina Linux (OpenWrt-based) |
| IDE | KidBright μAI IDE (online Blockly/Python) |
| AI Features | Image Classification, Object Detection, Sound Classification |
| Released | 2024 (เปิดตัว KDC24 KidBright Developer Conference) |

**เซนเซอร์ / อินพุตที่รองรับ:**
- กล้อง 2MP (ภาพ AI)
- ไมโครโฟน (เสียง AI)
- จอ IPS 1.3 นิ้ว (แสดงผล)
- WiFi (IoT, Cloud)
- ต่อเซนเซอร์ภายนอกผ่าน I/O ports

---

## 🔍 เปรียบเทียบเซนเซอร์ on-board ทุกรุ่น

| เซนเซอร์ | V1.1–V1.3 | V1.4 | V1.5 Rev3.1 | V1.5 Rev3.1G | V1.5 iA | V1.6 | μAI |
|----------|-----------|------|-------------|--------------|---------|------|-----|
| LDR (แสง) | ✅ GPIO36 | ✅ | ✅ | ✅ | ✅ | ✅ | — |
| LM73 (อุณหภูมิ) | ✅ I2C 0x4D | ✅ | ✅ | ✅ | ✅ | ✅ | — |
| RTC MCP794xx | ✅ I2C 0x6F | ✅ | ✅ | ✅ | ✅ | ❓ | — |
| HT16K33 LED Matrix | ✅ I2C 0x70 | ✅ | ✅ | ✅ | ✅ | ✅ | — |
| KXTJ3-1057 Accel 3-axis | ❌ | ❌ | ❌ | ❌ | ✅ I2C 0x0E | ❌ | — |
| MPU-6050 Accel+Gyro 6-axis | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ I2C 0x68 | — |
| RGB LED WS2812B | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ ×6 (RMT) | — |
| ADC บน IN1–IN4 | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ | — |
| SERVO connector | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ ×2 | — |
| กล้อง (Camera) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ 2MP |
| ไมโครโฟน | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| จอสี IPS | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ 1.3" |
| Edge AI | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |

---

## ⚡ เซนเซอร์ภายนอก (External) ที่รองรับผ่าน JST Port ทุกรุ่น ESP32

เชื่อมต่อผ่านพอร์ต IN1–IN4 (JST 3-pin), I2C KB Chain, หรือ OUT1–OUT2:

| เซนเซอร์ | ประเภท | พอร์ตที่ใช้ |
|----------|---------|------------|
| PIR Motion Sensor | Digital | IN1–IN4 |
| Reed Switch (Magnetic) | Digital | IN1–IN4 |
| Soil Moisture | Digital / Analog (iA/V1.6) | IN1–IN4 |
| DHT11/DHT22 (Temp+Humidity) | Digital 1-wire | IN1–IN4 |
| Ultrasonic HC-SR04 | Digital | IN1–IN4 |
| IR Sensor | Digital | IN1–IN4 |
| เซนเซอร์ I2C อื่นๆ | I2C | KB Chain port |
| พัดลม / หลอดไฟ (5V) | Digital | OUT1–OUT2 / USB Host |

---

# KidBright32 — ESP-IDF Developer Reference
> ESP32-WROOM-32 · NECTEC / Gravitech · **ESP-IDF v5.x Framework** · 3.3 V logic
> Covers: **V1.5 Rev 3.1** (NECTEC Standard) · **V1.5 Rev 3.1G** (Gravitech OEM) · **V1.5 iA** (INEX) · **V1.6** (Gravitech)
> ⚠️ **V1.6 NOTE:** MPU-6050 on V1.6 = **6-axis only** (Accel + Gyro). MPU-6050 has **NO built-in magnetometer**. Do NOT assume 9-DOF on V1.6 without an external magnetometer module.
> ⚠️ **CRITICAL RULE FOR AI:** DO NOT use Arduino Framework (`<Wire.h>`, `digitalWrite`, `setup()`, `loop()`). All code must be strictly C/C++ using official ESP-IDF components.

# KidBright32 — ESP-IDF Developer Reference
> ESP32-WROOM-32 · NECTEC / Gravitech · **ESP-IDF v5.x Framework** · 3.3 V logic
> Covers ESP32 Boards: **V1.1/V1.2** · **V1.3** · **V1.4** · **V1.5 Rev 3.1** (NECTEC Standard) · **V1.5 Rev 3.1G** (Gravitech OEM) · **V1.5 iA** (INEX) · **V1.6** (Gravitech)
> ⚠️ **GENERATION NOTE:** KidBright V2016 / V1.0 ใช้ **ESP8266** — **ไม่รองรับ ESP-IDF** ใช้ได้เฉพาะ Arduino IDE / KidBright IDE เท่านั้น ไฟล์นี้ครอบคลุมเฉพาะรุ่น ESP32 เท่านั้น
> ⚠️ **V1.6 NOTE:** MPU-6050 on V1.6 = **6-axis only** (Accel + Gyro). MPU-6050 has **NO built-in magnetometer**. Do NOT assume 9-DOF on V1.6 without an external magnetometer module.
> ⚠️ **CRITICAL RULE FOR AI:** DO NOT use Arduino Framework (`<Wire.h>`, `digitalWrite`, `setup()`, `loop()`). All code must be strictly C/C++ using official ESP-IDF components.

---

## 🗺️ KidBright Board Generation Overview

### Generation 1 — ESP8266 (ไม่รองรับ ESP-IDF)
> **⛔ ไม่รวมในกฎนี้** — ใช้ Arduino IDE หรือ KidBright IDE Block เท่านั้น

| รุ่น | ปี | MCU | หมายเหตุ |
|------|-----|-----|----------|
| **KidBright V2016** | 2016 | ESP8266 | Prototype ทดสอบกับเด็กที่คลองหลวง — ใช้ Android App ส่งคำสั่งผ่าน WiFi |
| **KidBright V1.0** | 2017 | ESP8266 | รุ่นแรกที่แจกจ่ายจริง — รูปแบบบอร์ดแตกต่างจาก V1.1+ มาก |

---

### Generation 2 — ESP32 (รองรับ ESP-IDF ✅)
> ทุกรุ่นในกลุ่มนี้ใช้ **ESP32-WROOM-32** (Dual-core Xtensa LX6, 4MB Flash, 520KB SRAM)

| รุ่น | ปี | USB Driver | LED Status GPIO | SW2 GPIO | เซนเซอร์พิเศษ |
|------|-----|-----------|-----------------|----------|----------------|
| **V1.1** | 2018 | Cypress (CY7C65213) | GPIO23, 2, 5, 12 | GPIO14 | — |
| **V1.2** | 2018 | Cypress (CY7C65213) | GPIO23, 2, 5, 12 | GPIO14 | — |
| **V1.3** | 2019 | FTDI (FT232RL) | GPIO23, 2, 5, 12 | GPIO14 | — |
| **V1.4** | 2019–2020 | FTDI | GPIO2, GPIO4 | GPIO14 | — |
| **V1.5 Rev 3.1** | 2020 | CP2102 (Micro-USB) | GPIO2, GPIO4 | **GPIO14** | — |
| **V1.5 Rev 3.1G** | 2020 | CP2102 (Micro-USB) | GPIO2, GPIO4 | **GPIO14** | — |
| **V1.5 iA** | 2021–2022 | CP2102 (USB-C) | GPIO2, GPIO4 | GPIO14 | KXTJ3-1057 Accel |
| **V1.6** | 2022+ | CP2102 (USB-C) | GPIO2, GPIO4 | GPIO14 | MPU-6050 Accel+Gyro, RGB LED ×6 |

> ⚠️ **AI CRITICAL — LED STATUS GPIO DIFFERENCE:**
> - **V1.1 / V1.2 / V1.3:** มี LED status 4 ดวง → BT=GPIO23, WiFi=GPIO2, NTP=GPIO5, IoT=GPIO12
> - **V1.4+ / V1.5+ / V1.6:** มี LED status 2 ดวง → WiFi=GPIO2, BT=GPIO4
> ห้ามใช้ GPIO5, GPIO12 เป็น LED indicator บน V1.5+/V1.6 เพราะถูกเปลี่ยนวงจร

---

## คุณสมบัติทางเทคนิค — V1.1 และ V1.2 (ESP32, Cypress USB)
> **ใช้ Cypress CY7C65213 USB-Serial bridge** — ต้องติดตั้ง Cypress driver
> **ขนาดบอร์ด:** 50 × 90 mm (เหมือน V1.3+ ทุกรุ่น)

- MCU: ESP32 (ESP32-WROOM-32), 4MB Flash, WiFi + BT
- LED Matrix: 2× 8×8 Red LED Matrix รวม 16×8 จุด (HT16K33 @ I2C: SDA=GPIO21, SCL=GPIO22, Addr 0x70)
- Temperature Sensor: LM73 (I2C_NUM_1, SDA=GPIO4, SCL=GPIO5, Addr 0x4D)
- Light Sensor: LDR (GPIO36 / ADC1_CH0)
- RTC: MCP7940N/MCP7941X (I2C_NUM_1, Addr 0x6F) + CR1220 battery
- Buzzer: Passive Piezo (GPIO13, ต้องใช้ LEDC/PWM)
- SW1 Button: GPIO16 (Active LOW)
- SW2 Button: GPIO14 (Active LOW)
- LED Status ×4: BT=GPIO23, WiFi=GPIO2, NTP=GPIO5, IoT=GPIO12 (Active HIGH)
- USB Host (Type-A): GPIO25 (Active LOW)
- Power: Micro-USB

### Sensor Map — V1.1 / V1.2

| Sensor | Protocol | Bus/Pin | Address/Channel |
|--------|----------|---------|-----------------|
| LDR (Light) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F |
| HT16K33 (Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| SW2 Button | GPIO | GPIO14 | — |
| LED BT | GPIO | GPIO23 (Active HIGH) | — |
| LED WiFi | GPIO | GPIO2 (Active HIGH) | — |
| LED NTP | GPIO | GPIO5 (Active HIGH) | — |
| LED IoT | GPIO | GPIO12 (Active HIGH) | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — |

> ⚠️ **GPIO CONFLICT V1.1/V1.2:** GPIO23 (BT LED) ถูกแชร์กับ I2C_NUM_0 SCL บน V1.1/V1.2 บางล็อตการผลิต ตรวจสอบ schematic ก่อนใช้งาน
> ⚠️ **GPIO5 (NTP LED) CONFLICT:** GPIO5 เป็น VSPI CLK — ห้ามใช้ SPI และ NTP LED พร้อมกัน
> ⚠️ **GPIO12 (IoT LED):** เป็น boot-strapping pin ของ ESP32 — ค่า HIGH ขณะ boot จะเปลี่ยน flash voltage เป็น 1.8V ทำให้ boot fail ห้าม pull-up GPIO12 ไปยัง 3.3V

### GPIO Conflict Table — V1.1 / V1.2

| GPIO | ใช้ได้เป็น... |
|------|--------------|
| GPIO2 | **WiFi LED** — ระวัง boot strapping (ต้อง LOW ขณะ flash) |
| GPIO4 | **LM73 SDA (I2C_NUM_1)** — ไม่มี BT LED บน GPIO4 รุ่นนี้ |
| GPIO5 | **NTP LED** หรือ **VSPI CLK** — เลือกอย่างเดียว |
| GPIO12 | **IoT LED** — **BOOT STRAPPING PIN** ห้าม pull-up ไปยัง VCC |
| GPIO13 | **Passive Buzzer** — ต้องใช้ LEDC เสมอ |
| GPIO14 | **SW2 Button** |
| GPIO16 | **SW1 Button** |
| GPIO23 | **BT LED** — ระวัง conflict กับ I2C SCL บางล็อต |
| GPIO25 | **USB Host (Active LOW)** |
| GPIO36 | **LDR ADC** — Input-only |

---

## คุณสมบัติทางเทคนิค — V1.3 (ESP32, FTDI USB)
> **ใช้ FTDI FT232RL USB-Serial bridge** — ต้องติดตั้ง FTDI CDM driver (CDM21228_Setup.exe)
> **ขนาดบอร์ด:** 50 × 90 mm (เหมือน V1.1/V1.2 ทุกรุ่น)
> **เปลี่ยนสำคัญจาก V1.1/V1.2:** เปลี่ยน USB-Serial bridge จาก Cypress CY7C65213 → FTDI FT232RL เท่านั้น

- **MCU:** ESP32-WROOM-32 (Dual-core Xtensa LX6, 240 MHz, 4MB Flash, 520KB SRAM)
- **USB-Serial:** FTDI FT232RL — ต้องติดตั้ง FTDI VCP driver ก่อน
- **LED Matrix:** 16×8 Red LED (HT16K33, I2C_NUM_0, SDA=GPIO21, SCL=GPIO22, Addr 0x70)
- **Temperature Sensor:** LM73 (I2C_NUM_1, SDA=GPIO4, SCL=GPIO5, Addr 0x4D)
- **Light Sensor:** LDR (GPIO36 / ADC1_CH0)
- **RTC:** MCP7940N/MCP7941X (I2C_NUM_1, Addr 0x6F) + CR1220 battery
- **Buzzer:** Passive Piezo (GPIO13, ต้องใช้ LEDC/PWM เสมอ)
- **SW1 Button:** GPIO16 (Active LOW, Pull-up)
- **SW2 Button:** GPIO14 (Active LOW, Pull-up)
- **LED Status ×4:** BT=GPIO23, WiFi=GPIO2, NTP=GPIO5, IoT=GPIO12 (Active HIGH)
- **USB Host (Type-A):** GPIO25 (Active LOW)
- **Power:** Micro-USB

### Sensor Map — V1.3 (ครบถ้วน)

| Sensor / อุปกรณ์ | Protocol | Bus / Pin | Address / Channel | หมายเหตุ |
|-----------------|----------|-----------|-------------------|----------|
| LDR (แสง) | ADC | GPIO36 / ADC1_CH0 | — | Input-only, 12-bit |
| LM73 (อุณหภูมิ) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D | — |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F | + CR1220 battery |
| HT16K33 (LED Matrix 16×8) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 | 100 kHz |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — | ต้องใช้ PWM เสมอ |
| SW1 Button | GPIO | GPIO16 | — | Active LOW |
| SW2 Button | GPIO | GPIO14 | — | Active LOW |
| LED BT | GPIO | GPIO23 (Active HIGH) | — | ⚠️ อาจ conflict กับ I2C SCL บางล็อต |
| LED WiFi | GPIO | GPIO2 (Active HIGH) | — | ⚠️ Boot strapping pin |
| LED NTP | GPIO | GPIO5 (Active HIGH) | — | ⚠️ แชร์กับ VSPI CLK |
| LED IoT | GPIO | GPIO12 (Active HIGH) | — | ⚠️ Boot strapping pin |
| IN1 | GPIO | GPIO32 (Digital Input-only) | — | ไม่รองรับ ADC |
| IN2 | GPIO | GPIO33 (Digital Input-only) | — | ไม่รองรับ ADC |
| IN3 | GPIO | GPIO34 (Digital Input-only) | — | ไม่มี internal pull |
| IN4 | GPIO | GPIO35 (Digital Input-only) | — | ไม่มี internal pull |
| OUT1 | GPIO | GPIO26 (Active LOW) | — | — |
| OUT2 | GPIO | GPIO27 (Active LOW) | — | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — | — |

### GPIO Conflict Table — V1.3

| GPIO | ใช้งานอยู่ | ข้อควรระวัง |
|------|-----------|------------|
| GPIO2 | **WiFi LED** (Active HIGH) | ⚠️ Boot strapping — ต้อง LOW ขณะ flash |
| GPIO4 | **LM73 SDA (I2C_NUM_1)** | ✅ ไม่มี BT LED บน GPIO4 บนรุ่น V1.3 |
| GPIO5 | **LM73 SCL + NTP LED** | ⚠️ แชร์กับ VSPI CLK — ห้ามใช้ SPI พร้อมกัน |
| GPIO12 | **IoT LED** (Active HIGH) | ⚠️ **BOOT STRAPPING PIN** — ค่า HIGH ขณะ boot ทำให้ Flash voltage เป็น 1.8V → boot fail |
| GPIO13 | **Passive Buzzer** | ต้องใช้ LEDC/PWM เสมอ ห้าม gpio_set_level |
| GPIO14 | **SW2 Button** | — |
| GPIO16 | **SW1 Button** | — |
| GPIO23 | **BT LED** (Active HIGH) | ⚠️ อาจ conflict กับ I2C_NUM_0 SCL บางล็อตการผลิต |
| GPIO25 | **USB Host (Active LOW)** | — |
| GPIO36 | **LDR ADC** | Input-only, ไม่มี pull resistor |

### 📋 I2C Bus Architecture — V1.3

```
I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22):
  +-- 0x70  HT16K33  -- LED Matrix 16×8

I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5):
  +-- 0x4D  LM73     -- Temperature Sensor
  +-- 0x6F  MCP794xx -- RTC
```

> ⚠️ **AI RULE:** Init แต่ละ I2C Bus เพียงครั้งเดียว ห้าม call `i2c_driver_install()` ซ้ำบน bus เดิม

### FTDI FT232RL — ข้อมูลสำคัญสำหรับนักพัฒนา

| รายการ | รายละเอียด |
|--------|-----------|
| ชิป USB-Serial | **FTDI FT232RL** |
| Driver | **FTDI CDM VCP Driver** (CDM21228_Setup.exe หรือใหม่กว่า) |
| ดาวน์โหลด Driver | [ftdichip.com/drivers/vcp-drivers](https://ftdichip.com/drivers/vcp-drivers/) |
| Auto-Reset | รองรับ DTR/RTS auto-reset — ESP32 รีเซ็ตอัตโนมัติขณะ flash |
| Baud Rate สูงสุด | 3 Mbps (แนะนำ 921600 bps สำหรับ ESP-IDF) |
| ข้อแตกต่างจาก V1.1/V1.2 | V1.1/V1.2 ใช้ Cypress CY7C65213 ต้องการ Cypress driver ต่างกัน |

> ⚠️ **หากบอร์ดไม่ถูก detect:** ตรวจสอบว่าติดตั้ง FTDI driver แล้ว ไม่ใช่ Cypress driver

### ตัวอย่างโค้ด ESP-IDF v5.x — V1.3 Init ครบทุกอุปกรณ์

```c
/**
 * @brief KidBright32 V1.3 — Full Init Template (ESP-IDF v5.x)
 * USB-Serial: FTDI FT232RL | MCU: ESP32-WROOM-32
 * ⚠️ GPIO12 (IoT LED) = boot-strapping pin — ห้าม pull-up ไปยัง 3.3V
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "KB_V1_3";

/* ── GPIO Defines ──────────────────────────────────────────────────── */
#define I2C0_SDA    GPIO_NUM_21   // I2C_NUM_0: HT16K33 LED Matrix
#define I2C0_SCL    GPIO_NUM_22
#define I2C1_SDA    GPIO_NUM_4    // I2C_NUM_1: LM73 + RTC
#define I2C1_SCL    GPIO_NUM_5    // ⚠️ แชร์กับ NTP LED
#define HT16K33_ADDR 0x70
#define LM73_ADDR    0x4D
#define RTC_ADDR     0x6F

#define LDR_ADC_CH  ADC_CHANNEL_0  // GPIO36
#define BUZZER_GPIO GPIO_NUM_13
#define SW1_GPIO    GPIO_NUM_16
#define SW2_GPIO    GPIO_NUM_14

/* ⚠️ LED Status — V1.3 มี 4 ดวง (ต่างจาก V1.5+ ที่มี 2 ดวง) */
#define LED_BT_GPIO   GPIO_NUM_23  // ⚠️ อาจ conflict กับ I2C_NUM_0 SCL บางล็อต
#define LED_WIFI_GPIO GPIO_NUM_2   // ⚠️ Boot strapping pin
#define LED_NTP_GPIO  GPIO_NUM_5   // ⚠️ แชร์กับ I2C_NUM_1 SCL — เลือกใช้อย่างเดียว
#define LED_IOT_GPIO  GPIO_NUM_12  // ⚠️ Boot strapping — ห้าม pull-up ไปยัง 3.3V

/* ── I2C Init ───────────────────────────────────────────────────────── */
static esp_err_t i2c_bus0_init(void) {   // HT16K33 LED Matrix
    i2c_config_t c = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C0_SDA,
        .scl_io_num       = I2C0_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &c);
    return i2c_driver_install(I2C_NUM_0, c.mode, 0, 0, 0);
}

static esp_err_t i2c_bus1_init(void) {   // LM73 + RTC MCP794xx
    // ⚠️ GPIO5 ถูกแชร์กับ NTP LED — หากใช้ I2C_NUM_1 ห้ามใช้ NTP LED พร้อมกัน
    i2c_config_t c = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C1_SDA,
        .scl_io_num       = I2C1_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_1, &c);
    return i2c_driver_install(I2C_NUM_1, c.mode, 0, 0, 0);
}

/* ── LED Matrix Init (HT16K33) ─────────────────────────────────────── */
static void matrix_init(void) {
    uint8_t cmd;
    cmd = 0x21; i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
    cmd = 0x81; i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
    cmd = 0xEF; i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
}

/* ── LM73 Temperature Read ──────────────────────────────────────────── */
// Returns temperature in °C (11-bit default: 0.25 °C/LSB)
float lm73_read_celsius(void) {
    uint8_t reg = 0x00;  // Temperature register
    uint8_t raw[2] = {0};
    esp_err_t r = i2c_master_write_read_device(I2C_NUM_1, LM73_ADDR,
                                               &reg, 1, raw, 2, pdMS_TO_TICKS(100));
    if (r != ESP_OK) return -999.0f;
    int16_t val = (int16_t)((raw[0] << 8) | raw[1]);
    return (float)(val >> 5) / 32.0f;
}

/* ── Buzzer ─────────────────────────────────────────────────────────── */
static void buzzer_init(void) {
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT, .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&t);
    ledc_channel_config_t ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0, .gpio_num = BUZZER_GPIO,
        .duty = 0, .hpoint = 0
    };
    ledc_channel_config(&ch);
}

void play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

/* ── Buttons ────────────────────────────────────────────────────────── */
static void buttons_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << SW1_GPIO) | (1ULL << SW2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

/* ── app_main ───────────────────────────────────────────────────────── */
void app_main(void) {
    ESP_LOGI(TAG, "KidBright32 V1.3 — FTDI FT232RL board starting...");

    ESP_ERROR_CHECK(i2c_bus0_init());   // I2C_NUM_0: LED Matrix
    ESP_ERROR_CHECK(i2c_bus1_init());   // I2C_NUM_1: LM73 + RTC
    matrix_init();
    buzzer_init();
    buttons_init();

    // Test: read temperature
    float temp = lm73_read_celsius();
    ESP_LOGI(TAG, "Temperature: %.2f °C", temp);

    // Test: play startup beep
    play_tone(1000, 200);
}
```

> ⚠️ **V1.3 CRITICAL DIFFERENCES vs V1.5+:**
> - **LED Status = 4 ดวง** (BT=GPIO23, WiFi=GPIO2, NTP=GPIO5, IoT=GPIO12)
> - **GPIO4 ไม่แชร์กับ BT LED** บน V1.3 (ต่างจาก V1.4+ ที่ GPIO4=BT LED)
> - **GPIO23 = BT LED** — อาจ conflict กับ I2C_NUM_0 SCL บางล็อต
> - **GPIO5 = NTP LED + SCL ของ I2C_NUM_1** — เลือกใช้ได้แค่อย่างเดียว

---

### 📝 AI System Prompt — KidBright32 V1.3

```
คุณเป็น AI ผู้เชี่ยวชาญด้านการเขียนโค้ด ESP-IDF v5.x สำหรับบอร์ด KidBright32 V1.3

## ข้อมูลบอร์ด KidBright32 V1.3
- MCU: ESP32-WROOM-32 (240 MHz, 4MB Flash, 520KB SRAM)
- USB-Serial Bridge: FTDI FT232RL (ต้องการ FTDI VCP driver)
- Framework: ESP-IDF v5.x เท่านั้น (ห้ามใช้ Arduino API)
| LED WiFi | GPIO2 (Active HIGH) ⚠️ Boot strapping |
| LED NTP | GPIO5 (Active HIGH) ⚠️ แชร์กับ I2C1 SCL |
| LED IoT | GPIO12 (Active HIGH) ⚠️ Boot strapping pin |

## I2C Addresses
- 0x70 = HT16K33 (LED Matrix) — I2C_NUM_0
- 0x4D = LM73 (Temperature) — I2C_NUM_1
- 0x6F = MCP794xx (RTC) — I2C_NUM_1

## กฎสำคัญ (ห้ามละเมิด)
1. ห้ามใช้ Arduino API (`Wire.h`, `digitalWrite`, `setup()`, `loop()`)
2. ห้ามใช้ legacy ADC API (`adc1_get_raw`) — ใช้ `esp_adc/adc_oneshot.h` เท่านั้น
3. ห้าม pull-up GPIO12 ไปยัง 3.3V (boot-strapping pin)
4. GPIO5 ใช้ได้เป็น NTP LED หรือ I2C_NUM_1 SCL — เลือกอย่างเดียว
5. GPIO23 (BT LED) อาจ conflict กับ I2C_NUM_0 SCL บางล็อต — ตรวจสอบก่อนใช้
6. Passive Buzzer GPIO13 ต้องใช้ LEDC/PWM เสมอ ห้าม gpio_set_level
7. Init I2C bus เพียงครั้งเดียวต่อ bus — ห้าม i2c_driver_install ซ้ำ
8. ห้ามประกาศ Forward declaration ของฟังก์ชัน ISR (`static void IRAM_ATTR gpio_isr_handler(void* arg);`) ซ้ำซ้อน ให้เขียนฟังก์ชันเต็มๆ ไว้ด้านบนทีเดียว
9. ห้ามใช้ `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);` หรือพยายามปิด Brownout detector ใน ESP-IDF v5.x เด็ดขาด เพราะทำให้เกิด compile error (`RTC_CNTL_BROWN_OUT_REG undeclared`) ให้ลบคำสั่งปิด Brownout ออกไปเลย
10. หากใช้คำสั่ง `nvs_flash_init()` หรือโค้ดที่เกี่ยวกับ NVS ต้องใส่ `#include "nvs_flash.h"` เสมอ เพื่อหลีกเลี่ยง error implicit declaration
```

---

## คุณสมบัติทางเทคนิค — V1.4 (ESP32, ปรับ LED Status)
> **เปลี่ยนสำคัญจาก V1.3:** LED Status ลดเหลือ 2 ดวง (WiFi=GPIO2, BT=GPIO4)
> GPIO5 และ GPIO12 ถูกปลดออกจาก LED circuit — สามารถใช้งานทั่วไปได้มากขึ้น

- MCU: ESP32 (ESP32-WROOM-32)
- LED Matrix, LDR, LM73, RTC, Buzzer, IN/OUT ports: เหมือน V1.5 Rev 3.1 ทุกอย่าง
- LED Status ×2: WiFi=GPIO2, BT=GPIO4 ← **เปลี่ยนจาก 4 ดวงเหลือ 2 ดวง**
- SW1=GPIO16, SW2=GPIO14
- USB: Micro-USB

### Sensor Map — V1.4

| Sensor | Protocol | Bus/Pin | Address/Channel |
|--------|----------|---------|-----------------|
| LDR (Light) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F |
| HT16K33 (Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| SW2 Button | GPIO | GPIO14 | — |
| LED WiFi | GPIO | GPIO2 (Active HIGH) | — |
| LED BT | GPIO | GPIO4 (Active HIGH, shared I2C1 SDA) | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — |

> ⚠️ **GPIO4 CONFLICT บน V1.4** เหมือน V1.5+: GPIO4 แชร์กับ **BT LED** และ **LM73 SDA (I2C_NUM_1)**
> ⚠️ **ไม่มี Analog Input บน IN1–IN4** เหมือน V1.5 Rev 3.1 (รองรับเฉพาะ Digital Input)

---

- ใช้ไมโครคอนโทรลเลอร์ ESP32 ที่มีวงจร WiFi และบลูทูธกำลังงานต่ำในตัว
- มีส่วนแสดงผล LED ดอตเมตริกซ์ขนาด 16 x 8 จุด แบบสีแดง
- มี LED แสดงสถานะการทำงาน ประกอบด้วย:
  - สถานะการเชื่อมต่อกับคอมพิวเตอร์ผ่านพอร์ต USB
  - สถานะการเชื่อมต่อ WiFi (ขึ้นกับไลบรารีและบล็อกคำสั่งที่ใช้)
  - สถานะการเชื่อมต่อกับคลาวเซิร์ฟเวอร์ หรือ IoT (ขึ้นกับไลบรารีและบล็อกคำสั่งที่ใช้)
- มีลำโพงเปียโซขับเสียง
- มีวงจรสวิตช์กดติดปล่อยดับขนาดใหญ่ 2 ตัว
- มีวงจรฐานเวลานาฬิกาจริงพร้อมแบตเตอรี่สำรองสำหรับรักษาค่าเวลาเมื่อไม่มีไฟเลี้ยง
- มีสวิตช์ RESET การทำงาน
- เชื่อมต่อกับคอมพิวเตอร์ผ่านพอร์ต USB โดยใช้คอนเน็กเตอร์แบบ USB-C (ปรับปรุงจาก V1.5) สำหรับการดาวน์โหลดโปรแกรมและสื่อสารข้อมูลอนุกรม (โดยความสามารถในการสื่อสารข้อมูลขึ้นกับ IDE ที่เลือกใช้) และยังใช้ในการรับไฟเลี้ยง +5V ผ่านพอร์ต USB-C ด้วย
- มีจุดต่อพอร์ตที่ใช้คอนเน็กเตอร์ JST 2 มม. 3 ขา (JST : Japan Standard Terminal) รวม 6 ขา:
  - พอร์ตอินพุตดิจิทัล ประกอบด้วย ขา IN1 (GPIO32), IN2 (GPIO33), IN3 (GPIO34) และ IN4 (GPIO35) ตามการกำหนดขาของ KidBright
  - พอร์ตเอาต์พุตดิจิทัล OUT1 (GPIO26) และ OUT2 (GPIO27)

---

## คุณสมบัติทางเทคนิคของบอร์ด KidBright32 V1.5 Rev 3.1 (NECTEC Standard)
> **บอร์ดมาตรฐาน สวทช.** — เป็น baseline ที่ iA และ iP ต่อยอดมา

- ใช้ไมโครคอนโทรลเลอร์ ESP32 (ESP32-WROOM-32) ที่มีวงจร WiFi และบลูทูธในตัว
- มีส่วนแสดงผล LED ดอตเมตริกซ์ขนาด 16×8 จุด แบบสีแดง (ไดรเวอร์ HT16K33 @ I2C 0x70)
- มีเซ็นเซอร์แสง LDR (GPIO36 / ADC1_CH0)
- มีเซ็นเซอร์อุณหภูมิ LM73 (I2C_NUM_1, SDA=GPIO4, SCL=GPIO5, Address 0x4D)
- มีลำโพงเปียโซขับเสียง (Passive Buzzer, GPIO13, ต้องใช้ PWM/LEDC)
- มีวงจรสวิตช์กดติดปล่อยดับขนาดใหญ่ 2 ตัว (SW1=GPIO16, **SW2=GPIO14**)
- มีวงจรฐานเวลานาฬิกาจริง (RTC) พร้อมแบตเตอรี่ CR1220 สำรอง
- มีสวิตช์ RESET การทำงาน
- เชื่อมต่อกับคอมพิวเตอร์ผ่านพอร์ต **Micro-USB** (ต่างจาก iA/iP ที่ใช้ USB-C)
- มีช่อง **USB Type-A** (Host) สำหรับต่ออุปกรณ์ภายนอก (Active LOW ผ่าน IO25)
- มีพอร์ต JST 3 ขา สำหรับ IN1–IN4 (GPIO32–35) และ OUT1–OUT2 (GPIO26–27)
- มีพอร์ต KB Chain (I2C_NUM_0) สำหรับต่ออุปกรณ์เสริม
- **ไม่มี** Accelerometer / Gyroscope / Magnetometer (เพิ่มเฉพาะใน iA และ V1.6)
- **ไม่รองรับ ADC บนพอร์ต IN1–IN4** (ต่างจาก iA และ V1.6 ที่รองรับ)

### Sensor Map — V1.5 Rev 3.1 (NECTEC Standard)
> ⚠️ **SW2 = GPIO14**

| Sensor | Protocol | Bus/Pin | Address/Channel |
|--------|----------|---------|-----------------|
| LDR (Light) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F |
| HT16K33 (Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| **SW2 Button** | GPIO | **GPIO14** | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — |

> ⚠️ **V1.5 Rev 3.1 ไม่มี KXTJ3 Accelerometer** — I2C_NUM_0 จึงมีเฉพาะ HT16K33 (0x70) เท่านั้น

---

### Sensor Map — V1.5 Rev 3.1G (Gravitech OEM)
> ⚠️ **SW2 = GPIO 14**
> ฮาร์ดแวร์อื่นทุกอย่างเหมือน Rev 3.1 (ไม่มี KXTJ3, ไม่รองรับ ADC บน IN1–IN4)

| Sensor | Protocol | Bus/Pin | Address/Channel |
|--------|----------|---------|-----------------|
| LDR (Light) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F |
| HT16K33 (Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| **SW2 Button** | GPIO | **GPIO14** | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — |

> 📋 **I2C Scan Result (V1.5 Rev 3.1G — confirmed Apr 17 2026)**
> - I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22): พบ `0x70` (HT16K33)
> - I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5): พบ `0x4D` (LM73) และ `0x6F` (RTC MCP794xx)
> - Address `0x6F` คือ RTC chip ในตระกูล MCP7940N/MCP7941X (Microchip) ซึ่งเป็น RTC ที่มาพร้อมกับ SRAM และ alarm ในตัว ใช้ร่วมกับแบตเตอรี่ CR1220 บนบอร์ด

---

### Sensor Map — V1.6 (Gravitech)
> ⚠️ **AI CRITICAL:** V1.6 ใช้ **MPU-6050** ซึ่งเป็น **6-axis เท่านั้น** (Accel + Gyro) — ไม่มี magnetometer ในตัว
> ⚠️ **SW2 = GPIO17** — เหมือนกับ Rev 3.1G และ iA
> ⚠️ **SERVO1 = GPIO15**, **SERVO2 = GPIO17** — แต่ GPIO17 ถูกแชร์กับ SW2 ด้วย เลือกใช้อย่างใดอย่างหนึ่งเท่านั้น
> ⚠️ **RGB LED (Gerora/WS2812)** = GPIO อ่านจาก silkscreen บอร์ด — ต้องใช้ RMT peripheral ไม่ใช่ GPIO ธรรมดา

| Sensor | Protocol | Bus/Pin | Address/Channel |
|--------|----------|---------|-----------------|
| LDR (Light) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| MPU-6050 (Accel+Gyro) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x68 |
| HT16K33 (LED Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| SW2 Button | GPIO | **GPIO14** | — |
| RGB LED ×6 (Gerora/WS2812) | RMT/1-wire | **ตรวจสอบ PCB silkscreen** | — |
| SERVO1 | GPIO/PWM | GPIO15 (LEDC) | — |
| SERVO2 | GPIO/PWM | GPIO17 | — |
| USB Host Control | GPIO | GPIO25 (Active LOW) | — |

> 📋 **I2C Scan Result (V1.6 — expected)**
> - I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22): พบ `0x68` (MPU-6050 Accel/Gyro) และ `0x70` (HT16K33)
> - I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5): พบ `0x4D` (LM73)
> - **ไม่มี RTC** บน I2C_NUM_1 เหมือน V1.5 — ตรวจสอบ PCB ว่ามี CR1220 socket หรือไม่

> ⚠️ **MPU-6050 I2C Address:** default = `0x68` (AD0 ต่อ GND) — ถ้า AD0 ต่อ VCC จะเป็น `0x69`

### GPIO Conflict Table — V1.6 (Gravitech)

| GPIO | ใช้ได้เป็น... |
|------|--------------|
| GPIO4 | **BT LED** หรือ **LM73 SDA** — เลือกได้แค่อย่างเดียว |
| GPIO13 | **Passive Buzzer** — ต้องใช้ LEDC/PWM เสมอ |
| GPIO15 | **SERVO1** — ห้ามใช้งานอื่น |
| GPIO16 | **SW1 Button** — ห้ามใช้งานอื่น |
| GPIO14 | **SW2 Button** — ห้ามใช้งานอื่น |
| GPIO17 | **SERVO2** — ห้ามใช้งานอื่น |
| GPIO25 | **USB Host (Active LOW)** — อย่าใช้งานอื่น |
| GPIO36 | **LDR ADC** — Input-only, ไม่มี pull resistor |
| GPIO2 | **Wi-Fi LED** — อย่าใช้งานอื่น |

---

### Sensor Map — V1.5 iA (INEX)
> (ดูส่วน "คุณสมบัติทางเทคนิคของบอร์ด KidBright32iA" ด้านบน)
> KXTJ3-1057 Accelerometer (I2C_NUM_0, 0x0E), ไม่มี Gyroscope, ไม่มี RGB LED onboard

---

## 📋 KidBright32 i — Hardware Specification (Standard, SW2=GPIO14)
> **บอร์ด:** KidBright32 i (รุ่น Standard / V1.5 Rev 3.1 compatible)
> **MCU:** ESP32-WROOM-32 · **Framework:** ESP-IDF v5.x
> **หมายเหตุ:** ข้อมูลนี้สรุปจากโค้ดที่ผ่านการทดสอบจริงบนบอร์ด

### GPIO Pinout — KidBright32 i

| อุปกรณ์ | GPIO | Protocol / ฟังก์ชัน | หมายเหตุ |
|---------|------|---------------------|----------|
| LED Matrix (HT16K33) SDA | GPIO21 | I2C_NUM_0 | — |
| LED Matrix (HT16K33) SCL | GPIO22 | I2C_NUM_0 | — |
| LDR Light Sensor | GPIO36 | ADC1_CH0 | Input-only |
| Temperature Sensor (LM73) SDA | GPIO4 | I2C_NUM_1 | ⚠️ แชร์กับ BT LED |
| Temperature Sensor (LM73) SCL | GPIO5 | I2C_NUM_1 | — |
| Passive Buzzer | GPIO13 | LEDC/PWM | Timer_0, Channel_0 |
| SW1 Button | GPIO16 | Digital Input | Active LOW, Pull-up |
| **SW2 Button** | **GPIO14** | Digital Input | **Active LOW, Pull-up** |

### Sensor Map — KidBright32 i

| เซนเซอร์ | Protocol | Bus/Pin | Address/Channel | หมายเหตุ |
|----------|----------|---------|-----------------|----------|
| LDR (แสง) | ADC | GPIO36 / ADC1_CH0 | — | 12-bit, 0–4095 |
| LM73 (อุณหภูมิ) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D | เหมือน iA ทุกประการ |
| HT16K33 (LED Matrix 16×8) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 | 100 kHz |
| Passive Buzzer | PWM/LEDC | GPIO13 | — | LEDC_TIMER_0, LEDC_CHANNEL_0 |
| SW1 Button | GPIO | GPIO16 | — | Active LOW, Falling Edge ISR |
| SW2 Button | GPIO | **GPIO14** | — | Active LOW, Falling Edge ISR |

### LDR Calibration Values (Hardware-Tested)
> ⚠️ **AI CRITICAL:** LDR บน KidBright32 i **ไม่ได้ใช้ full range 0–4095** เนื่องจากวงจร voltage divider
> ค่าดิบ (raw) จะ **แปรผกผันกับแสง** — ยิ่งสว่างมาก ค่าจะยิ่งน้อย

| สภาวะ | Raw ADC (approx.) |
|-------|------------------|
| สว่างสุด (Bright) | ~100 |
| มืดสุด / ปิดทึบ (Dark) | ~900 |

**สูตรแปลงค่า (Linear Mapping, Inverted):**
```c
// ADC_ATTEN_DB_12, ADC_BITWIDTH_12 (0–4095), GPIO36 / ADC1_CH0
#define LDR_ADC_MIN_RAW  100   // Bright (สว่างสุด)
#define LDR_ADC_MAX_RAW  900   // Dark   (มืดสุด)

int ldr_get_brightness_percent(int raw) {
    if (raw <= LDR_ADC_MIN_RAW) return 100; // สว่างสุด
    if (raw >= LDR_ADC_MAX_RAW) return 0;   // มืดสุด
    int pct = (int)(((float)(LDR_ADC_MAX_RAW - raw) /
                     (LDR_ADC_MAX_RAW - LDR_ADC_MIN_RAW)) * 100.0f);
    if (pct < 0)  pct = 0;
    if (pct > 99) pct = 99; // Cap ไว้ที่ 99 สำหรับ 2-digit display
    return pct;
}
```

### Buzzer — LEDC Configuration (Hardware-Tested)
> ⚠️ **Passive Buzzer ต้องใช้ LEDC/PWM เสมอ** ห้ามใช้ `gpio_set_level` โดยตรง

```c
#define BUZZER_GPIO GPIO_NUM_13

// Timer config
ledc_timer_config_t ledc_timer = {
    .speed_mode      = LEDC_LOW_SPEED_MODE,
    .timer_num       = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .freq_hz         = 1000,  // Default — เปลี่ยนได้ด้วย ledc_set_freq()
    .clk_cfg         = LEDC_AUTO_CLK
};

// Channel config
ledc_channel_config_t ledc_channel = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel    = LEDC_CHANNEL_0,
    .timer_sel  = LEDC_TIMER_0,
    .intr_type  = LEDC_INTR_DISABLE,
    .gpio_num   = BUZZER_GPIO,
    .duty       = 0,    // เริ่มต้น OFF
    .hpoint     = 0
};

// เล่นเสียง: duty=512 (50% ของ 10-bit) = เสียงดัง
// หยุดเสียง: duty=0
void play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
```

### Button ISR Setup (Hardware-Tested)
> ⚠️ **SW2 = GPIO14** บน KidBright32 i

```c
#define SW1_GPIO GPIO_NUM_16
#define SW2_GPIO GPIO_NUM_14

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

void setup_buttons_isr(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SW1_GPIO) | (1ULL << SW2_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE   // Falling Edge (กดปุ่ม = LOW)
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(SW1_GPIO, gpio_isr_handler, (void*) SW1_GPIO);
    gpio_isr_handler_add(SW2_GPIO, gpio_isr_handler, (void*) SW2_GPIO);
}
```

### ⚠️ ความแตกต่างสำคัญระหว่าง KidBright32 i และ iA

| จุด | KidBright32 i (Standard) | KidBright32 iA (INEX) |
|-----|--------------------------|----------------------|
| SW2 Button | **GPIO14** | **GPIO14** |
| Accelerometer | ❌ ไม่มี | ✅ KXTJ3-1057 (I2C_NUM_0, 0x0E) |
| USB Connector | Micro-USB | USB-C |
| ADC บน IN1–IN4 | ❌ Digital เท่านั้น | ✅ รองรับ ADC |
| อุณหภูมิ (LM73) | ✅ I2C_NUM_1, 0x4D (GPIO4/5) | ✅ เหมือนกัน |
| LED Matrix | ✅ HT16K33, I2C_NUM_0, 0x70 | ✅ เหมือนกัน |
| LDR | ✅ GPIO36, ADC1_CH0 | ✅ เหมือนกัน |
| Buzzer | ✅ GPIO13, LEDC | ✅ เหมือนกัน |

---

### GPIO Conflict Table — V1.5 Rev 3.1 (NECTEC Standard)

| GPIO | ใช้ได้เป็น... |
|------|--------------|
| GPIO4 | **BT LED** หรือ **LM73 SDA** — เลือกได้แค่อย่างเดียว |
| GPIO13 | **Passive Buzzer** — ต้องใช้ LEDC/PWM เสมอ |
| GPIO14 | **SW2 Button** — ห้ามใช้งานอื่น บน V1.5 Rev 3.1 |
| GPIO16 | **SW1 Button** — ห้ามใช้งานอื่น บน V1.5 Rev 3.1 |
| GPIO25 | **USB Host (Active LOW)** — อย่าใช้งานอื่น |
| GPIO36 | **LDR ADC** — Input-only, ไม่มี pull resistor |
| GPIO2 | **Wi-Fi LED** — อย่าใช้งานอื่น |

### GPIO Conflict Table — V1.5 Rev 3.1G (Gravitech OEM)

| GPIO | ใช้ได้เป็น... |
|------|--------------|
| GPIO4 | **BT LED** หรือ **LM73 SDA** — เลือกได้แค่อย่างเดียว |
| GPIO13 | **Passive Buzzer** — ต้องใช้ LEDC/PWM เสมอ |
| GPIO16 | **SW1 Button** — ห้ามใช้งานอื่น บน V1.5 Rev 3.1G |
| GPIO14 | **SW2 Button** — ห้ามใช้งานอื่น บน V1.5 Rev 3.1G |
| GPIO25 | **USB Host (Active LOW)** — อย่าใช้งานอื่น |
| GPIO36 | **LDR ADC** — Input-only, ไม่มี pull resistor |
| GPIO2 | **Wi-Fi LED** — อย่าใช้งานอื่น |

---

## 1. Core MCU — ESP32-WROOM-32

| Parameter | Value |
|---|---|
| CPU | Dual-core Xtensa LX6, up to 240 MHz |
| Flash | 4 MB |
| RAM | 520 KB SRAM |
| Wi-Fi | 802.11 b/g/n 2.4 GHz |
| Bluetooth | Classic BT 4.2 + BLE |
| Logic voltage | **3.3 V** (GPIO are NOT 5 V tolerant) |
| ADC | 18 × 12-bit SAR ADC |
| DAC | 2 × 8-bit DAC |
| PWM | 16 channels (LEDC) |
| Touch | 10 × capacitive touch GPIO |

---

## 2. On-Board Peripherals

### LED Dot Matrix (16×8) — CRITICAL SECTION
> **AI INSTRUCTION:** The KidBright32 iA uses a **SINGLE HT16K33** driver chip for its entire 16×8 dot matrix display. You MUST write all 16 columns of data to the single I2C address `0x70` using the interleaved RAM mapping technique.

| Property | Detail |
|---|---|
| Driver IC | HT16K33 (Single Chip) |
| I2C Address | `0x70` |
| Display resolution | 16 columns × 8 rows |
| I2C Bus | `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) |

#### HT16K33 Register Map
| Command | Value | Description |
|---|---|---|
| Oscillator ON | `0x21` | Turn on system oscillator |
| Display ON | `0x81` | Display ON, no blink |
| Brightness MAX | `0xEF` | Maximum brightness (16/16 duty) |
| RAM Start | `0x00` | Start address for display RAM write |

#### HT16K33 Display RAM Layout (INTERLEAVED & ROTATED MAPPING)
> **NOTE:** See `led_16x8_matrix_mapping.md` for a complete breakdown of how to construct native hexadecimal arrays that bypass `rows_to_columns_16x8` entirely.

The 16x8 LED matrix on the KidBright32 iA is wired in an interleaved, 90-degree rotated fashion to the single HT16K33 chip. The driver has 16 bytes of display RAM (addresses 0x00 to 0x0F).
```
buffer[0]  = 0x00 (RAM start address)
buffer[1]  = column_0_rows  (Left Matrix Col 0)
buffer[2]  = column_8_rows  (Right Matrix Col 0)
buffer[3]  = column_1_rows  (Left Matrix Col 1)
buffer[4]  = column_9_rows  (Right Matrix Col 1)
...
buffer[15] = column_7_rows  (Left Matrix Col 7)
buffer[16] = column_15_rows (Right Matrix Col 7)
```

#### Helper: Convert row-major bitmap to column-major (cols[16])
> ⚠️ **CRITICAL HARDWARE QUIRK FOR AI:** The KidBright32 matrix hardware is wired upside-down (Y-axis inverted). When converting rows to columns, you **MUST** invert the row bit shifting by using `(7 - row)`. Never use just `row`.

```c
// Convert human-readable row-major array to hardware-ready column-major array
void rows_to_columns_16x8(const uint16_t row_data[8], uint8_t out_cols[16]) {
    memset(out_cols, 0, 16);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 16; col++) {
            if (row_data[row] & (1 << (15 - col))) {
                // HARDWARE QUIRK: Y-axis inversion required here -> (7 - row)
                out_cols[col] |= (1 << (7 - row));
            }
        }
    }
}
```

#### Complete Working ESP-IDF Example: Matrix Init & Draw

```c
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_FREQ   100000
#define HT16K33_ADDR      0x70

// Initialize I2C and the HT16K33 chip
esp_err_t matrix_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    uint8_t cmd;
    cmd = 0x21; // Oscillator ON
    i2c_master_write_to_device(I2C_MASTER_NUM, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
    cmd = 0x81; // Display ON
    i2c_master_write_to_device(I2C_MASTER_NUM, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
    cmd = 0xEF; // Max Brightness
    return i2c_master_write_to_device(I2C_MASTER_NUM, HT16K33_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
}

// Draw a full 16×8 framebuffer to the matrix using Interleaved Mapping
void matrix_draw(const uint8_t cols[16]) {
    uint8_t buf[17] = {0};
    buf[0] = 0x00; // RAM start address
    for (int c = 0; c < 8; c++) {
        buf[1 + (c * 2)] = cols[c];       // Left half (Even addresses)
        buf[2 + (c * 2)] = cols[c + 8];   // Right half (Odd addresses)
    }
    i2c_master_write_to_device(I2C_MASTER_NUM, HT16K33_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
}
```

#### ⚠️ TWO-DIGIT DISPLAY — เทคนิคบังคับสำหรับตัวเลข 2 หลัก (MANDATORY FOR AI)

> **CRITICAL:** DIGIT patterns (DIGIT_0–DIGIT_9) มาตรฐานจะส่องเฉพาะ **columns 3–7** (ฝั่ง LEFT เท่านั้น)
> ถ้าแสดง DIGIT_x เดี่ยวๆ ฝั่งขวาจะ **ดับสนิท**
> ต้องใช้ `display_two_digits()` เสมอเมื่อแสดงตัวเลข 2 หลัก

**❌ WRONG — ฝั่งขวาดับ (common AI mistake):**
```c
display_pattern(DIGITS[tens]);   // only cols 3–7 light up, right is dark
display_pattern(DIGITS[units]);  // same problem
```

**✅ CORRECT — ทั้งสองฝั่งติดพร้อมกัน:**
```c
display_two_digits(tens, units); // tens on LEFT cols 3-7, units on RIGHT cols 11-15
// Note: DIGITS[units] is shifted right by 8 bits to move it to the right panel.
```

### ✅ VERIFIED Functions (copy-paste ready):

```c
// Lookup table — declare globally after DIGIT_0..DIGIT_9 definitions
static const uint16_t *DIGITS[10] = {
    DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4,
    DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9
};

// Display tens on LEFT panel, units on RIGHT panel — full 16x8 display
void display_two_digits(int tens, int units) {
    if (tens  < 0) { tens  = 0; }
    if (tens  > 9) { tens  = 9; }
    if (units < 0) { units = 0; }
    if (units > 9) { units = 9; }
    uint16_t combined[8];
    for (int i = 0; i < 8; i++) {
        combined[i] = DIGITS[tens][i] | (DIGITS[units][i] >> 8);
    }
    uint8_t cols[16];
    rows_to_columns_16x8(combined, cols);
    matrix_draw(cols);
}

// Convenience function: integer 0-99 → 2-digit display
void display_number(int value) {
    if (value < 0)  value = 0;
    if (value > 99) value = 99;
    display_two_digits((value / 10) % 10, value % 10);
}
```

#### Verified Patterns (ห้ามประดิษฐ์ค่า hex เอง!)

> ⚠️ **CRITICAL:** ห้าม invent ค่า `uint16_t` hex สำหรับ digit/icon เองเด็ดขาด
> ค่าที่ AI คิดเองจะแสดงผล garbled บน hardware เสมอ
> ใช้เฉพาะ verified patterns ด้านล่างเท่านั้น

```c
// --- Digits 0–9 (verified hardware-tested, left-panel positioned) ---
// Each pattern occupies bit positions 12–8 on the left half (cols 3–7).
// To display on RIGHT panel: shift right by 8 bits → bit positions 4–0 (cols 11–15).
const uint16_t DIGIT_0[8] = {0x0E00,0x1100,0x1100,0x1100,0x1100,0x1100,0x1100,0x0E00};
const uint16_t DIGIT_1[8] = {0x0200,0x0600,0x0A00,0x0200,0x0200,0x0200,0x0200,0x1F00};
const uint16_t DIGIT_2[8] = {0x0E00,0x1100,0x0100,0x0200,0x0400,0x0800,0x1000,0x1F00};
const uint16_t DIGIT_3[8] = {0x0E00,0x1100,0x0100,0x0600,0x0100,0x0100,0x1100,0x0E00};
const uint16_t DIGIT_4[8] = {0x0200,0x0600,0x0A00,0x1200,0x1F00,0x0200,0x0200,0x0200};
const uint16_t DIGIT_5[8] = {0x1F00,0x1000,0x1E00,0x0100,0x0100,0x0100,0x1100,0x0E00};
const uint16_t DIGIT_6[8] = {0x0E00,0x1100,0x1000,0x1E00,0x1100,0x1100,0x1100,0x0E00};
const uint16_t DIGIT_7[8] = {0x1F00,0x0100,0x0200,0x0400,0x0400,0x0400,0x0400,0x0400};
const uint16_t DIGIT_8[8] = {0x0E00,0x1100,0x1100,0x0E00,0x1100,0x1100,0x1100,0x0E00};
const uint16_t DIGIT_9[8] = {0x0E00,0x1100,0x1100,0x0F00,0x0100,0x0100,0x1100,0x0E00};

// Helper: get digit pattern
const uint16_t* get_digit_pattern(int digit) {
    static const uint16_t* digits[10] = {
        DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4,
        DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9
    };
    if (digit < 0 || digit > 9) return DIGIT_0;
    return digits[digit];
}
```

### Built-in Indicator LEDs
| LED | GPIO | Behavior | Notes |
|---|---|---|---|
| Wi-Fi LED | GPIO2 | Active HIGH | Use `gpio_set_level(GPIO_NUM_2, 1)` |
| Bluetooth LED | GPIO4 | Active HIGH | Use `gpio_set_level(GPIO_NUM_4, 1)` |
| Power LED | — | Always ON | Hardware controlled |

> ⚠️ GPIO2 and GPIO4 are shared with the Wi-Fi/BT indicator LEDs. Writing to them will light the LEDs; avoid using them for other purposes.

### Sensors

| Sensor | Interface | Detail |
|---|---|---|
| Temperature | I2C | LM73, address `0x4D`, I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5) |
| Light (LDR) | ADC | GPIO36 / ADC1_CH0 |
| Accelerometer | I2C | KXTJ3-1057, address `0x0E`, I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22) |

> ⚠️ **GPIO36** เป็น input-only สำหรับอ่านค่าแสง LDR (ADC1_CH0) โดยเฉพาะ

> ⚠️ **GPIO4 CONFLICT WARNING:** GPIO4 ถูกใช้ร่วมกันระหว่าง **BT LED** (output) และ **SDA ของ LM73** (I2C_NUM_1) หากใช้ทั้งสองในโปรเจคเดียวกัน การควบคุม BT LED ด้วย `gpio_set_level` จะรบกวนการสื่อสาร I2C ให้เลือกใช้อย่างใดอย่างหนึ่งเท่านั้น

#### ADC Usage — CRITICAL ESP-IDF v5.x API (MANDATORY READING FOR AI)
> ⚠️ **CRITICAL AI INSTRUCTION:** The legacy ADC API (`adc1_config_width`, `adc1_config_channel_atten`, `adc1_get_raw`) was **REMOVED** in ESP-IDF v5.x. You MUST **NEVER** use those functions. They will cause compilation failure. You MUST use the `esp_adc/adc_oneshot.h` API shown below. There are no exceptions.

**Standard ADC Read Template (LDR on ADC1_CH0 / GPIO36):**

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// --- ADC Initialization (call once in app_main) ---
adc_oneshot_unit_handle_t adc1_handle;

esp_err_t adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten    = ADC_ATTEN_DB_12,   // 0–3.3 V range
        .bitwidth = ADC_BITWIDTH_12,    // 12-bit (0–4095)
    };
    // LDR is on ADC1 Channel 0 (GPIO36)
    return adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &chan_config);
}

// --- Read a single ADC sample and convert to percentage ---
// ⚠️ HARDWARE CALIBRATION NOTE: On KidBright32 iA, the voltage divider 
// limits the LDR ADC range. It does NOT reach full 0-4095.
// Typical values: ~100 (Brightest) to ~900 (Darkest/Covered).
int adc_read_ldr_percent(void) {
    int raw = 0;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &raw);
    
    const int LDR_MIN_RAW = 100; // Bright
    const int LDR_MAX_RAW = 900; // Dark
    
    int pct = (int)(((float)(LDR_MAX_RAW - raw) / (LDR_MAX_RAW - LDR_MIN_RAW)) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 99) pct = 99; // Cap at 99 for 2-digit display
    
    return pct;
}

// --- Cleanup (call if unit is no longer needed) ---
void adc_deinit(void) {
    adc_oneshot_del_unit(adc1_handle);
}
```

> **Note for AI:** `ADC_ATTEN_DB_12` (formerly `ADC_ATTEN_DB_11`) is the correct constant for full 3.3 V range in ESP-IDF v5.x. Never use `ADC_ATTEN_DB_11` — it is deprecated and may be removed.

### Push Buttons (CRITICAL PINOUT FOR AI)
> ⚠️ **AI INSTRUCTION:** The button pins depend on the board revision!
> - **V1.5 Rev 3.1 (NECTEC Standard)**: SW1 = **GPIO16**, SW2 = **GPIO14**
> - **V1.5 Rev 3.1G (Gravitech OEM)**: SW1 = **GPIO16**, SW2 = **GPIO14**
> - **V1.5 iA (INEX)** and **V1.6 (Gravitech)**: SW1 = **GPIO16**, SW2 = **GPIO17** ← DIFFERENT!

| Button | Rev 3.1 / Rev 3.1G | V1.5 iA / V1.6 |
|---|---|---|
| SW1 (Left) — Active LOW, `GPIO_PULLUP_ENABLE` | **GPIO_NUM_16** | **GPIO_NUM_16** |
| SW2 (Right) — Active LOW, `GPIO_PULLUP_ENABLE` | **GPIO_NUM_14** | **GPIO_NUM_17** |

> ⚠️ **CRITICAL:** iA และ V1.6 ใช้ SW2 = **GPIO17** ไม่ใช่ GPIO14! ห้ามสลับกัน!

### Buzzer
| Property | Detail |
|---|---|
| GPIO | GPIO_NUM_13 |
| Type | Passive piezo — drive with `driver/ledc.h` (PWM) |

### RTC
| Property | Detail |
|---|---|
| IC | **MCP7940N / MCP7941X** (Microchip) |
| Interface | I2C — **I2C_NUM_1** (SDA=GPIO4, SCL=GPIO5) |
| I2C Address | **0x6F** |
| Backup | CR1220 coin cell socket |
| Note | มีเฉพาะ Rev 3.1 / Rev 3.1G — **iA และ V1.6 ไม่มี RTC on-board** |

> ⚠️ **AI RULE:** ห้ามใช้ DS1307 หรือ PCF8523 — KidBright ใช้ **MCP794xx @ 0x6F บน I2C_NUM_1** เสมอ

---

## 3. GPIO & Connectors

### JST Connectors (Digital I/O)
บอร์ด KidBright32iA ใช้คอนเน็กเตอร์แบบ JST 2 มม. 3 ขา (JST : Japan Standard Terminal) จำนวน 6 พอร์ต แทนที่รูเสียบขนาดใหญ่ในรุ่นก่อนหน้า

| Label | GPIO | Direction | Capabilities | Notes |
|---|---|---|---|---|
| IN1 | GPIO32 | Input / Output | Digital I/O · ADC1_CH4 · touch9 | |
| IN2 | GPIO33 | Input / Output | Digital I/O · ADC1_CH5 · touch8 | |
| IN3 | GPIO34 | **Input only** | ADC1_CH6 | No internal pull-up/down |
| IN4 | GPIO35 | **Input only** | ADC1_CH7 | No internal pull-up/down |
| OUT1| GPIO26 | Input / Output | Digital I/O · DAC2 · ADC2_CH9 | |
| OUT2| GPIO27 | Input / Output | Digital I/O · ADC2_CH7 · touch7 | |

> ⚠️ **GPIO34** และ **GPIO35** เป็นขาแบบ Input-only (ไม่มี internal pull-up/down)

### Power & Ground Headers
| Label | Capabilities |
|---|---|
| 5V | 5 V from USB — ~500 mA shared |
| 3.3V | 3.3 V regulated — ~300 mA max |
| GND | Ground |

### Servo Connectors
| Connector | GPIO | Notes |
|---|---|---|
| SERVO1 | GPIO16 | 50 Hz PWM, 1–2 ms pulse = 0–180° |
| SERVO2 | GPIO17 | Same spec as SERVO1 |

> Servo power comes from the dedicated **servo power terminal** (screw terminal), not the 3.3 V rail.

### I²C Header
| Pin | GPIO | Notes |
|---|---|---|
| SDA | GPIO21 | 4.7 kΩ pull-up on board |
| SCL | GPIO22 | 4.7 kΩ pull-up on board |
| 3.3V | Power | For external I2C devices |
| GND | Power | Ground |

---

## 4. Communication Buses

| Bus | Pins | Notes |
|---|---|---|
| I2C (`I2C_NUM_0`) | SDA=GPIO21 · SCL=GPIO22 | On-board: LED matrix (HT16K33 @ 0x70), Accelerometer (KXTJ3 @ 0x0E on iA), I2C header |
| I2C (`I2C_NUM_1`) | SDA=GPIO4 · SCL=GPIO5 | On-board: Temperature (LM73 @ 0x4D) + **RTC MCP794xx (@ 0x6F) บน Rev 3.1/3.1G เท่านั้น** — shared with BT LED GPIO4 |
| UART0 | TX=GPIO1 · RX=GPIO3 | USB bridge / Serial monitor |


---

## 5. ESP-IDF Quick-Start Snippets (v5.x Syntax)

### Digital Input (Button SW1 & SW2)

> ⚠️ **WARNING:** KidBright SW1 is `GPIO_NUM_16` and SW2 is `GPIO_NUM_14`.

#### Option A: Pulling / Polling (Simple)
```c
#include "driver/gpio.h"

gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_16) | (1ULL << GPIO_NUM_14),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
gpio_config(&io_conf);

// Read state (0 = Pressed, Active LOW)
int sw1_state = gpio_get_level(GPIO_NUM_16);
```

#### Option B: Hardware Interrupts (ISR + FreeRTOS Queue) — PREFERRED FOR AI
> ⚠️ **CRITICAL INSTRUCTION:** Always define `ESP_INTR_FLAG_DEFAULT 0` at the top of your file when using `gpio_install_isr_service()`, otherwise the code will fail to compile.

```c
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// MANDATORY DECLARATION FOR ESP-IDF ISR
#define ESP_INTR_FLAG_DEFAULT 0

QueueHandle_t button_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

void setup_buttons_isr() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_16) | (1ULL << GPIO_NUM_14),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // Interrupt on falling edge (pressed)
    };
    gpio_config(&io_conf);

    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    // Install ISR service with default flag MUST be explicitly 0
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    
    // Hook ISR handlers
    gpio_isr_handler_add(GPIO_NUM_16, gpio_isr_handler, (void*) GPIO_NUM_16);
    gpio_isr_handler_add(GPIO_NUM_14, gpio_isr_handler, (void*) GPIO_NUM_14);
}
```

### Buzzer (LEDC PWM)

```c
#include "driver/ledc.h"

// Timer configuration
ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_LOW_SPEED_MODE,
    .timer_num        = LEDC_TIMER_0,
    .duty_resolution  = LEDC_TIMER_10_BIT,
    .freq_hz          = 1000,  // 1 kHz tone
    .clk_cfg          = LEDC_AUTO_CLK
};
ledc_timer_config(&ledc_timer);

// Channel configuration
ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_LOW_SPEED_MODE,
    .channel        = LEDC_CHANNEL_0,
    .timer_sel      = LEDC_TIMER_0,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = GPIO_NUM_13, // Buzzer pin
    .duty           = 0,
    .hpoint         = 0
};
ledc_channel_config(&ledc_channel);

// Play Tone (50% duty)
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512); // 512 is 50% for 10-bit
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

// Stop Tone -- ESP-IDF v5.x: ledc_stop() requires 3 arguments!
// 3rd arg = idle_level: 0 = GPIO LOW (correct for buzzer), 1 = GPIO HIGH
ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); // Correct v5.x
// ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); // WRONG - compile error in v5.x

// Alternative: mute without stopping the timer (set duty=0)
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
```

### Common 16x8 Patterns (Row-Major format)

```c
const uint16_t PATTERN_HEART[8] = {
    0x0000, 0x0660, 0x0FF0, 0x1FF8, 0x0FF0, 0x07E0, 0x03C0, 0x0180
};

const uint16_t PATTERN_SMILEY[8] = {
    0x0000, 0x0C30, 0x0C30, 0x0000, 0x0000, 0x1008, 0x07E0, 0x0000
};
```

#### Complete Display Pipeline Example (Patterns → Screen)
> **AI INSTRUCTION:** This is the mandatory 3-step pipeline to display any pattern. Never skip `rows_to_columns_16x8` — writing row-major data directly to `matrix_draw` produces garbled output.

```c
// Step 1: Define pattern in human-readable row-major format
const uint16_t PATTERN_HEART[8] = {
    0x0000, 0x0660, 0x0FF0, 0x1FF8, 0x0FF0, 0x07E0, 0x03C0, 0x0180
};

// Step 2 & 3: Convert and draw (call after matrix_init)
void show_heart(void) {
    uint8_t cols[16];
    rows_to_columns_16x8(PATTERN_HEART, cols); // Step 2: convert (applies Y-inversion)
    matrix_draw(cols);                          // Step 3: send to HT16K33 over I2C
}
```

### Random Numbers (CRITICAL FOR GAMES/LOGIC)
> ⚠️ **AI INSTRUCTION:** NEVER use `random()` or `srandom()`. They cause compilation errors in ESP-IDF v5.x. You MUST use the hardware RNG via `esp_random()`.
> ⚠️ **HEADER REQUIRED:** As of ESP-IDF v5.4+, you MUST include `esp_random.h` explicitly.

To ensure compatibility, ALWAYS copy and use this exact helper function:

```c
#include "esp_random.h" // Required for ESP-IDF v5.4+
#include "esp_system.h"

// Returns a random integer between min and max (inclusive)
uint32_t kb_random_range(uint32_t min, uint32_t max) {
    if (min >= max) return min;
    return min + (esp_random() % (max - min + 1));
}
```

---

## 6. Best Practices & Boilerplate (CRITICAL)

### Safe String Formatting (Prevent `-Werror=format-overflow=`)
> ⚠️ **AI INSTRUCTION:** ESP-IDF v5.x uses strict GCC warnings. NEVER use `sprintf` with tightly packed buffers. ALWAYS use `snprintf` and allocate buffers of at least 16 bytes for integers/floats to prevent compilation failure from format-overflow warnings.

```c
// WRONG — Will cause -Werror=format-overflow=
char str[5];
sprintf(str, "%03d", raw_value); 

// CORRECT — Safe from buffer overflow and GCC warnings
char str[16];
snprintf(str, sizeof(str), "%03d", raw_value);
```

### Standard Headers for KidBright32 (Copy-Paste)
> **AI INSTRUCTION:** Include ALL headers relevant to your project from this list. Never omit a header and attempt to use its API — this causes implicit declaration errors.

```c
// --- Core ---
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"          // Required for ISR-safe button handling
#include "freertos/semphr.h"         // Required for binary/counting semaphores

// --- Drivers ---
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"             // Buzzer, Servo PWM
#include "driver/uart.h"             // UART communication

// --- ADC (ESP-IDF v5.x — legacy adc1_get_raw is REMOVED) ---
#include "esp_adc/adc_oneshot.h"     // ALWAYS use this, NEVER adc1_get_raw

// --- System ---
#include "esp_log.h"
#include "esp_random.h"              // Required for esp_random() in v5.4+
#include "esp_timer.h"               // Lightweight periodic timers
#include "esp_intr_alloc.h"          // Required for GPIO ISR service

// --- NVS (persistent storage) ---
#include "nvs_flash.h"
#include "nvs.h"

#define ESP_INTR_FLAG_DEFAULT 0
```

### ⛔ BANNED HEADERS — AI MUST NEVER USE (These cause fatal compile errors)
> ⚠️ **CRITICAL AI VACCINE RULE:** The following headers do NOT exist in ESP-IDF v5.x. Using them causes `fatal error: No such file or directory`. NEVER generate code with these includes under ANY circumstances.

| BANNED Header | Why Banned | Correct Replacement |
|---|---|---|
| `esp_rom_delay_us.h` | **Does not exist** — AI hallucination | Use `vTaskDelay(pdMS_TO_TICKS(ms))` in tasks |
| `rom/ets_sys.h` | Deprecated / unreliable in v5.x | Use `vTaskDelay()` instead |
| `esp_rom_gpio.h` | Private ROM header, not public API | Use `driver/gpio.h` |
| `adc1_config_width` / `adc1_get_raw` | **REMOVED** in v5.x | Use `esp_adc/adc_oneshot.h` |
| `Arduino.h` / `Wire.h` | Arduino framework only | Use ESP-IDF driver/i2c.h |

### ✅ CORRECT Delay Patterns in ESP-IDF v5.x

```c
// ✅ CORRECT — inside any FreeRTOS task (most common, preferred)
vTaskDelay(pdMS_TO_TICKS(100));   // delay 100 ms

// ✅ CORRECT — if you need microsecond busy-wait (use sparingly, blocks CPU)
// Header: #include "esp_rom_sys.h"   ← this is the correct header (NOT esp_rom_delay_us.h)
#include "esp_rom_sys.h"
esp_rom_delay_us(50);             // busy-wait 50 µs — use ONLY outside tasks / in ISR context

// ❌ WRONG — these will NOT compile:
// #include "esp_rom_delay_us.h"   ← DOES NOT EXIST
// ets_delay_us(50);               ← deprecated, avoid
// delay(100);                     ← Arduino only
// usleep(100000);                 ← POSIX, not available in FreeRTOS
```

> **AI RULE:** In **99% of cases** you should use `vTaskDelay(pdMS_TO_TICKS(...))`. Only use `esp_rom_delay_us()` with `#include "esp_rom_sys.h"` for sub-millisecond hardware timing where blocking is acceptable.

### Proper Function Ordering Template
> **AI INSTRUCTION:** You MUST define functions in this exact order to prevent "implicit declaration" errors in C. Defining `app_main` before a function it calls is a fatal compile error.

1. **Includes & Defines**
2. **Global handles & variables** (e.g., `adc_oneshot_unit_handle_t`, `QueueHandle_t`)
3. **Function Prototypes** (declare all functions here if forward references are needed)
4. **ISR handlers** — tagged `IRAM_ATTR`, non-blocking only (e.g., `gpio_isr_handler`)
5. **Low-level helpers** (e.g., `rows_to_columns_16x8`, `matrix_draw`, `adc_read_ldr`)
6. **Mid-level logic** (e.g., `display_number`, `play_tone`, `servo_set_angle`)
7. **FreeRTOS task functions** (e.g., `button_task`, `display_task`)
8. **`app_main`** — entry point, calls init functions and launches tasks

---

## 7. FreeRTOS Task Safety Rules (CRITICAL — AI MUST FOLLOW STRICTLY)

### Task Creation Boilerplate
> ⚠️ **AI INSTRUCTION:** NEVER create bare threads. ALL background work MUST run inside FreeRTOS tasks. NEVER call blocking functions (I2C writes, `vTaskDelay`) from an ISR.

```c
// CORRECT task signature
void my_task(void *pvParameters) {
    while (1) {
        // ... your work ...
        vTaskDelay(pdMS_TO_TICKS(10)); // MANDATORY: yield to scheduler every loop
    }
    vTaskDelete(NULL); // Unreachable but required by convention
}

// CORRECT task launch (inside app_main)
// Stack size in WORDS (not bytes). 4096 words = 16 KB — minimum for most tasks.
// Increase to 8192 if the task uses printf, floating-point, or large local arrays.
xTaskCreate(my_task, "my_task", 4096, NULL, 5, NULL);
```

### Watchdog Timer (WDT) — Prevent Silent Resets
> ⚠️ **CRITICAL AI INSTRUCTION:** The ESP-IDF Task Watchdog (TWDT) will reset the board if a task holds the CPU without yielding for more than ~5 seconds. NEVER write a `while(1)` loop without a `vTaskDelay`. Even a `vTaskDelay(pdMS_TO_TICKS(1))` is sufficient to pet the watchdog.

```c
// WRONG — will trigger watchdog reset:
while (1) {
    do_work();
}

// CORRECT — always yield:
while (1) {
    do_work();
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### GPIO Interrupt (ISR) — Safe Pattern
> ⚠️ **CRITICAL AI INSTRUCTION:** ISR functions MUST be tagged `IRAM_ATTR`. You MUST NEVER call `ESP_LOGI`, `i2c_master_write_to_device`, `vTaskDelay`, or any blocking function from inside an ISR. Use a FreeRTOS queue to pass data out of the ISR safely.

```c
#include "freertos/queue.h"
#include "esp_intr_alloc.h"

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

// ISR handler — MUST be IRAM_ATTR, MUST be non-blocking
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); // ISR-safe send
}

// Worker task — runs in normal context, safe to call all APIs
void button_task(void *pvParameters) {
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // Safe to log, update display, play tone here
            ESP_LOGI("BTN", "Button on GPIO %lu pressed", io_num);
        }
    }
}

// Setup — call from app_main
void gpio_interrupt_init(void) {
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Configure SW1 (GPIO16) and SW2 (GPIO14) as interrupt on falling edge (active LOW button)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_16) | (1ULL << GPIO_NUM_14),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE, // Trigger on button press (HIGH→LOW)
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_NUM_16, gpio_isr_handler, (void *)GPIO_NUM_16);
    gpio_isr_handler_add(GPIO_NUM_14, gpio_isr_handler, (void *)GPIO_NUM_14);

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
}
```

### Memory Allocation Rules
> ⚠️ **AI INSTRUCTION:** When allocating heap memory, ALWAYS check the return value. NEVER dereference a NULL pointer. Prefer stack allocation for buffers under 512 bytes inside tasks.

```c
// CORRECT heap allocation pattern
uint8_t *buf = malloc(256);
if (buf == NULL) {
    ESP_LOGE("TAG", "Heap allocation failed");
    return ESP_ERR_NO_MEM;
}
// ... use buf ...
free(buf);
```

---

## 8. esp_timer — Lightweight Periodic Callbacks (Preferred over xTimerCreate)

> **AI INSTRUCTION:** For simple periodic actions (e.g., polling a sensor, refreshing the display every 50 ms), use `esp_timer`. It has lower overhead than FreeRTOS software timers. NEVER use `esp_timer` callbacks for tasks that take more than a few microseconds — offload heavy work to a FreeRTOS task via a queue instead.

```c
#include "esp_timer.h"

static void periodic_callback(void *arg) {
    // ⚠️ Keep this SHORT. No I2C, no printf, no vTaskDelay.
    // Signal a task via queue/semaphore for heavy work.
    int *counter = (int *)arg;
    (*counter)++;
}

void timer_example(void) {
    static int tick_count = 0;

    const esp_timer_create_args_t timer_args = {
        .callback = &periodic_callback,
        .arg      = (void *)&tick_count,
        .name     = "periodic_tick"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 50000)); // 50 ms in µs

    // To stop: esp_timer_stop(periodic_timer);
    // To free: esp_timer_delete(periodic_timer);
}
```

---

## 9. NVS — Non-Volatile Storage (Saving Data Across Reboots)

> **AI INSTRUCTION:** Use NVS to persist any data that must survive a power cycle: high scores, device configuration, calibration values, Wi-Fi credentials. ALWAYS call `nvs_flash_init()` at the top of `app_main` before any other NVS operation. Handle the `ESP_ERR_NVS_NO_FREE_PAGES` error by erasing and re-initialising.

```c
#include "nvs_flash.h"
#include "nvs.h"

// ALWAYS call this at the start of app_main
void nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// Write an integer (e.g., high score)
esp_err_t nvs_write_int(const char *key, int32_t value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) err = nvs_commit(handle); // ALWAYS commit after write
    nvs_close(handle);
    return err;
}

// Read an integer (returns default_val if key does not exist)
int32_t nvs_read_int(const char *key, int32_t default_val) {
    nvs_handle_t handle;
    int32_t value = default_val;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, key, &value); // Silently keeps default on ESP_ERR_NVS_NOT_FOUND
        nvs_close(handle);
    }
    return value;
}
```

---

## 10. Hardware Conflicts & Servo PWM

### ⚠️ CRITICAL GPIO Conflict Table — AI MUST CHECK BEFORE ASSIGNING PINS

| GPIO | Peripheral A | Peripheral B | Conflict? |
|------|-------------|-------------|-----------|
| GPIO16 | SW1 (Button) | SERVO1 Connector | **YES — mutually exclusive** |
| GPIO17 | (free) | SERVO2 Connector | No conflict |
| GPIO36 | LDR (ADC) | IO36 Pad (ADC input) | **YES — shared analog input** |
| GPIO2  | Wi-Fi LED | General GPIO | Avoid for user logic |
| GPIO4  | BT LED | LM73 SDA (I2C_NUM_1) | **YES — mutually exclusive** |

> ⚠️ **AI INSTRUCTION:** GPIO16 is used by BOTH SW1 (button) and the SERVO1 connector. You MUST NEVER configure GPIO16 for servo output if buttons are also required in the same project. SERVO2 (GPIO17) has no conflict with SW2 (GPIO14) and is safe to use in all configurations.

> ⚠️ **AI INSTRUCTION:** GPIO4 is used by BOTH the BT LED indicator and the SDA line of the LM73 temperature sensor (I2C_NUM_1). In any project that reads the temperature sensor, do NOT use `gpio_set_level(GPIO_NUM_4, ...)` to control the BT LED — doing so will corrupt the I2C bus.

### Servo PWM Template (use GPIO17 / SERVO2 to avoid conflict)

```c
#include "driver/ledc.h"

// Servo: 50 Hz, pulse 500 µs (0°) to 2500 µs (180°)
// Period = 20 ms = 20,000 µs. With 16-bit resolution: 65535 ticks = 20,000 µs
// 1 tick = 20000/65535 µs ≈ 0.305 µs
// 0°   → 500 µs  / 0.305 µs ≈  1638 ticks
// 90°  → 1500 µs / 0.305 µs ≈  4915 ticks
// 180° → 2500 µs / 0.305 µs ≈  8192 ticks

#define SERVO_GPIO       GPIO_NUM_17  // SERVO2 — safe, no button conflict
#define SERVO_MIN_TICKS  1638
#define SERVO_MAX_TICKS  8192

void servo_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .gpio_num   = SERVO_GPIO,
        .duty       = SERVO_MIN_TICKS,
        .hpoint     = 0,
    };
    ledc_channel_config(&channel);
}

// Set servo angle 0–180 degrees
void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    uint32_t ticks = SERVO_MIN_TICKS + ((SERVO_MAX_TICKS - SERVO_MIN_TICKS) * angle / 180);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, ticks);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}
```

---

## 11. LM73 Temperature Sensor (I2C_NUM_1, Address 0x4D)
> **AI INSTRUCTION:** The LM73 temperature sensor is on a separate I2C bus (`I2C_NUM_1`) using SDA=GPIO4, SCL=GPIO5. You must initialize this bus separately from the main `I2C_NUM_0` bus. Do NOT use external libraries. Read directly via standard ESP-IDF I2C functions.
>
> ⚠️ **GPIO4 CONFLICT:** GPIO4 (SDA of I2C_NUM_1) is also connected to the BT LED indicator. Do NOT call `gpio_set_level(GPIO_NUM_4, ...)` anywhere in a project that uses `temp_sensor_init()`.

```c
#define I2C_TEMP_NUM    I2C_NUM_1
#define I2C_TEMP_SDA_IO GPIO_NUM_4
#define I2C_TEMP_SCL_IO GPIO_NUM_5
#define LM73_ADDR       0x4D

void temp_sensor_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_TEMP_SDA_IO,
        .scl_io_num = I2C_TEMP_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_TEMP_NUM, &conf);
    i2c_driver_install(I2C_TEMP_NUM, conf.mode, 0, 0, 0);
}

// Read temperature from LM73 (Address 0x4D)
float read_lm73(void) {
    uint8_t raw[2];
    uint8_t reg = 0x00; // Temperature register
    esp_err_t ret = i2c_master_write_read_device(I2C_TEMP_NUM, LM73_ADDR, &reg, 1, raw, 2, pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        int16_t temp = (raw[0] << 8) | raw[1];
        return (float)temp / 128.0; // 14-bit resolution, 1 LSB = 0.03125°C
    }
    return -999.0; // Error value
}
```

---

## 12. KXTJ3-1057 Accelerometer (I2C_NUM_0, Address 0x0E)
> **AI INSTRUCTION:** The KidBright32iA has an on-board **KXTJ3-1057** tri-axis digital accelerometer from Rohm/Kionix. It shares **I2C_NUM_0** (SDA=GPIO21, SCL=GPIO22) with the LED matrix. Initialize I2C_NUM_0 once (via `matrix_init` or standalone) before calling accelerometer functions. Do NOT re-initialize I2C_NUM_0 separately.

| Property | Detail |
|---|---|
| IC | KXTJ3-1057 (Rohm/Kionix) |
| I2C Bus | `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) — shared with LED Matrix |
| I2C Address | `0x0E` (default, ADDR pin LOW) |
| Axes | 3-axis: X, Y, Z |
| Range | ±2g / ±4g / ±8g / ±16g (selectable) |
| Resolution | 8-bit (low power) / 12-bit / 14-bit (high-res) |
| WHO_AM_I Register | `0x0F` → returns `0x35` |

#### KXTJ3-1057 Key Registers

| Register | Address | Description |
|---|---|---|
| XOUT_L | `0x06` | X-axis output LSB |
| XOUT_H | `0x07` | X-axis output MSB |
| YOUT_L | `0x08` | Y-axis output LSB |
| YOUT_H | `0x09` | Y-axis output MSB |
| ZOUT_L | `0x0A` | Z-axis output LSB |
| ZOUT_H | `0x0B` | Z-axis output MSB |
| WHO_AM_I | `0x0F` | Device ID — should return `0x35` |
| CTRL_REG1 | `0x1B` | Main control: enable, resolution, range |
| DATA_CTRL_REG | `0x21` | Output Data Rate (ODR) |

#### CTRL_REG1 Bit Field (0x1B)

| Bit | Name | Description |
|---|---|---|
| 7 | PC1 | 1 = Operating mode, 0 = Stand-by |
| 6 | RES | 1 = High-resolution (12/14-bit), 0 = Low-power (8-bit) |
| 4 | DRDYE | Data ready engine enable |
| 3:2 | GSEL[1:0] | Range: `00`=±2g, `01`=±4g, `10`=±8g, `11`=±16g |

**Example CTRL_REG1 values:**
- `0xC0` = PC1=1, RES=1 → High-res, ±2g, operating mode
- `0x80` = PC1=1, RES=0 → Low-power 8-bit, ±2g, operating mode

#### Complete ESP-IDF Example: KXTJ3 Init & Read

```c
#include "driver/i2c.h"
#include "esp_log.h"

#define KXTJ3_ADDR          0x0E
#define KXTJ3_WHO_AM_I      0x0F
#define KXTJ3_XOUT_L        0x06
#define KXTJ3_CTRL_REG1     0x1B
#define KXTJ3_DATA_CTRL_REG 0x21
#define KXTJ3_EXPECTED_ID   0x35

// NOTE: I2C_NUM_0 must already be initialized (e.g., by matrix_init) before calling these.

static esp_err_t kxtj3_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(I2C_NUM_0, KXTJ3_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t kxtj3_read_reg(uint8_t reg, uint8_t *out) {
    return i2c_master_write_read_device(I2C_NUM_0, KXTJ3_ADDR, &reg, 1, out, 1, pdMS_TO_TICKS(100));
}

// Initialize the KXTJ3 accelerometer
// Call AFTER matrix_init() since both share I2C_NUM_0
esp_err_t kxtj3_init(void) {
    // Verify device identity
    uint8_t who_am_i = 0;
    esp_err_t ret = kxtj3_read_reg(KXTJ3_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK || who_am_i != KXTJ3_EXPECTED_ID) {
        ESP_LOGE("KXTJ3", "WHO_AM_I mismatch: got 0x%02X, expected 0x%02X", who_am_i, KXTJ3_EXPECTED_ID);
        return ESP_ERR_NOT_FOUND;
    }

    // Step 1: Go to stand-by mode before configuring
    kxtj3_write_reg(KXTJ3_CTRL_REG1, 0x00);

    // Step 2: Set ODR to 50 Hz (DATA_CTRL_REG = 0x03)
    // ODR options: 0x00=0.781Hz 0x01=1.563Hz 0x02=3.125Hz 0x03=6.25Hz
    //              0x04=12.5Hz  0x05=25Hz    0x06=50Hz    0x07=100Hz
    //              0x08=200Hz   0x09=400Hz   0x0A=800Hz   0x0B=1600Hz
    kxtj3_write_reg(KXTJ3_DATA_CTRL_REG, 0x06); // 50 Hz

    // Step 3: Enable operating mode — High-res (12-bit), ±2g range
    // CTRL_REG1: PC1=1, RES=1, GSEL=00 → 0xC0
    ret = kxtj3_write_reg(KXTJ3_CTRL_REG1, 0xC0);
    if (ret == ESP_OK) {
        ESP_LOGI("KXTJ3", "Initialized OK (WHO_AM_I=0x%02X)", who_am_i);
    }
    return ret;
}

typedef struct {
    float x_g;
    float y_g;
    float z_g;
} kxtj3_data_t;

// Read X, Y, Z acceleration in g (12-bit high-res mode, ±2g)
// Sensitivity in 12-bit ±2g mode: 1024 LSB/g (data in upper 12 bits, left-justified)
esp_err_t kxtj3_read(kxtj3_data_t *out) {
    uint8_t raw[6];
    uint8_t reg = KXTJ3_XOUT_L;
    esp_err_t ret = i2c_master_write_read_device(
        I2C_NUM_0, KXTJ3_ADDR, &reg, 1, raw, 6, pdMS_TO_TICKS(100)
    );
    if (ret != ESP_OK) return ret;

    // Combine MSB and LSB — data is left-justified, shift right by 4 for 12-bit value
    int16_t x_raw = (int16_t)((raw[1] << 8) | raw[0]) >> 4;
    int16_t y_raw = (int16_t)((raw[3] << 8) | raw[2]) >> 4;
    int16_t z_raw = (int16_t)((raw[5] << 8) | raw[4]) >> 4;

    // Convert to g: sensitivity = 1024 LSB/g in 12-bit ±2g mode
    out->x_g = (float)x_raw / 1024.0f;
    out->y_g = (float)y_raw / 1024.0f;
    out->z_g = (float)z_raw / 1024.0f;
    return ESP_OK;
}
```

#### Usage Example

```c
void app_main(void) {
    matrix_init();       // Initializes I2C_NUM_0 — MUST be called first
    kxtj3_init();        // Uses I2C_NUM_0 shared with matrix

    kxtj3_data_t accel;
    while (1) {
        if (kxtj3_read(&accel) == ESP_OK) {
            ESP_LOGI("ACCEL", "X=%.3f g  Y=%.3f g  Z=%.3f g",
                     accel.x_g, accel.y_g, accel.z_g);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

#### Sensitivity Reference (High-res 12-bit mode)

| Range | Sensitivity |
|---|---|
| ±2g | 1024 LSB/g |
| ±4g | 512 LSB/g |
| ±8g | 256 LSB/g |
| ±16g | 128 LSB/g |

---

## 13. Build System (CMakeLists.txt Template)
> **AI INSTRUCTION:** If the user encounters a CMake parse error, strictly follow this root CMakeLists.txt format. NEVER use spaces in the project name.

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# CORRECT: No spaces
project(KidBright_Project)
```

---

## 14. FINAL SANITY CHECK & HARDWARE RULES

**DEFAULT BOARD = KidBright32 iA.**

### ═══ LED MATRIX ═══
- Single HT16K33 at I2C address `0x70` on `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22).
- Resolution: 16×8 pixels. Init sequence: `0x21` (Oscillator ON), `0x81` (Display ON), `0xEF` (Brightness MAX).
- ALWAYS use `rows_to_columns_16x8()` with `(7 - row)` for Y-axis inversion. NEVER write row-major data directly.
- ALWAYS use the 3-step pipeline: define pattern → `rows_to_columns_16x8()` → `matrix_draw()`.

### ═══ BUZZER ═══
- Passive piezo at `GPIO_NUM_13`. Drive with `driver/ledc.h` (PWM).
- Use `LEDC_TIMER_0`, `LEDC_TIMER_10_BIT`. NEVER use `tone()` — that is Arduino only.
- **ESP-IDF v5.x BREAKING CHANGE — `ledc_stop()` requires 3 arguments:**
  - `ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);` ← CORRECT (idle_level=0 → GPIO LOW)
  - `ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);` ← WRONG — compile error in v5.x!
  - Error message: `error: too few arguments to function 'ledc_stop'`

### ═══ BUTTONS ═══
- **SW1** = `GPIO_NUM_16` (Active LOW, GPIO_PULLUP_ENABLE)
- **SW2** = `GPIO_NUM_14` (Active LOW, GPIO_PULLUP_ENABLE)
- NEVER use GPIO0, GPIO2, or GPIO35 for buttons.
- ALWAYS use ISR + FreeRTOS queue pattern. NEVER poll buttons in a bare `while(1)` loop.

### ═══ TEMPERATURE SENSOR ═══
- IC: LM73, I2C address `0x4D`.
- Bus: `I2C_NUM_1` — SDA=GPIO4, SCL=GPIO5.
- Initialize `I2C_NUM_1` SEPARATELY from `I2C_NUM_0`. Do NOT mix them up.
- Read via `i2c_master_write_read_device()`. Raw value / 128.0 = degrees Celsius.
- ⚠️ **GPIO4 CONFLICT**: GPIO4 is shared between the BT LED indicator and LM73 SDA. In ANY project that calls `temp_sensor_init()`, you MUST NEVER call `gpio_set_level(GPIO_NUM_4, ...)`. Doing so will corrupt the I2C bus. Choose one or the other — they cannot be used together.

### ═══ ACCELEROMETER ═══
- IC: KXTJ3-1057 (Rohm/Kionix), I2C address `0x0E`.
- Bus: `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) — shared with LED matrix.
- `WHO_AM_I` register `0x0F` must return `0x35`. Verify on init.
- ALWAYS call `matrix_init()` FIRST to initialize `I2C_NUM_0`. NEVER re-initialize `I2C_NUM_0` separately for the accelerometer.
- Default config: `CTRL_REG1` = `0xC0` (High-res 12-bit, ±2g, operating mode).
- Sensitivity in 12-bit ±2g mode: 1024 LSB/g. Formula: `raw >> 4`, then `/ 1024.0f`.

### ═══ LDR & ANALOG SENSORS (ADC VACCINE) ═══
- LDR is strictly on **GPIO36 / ADC1_CHANNEL_0**. Input-only pin — no pull-up/pull-down available.
- Other external analog sensors (e.g., LM35, Potentiometer) can be connected to IN1 (**GPIO32** / `ADC_CHANNEL_4`) or IN2 (**GPIO33** / `ADC_CHANNEL_5`).
- **⚠️ CRITICAL ESP-IDF v5.x VACCINE**:
  - NEVER use legacy API: `adc1_config_width`, `adc1_config_channel_atten`, `adc1_get_raw`, `esp_adc_cal_characterize`. These were **REMOVED** and will cause compilation errors.
  - `#include "driver/adc.h"` and `#include "esp_adc_cal.h"` are **BANNED**.
  - **`ADC_ATTEN_DB_11` was RENAMED to `ADC_ATTEN_DB_12` in ESP-IDF v5.x** — always use `ADC_ATTEN_DB_12` for the full 0–3.3V input range.
  - **`#include "esp_rom_delay_us.h"` does NOT EXIST in ESP-IDF v5.x** — this file was never a public header. Use `vTaskDelay(pdMS_TO_TICKS(ms))` for millisecond delays instead. For microsecond delays, use `esp_rom_delay_us(us)` from `#include "rom/ets_sys.h"` (but prefer `vTaskDelay` unless sub-ms precision is critical).

**CORRECT ESP-IDF v5.x ADC TEMPLATE (Oneshot + Calibration):**
```c
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Define the channel based on your pin (e.g., IN1 uses GPIO32 = ADC_CHANNEL_4)
#define SENSOR_ADC_CHAN ADC_CHANNEL_4

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_handle = NULL;
static bool cali_enable = false;

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;
    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) calibrated = true;
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) calibrated = true;
    }
#endif
    *out_handle = handle;
    return calibrated;
}

void init_adc(void) {
    // 1. Init ADC Unit
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configure Channel (Attenuation 12dB for 0-3.3V)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, 
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CHAN, &config));

    // 3. Init Calibration (Converts raw to mV accurately)
    cali_enable = example_adc_calibration_init(ADC_UNIT_1, SENSOR_ADC_CHAN, ADC_ATTEN_DB_12, &cali_handle);
}

void read_adc_task(void *pvParameters) {
    init_adc();
    while (1) {
        int raw_value = 0;
        int voltage_mv = 0;

        // Take a single reading (Or average multiple readings if you want)
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, SENSOR_ADC_CHAN, &raw_value));
        
        // Convert to Voltage using Calibration
        if (cali_enable) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv));
            ESP_LOGI("ADC", "Raw: %d, Voltage: %d mV", raw_value, voltage_mv);
            
            // Example for LM35 (10mV = 1 Degree C):
            // float temp_c = voltage_mv / 10.0f;
            // ESP_LOGI("TEMP", "Temp: %.2f C", temp_c);
        } else {
            ESP_LOGI("ADC", "Raw: %d (No calibration)", raw_value);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### ═══ INDICATOR LEDs ═══
- **Wi-Fi LED**: GPIO2 (Active HIGH)
- **BT LED**: GPIO4 (Active HIGH) ⚠️ Conflicts with LM73 SDA — see TEMPERATURE SENSOR above.
- **Power LED**: Always ON, hardware controlled.
- AVOID using GPIO2 and GPIO4 for any other purpose.

### ═══ JST CONNECTOR PINS ═══
- **IN1**=GPIO32, **IN2**=GPIO33 — Digital I/O, ADC1, touch capable.
- **IN3**=GPIO34, **IN4**=GPIO35 — Input-only. NO internal pull-up/pull-down.
- **OUT1**=GPIO26 (DAC2), **OUT2**=GPIO27 — Digital I/O.

### ═══ CRITICAL GPIO CONFLICT TABLE ═══
- **GPIO4** → BT LED output AND LM73 SDA (`I2C_NUM_1`) — MUTUALLY EXCLUSIVE
- **GPIO16** → SW1 Button AND SERVO1 connector — MUTUALLY EXCLUSIVE
- **GPIO36** → LDR ADC input only — no output, no pull resistors
- **GPIO2** → Wi-Fi LED — avoid for user logic

⚠️ **SERVO RULE:** If the project uses buttons (SW1), NEVER assign servo to GPIO16 (SERVO1). Use SERVO2 (GPIO17) as the default servo output in all projects that also use buttons.

### ═══ I2C BUS SUMMARY ═══
- **I2C_NUM_0** (SDA=GPIO21, SCL=GPIO22): LED Matrix (`0x70`), Accelerometer KXTJ3 (`0x0E` on iA), I2C header.
- **I2C_NUM_1** (SDA=GPIO4,  SCL=GPIO5): Temperature Sensor LM73 (`0x4D`) + **RTC MCP794xx (`0x6F`) บน Rev 3.1/3.1G เท่านั้น**.
- Initialize each bus only ONCE. Never call `i2c_driver_install()` twice on the same bus number.
> ⚠️ **RTC อยู่บน I2C_NUM_1 เสมอ** — ห้าม init RTC บน I2C_NUM_0 โดยเด็ดขาด!

### ═══ RANDOM NUMBERS ═══
- NEVER use `random()` or `srandom()` — compilation error in ESP-IDF v5.x.
- ALWAYS use `esp_random()` or `kb_random_range()`.
- ALWAYS `#include "esp_random.h"` (required from ESP-IDF v5.4+).

### ═══ FREERTOS RULES ═══
- EVERY task MUST call `vTaskDelay(pdMS_TO_TICKS(N))` inside its `while(1)` loop.
- NEVER call `i2c_master_write_to_device`, `ESP_LOGI`, or `vTaskDelay` from inside an ISR.
- ALWAYS tag ISR functions with `IRAM_ATTR`.
- ALWAYS use a FreeRTOS queue to pass data out of an ISR safely.
- Minimum stack size: 4096 words. Use 8192 if the task uses `printf` or large arrays.

---

## 15. SENSOR RULES — Complete Reference (On-Board & External)
> **AI INSTRUCTION:** ส่วนนี้รวบรวมกฎการใช้งานเซ็นเซอร์ทั้งหมดของ KidBright32 iA ทั้งเซ็นเซอร์บนบอร์ดและเซ็นเซอร์ภายนอก ต้องอ่านก่อนทุกครั้งที่เขียนโค้ดที่เกี่ยวข้องกับเซ็นเซอร์

### 15.1 On-Board Sensor Summary Table
| Sensor | IC | Interface | Bus | Address/Pin | GPIO | หมายเหตุ |
|---|---|---|---|---|---|---|
| Temperature | LM73 | I2C | I2C_NUM_1 | 0x4D | SDA=GPIO4, SCL=GPIO5 | ⚠️ GPIO4 conflict กับ BT LED |
| Light (LDR) | Photoresistor | ADC | ADC1 | ADC_CHANNEL_0 | GPIO36 | Input-only, ไม่มี pull-up |
| Accelerometer | KXTJ3-1057 | I2C | I2C_NUM_0 | 0x0E | SDA=GPIO21, SCL=GPIO22 | ร่วมกับ LED Matrix |

### 15.2 LM73 Temperature Sensor — Full Rule Set
**ข้อมูล IC (จาก TI Datasheet)**
| Property | Detail |
|---|---|
| ผู้ผลิต | Texas Instruments (เดิม National Semiconductor) |
| แรงดันใช้งาน | 2.7V – 5.5V |
| ช่วงอุณหภูมิ | –40°C ถึง 150°C |
| ความแม่นยำ | ±1°C (–10°C ถึง 80°C) |
| Interface | I2C / SMBus (สูงสุด 400 kHz) |
| Default Resolution | 11-bit (0.25°C/LSB) หลัง Power-On Reset |
| Max Resolution | 14-bit (0.03125°C/LSB) |
| Data Format | Two's complement, left-justified ใน 16-bit register |

**LM73 Register Map**
| Register | Pointer Address | ขนาด | คำอธิบาย |
|---|---|---|---|
| Temperature | 0x00 | 2 bytes | อ่านอุณหภูมิ (default ชี้ที่นี่หลัง POR) |
| Configuration | 0x01 | 2 bytes | ตั้งค่า resolution, shutdown, alert |
| THIGH | 0x02 | 2 bytes | Upper limit สำหรับ ALERT |
| TLOW | 0x03 | 2 bytes | Lower limit สำหรับ ALERT |
| Control/Status | 0x04 | 1 byte | สถานะ resolution ปัจจุบัน, alert flag |
| ID Register | 0x07 | 1 byte | คืนค่า 0x09 เสมอ (LM73 identifier) |

**Resolution Configuration (Configuration Register Bits 6:5)**
| RES[1:0] | Resolution | LSB Value | Max Conversion Time |
|---|---|---|---|
| 00 | 11-bit (default) | 0.25°C | 14 ms |
| 01 | 12-bit | 0.125°C | 28 ms |
| 10 | 13-bit | 0.0625°C | 56 ms |
| 11 | 14-bit (max) | 0.03125°C | 112 ms |

> ⚠️ **CRITICAL AI INSTRUCTION — RAW-TO-CELSIUS FORMULA:**
>
> ข้อมูลอุณหภูมิเก็บในรูป left-justified (MSB อยู่ทางซ้ายสุดคือ Sign bit) ใน 16-bit register
> โดยที่ **Bit 7 มีค่าเท่ากับ 1°C** เสมอ ไม่ว่าจะอ่านด้วยความละเอียดกี่ bit ก็ตาม (11-bit ถึง 14-bit)
> 
> **สูตรการแปลงที่ถูกต้องสูตรเดียว (UNIVERSAL FORMULA):**
> นำ byte 0 และ byte 1 มาต่อกันเป็น `int16_t` แล้ว **หารด้วย 128.0f** ตรงๆ โดย **ไม่ต้อง Shift bit**
> - `return (float)((int16_t)((raw[0] << 8) | raw[1])) / 128.0f;`
>
> ⚠️ **ความสับสนที่พบบ่อย (AI Hallucination):** ห้ามนำค่าไป shift right `>> 2` หรือ `>> 5` แล้วค่อยหารด้วย 32 หรือ 128 เด็ดขาด เพราะจะทำให้อุณหภูมิเพี้ยนต่ำกว่าความเป็นจริง (เช่น ได้ 3°C แทนที่จะเป็น 26°C)

```c
// ═══════════════════════════════════════════════════════
// LM73 อ่านอุณหภูมิ — Universal Formula (รองรับทุก resolution)
// ═══════════════════════════════════════════════════════
float read_lm73_temp(void) {
    uint8_t raw[2];
    uint8_t reg = 0x00; // Temperature register
    esp_err_t ret = i2c_master_write_read_device(
        I2C_NUM_1, 0x4D, &reg, 1, raw, 2, pdMS_TO_TICKS(100)
    );
    if (ret != ESP_OK) return -999.0f;

    // Left-justified 16-bit: Bit 7 = 1°C. ดังนั้นหารด้วย 128 (2^7)
    int16_t temp_raw = (int16_t)((raw[0] << 8) | raw[1]);
    return (float)temp_raw / 128.0f; 
}
```

**กฎที่ต้องปฏิบัติอย่างเคร่งครัด — LM73**
- **I2C Bus:** ใช้ `I2C_NUM_1` (SDA=GPIO4, SCL=GPIO5) เสมอ — ห้ามใช้ `I2C_NUM_0`
- **GPIO4 Conflict:** ห้าม `gpio_set_level(GPIO_NUM_4, ...)` ใดๆ ในโปรเจคที่ใช้ LM73 (ชนกับ BT LED)
- **Formula:** ใช้ `(float)raw / 128.0f` เสมอ ห้ามแต่งสูตรเอง
- **Error value:** คืน -999.0f เมื่อ I2C error — ตรวจสอบค่านี้ก่อน display เสมอ
- **Conversion time:** รอ อย่างน้อย 14 ms หลัง power-on ก่อนอ่านค่าแรก

### 15.3 LDR Light Sensor — Full Rule Set
| Property | Detail |
|---|---|
| ประเภท | Photoresistor (LDR — Light Dependent Resistor) |
| GPIO | GPIO36 — Input-only |
| ADC Unit | ADC_UNIT_1 |
| ADC Channel | ADC_CHANNEL_0 |
| วงจร | Voltage divider — ค่าลดลงเมื่อแสงมาก (inverted) |
| ช่วงค่า | 0 (แสงมาก / สว่าง) ถึง 4095 (มืด) |
| Attenuation | ADC_ATTEN_DB_12 (0–3.3V range) |

**กฎที่ต้องปฏิบัติอย่างเคร่งครัด — LDR**
- **GPIO36 คือ Input-only** — ห้ามพยายาม `gpio_set_level`, `gpio_config` เป็น output, หรือใส่ pull-up/pull-down
- **ADC API:** ใช้เฉพาะ `esp_adc/adc_oneshot.h` เท่านั้น — ห้ามใช้ `adc1_get_raw`, `adc1_config_width`, `driver/adc.h`
- **ค่า inverted:** ค่า ADC สูง = มืด, ค่า ADC ต่ำ = สว่าง (วงจร LDR บนบอร์ด)
- **WiFi + ADC:** LDR อยู่บน ADC1 (GPIO36) จึงใช้งานได้แม้เปิด WiFi — ห้ามย้ายไป ADC2
- **อย่าใช้ GPIO36 ร่วมกับ peripheral อื่น:** GPIO36 ถูก tie ไว้กับ LDR hardware divider แล้ว

```c
// ═══════════════════════════════════════════════════════
// LDR Sensor — อ่านค่าแสง และแปลงเป็น Lux โดยประมาณ
// ═══════════════════════════════════════════════════════
// ค่า raw:   0   = สว่างมาก (bright)
// ค่า raw: 4095 = มืดมาก (dark)
// หมายเหตุ: ตัวเลข Lux เป็นเพียงค่าประมาณ ไม่ใช่ calibrated จริง

int ldr_get_raw(adc_oneshot_unit_handle_t handle) {
    int raw = 0;
    adc_oneshot_read(handle, ADC_CHANNEL_0, &raw);
    return raw; // 0 = bright, 4095 = dark
}

// แปลง raw เป็น % ความสว่างที่ใช้งานได้จริง (Calibrated 0-100%)
#define LDR_ADC_MIN_VAL 0    // ค่าดิบตอนสว่างสุด (สว่างจ้า)
#define LDR_ADC_MAX_VAL 900  // ค่าดิบตอนมืดสุด (มืดสนิท - ปรับตามสภาพแวดล้อมห้องจริง)

int ldr_get_brightness_percent(int raw) {
    if (raw <= LDR_ADC_MIN_VAL) return 100; // สว่าง 100%
    if (raw >= LDR_ADC_MAX_VAL) return 0;   // มืด 0%
    // Linear Mapping แจกแจงเปอร์เซ็นต์ตามสเกลจริงที่ Calibrate ไว้
    return 100 - ((raw - LDR_ADC_MIN_VAL) * 100 / (LDR_ADC_MAX_VAL - LDR_ADC_MIN_VAL));
}

// ตัวอย่างการจัดประเภทแสง
const char* ldr_classify(int raw) {
    if (raw < 500)        return "Very Bright";
    else if (raw < 1500)  return "Bright";
    else if (raw < 2500)  return "Medium";
    else if (raw < 3500)  return "Dim";
    else                  return "Dark";
}
```

**กฎการแก้ปัญหา ADC Noise แกว่ง (Jumping Digits)**
> ⚠️ **CRITICAL ADC NOISE RULE FOR AI:** ESP32 ADC มีสัญญาณรบกวนสูงมากและไวต่อความถี่แสงไฟบ้าน 50Hz เพื่อป้องกันไม่ให้ตัวเลขบนจอภาพกระโดดไปมา (Tens/Units fluctuation) 
> 1. **Time-Spaced Sampling:** ห้ามอ่าน Multisampling รัวๆ ในลูปเดียวโดยไม่มีการหน่วงเวลา ต้องใส่ `esp_rom_delay_us(500);` (ต้อง `#include "esp_rom_sys.h"` — **ไม่ใช่** `esp_rom_delay_us.h` ซึ่งไม่มีจริง) ระหว่างแต่ละ Sample เสมอ
> 2. **EMA Filter:** ต้องจัดทำ Exponential Moving Average (EMA) เพื่อกรองความสมูทของค่าก่อนนำไปแสดงผลเสมอ เช่น `filtered = (filtered * 9 + current) / 10;`

### 15.4 KXTJ3-1057 Accelerometer — Additional Rules
กฎเต็มอยู่ใน Section 12 แล้ว ส่วนนี้สรุปกฎสำคัญที่มักเกิดข้อผิดพลาด
- **ห้าม init I2C_NUM_0 ซ้ำ:** เรียก `matrix_init()` ก่อนเสมอ — KXTJ3 ใช้ bus เดียวกัน
- **ตรวจ WHO_AM_I:** register 0x0F ต้องคืน 0x35 — ถ้าไม่ใช่ให้ return error ทันที
- **Stand-by ก่อน config:** เขียน 0x00 ไปที่ `CTRL_REG1` ก่อนเปลี่ยน resolution หรือ range
- **Sensitivity ตามตาราง:**
  | GSEL | Range | Sensitivity (12-bit) |
  |---|---|---|
  | 00 | ±2g | 1024 LSB/g |
  | 01 | ±4g | 512 LSB/g |
  | 10 | ±8g | 256 LSB/g |
  | 11 | ±16g | 128 LSB/g |
- **Data shift:** ข้อมูลเป็น left-justified — ต้อง `>> 4` เสมอสำหรับ 12-bit mode
- **Shake/Tilt Detection:** ใช้ Z-axis เทียบกับ ±1g เพื่อตรวจสอบการเอียง

```c
// ═══════════════════════════════════════════════════════
// ตัวอย่าง: ตรวจจับการเขย่า (Shake Detection)
// ═══════════════════════════════════════════════════════
bool kxtj3_is_shaking(kxtj3_data_t *data, float threshold_g) {
    // คำนวณ total acceleration magnitude
    float mag = sqrtf(
        data->x_g * data->x_g +
        data->y_g * data->y_g +
        data->z_g * data->z_g
    );
    // ถ้า magnitude เบี่ยงออกจาก 1g (gravity) เกิน threshold → กำลังถูกเขย่า
    return fabsf(mag - 1.0f) > threshold_g;
}

// ตัวอย่าง: ตรวจสอบการเอียง (Tilt Detection) จาก Z-axis
typedef enum {
    TILT_FLAT,      // วางราบ Z ≈ +1g
    TILT_UPSIDE,    // คว่ำ Z ≈ -1g
    TILT_SIDEWAYS,  // เอียง |Z| < 0.5g
} tilt_state_t;

tilt_state_t kxtj3_get_tilt(kxtj3_data_t *data) {
    if (data->z_g > 0.7f)       return TILT_FLAT;
    else if (data->z_g < -0.7f) return TILT_UPSIDE;
    else                         return TILT_SIDEWAYS;
}
```

### 15.5 External Sensors — กฎการเชื่อมต่อผ่าน JST Connector
> **AI INSTRUCTION:** เมื่อผู้ใช้ต้องการต่อเซ็นเซอร์ภายนอก ให้ใช้ขาตามตารางนี้และตรวจสอบ conflict ก่อนเสมอ

**พอร์ต JST ที่แนะนำสำหรับเซ็นเซอร์ภายนอก**
| พอร์ต | GPIO | ADC Channel | ใช้สำหรับ | ข้อจำกัด |
|---|---|---|---|---|
| IN1 | GPIO32 | ADC1_CH4 | Analog sensor (LM35, Potentiometer, Soil) | ใช้ ADC1 → WiFi-safe |
| IN2 | GPIO33 | ADC1_CH5 | Analog sensor (Gas, MQ-series) | ใช้ ADC1 → WiFi-safe |
| IN3 | GPIO34 | ADC1_CH6 | Digital/Analog input-only | ไม่มี pull-up/pull-down |
| IN4 | GPIO35 | ADC1_CH7 | Digital/Analog input-only | ไม่มี pull-up/pull-down |
| OUT1 | GPIO26 | — | Digital output, DAC, PWM trigger | DAC2 capable |
| OUT2 | GPIO27 | — | Digital output, PWM | — |

> ⚠️ **US-016 Analog Ultrasonic:** ถ้าต้องการใช้ US-016 บน IN1 (GPIO32/ADC1_CH4) ต้องต่อ Voltage Divider (R1=10kΩ, R2=20kΩ) ก่อน pin เพื่อลดแรงดัน 5V → 3.3V และต้องเป็นบอร์ด iA หรือ V1.6 เท่านั้น

> ⚠️ **WIFI + ADC กฎสำคัญ:** ADC2 (GPIO25, GPIO26, GPIO27 ฯลฯ) ไม่สามารถใช้งานได้ ขณะที่ WiFi ทำงานอยู่ ให้ใช้ ADC1 บน IN1 (GPIO32) และ IN2 (GPIO33) สำหรับ analog sensor ทุกกรณีที่มี WiFi

**กฎการต่อ Digital Sensor (1-Wire / GPIO)**
```c
// ═══════════════════════════════════════════════════════════════
// ตัวอย่าง: ตรวจสอบ PIR / IR Sensor บน IN1 (GPIO32) — Digital
// ═══════════════════════════════════════════════════════════════
// ⚠️ ห้ามใช้ IN3/IN4 (GPIO34/35) กับ sensor ที่ต้องการ pull-up
// เพราะไม่มี internal pull-up — ต้องใช้ external 10kΩ pull-up resistor

void external_digital_sensor_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_32),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   // ✅ IN1 มี pull-up ใช้ได้
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// สำหรับ IN3/IN4 (GPIO34/35) ต้องใส่ pull-up ภายนอก
// gpio_config_t สำหรับ GPIO34:
// .pull_up_en = GPIO_PULLUP_DISABLE  ← MUST — ไม่มี internal pull-up
// .pull_down_en = GPIO_PULLDOWN_DISABLE ← MUST — ไม่มี internal pull-down
```

**กฎการต่อ Analog Sensor (ADC)**
```c
// ═══════════════════════════════════════════════════════════════
// ตัวอย่าง: LM35 Temperature Sensor บน IN1 (GPIO32 / ADC_CHANNEL_4)
// LM35: output = 10mV/°C → voltage_mv / 10.0 = °C
// ═══════════════════════════════════════════════════════════════

// ใน init_adc() ให้เปลี่ยน channel:
// #define EXTERNAL_SENSOR_CHAN  ADC_CHANNEL_4   // IN1 = GPIO32

// การอ่านค่า:
// ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw));
// adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv);
// float temp_c = voltage_mv / 10.0f;  // สำหรับ LM35

// ═══════════════════════════════════════════════════════════════
// ADC Channel Map สำหรับ JST External Sensor
// ═══════════════════════════════════════════════════════════════
// IN1 = GPIO32 → ADC_CHANNEL_4 (ADC_UNIT_1)
// IN2 = GPIO33 → ADC_CHANNEL_5 (ADC_UNIT_1)
// IN3 = GPIO34 → ADC_CHANNEL_6 (ADC_UNIT_1)
// IN4 = GPIO35 → ADC_CHANNEL_7 (ADC_UNIT_1)
// LDR = GPIO36 → ADC_CHANNEL_0 (ADC_UNIT_1) ← built-in เท่านั้น
```

**กฎ HC-SR04 Ultrasonic Sensor (ผ่าน ESP-IDF esp_timer)**
> ⚠️ **AI INSTRUCTION:** HC-SR04 ต้องการการวัด pulse width อย่างแม่นยำ ห้ามใช้ `vTaskDelay` หรือ polling loop ธรรมดาในการวัด ให้ใช้ `esp_timer_get_time()` ซึ่งให้ค่า microsecond จาก hardware timer

```c
// ═══════════════════════════════════════════════════════════════
// HC-SR04 บน OUT1 (TRIG=GPIO26) และ IN1 (ECHO=GPIO32)
// ⚠️ ห้ามใช้ Arduino pulseIn() — ไม่มีใน ESP-IDF
// ═══════════════════════════════════════════════════════════════
#include "esp_timer.h"
#include "driver/gpio.h"

#define HCSR04_TRIG_GPIO  GPIO_NUM_26  // OUT1
#define HCSR04_ECHO_GPIO  GPIO_NUM_32  // IN1

void hcsr04_init(void) {
    // TRIG: Output
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << HCSR04_TRIG_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_conf);

    // ECHO: Input
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << HCSR04_ECHO_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_conf);
    gpio_set_level(HCSR04_TRIG_GPIO, 0);
}

// คืนค่าระยะทางเป็น cm, คืน -1.0f ถ้า timeout
float hcsr04_measure_cm(void) {
    // ส่ง trigger pulse 10µs
    gpio_set_level(HCSR04_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(HCSR04_TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(HCSR04_TRIG_GPIO, 0);

    // รอ ECHO เป็น HIGH (timeout 30ms)
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(HCSR04_ECHO_GPIO) == 0) {
        if ((esp_timer_get_time() - start) > 30000) return -1.0f;
    }

    // จับเวลา pulse width
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(HCSR04_ECHO_GPIO) == 1) {
        if ((esp_timer_get_time() - pulse_start) > 30000) return -1.0f;
    }
    int64_t pulse_end = esp_timer_get_time();

    // แปลง pulse duration → ระยะทาง
    // ความเร็วเสียง ≈ 343 m/s = 0.0343 cm/µs
    // ระยะ = (pulse_us * 0.0343) / 2 (ไป-กลับ)
    float pulse_us = (float)(pulse_end - pulse_start);
    return (pulse_us * 0.0343f) / 2.0f;
}
```

### 15.6 ADC Anti-Pattern Checklist (สำหรับ AI และนักพัฒนา)
ก่อนเขียนโค้ด ADC ทุกครั้ง ตรวจสอบ:

| ❌ ห้ามทำ | ✅ ให้ทำแทน |
|---|---|
| `#include "driver/adc.h"` | `#include "esp_adc/adc_oneshot.h"` |
| `adc1_config_width(...)` | `adc_oneshot_new_unit(...)` |
| `adc1_config_channel_atten(...)` | `adc_oneshot_config_channel(...)` |
| `adc1_get_raw(ADC1_CHANNEL_0)` | `adc_oneshot_read(handle, ADC_CHANNEL_0, &raw)` |
| `esp_adc_cal_characterize(...)` | `adc_cali_create_scheme_curve_fitting(...)` |
| `ADC_ATTEN_DB_11` | `ADC_ATTEN_DB_12` |
| GPIO36 เป็น output | GPIO36 เป็น input เท่านั้น |
| ADC2 channel ขณะ WiFi เปิด | ใช้เฉพาะ ADC1 (GPIO32–39) |
| อ่าน analog จาก OUT1/OUT2 (GPIO26/27) ขณะ WiFi | ย้ายมาใช้ IN1/IN2 แทน |

### 15.7 Sensor GPIO Conflict Matrix — FINAL CHECK

| GPIO | On-Board Sensor | JST Port | ความขัดแย้ง |
|---|---|---|---|
| GPIO36 | LDR (ADC1_CH0) | — | Input-only, ห้าม output |
| GPIO4 | LM73 SDA (I2C_NUM_1) | — | ⚠️ ขัดกับ BT LED |
| GPIO21 | KXTJ3 SDA + LED Matrix SDA | I2C Header SDA | ใช้ร่วมได้บน I2C_NUM_0 |
| GPIO22 | KXTJ3 SCL + LED Matrix SCL | I2C Header SCL | ใช้ร่วมได้บน I2C_NUM_0 |
| GPIO32 | — | IN1 (Analog/Digital) | ADC1_CH4, touch9 — WiFi-safe / US-016 ANALOG OUT (ผ่าน Voltage Divider) |
| GPIO33 | — | IN2 (Analog/Digital) | ADC1_CH5, touch8 — WiFi-safe |
| GPIO34 | — | IN3 (Input-only) | ไม่มี pull-up/down internal |
| GPIO35 | — | IN4 (Input-only) | ไม่มี pull-up/down internal |
| GPIO26 | — | OUT1 | DAC2, ADC2_CH9 — ห้ามใช้ ADC ขณะ WiFi |
| GPIO27 | — | OUT2 | ADC2_CH7 — ห้ามใช้ ADC ขณะ WiFi |

## 16. OUTPUT RULES — Compatible Displays & Actuators
> **AI INSTRUCTION:** ส่วนนี้รวบรวมกฎการเชื่อมต่ออุปกรณ์แสดงผลและกลไกควบคุม (Output) ทั้งบนบอร์ดและภายนอก ตรวจสอบข้อจำกัดของพินก่อนสั่งงานเสมอ

### 16.1 On-Board Output Summary
| อุปกรณ์ | GPIO | Interface / API | ข้อจำกัด / หมายเหตุ |
|---|---|---|---|
| **LED Matrix (16x8)** | SDA=21, SCL=22 | `I2C_NUM_0` (HT16K33) | ⚠️ บังคับใช้ `rows_to_columns_16x8()` เสมอ |
| **Buzzer (Piezo)** | GPIO13 | `driver/ledc.h` (PWM) | ห้ามใช้คำสั่ง `tone()` ของ Arduino |
| **Wi-Fi LED** | GPIO2 | `driver/gpio.h` (Active HIGH) | เลี่ยงการใช้เป็น Output อย่างอื่น |
| **BT LED** | GPIO4 | `driver/gpio.h` (Active HIGH) | ⚠️ **CONFLICT:** ห้ามใช้ถ้าเปิด I2C LM73 |

> ⚠️ **LED MATRIX DIGIT ALIGNMENT RULES (FONT 4x7):**
> หน้าจอ 16x8 ของ KidBright ประกอบด้วยจอ 8x8 สองจอต่อกัน (ซ้าย: col 0-7, ขวา: col 8-15) เพื่อความสวยงาม โปรดใช้ `col_offset` ดังนี้:
> - **กรณีแสดงเลข 1 ตัว (ให้อยู่ตรงกลางระหว่าง 2 จอ):** ใช้ `col_offset = 6` (ครอบคลุม col 6,7,8,9)
> - **กรณีแสดงเลข 2 ตัว (ให้แต่ละตัวอยู่ตรงกลางของแต่ละจอ):** ตัวหน้าใช้ `col_offset = 2` (ซ้าย), ตัวหลังใช้ `col_offset = 10` (ขวา)

### 16.2 External Outputs via JST Connectors
การเชื่อมต่ออุปกรณ์ Output ภายนอกผ่านพอร์ต JST ให้ใช้ขา **OUT1** และ **OUT2** เป็นหลัก

| พอร์ต JST | GPIO | การใช้งานที่แนะนำ | ความสามารถพิเศษ |
|---|---|---|---|
| **OUT1** | GPIO26 | โมดูลรีเลย์, สัญญาณควบคุมมอเตอร์ (EN), เสียง | รองรับ **DAC_CHANNEL_2** (สร้างสัญญาณแอนะล็อกแท้ 0-3.3V) |
| **OUT2** | GPIO27 | โมดูลรีเลย์, ไฟ LED ภายนอก, สัญญาณควบคุมมอเตอร์ | Digital I/O, PWM (`ledc`) |

> ⚠️ **คำเตือนวงจรขับมอเตอร์ (Motor Driver):** โมดูลอย่าง L298N หรือ TB6612FNG ต้องการไฟเลี้ยงมอเตอร์แยกต่างหาก **ห้ามดึงไฟ 5V/3.3V จากบอร์ด KidBright ไปขับมอเตอร์โดยตรง** ให้ใช้ขา OUT1/OUT2 ส่งเฉพาะสัญญาณ Logic/PWM ไปควบคุมเท่านั้น

**ตัวอย่าง: การควบคุมโมดูลรีเลย์บน OUT1 (GPIO26)**
```c
#include "driver/gpio.h"

#define RELAY_PIN GPIO_NUM_26

void relay_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(RELAY_PIN, 0); // ปิดรีเลย์เป็นค่าเริ่มต้น
}
```

### 16.3 Servo Motor Rules (CRITICAL CONFLICT)
บอร์ดมีช่องเสียบ Servo สีเหลือง/แดง/ดำ (Terminal) แยกให้โดยเฉพาะ ซึ่งดึงไฟเลี้ยงจากจุดต่อ Servo Power แยกต่างหาก

| ช่องเชื่อมต่อ | GPIO | สัญญาณ | กฎความปลอดภัย (Conflict Rule) |
|---|---|---|---|
| **SERVO1** | GPIO16 | PWM 50Hz | ⚠️ **MUTUALLY EXCLUSIVE กับปุ่ม SW1** ห้ามใช้ถ้าโปรเจกต์มีปุ่มกด |
| **SERVO2** | GPIO17 | PWM 50Hz | ✅ **แนะนำให้ใช้เป็นค่าเริ่มต้น** ปลอดภัยจากการชนกันของฮาร์ดแวร์ |

**กฎการขับเซอร์โว:**
- ต้องใช้ `driver/ledc.h` สร้าง PWM ความถี่ 50Hz (Period = 20ms)
- ความกว้างพัลส์ (Pulse Width) ทั่วไปคือ 500µs (0 องศา) ถึง 2500µs (180 องศา)

### 16.4 I2C External Displays (OLED / LCD)
สามารถนำจอ I2C ภายนอกมาเสียบที่พิน I2C Header ของบอร์ดได้ (SDA, SCL, 3.3V, GND)

| หน้าจอ | IC / Interface | I2C Address ทั่วไป | หมายเหตุ |
|---|---|---|---|
| **OLED 0.96"** | SSD1306 | `0x3C` หรือ `0x3D` | แสดงผลกราฟิกได้ดี |
| **LCD 1602 / 2004** | PCF8574 (I2C Backpack) | `0x27` หรือ `0x3F` | ต้องใช้จอแบบ 3.3V Logic |

> ⚠️ **I2C BUS RULE:** พิน I2C Header ถูกต่อเข้ากับ `I2C_NUM_0` (SDA=21, SCL=22) ซึ่งเป็นบัสเดียวกับ LED Matrix และ Accelerometer
> - **ห้าม** เรียก `i2c_driver_install()` หรือ `i2c_param_config()` ซ้ำสำหรับจอภาพ
> - ให้เรียกใช้ `matrix_init()` เพื่อเปิดบัสครั้งเดียว แล้วใช้ `i2c_master_write_to_device()` ส่งข้อมูลไปยัง Address ของหน้าจอภายนอกได้เลย> - ให้เรียกใช้ `matrix_init()` เพื่อเปิดบัสครั้งเดียว แล้วใช้ `i2c_master_write_to_device()` ส่งข้อมูลไปยัง Address ของหน้าจอภายนอกได้เลย

---

## 17. EXTERNAL SENSOR RULES — Compatible Sensors (V1.3 / V1.6)
> **AI INSTRUCTION:** กฎในส่วนนี้ครอบคลุมเซนเซอร์ภายนอกทั้งหมดที่รองรับบน KidBright V1.3 และ V1.6 ตรวจสอบ PIN และ Interface ก่อนเชื่อมต่อทุกครั้ง

### 17.1 Sensor Compatibility Table

| Sensor | Interface | Pins | Measure | Unit | Notes |
|---|---|---|---|---|---|
| **HC-SR04** (Ultrasonic) | Digital (2-pin) | TRIG=OUT1/18/19/23, ECHO=IN1/IN2/IN3/IN4 | Distance | cm | ใช้ `esp_timer_get_time()` วัด pulse width เท่านั้น |
| **US-016** (Ultrasonic Analog) | Analog (1-pin) | OUT=IN1/IN2 (ADC1_CH4/CH5) | Distance | cm | **iA/V1.6 เท่านั้น** — Analog output แตกต่างจาก HC-SR04 ไม่มี TRIG/ECHO |
| **HC-SR501** (PIR) | Digital (1-pin) | IN1 / IN2 / IN3 / IN4 | Motion (heat change) | — | Output เป็น HIGH เมื่อตรวจพบการเคลื่อนไหว |
| **DHT11** | Digital (1-pin) | IN1 / IN2 / IN3 / IN4 | Temp + Humidity | °C / %RH | ต้องการ timing-critical bit-bang protocol |
| **DS18B20** (Waterproof) | 1-Wire (1-pin) | IN1 / IN2 / IN3 / IN4 | Temperature | °C | **บังคับ** ต่อ pull-up resistor 4.7 kΩ ระหว่าง DATA และ VCC |
| **DS18B20** (Air) | 1-Wire (1-pin) | IN1 / IN2 / IN3 / IN4 | Temperature | °C | ไม่ต้องการ external pull-up ถ้าใช้ parasitic power |
| **HW-511** (Line Tracking) | Digital (1-pin) | IN1 / IN2 / IN3 / IN4 | Line detection | — | Output เป็น HIGH/LOW ตามสีพื้นผิว |
| **BME280** | I2C | SDA0 / SCL0 (`I2C_NUM_0`) | Temp / Pressure / Altitude / Humidity | °C / hPa / m / % | ⚠️ **ใช้บัสเดียวกับ LED Matrix** อย่า re-install I2C driver |
| **Light Sensor Module** | Analog (1-pin) | IN1 / IN2 / IN3 / IN4 (ADC) | Light intensity | — | **V1.6 เท่านั้น** — ต้องอ่านด้วย ADC oneshot API |

---

### 17.2 HC-SR501 PIR Sensor Rules
> ⚠️ **AI INSTRUCTION:** PIR sensor ใช้เพียง 1 digital pin เป็น input เท่านั้น ห้ามกำหนดเป็น Output

```c
// HC-SR501 บน IN1 (GPIO32)
#include "driver/gpio.h"

#define PIR_GPIO GPIO_NUM_32

void pir_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIR_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// คืน 1 = ตรวจพบการเคลื่อนไหว, 0 = ไม่พบ
int pir_read(void) {
    return gpio_get_level(PIR_GPIO);
}
```

---

### 17.2b US-016 Analog Ultrasonic Distance Sensor Rules
> ⚠️ **AI INSTRUCTION:** US-016 **แตกต่างจาก HC-SR04 อย่างสิ้นเชิง** — ไม่มี TRIG/ECHO pin ใช้ **Analog Voltage Output** แทน ต้องอ่านผ่าน ADC เท่านั้น ห้ามเขียนโค้ดที่ส่ง trigger pulse ไปยัง US-016
> ⚠️ **iA / V1.6 เท่านั้น** — V1.3 / V1.5 Rev 3.1 / Rev 3.1G ไม่รองรับ Analog Input บน IN1–IN4

**ข้อมูล IC / Module**

| Property | Detail |
|---|---|
| ชื่อ | US-016 Analog Ultrasonic Distance Sensor |
| Output Type | **Analog Voltage** (ไม่ใช่ Digital Pulse) |
| Working Voltage | DC 5V |
| Working Current | 3.8 mA |
| Operating Temp | 0 ถึง +70°C |
| Detecting Distance | **2 cm – 300 cm** (3m range) หรือ 2 cm – 100 cm (1m range) |
| Detecting Accuracy | ±0.3 cm + 1% |
| Resolution | 1 mm |
| Induction Angle | < 15° |
| Output Voltage | 0 V – Vcc (proportional to distance) |
| Interface | **Analog Voltage Output** — 1 signal wire |

**Pin Configuration**

| ขา | คำอธิบาย |
|---|---|
| VCC | ต่อกับไฟ 5V (ห้ามต่อ 3.3V — sensor ต้องการ 5V) |
| GND | ต่อ GND |
| OUT | Analog voltage output — ต่อเข้า ADC input |
| RANGE | ตั้งค่าระยะสูงสุด: **Float (ไม่ต่อ) = 3m**, ต่อ GND = 1m |

**สูตรแปลงค่า Voltage → Distance**

```
ระยะ 3 เมตร: Distance (cm) = (ADC_raw × 3 × Vcc) / (1024 × Vcc) × 100
            หรือในทางปฏิบัติสำหรับ ESP32 (ADC 12-bit, 3.3V reference):
            Distance (cm) = (ADC_raw / 4095.0) × 300.0

ระยะ 1 เมตร: Distance (cm) = (ADC_raw / 4095.0) × 100.0
```

> ⚠️ **VOLTAGE DIVIDER WARNING:** US-016 ใช้ไฟ 5V แต่ ESP32 ADC รับแรงดันสูงสุดที่ 3.3V
> ต้องต่อ Voltage Divider (ตัวต้านทาน) ก่อน ADC pin เพื่อลดแรงดันจาก 5V → 3.3V
> แนะนำ: R1=10kΩ (ระหว่าง OUT กับ ADC pin), R2=20kΩ (ระหว่าง ADC pin กับ GND)
> **ห้ามต่อ 5V โดยตรงเข้า GPIO ของ ESP32 เด็ดขาด — จะทำให้ชิปเสียหาย**

**ตัวอย่างโค้ด ESP-IDF v5.x สำหรับ US-016 (Analog ADC)**

```c
// US-016 Analog Ultrasonic Sensor — อ่านระยะทางผ่าน ADC
// ต่อ OUT ของ US-016 (ผ่าน Voltage Divider 5V→3.3V) → IN1 (GPIO32 / ADC1_CH4)
// RANGE pin ปล่อย float = ช่วงวัด 3 เมตร

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define US016_ADC_CHAN    ADC_CHANNEL_4   // IN1 = GPIO32
#define US016_MAX_DIST_CM 300.0f          // เมื่อ RANGE pin floating = 3m

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t us016_cali = NULL;
static bool us016_cali_ok = false;

void us016_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,  // 0–3.3V range
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, US016_ADC_CHAN, &chan_cfg));

    // Calibration (optional but recommended)
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &us016_cali) == ESP_OK) {
        us016_cali_ok = true;
    }
#endif
}

// อ่านระยะทางเป็น cm โดยเฉลี่ย N ครั้ง (ลด noise)
// คืน -1.0f ถ้าอ่านค่าไม่ได้
float us016_read_cm(int samples) {
    if (samples <= 0) samples = 1;
    int32_t sum = 0;
    for (int i = 0; i < samples; i++) {
        int raw = 0;
        if (adc_oneshot_read(adc1_handle, US016_ADC_CHAN, &raw) != ESP_OK) {
            return -1.0f;
        }
        sum += raw;
        esp_rom_delay_us(500); // หน่วงระหว่างตัวอย่างเพื่อลด noise
    }
    int avg_raw = sum / samples;

    // แปลง raw → ระยะทาง
    // ถ้าใช้ Voltage Divider (5V→3.3V) ค่า raw จะแทนแรงดัน 0–3.3V
    // ซึ่งสอดคล้องกับ output ของ US-016 ที่ 0–Vcc (0–5V)
    // จึงสามารถคำนวณตรงได้ว่า: distance = (raw / 4095) × MAX_DIST
    float dist_cm = ((float)avg_raw / 4095.0f) * US016_MAX_DIST_CM;

    // กรองค่านอกช่วง (sensor dead zone < 2cm)
    if (dist_cm < 2.0f) return 2.0f;
    if (dist_cm > US016_MAX_DIST_CM) return US016_MAX_DIST_CM;
    return dist_cm;
}
```

**กฎที่ต้องปฏิบัติอย่างเคร่งครัด — US-016**

| กฎ | รายละเอียด |
|---|---|
| **ห้ามส่ง Trigger pulse** | US-016 ไม่มี TRIG pin — ต่างจาก HC-SR04 โดยสิ้นเชิง |
| **Voltage Divider บังคับ** | OUT ของ US-016 อาจสูงถึง 5V — ต้องลดเหลือ ≤ 3.3V ก่อน ADC |
| **ใช้ ADC1 เท่านั้น** | ADC2 ใช้ไม่ได้ขณะ WiFi เปิด — ต่อเข้า IN1 (GPIO32/ADC1_CH4) |
| **iA / V1.6 เท่านั้น** | V1.3 / V1.5 Rev 3.1 / Rev 3.1G IN1–IN4 ไม่รองรับ ADC |
| **RANGE pin** | Float = 3m, ต่อ GND = 1m — ปรับตามความต้องการ |
| **Averaging** | อ่านหลายครั้งแล้วเฉลี่ยเสมอ — US-016 อาจมี noise เหมือน LDR |
| **Power** | ต้องใช้ไฟ 5V จากบอร์ด (ขา VCC/USB 5V) ห้ามใช้ 3.3V |

---

### 17.3 DHT11 Temperature & Humidity Sensor Rules
> ⚠️ **AI INSTRUCTION:** DHT11 ใช้ single-wire bit-bang protocol ที่ต้องการ timing แม่นยำ ต้องใช้ `esp_rom_delay_us()` สำหรับ microsecond delay และ `esp_timer_get_time()` สำหรับวัดเวลา ห้ามใช้ `vTaskDelay()` ในส่วน bit-reading

**กฎการอ่าน DHT11:**
- เริ่มต้นด้วยการส่ง start signal: ดึง DATA ลง LOW อย่างน้อย 18 ms แล้วปล่อยขึ้น HIGH
- รอ DHT11 ตอบสนอง (LOW 80µs → HIGH 80µs)
- อ่าน 40 bits: bit 0 = pulse ประมาณ 26-28µs, bit 1 = pulse ประมาณ 70µs
- ตรวจสอบ checksum ก่อนใช้ข้อมูลเสมอ

```c
// DHT11 บน IN2 (GPIO33)
#define DHT11_GPIO GPIO_NUM_33
// ⚠️ ต้องสลับ mode ระหว่าง OUTPUT (start signal) และ INPUT (reading)
// ใช้ gpio_set_direction() เพื่อเปลี่ยน direction แบบ dynamic
```

---

### 17.4 DS18B20 Temperature Sensor Rules
> ⚠️ **AI INSTRUCTION:** DS18B20 Waterproof version **บังคับ** ต่อ external pull-up resistor 4.7 kΩ ระหว่างขา DATA และ VCC เสมอ — ถ้าไม่ใส่ resistor จะอ่านค่าไม่ได้หรืออ่านได้ผิดพลาด

| รุ่น | External Resistor | หมายเหตุ |
|---|---|---|
| DS18B20 Waterproof | **บังคับ 4.7 kΩ** (DATA → VCC) | ไม่มี resistor = อ่านค่าไม่ได้ |
| DS18B20 Air (module) | ไม่จำเป็น | มี resistor built-in บนโมดูลแล้ว |

```c
// DS18B20 ใช้ 1-Wire protocol บน IN1 (GPIO32)
// ⚠️ ห้ามใช้ I2C หรือ SPI API — DS18B20 ใช้ OneWire protocol เท่านั้น
// ต้องส่ง Reset Pulse → Presence Pulse → ROM Command → Function Command → Read Scratchpad
```

---

### 17.5 BME280 I2C Sensor Rules
> ⚠️ **AI INSTRUCTION:** BME280 ใช้บัส `I2C_NUM_0` ซึ่งเป็นบัสเดียวกับ LED Matrix (HT16K33 @ 0x70) ห้ามเรียก `i2c_driver_install()` ซ้ำ

| Property | Value |
|---|---|
| Interface | I2C — `I2C_NUM_0` |
| I2C Address | `0x76` (SDO → GND) หรือ `0x77` (SDO → VCC) |
| Pins | SDA = GPIO21, SCL = GPIO22 |
| Measures | Temperature (°C), Pressure (hPa), Altitude (m), Humidity (%) |

**กฎการใช้งาน BME280 ร่วมกับ LED Matrix:**
- เรียก `matrix_init()` ก่อน (เปิด I2C_NUM_0 ครั้งเดียว)
- ใช้ `i2c_master_write_to_device()` / `i2c_master_read_from_device()` กับ address `0x76` หรือ `0x77` ได้เลย โดยไม่ต้อง install driver ใหม่

```c
// ตัวอย่าง: อ่าน raw temperature register จาก BME280
#define BME280_ADDR 0x76
uint8_t reg = 0xFA; // temp_msb register
uint8_t data[3];
i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                             &reg, 1, data, 3, pdMS_TO_TICKS(100));
```

---

### 17.6 Light Sensor Module Rules (V1.6 Only — Analog)
> ⚠️ **AI INSTRUCTION:** Light Sensor Module แบบ Analog (ไม่ใช่ LDR บนบอร์ด) ใช้ได้เฉพาะ **KidBright V1.6** เท่านั้น เพราะ V1.6 รองรับ Analog Input บนพอร์ต IN1–IN4 ส่วน V1.3 IN1–IN4 **ไม่รองรับ Analog Input**

| Board | IN1–IN4 Analog Input |
|---|---|
| **V1.3** | ❌ ไม่รองรับ |
| **V1.6** | ✅ รองรับ (ใช้ ADC1 Channel 4–7) |

```c
// Light Sensor Module (Analog) บน IN1 (GPIO32 / ADC1_CH4) — V1.6 เท่านั้น
// ใช้ adc_oneshot API เหมือนกับ LDR บนบอร์ด แต่เปลี่ยน channel เป็น ADC_CHANNEL_4
adc_oneshot_chan_cfg_t chan_cfg = {
    .atten    = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &chan_cfg); // IN1

int raw = 0;
adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw);
// raw: 0 (มืด) → 4095 (สว่าง)
```

---

## 18. EXTERNAL OUTPUT DEVICE RULES — Compatible Actuators (V1.3 / V1.6)
> **AI INSTRUCTION:** กฎในส่วนนี้ครอบคลุมอุปกรณ์ Output ภายนอกทั้งหมดที่รองรับ ตรวจสอบขั้วและ logic level ก่อนเชื่อมต่อทุกครั้ง

### 18.1 Output Device Compatibility Table

| อุปกรณ์ | Pins | Output Type | Notes |
|---|---|---|---|
| **Active Buzzer** | OUT1 / OUT2 / 18 / 19 / 23 | Digital HIGH/LOW | ไม่ต้องการ PWM — HIGH = เปิดเสียง |
| **Passive Buzzer** | OUT1 / OUT2 / 18 / 19 / 23 | PWM | ต้องการสัญญาณ PWM จาก `ledc` เพื่อกำหนดความถี่เสียง |
| **Relay 5V** | OUT1 / OUT2 / 18 / 19 / 23 | Digital | ⚠️ ดูกฎ Active LOW / Active HIGH ตามช่องที่เชื่อมต่อ |
| **LED RGB** | OUT1 / OUT2 / 18 / 19 / 23 (3 ขา) | Digital / PWM | ต้องการ 3 พิน สำหรับ R, G, B แยกกัน |
| **LED** | OUT1 / OUT2 / 18 / 19 / 23 | Digital / PWM | ต่อ current-limiting resistor เสมอ |
| **Fan Motor DC 5V** | OUT1 / OUT2 / 18 / 19 / 23 | Digital | ⚠️ ห้ามขับมอเตอร์โดยตรงจากพิน — ต้องใช้ transistor หรือ driver module |
| **Vibration Motor DC** | OUT1 / OUT2 / 18 / 19 / 23 | Digital | ⚠️ ต้องใช้ transistor ขับกระแสเช่นเดียวกับ Fan Motor |
| **LCD 1602 (I2C)** | SDA0 / SCL0 (`I2C_NUM_0`) | I2C | ดูกฎใน Section 16.4 |
| **Servo Motor 180°** | SERVO1 / SERVO2 | PWM 50Hz | **V1.6 เท่านั้น** — ดูกฎใน Section 16.3 |

---

### 18.2 Active Buzzer Rules
> **กฎ:** Active Buzzer มีวงจรกำเนิดเสียงในตัว ไม่ต้องการ PWM — ส่ง `HIGH` เพื่อเปิดเสียง, `LOW` เพื่อปิด

```c
// Active Buzzer บน OUT1 (GPIO26)
#include "driver/gpio.h"
#define ACTIVE_BUZZER_PIN GPIO_NUM_26

void active_buzzer_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ACTIVE_BUZZER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(ACTIVE_BUZZER_PIN, 0); // ปิดเสียงเป็นค่าเริ่มต้น
}

void active_buzzer_on(void)  { gpio_set_level(ACTIVE_BUZZER_PIN, 1); }
void active_buzzer_off(void) { gpio_set_level(ACTIVE_BUZZER_PIN, 0); }
```

---

### 18.3 Passive Buzzer Rules (External Module)
> **กฎ:** Passive Buzzer ภายนอก (ไม่ใช่ buzzer บนบอร์ด GPIO13) ต้องการ PWM เพื่อกำหนดความถี่เสียง ใช้ `ledc` เหมือนกับ buzzer บนบอร์ด

```c
// Passive Buzzer ภายนอก บน OUT2 (GPIO27)
// ใช้ ledc API เหมือน Section 5 (Buzzer LEDC PWM) แต่เปลี่ยน gpio_num เป็น GPIO_NUM_27
// ledc_channel.gpio_num = GPIO_NUM_27;
```

---

### 18.4 Relay 5V Rules — CRITICAL Logic Level Warning
> ⚠️ **AI INSTRUCTION:** ทิศทาง logic (Active HIGH / Active LOW) ของรีเลย์ **ขึ้นกับช่องที่เชื่อมต่อ** ไม่ใช่ตัวรีเลย์เอง

| ช่องเชื่อมต่อ | เปิดรีเลย์ | ปิดรีเลย์ | หมายเหตุ |
|---|---|---|---|
| ขา **18 / 19 / 23** | `gpio_set_level(..., 1)` HIGH | `gpio_set_level(..., 0)` LOW | Active HIGH |
| ช่อง **Out1 / Out2** | `gpio_set_level(..., 0)` LOW | `gpio_set_level(..., 1)` HIGH | **Active LOW** |
| ช่อง **USB Port** | `gpio_set_level(..., 0)` LOW | `gpio_set_level(..., 1)` HIGH | **Active LOW** |

> ⚠️ **คำเตือน:** รีเลย์ควบคุมไฟ AC หรือไฟแรงดันสูง — ต้องต่อผ่านวงจรฟิวส์และ power supply ภายนอกแยกต่างหากเสมอ **ห้ามต่อไฟ AC เข้ากับพิน KidBright โดยตรงโดยเด็ดขาด**

---

### 18.5 LED RGB Rules
> **กฎ:** LED RGB ต้องการพิน 3 ขา (R, G, B) แยกกัน ต่อ current-limiting resistor บนทุกสี ใช้ PWM (`ledc`) เพื่อผสมสีได้

```c
// LED RGB บน OUT1(R=GPIO26), OUT2(G=GPIO27), IO18(B=GPIO18)
// ⚠️ ถ้าใช้ V1.3: ใช้ขา 18, 19, 23 สำหรับพิน 3 ตัว
// ⚠️ ถ้าใช้ V1.6: ใช้ OUT1, OUT2 และ connector 3-pin

// ตัวอย่าง: ตั้ง R=100%, G=0%, B=50% (ม่วงแดง)
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_R, 1023); // R full
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_G, 0);    // G off
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_B, 512);  // B half
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_R);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_G);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_B);
```

---

### 18.6 Motor (Fan / Vibration) Rules
> ⚠️ **AI INSTRUCTION: ห้ามขับมอเตอร์โดยตรงจากพิน GPIO ของ KidBright** กระแสสูงสุดของพิน GPIO ESP32 คือประมาณ 40 mA ซึ่งไม่เพียงพอสำหรับมอเตอร์ทุกชนิด

**วิธีที่ถูกต้อง:**
- ใช้ **NPN transistor** (เช่น 2N2222, BC547) เป็น switch ขับมอเตอร์
- ใช้ **motor driver module** (เช่น L298N, TB6612FNG, DRV8833)
- ต่อ **flyback diode** (เช่น 1N4007) ขนานกับมอเตอร์เพื่อป้องกัน back-EMF เสมอ

```
KidBright OUT1 (GPIO26) → Base ของ NPN transistor (ผ่าน 1kΩ)
Collector ของ NPN → ขา (-) ของมอเตอร์
Emitter ของ NPN → GND
ไฟ 5V ภายนอก → ขา (+) ของมอเตอร์ (ผ่าน flyback diode)
```

---

### 18.7 DS18B20 (1-Wire) Rules
> **AI INSTRUCTION:** เซ็นเซอร์วัดอุณหภูมิแบบกันน้ำ **DS18B20** ใช้โปรโตคอล 1-Wire ในการอ่านค่าคุณ **MUST** ต่อตัวต้านทาน Pull-up ขนาด **4.7kΩ** คร่อมระหว่างขา DATA และ VCC (3.3V) เสมอ (หากไม่ได้ใช้โมดูลที่มี Pull-up ในตัว)

---

### 18.8 I2C Bus Sharing (BME280 / I2C LCD) Rules
> **AI INSTRUCTION:** อุปกรณ์ I2C ภายนอกทั้งหมด (เช่น จอ LCD 16x2 I2C, เซ็นเซอร์ BME280) จะต่อผ่านพอร์ต I2C ของ KidBright ซึ่งใช้บัส `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) ร่วมกับ LED Matrix และ Accelerometer (KXTJ3) บนบอร์ด
> ⚠️ **DO NOT reinstall the I2C driver** หากมี `matrix_init()` หรือมีการเรียก `i2c_driver_install` สำหรับ `I2C_NUM_0` ไปแล้ว เพราะจะทำให้เกิด error `ESP_ERR_INVALID_STATE`

---

### 18.9 Buzzer Type Rules
> **AI INSTRUCTION:** บอร์ด KidBright ตัวแผ่นหลักใช้ **Passive Buzzer** (ต้องใช้ PWM ผ่าน `ledc` ในการกำเนิดเสียง)
> หากต่อ Buzzer ภายนอก:
> - **Active Buzzer:** ใช้เพียงสัญญาณ Digital `HIGH` / `LOW` ควบคุม (`gpio_set_level`)
> - **Passive Buzzer:** ต้องใช้สัญญาณ PWM (`ledc`) เพื่อสร้างความถี่เสียงที่ต้องการ

> ⚠️ **ESP-IDF v5.x BREAKING CHANGE: `ledc_stop()` signature เปลี่ยนแปลง!**
>
> **Function Signature (ESP-IDF v5.x):**
> ```c
> esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level);
> ```
>
> **ตัวอย่างที่ถูกต้อง:**
> ```c
> // ✅ CORRECT — ESP-IDF v5.x
> // idle_level=0: ขา GPIO กลับเป็น LOW เมื่อหยุด (ถูกต้องสำหรับ Buzzer)
> ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
>
> // ❌ WRONG — compile error ใน ESP-IDF v5.x ทุกเวอร์ชัน
> // error: too few arguments to function 'ledc_stop'
> ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
> ```
>
> **ทางเลือก: ปิดเสียงโดยไม่หยุด Timer (แนะนำสำหรับ melody player)**
> ```c
> ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
> ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
> ```

---

## 19. PORT CAPABILITY RULES — Complete Version Comparison (V1.1 ถึง V1.6)
> **AI INSTRUCTION:** ความสามารถของพอร์ตและเซนเซอร์แตกต่างกันระหว่างแต่ละเวอร์ชัน ต้องตรวจสอบบอร์ดเวอร์ชันก่อนเขียนโค้ดเสมอ

### 19.1 Port Capability Comparison Table

| KidBright Pin | GPIO | Digital Input | Digital Output | Analog Input (ADC) | Analog Output |
|---|---|---|---|---|---|
| **IN1** | GPIO32 | ✅ All versions | ❌ | ✅ **iA / V1.6 เท่านั้น** (ADC1_CH4) | ❌ |
| **IN2** | GPIO33 | ✅ All versions | ❌ | ✅ **iA / V1.6 เท่านั้น** (ADC1_CH5) | ❌ |
| **IN3** | GPIO34 | ✅ All versions | ❌ | ✅ **iA / V1.6 เท่านั้น** (ADC1_CH6) | ❌ |
| **IN4** | GPIO35 | ✅ All versions | ❌ | ✅ **iA / V1.6 เท่านั้น** (ADC1_CH7) | ❌ |
| **Out1** | GPIO26 | ❌ | ✅ All versions | ❌ | ✅ All versions (DAC2) |
| **Out2** | GPIO27 | ❌ | ✅ All versions | ❌ | ❌ |
| **IO18** | GPIO18 | ✅ All versions | ✅ All versions | ❌ | ✅ All versions |
| **IO19** | GPIO19 | ✅ All versions | ✅ All versions | ❌ | ✅ All versions |
| **IO23** | GPIO23 | ✅ All versions | ✅ All versions | ❌ | ✅ All versions |

> ✅ = รองรับ | ❌ = ไม่รองรับ

---

### 19.2 Critical Differences — All Versions V1.1 → V1.6

| Feature | V1.1 / V1.2 | V1.3 | V1.4 | V1.5 Rev 3.1 (NECTEC) | V1.5 Rev 3.1**G** (Gravitech OEM) | V1.5 iA (INEX) | V1.6 (Gravitech) |
|---|---|---|---|---|---|---|---|
| **USB Bridge** | Cypress CY7C65213 | FTDI FT232RL | FTDI | CP2102 | CP2102 | CP2102 | CP2102 |
| **USB Connector** | Micro-USB | Micro-USB | Micro-USB | **Micro-USB** | **Micro-USB** | **USB-C** | USB-C |
| **SW2 GPIO** | GPIO14 | GPIO14 | GPIO14 | **GPIO14** | **GPIO14** | GPIO14 | GPIO14 |
| **LED Status GPIOs** | GPIO23,2,5,12 (×4) | GPIO23,2,5,12 (×4) | GPIO2,4 (×2) | GPIO2,4 (×2) | GPIO2,4 (×2) | GPIO2,4 (×2) | GPIO2,4 (×2) |
| Analog Input บน IN1–IN4 | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ (ADC1_CH4–CH7) | ✅ (ADC1_CH4–CH7) |
| Accelerometer | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ KXTJ3-1057 (0x0E) | ✅ MPU-6050 (0x68) |
| Gyroscope | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ MPU-6050 (0x68) |
| Magnetometer | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ (MPU-6050 = 6-axis) |
| RGB LED on-board | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ 6 ดวง (WS2812) |
| Servo Connector | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ (GPIO15, GPIO17) |
| USB Host (Type-A) | ✅ GPIO25 | ✅ GPIO25 | ✅ GPIO25 | ✅ GPIO25 | ✅ GPIO25 | ✅ | ✅ |
| RTC on I2C_NUM_1 | ✅ 0x6F | ✅ 0x6F | ✅ 0x6F | ✅ 0x6F | ✅ 0x6F | — | — |
| I2C_NUM_0 devices | HT16K33 (0x70) | HT16K33 (0x70) | HT16K33 (0x70) | HT16K33 (0x70) | HT16K33 (0x70) | HT16K33+KXTJ3 | HT16K33+MPU6050 |

> ⚠️ **AI CRITICAL — SW2 PIN DIFFERENCE:**
> - V1.1 / V1.2 / V1.3 / V1.4: SW2 = **GPIO14**
> - V1.5 Rev 3.1 (NECTEC): SW2 = **GPIO14**
> - V1.5 Rev 3.1G / V1.5 iA / V1.6: SW2 = **GPIO17**
>
> ⚠️ **AI CRITICAL — LED STATUS GPIO DIFFERENCE:**
> - V1.1 / V1.2 / V1.3: LED×4 = **GPIO23 (BT), GPIO2 (WiFi), GPIO5 (NTP), GPIO12 (IoT)**
> - V1.4 / V1.5 / V1.6: LED×2 = **GPIO2 (WiFi), GPIO4 (BT)**
> ห้ามเขียนโค้ด GPIO5/GPIO12 เป็น LED indicator บน V1.4+ เพราะวงจรถูกเปลี่ยนแล้ว
>
> ⚠️ **AI CRITICAL — GPIO12 BOOT-STRAPPING (V1.1/V1.2/V1.3 เท่านั้น):**
> GPIO12 บน V1.1/V1.2/V1.3 ถูกใช้เป็น IoT LED แต่ยังคงเป็น boot-strapping pin
> ห้าม pull-up GPIO12 ไปยัง VCC 3.3V ขณะ boot — จะทำให้ ESP32 เลือก 1.8V flash voltage และ boot fail
>
> ⚠️ **AI CRITICAL:** V1.5 Rev 3.1 และ Rev 3.1G ใช้ **Micro-USB** ไม่ใช่ USB-C และ **ไม่มี KXTJ3** — ห้ามเขียนโค้ดที่ init KXTJ3 สำหรับบอร์ดทั้งสอง
>
> ⚠️ **AI CRITICAL — V1.6 MPU-6050 ≠ 9-DOF:**
> - MPU-6050 คือ **6-axis IMU** (Accel 3-axis + Gyro 3-axis) เท่านั้น
> - **ไม่มี magnetometer ในตัว** — ห้ามเรียก API หรือ address ที่เกี่ยวกับ magnetometer จาก MPU-6050 โดยตรง
> - ถ้าต้องการ compass/heading ต้องต่อ **external magnetometer** (เช่น QMC5883L @ 0x0D หรือ HMC5883L @ 0x1E) เข้า I2C_NUM_0 แยกต่างหาก
>
> ⚠️ **AI CRITICAL — V1.6 RGB LED (Gerora/WS2812):**
> - ใช้โปรโตคอล **1-wire NZR** ไม่ใช่ GPIO ธรรมดา — ต้องใช้ ESP-IDF **RMT peripheral** หรือ `led_strip` component
> - ห้ามใช้ `gpio_set_level()` ควบคุม RGB LED โดยตรง — จะไม่ทำงาน
> - ตรวจสอบ GPIO pin จาก PCB silkscreen เสมอก่อนเขียนโค้ด

---

---

### 19.2b V1.6 — MPU-6050 (Accel + Gyro) ESP-IDF Code
> **AI INSTRUCTION:** MPU-6050 บน V1.6 ใช้ I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22) address `0x68`
> ⚠️ ห้าม reinstall I2C driver ถ้ามี `matrix_init()` ไปแล้ว (จะเกิด `ESP_ERR_INVALID_STATE`)

```c
#include "driver/i2c.h"
#include "esp_log.h"

#define MPU6050_ADDR       0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT 0x3B
#define MPU6050_GYRO_XOUT  0x43
#define I2C_MASTER_NUM     I2C_NUM_0   // Shared with HT16K33 on V1.6
#define I2C_TIMEOUT_MS     100

// Wake up MPU-6050 (call AFTER matrix_init or I2C already installed)
esp_err_t mpu6050_init(void) {
    // MPU-6050 boots in sleep mode — clear bit 6 of PWR_MGMT_1 to wake
    uint8_t data[2] = { MPU6050_PWR_MGMT_1, 0x00 };
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR,
                                      data, 2, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

// Read raw 16-bit accelerometer values (X, Y, Z) in ±2g range (default)
esp_err_t mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t reg = MPU6050_ACCEL_XOUT;
    uint8_t buf[6];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR,
                        &reg, 1, buf, 6, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (ret == ESP_OK) {
        *ax = (int16_t)((buf[0] << 8) | buf[1]);
        *ay = (int16_t)((buf[2] << 8) | buf[3]);
        *az = (int16_t)((buf[4] << 8) | buf[5]);
    }
    return ret;
}

// Read raw 16-bit gyroscope values (X, Y, Z) in ±250°/s range (default)
esp_err_t mpu6050_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t reg = MPU6050_GYRO_XOUT;
    uint8_t buf[6];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR,
                        &reg, 1, buf, 6, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (ret == ESP_OK) {
        *gx = (int16_t)((buf[0] << 8) | buf[1]);
        *gy = (int16_t)((buf[2] << 8) | buf[3]);
        *gz = (int16_t)((buf[4] << 8) | buf[5]);
    }
    return ret;
}

// Convert raw accel to g (±2g scale = 16384 LSB/g)
float mpu6050_raw_to_g(int16_t raw) {
    return (float)raw / 16384.0f;
}

// Convert raw gyro to °/s (±250°/s scale = 131 LSB/°/s)
float mpu6050_raw_to_dps(int16_t raw) {
    return (float)raw / 131.0f;
}
```

> ⚠️ **REGISTER MAP NOTE:**
> - `0x6B` PWR_MGMT_1: เขียน `0x00` เพื่อ wake up (reset บอร์ดจะ sleep ทันที)
> - `0x3B` ACCEL_XOUT_H: burst read 6 bytes = AX_H, AX_L, AY_H, AY_L, AZ_H, AZ_L
> - `0x43` GYRO_XOUT_H: burst read 6 bytes = GX_H, GX_L, GY_H, GY_L, GZ_H, GZ_L

---

### 19.2c V1.6 — RGB LED (Gerora / WS2812B) ESP-IDF RMT Code
> **AI INSTRUCTION:** RGB LED 6 ดวงบน V1.6 ใช้ WS2812B protocol — ต้องใช้ **RMT peripheral** ไม่ใช่ GPIO
> ตรวจสอบ GPIO pin จาก PCB silkscreen ก่อนใช้งาน (ตัวอย่างใช้ `GPIO_NUM_18` ซึ่งเป็นค่าทั่วไป)

```c
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "led_strip.h"              // Requires esp-idf/components/led_strip or idf-extra-components

// ⚠️ ตรวจสอบ GPIO pin จาก silkscreen บอร์ด V1.6 ก่อน
#define RGB_LED_GPIO       GPIO_NUM_18   // ตัวอย่าง — อาจต่างกันในแต่ละล็อต
#define RGB_LED_COUNT      6             // V1.6 มี 6 ดวง

static led_strip_handle_t led_strip = NULL;

// Initialize RGB LED strip (call once in app_main)
esp_err_t rgb_led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num   = RGB_LED_GPIO,
        .max_leds         = RGB_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812B = GRB order
        .led_model        = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src         = RMT_CLK_SRC_DEFAULT,
        .resolution_hz   = 10 * 1000 * 1000,  // 10 MHz
        .flags.with_dma  = false,
    };
    return led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

// Set one LED color (index 0–5, r/g/b = 0–255)
esp_err_t rgb_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    return led_strip_set_pixel(led_strip, index, r, g, b);
}

// Apply all pending color changes — must call after rgb_set()
esp_err_t rgb_refresh(void) {
    return led_strip_refresh(led_strip);
}

// Turn off all LEDs
esp_err_t rgb_clear(void) {
    return led_strip_clear(led_strip);
}

// Example: fill all 6 LEDs with one color
void rgb_fill_all(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < RGB_LED_COUNT; i++) {
        rgb_set(i, r, g, b);
    }
    rgb_refresh();
}
```

> **CMakeLists.txt — เพิ่ม dependency:**
> ```cmake
> idf_component_register(SRCS "main.c"
>     INCLUDE_DIRS "."
>     REQUIRES led_strip driver)
> ```
>
> **idf_component.yml — เพิ่ม component (ถ้าใช้ idf-extra-components):**
> ```yaml
> dependencies:
>   espressif/led_strip: ">=2.4.0"
> ```

---

### 19.3 GPIO Output Logic Rules Summary

> ⚠️ **AI INSTRUCTION: กฎ Active HIGH / Active LOW ต้องตรงกับช่องเชื่อมต่อ** — ผิดพลาดทำให้อุปกรณ์ทำงานกลับทิศ

| ช่องเชื่อมต่อ | GPIO | เปิดอุปกรณ์ | ปิดอุปกรณ์ |
|---|---|---|---|
| ขา **18** | GPIO18 | `HIGH (1)` | `LOW (0)` |
| ขา **19** | GPIO19 | `HIGH (1)` | `LOW (0)` |
| ขา **23** | GPIO23 | `HIGH (1)` | `LOW (0)` |
| ช่อง **Out1** | GPIO26 | **`LOW (0)`** | **`HIGH (1)`** |
| ช่อง **Out2** | GPIO27 | **`LOW (0)`** | **`HIGH (1)`** |
| ช่อง **USB Port** | IO25 (V1.6) | **`LOW (0)`** | **`HIGH (1)`** |
| ช่อง **IN1–IN4** (Input) | GPIO32–35 | `HIGH (1)` | `LOW (0)` |
| ช่อง **3-pin connector O1** (V1.6) | IO26 | **`LOW (0)`** | **`HIGH (1)`** |
| ช่อง **3-pin connector O2** (V1.6) | IO27 | **`LOW (0)`** | **`HIGH (1)`** |

---

### 19.4 Input Port Rules Summary

| ช่องเชื่อมต่อ | GPIO | Digital In | Analog In | ข้อจำกัด |
|---|---|---|---|---|
| **IN1** | GPIO32 | ✅ | ✅ (iA / V1.6) | ต้องการ external pull-up/down ถ้าไม่มีบน sensor module |
| **IN2** | GPIO33 | ✅ | ✅ (iA / V1.6) | ต้องการ external pull-up/down ถ้าไม่มีบน sensor module |
| **IN3** | GPIO34 | ✅ | ✅ (iA / V1.6) | **Input-only** ไม่มี internal pull-up/down |
| **IN4** | GPIO35 | ✅ | ✅ (iA / V1.6) | **Input-only** ไม่มี internal pull-up/down |
| **IO18** | GPIO18 | ✅ | ❌ | รองรับ Digital Input + Output |
| **IO19** | GPIO19 | ✅ | ❌ | รองรับ Digital Input + Output |
| **IO23** | GPIO23 | ✅ | ❌ | รองรับ Digital Input + Output |

> ⚠️ **GPIO34 / GPIO35 (IN3 / IN4): ไม่มี internal pull-up หรือ pull-down** — ถ้า sensor ไม่มี pull-up resistor ในตัว ต้องต่อ external pull-up (10 kΩ ไปยัง 3.3V) เสมอ มิฉะนั้น pin จะ floating และอ่านค่าสุ่ม

---

## 20. Formula Kid CAR + DRV8833 Motor Driver — Verified Hardware Rules

> 🔬 **ข้อมูลนี้ยืนยันจากการทดสอบจริง 100% บนฮาร์ดแวร์จริง**
> บอร์ดรับ: KidBright32 V1.4 + Formula Kid CAR rev 1.0
> บอร์ดส่ง: KidBright32 V1.5 Rev 3.1G + Formula Kid rev 1.1

### 20.1 GPIO Pinout — Formula Kid CAR rev 1.0

| สัญญาณ | GPIO | หมายเหตุ |
|---|---|---|
| DRV_NSLEEP | GPIO_NUM_23 | ต้องตั้งเป็น HIGH เสมอ เพื่อ Enable DRV8833 |
| DRV_AIN1 | GPIO_NUM_18 | Motor A — phase 1 |
| DRV_AIN2 | GPIO_NUM_26 | Motor A — phase 2 |
| DRV_BIN1 | GPIO_NUM_19 | Motor B — phase 1 |
| DRV_BIN2 | GPIO_NUM_27 | Motor B — phase 2 |

> ⚠️ **AI RULE:** ห้ามใช้ GPIO25/14 สำหรับมอเตอร์บน Formula Kid CAR rev 1.0 — GPIO ที่ถูกต้องคือ 18/26/19/27 พร้อม nSLEEP=23

### 20.2 DRV8833 Motor Truth Table (ยืนยันจากฮาร์ดแวร์จริง)

> ⚠️ **CRITICAL:** DRV8833 บน Formula Kid CAR rev 1.0 มีพฤติกรรม **กลับขั้ว** จาก datasheet ทั่วไป
> ต้องใช้ค่าต่อไปนี้เท่านั้น ห้ามคำนวณเองจาก datasheet

```
motor_raw(a1, a2, b1, b2)  ←  ลำดับ: AIN1, AIN2, BIN1, BIN2

motor_raw(0,   0,   0,   0  ) = วิ่งหน้า (Forward)   ✅
motor_raw(255, 255, 255, 255) = ถอยหลัง (Backward)   ✅
motor_raw(0,   255, 0,   255) = หยุด (Stop/Brake)    ✅
motor_raw(D,   0,   0,   0  ) = เลี้ยวขวา 90°       ✅
motor_raw(0,   0,   D,   0  ) = เลี้ยวซ้าย 90°      ✅
(โดยที่ D = pct_to_duty ค่า 0-255)
```

### 20.3 Verified Receiver Code Reference

> ℹ️ **AI IMPLEMENTATION GUIDE:** โค้ดด้านล่างคือ implementation ที่ได้รับการทดสอบบนฮาร์ดแวร์จริงแล้วว่าทำงานได้สมบูรณ์ (100% Verified) เมื่อผู้ใช้ขอให้สร้างโค้ด Receiver สำหรับ Formula Kid CAR ให้ใช้โครงสร้างและ Logic ตามนี้เป็นมาตรฐานหลัก เพื่อให้ได้โค้ดที่ใช้งานได้ทันทีโดยไม่ติด error หรือปัญหาทางฮาร์ดแวร์
>
> ⚠️ **AI READING RULE:** เมื่อต้องสร้างโค้ด Receiver ให้อ่านโค้ดทั้งหมดในบล็อก `code block` นี้ **ทุก character** โดยไม่ตัดทอน แล้วใช้ verbatim — ห้ามสร้างโค้ดจากความจำหรือ semantic search เด็ดขาด

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define TAG "CAR_RX"

// ── I2C / LED Matrix ──────────────────────────────────────
#define I2C_SDA_GPIO  GPIO_NUM_21
#define I2C_SCL_GPIO  GPIO_NUM_22
#define HT16K33_ADDR  0x70

// ── DRV8833 GPIO ──────────────────────────────────────────
#define DRV_NSLEEP    GPIO_NUM_23
#define DRV_AIN1      GPIO_NUM_18
#define DRV_AIN2      GPIO_NUM_26
#define DRV_BIN1      GPIO_NUM_19
#define DRV_BIN2      GPIO_NUM_27

// ── LEDC ──────────────────────────────────────────────────
#define LEDC_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_RES      LEDC_TIMER_8_BIT
#define CH_AIN1       LEDC_CHANNEL_0
#define CH_AIN2       LEDC_CHANNEL_1
#define CH_BIN1       LEDC_CHANNEL_2
#define CH_BIN2       LEDC_CHANNEL_3

// ── ESP-NOW ───────────────────────────────────────────────
#define ESPNOW_CHANNEL  1

// ════════════════════════════════════════════════════════
//  Truth table ยืนยันจากการทดสอบจริง 100%:
//
//  motor_raw(0,   0,   0,   0  ) = วิ่งหน้า        ✅
//  motor_raw(255, 255, 255, 255) = ถอยหลัง         ✅
//  motor_raw(0,   255, 0,   255) = หยุด            ✅
//  motor_raw(D,   0,   0,   0  ) = เลี้ยวขวา 90°  ✅
//  motor_raw(0,   0,   D,   0  ) = เลี้ยวซ้าย 90° ✅
//
//  Protocol จาก TX:
//    999        = STOP
//    10 ~ 100   = เดินหน้า  (ค่า = ความเร็ว %)
//    -10 ~ -100 = ถอยหลัง  (ค่า = ความเร็ว %)
//    300 ~ 500  = เลี้ยว   (offset 400, <400=ซ้าย, >400=ขวา)
// ════════════════════════════════════════════════════════

static QueueHandle_t g_cmd_queue;

// ── LED Matrix Images ─────────────────────────────────────
static const uint8_t img_up[16]    = {0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01,
                                       0x01, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0x00, 0x00}; // U
static const uint8_t img_down[16]  = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x81, 0x81, 0x81,
                                       0x81, 0x81, 0x81, 0x7E, 0x3C, 0x00, 0x00, 0x00}; // D
static const uint8_t img_left[16]  = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0x01,
                                       0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00}; // L
static const uint8_t img_right[16] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x90, 0x90,
                                       0x98, 0x94, 0x62, 0x01, 0x00, 0x00, 0x00, 0x00}; // R
static const uint8_t img_stop[16]  = {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00,
                                       0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00}; // --

// ── I2C ───────────────────────────────────────────────────
static void i2c_init(void) {
    i2c_config_t c = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_SDA_GPIO,
        .scl_io_num       = I2C_SCL_GPIO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &c);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

static void matrix_cmd(uint8_t cmd) {
    i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR,
                               &cmd, 1, pdMS_TO_TICKS(50));
}

static void matrix_init(void) {
    matrix_cmd(0x21);
    matrix_cmd(0x81);
    matrix_cmd(0xEF);
}

static void matrix_draw(const uint8_t cols[16]) {
    uint8_t buf[17] = {0x00};
    for (int c = 0; c < 8; c++) {
        buf[1 + c*2] = cols[c];
        buf[2 + c*2] = cols[c + 8];
    }
    i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR,
                               buf, 17, pdMS_TO_TICKS(50));
}

// ── DRV8833 Init ──────────────────────────────────────────
static void drv8833_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << DRV_NSLEEP),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(DRV_NSLEEP, 1);

    ledc_timer_config_t t = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);

    gpio_num_t     pins[] = {DRV_AIN1, DRV_AIN2, DRV_BIN1, DRV_BIN2};
    ledc_channel_t chs[]  = {CH_AIN1,  CH_AIN2,  CH_BIN1,  CH_BIN2};
    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t ch = {
            .gpio_num   = pins[i],
            .channel    = chs[i],
            .speed_mode = LEDC_MODE,
            .timer_sel  = LEDC_TIMER_0,
            .duty       = 0,
            .hpoint     = 0,
        };
        ledc_channel_config(&ch);
    }
    ESP_LOGI(TAG, "DRV8833 init OK");
}

// ── Motor Low-level ───────────────────────────────────────
static void motor_raw(uint32_t a1, uint32_t a2,
                      uint32_t b1, uint32_t b2) {
    ledc_set_duty(LEDC_MODE, CH_AIN1, a1);
    ledc_update_duty(LEDC_MODE, CH_AIN1);
    ledc_set_duty(LEDC_MODE, CH_AIN2, a2);
    ledc_update_duty(LEDC_MODE, CH_AIN2);
    ledc_set_duty(LEDC_MODE, CH_BIN1, b1);
    ledc_update_duty(LEDC_MODE, CH_BIN1);
    ledc_set_duty(LEDC_MODE, CH_BIN2, b2);
    ledc_update_duty(LEDC_MODE, CH_BIN2);
}

static uint32_t pct_to_duty(int pct) {
    if (pct < 0)   pct = -pct;
    if (pct > 100) pct = 100;
    return (uint32_t)(pct * 255 / 100);
}

// ── คำสั่งพื้นฐาน ─────────────────────────────────────────

static void cmd_stop(void) {
    motor_raw(0, 255, 0, 255);
}

static void cmd_forward(int pct) {
    uint32_t brake = 255 - pct_to_duty(pct);
    motor_raw(0, brake, 0, brake);
}

static void cmd_backward(int pct) {
    uint32_t d = pct_to_duty(pct);
    motor_raw(d, 255, d, 255);
}

static void cmd_turn_left(int pct) {
    uint32_t d = pct_to_duty(pct);
    motor_raw(0, 0, d, 0);
}

static void cmd_turn_right(int pct) {
    uint32_t d = pct_to_duty(pct);
    motor_raw(d, 0, 0, 0);
}

// ── ESP-NOW Callback ──────────────────────────────────────
static void recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len) {
    if (len != sizeof(int32_t)) return;
    int32_t val;
    memcpy(&val, data, sizeof(int32_t));
    xQueueOverwrite(g_cmd_queue, &val);
}

// ── WiFi + ESP-NOW Init ───────────────────────────────────
static void espnow_init(void) {
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES ||
        r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_max_tx_power(40);
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Receiver MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    ESP_LOGI(TAG, "========================================");
}

// ── app_main ──────────────────────────────────────────────
void app_main(void) {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    g_cmd_queue = xQueueCreate(1, sizeof(int32_t));

    i2c_init();
    matrix_init();
    matrix_draw(img_stop);

    drv8833_init();
    cmd_stop();
    ESP_LOGI(TAG, "Initial STOP");

    espnow_init();
    ESP_LOGI(TAG, "รอรับคำสั่ง...");

    const uint8_t *cur_img = img_stop;
    int32_t val = 999;

    while (1) {
        // รอรับคำสั่งโดยไม่มี timeout (portMAX_DELAY)
        xQueueReceive(g_cmd_queue, &val, portMAX_DELAY);

        const uint8_t *new_img = img_stop;

        if (val == 999) {
            cmd_stop();
            new_img = img_stop;
            ESP_LOGI(TAG, "STOP");

        } else if (val >= 10 && val <= 100) {
            cmd_forward((int)val);
            new_img = img_up;
            ESP_LOGI(TAG, "FORWARD %ld%%", (long)val);

        } else if (val >= -100 && val <= -10) {
            cmd_backward((int)(-val));
            new_img = img_down;
            ESP_LOGI(TAG, "BACKWARD %ld%%", (long)(-val));

        } else if (val >= 300 && val <= 500) {
            int32_t js = val - 400;
            if (js <= -10) {
                cmd_turn_left((int)(-js));
                new_img = img_left;
                ESP_LOGI(TAG, "LEFT %ld%%", (long)(-js));
            } else if (js >= 10) {
                cmd_turn_right((int)js);
                new_img = img_right;
                ESP_LOGI(TAG, "RIGHT %ld%%", (long)js);
            } else {
                cmd_stop();
                new_img = img_stop;
            }

        } else {
            cmd_stop();
            new_img = img_stop;
            ESP_LOGW(TAG, "Unknown val: %ld", (long)val);
        }

        if (new_img != cur_img) {
            matrix_draw(new_img);
            cur_img = new_img;
        }
    }
}
```


### 20.4 ESP-NOW Protocol — Formula Kid CAR

| ค่า (int32_t) | ความหมาย | Action |
|---|---|---|
| `999` | STOP ฉุกเฉิน | `cmd_stop()` |
| `10` ถึง `100` | เดินหน้า | `cmd_forward(val)` |
| `-10` ถึง `-100` | ถอยหลัง | `cmd_backward(-val)` |
| `300` ถึง `500` | เลี้ยว (offset 400) | decode: js=val-400 |
| → js `-10` ถึง `-100` | เลี้ยวซ้าย | `cmd_turn_left(-js)` |
| → js `10` ถึง `100` | เลี้ยวขวา | `cmd_turn_right(js)` |

### 20.5 กฎบังคับสำหรับ AI

1. **ห้าม** ใช้ `motor_raw(d, 0, d, 0)` สำหรับ Forward — เป็นผลลัพธ์ที่ผิดบนฮาร์ดแวร์นี้
2. **ต้องใช้** `cmd_forward(pct)` ด้วยสูตร `brake = 255 - pct_to_duty(pct)` เสมอ
3. **nSLEEP (GPIO23) ต้อง HIGH** ก่อนใช้งาน — ถ้าไม่ set DRV8833 จะไม่ทำงาน
4. **ใช้ FreeRTOS Queue** (`xQueueCreate(1, sizeof(int32_t))` + `xQueueOverwrite`) แทน `volatile bool` เพื่อความ thread-safe
5. **ปิด Brownout** `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0)` เสมอ เพราะ WiFi+Motor ดึงกระแสสูง
6. **ลด WiFi TX Power** `esp_wifi_set_max_tx_power(40)` เพื่อลดการดึงกระแสสูงสุด