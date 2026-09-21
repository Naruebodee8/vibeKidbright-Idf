# KidBright32 — Developer Reference (ทุกรุ่น)

> **Raw source**: `raw/kidbright32iA_updated.md` (3600+ lines — ไฟล์ฉบับเต็ม)
> **Related**: [[kidbright32_pinout]] · [[sensor_guide]] · [[adc_api_guide]] · [[gpio_conflict]]

---

## ประวัติรุ่น KidBright (Timeline)

| รุ่น | ปี | MCU | USB | หมายเหตุ |
|------|-----|-----|-----|---------|
| V2016 | 2016 | ESP8266 | Micro-USB | Prototype — Android App ผ่าน WiFi |
| V1.0 | 2017 | ESP8266 | Micro-USB | รุ่นแรกที่แจกจ่าย |
| V1.1 | 2018 | ESP32-WROOM-32 | Micro-USB (Cypress) | รุ่นแรกที่ใช้ ESP32 |
| V1.2 | 2018 | ESP32-WROOM-32 | Micro-USB (Cypress) | PCB เล็กน้อย |
| V1.3 | 2019 | ESP32-WROOM-32 | Micro-USB (FTDI) | เปลี่ยน USB bridge |
| V1.4 | 2019–20 | ESP32-WROOM-32 | Micro-USB (FTDI) | LED ลดเหลือ 2 ดวง |
| **V1.5 Rev 3.1** | 2020 | ESP32-WROOM-32 | Micro-USB (CP2102) | NECTEC Standard |
| **V1.5 Rev 3.1G** | 2020 | ESP32-WROOM-32 | Micro-USB (CP2102) | Gravitech OEM |
| **V1.5 iA** | 2021–22 | ESP32-WROOM-32 | **USB-C** (CP2102) | INEX, เพิ่ม KXTJ3 Accel |
| **V1.6** | 2022+ | ESP32-WROOM-32 | **USB-C** (CP2102) | Gravitech, MPU6050 + RGB LED ×6 |
| **KidBright μAI** | 2024 | AllWinner V831 + ESP32-S3 | USB-C | Edge AI, Camera 2MP, IPS 1.3" |

> ⚠️ **SW2 = GPIO14** ทุกรุ่น V1.5+

---

## ESP32 GPIO Quick Reference (V1.5 iA — Main Target)

| GPIO | ฟังก์ชัน | หมายเหตุ |
|------|---------|---------|
| GPIO2 | WiFi LED | ⚠️ Boot strapping |
| GPIO4 | I2C_NUM_1 SDA / BT LED | เลือกได้อย่างเดียว |
| GPIO5 | I2C_NUM_1 SCL | — |
| GPIO13 | Passive Buzzer (LEDC) | — |
| **GPIO14** | **SW2 Button** (Active LOW) | ทุกรุ่น V1.5+ |
| GPIO16 | SW1 Button (Active LOW) / SERVO1 | เลือกได้อย่างเดียว |
| GPIO21 | I2C_NUM_0 SDA | LED Matrix + KXTJ3/MPU6050 |
| GPIO22 | I2C_NUM_0 SCL | LED Matrix + KXTJ3/MPU6050 |
| GPIO25 | USB Host (Active LOW) | — |
| GPIO26 | OUT1 (DAC2) | — |
| GPIO27 | OUT2 | — |
| GPIO32 | IN1 (ADC1_CH4 / Touch) | — |
| GPIO33 | IN2 (ADC1_CH5 / Touch) | — |
| GPIO34 | IN3 (ADC1_CH6) | Input-only, no pull |
| GPIO35 | IN4 (ADC1_CH7) | Input-only, no pull |
| GPIO36 | LDR (ADC1_CH0) | Input-only, no pull |

---

## I2C Device Map (V1.5 iA)

| Bus | Address | Device |
|-----|---------|--------|
| I2C_NUM_0 | 0x70 | HT16K33 (LED Matrix) |
| I2C_NUM_0 | 0x0E | KXTJ3-1057 (Accelerometer) |
| I2C_NUM_1 | 0x4D | LM73 (Temperature) |
| I2C_NUM_1 | 0x6F | MCP794xx (RTC) |

---

## ความแตกต่างระหว่างรุ่น

| Feature | Rev 3.1 | Rev 3.1G | iA | V1.6 |
|---------|---------|---------|-----|------|
| Accelerometer | ❌ | ❌ | ✅ KXTJ3 | ✅ MPU6050 |
| ADC บน IN1-IN4 | ❌ | ❌ | ✅ | ✅ |
| RGB LED | ❌ | ❌ | ❌ | ✅ ×6 |
| USB | Micro | Micro | **USB-C** | **USB-C** |
| SW2 GPIO | GPIO14 | GPIO14 | GPIO14 | GPIO14 |

---

## ESP8266 Boards (V2016, V1.0)

> ⚠️ ไม่รองรับ ESP-IDF — ใช้ได้เฉพาะ KidBright IDE / Arduino เท่านั้น

- ADC: 1× 10-bit (A0)
- I2C: Software I2C (GPIO4=SDA, GPIO5=SCL)
- ไม่มี GPIO header เหมือน ESP32

---

## KidBright μAI (2024)

MCU ใหม่ทั้งหมด:
- **AllWinner V831** (ARM Cortex-A7, Tina Linux) — Edge AI
- **ESP32-S3** — Wireless + UART
- Camera 2MP
- ไมโครโฟน
- จอ IPS 1.3"
- USB-C OTG + UART

---

## โปรเจกต์ที่ใช้ KidBright32

- [[formula_kid]] — Formula Kid Controller สนามแข่ง
- [[skate]] — Self-balancing 2-wheel robot
- [[minibike]] — Minibike workshop (ทรงตัว + วิทยุ + Line Follower)
- [[micromouse]] — Micromouse (ใช้ ESP32-S3 แยก)

---

## See Also

- [[kidbright32_pinout]] — Pinout โดยละเอียด
- [[sensor_guide]] — Sensor code + I2C init order
- [[adc_api_guide]] — ADC Oneshot API (v5.x)
- [[gpio_conflict]] — GPIO conflict table ทุกรุ่น

> 📄 **สำหรับข้อมูลฉบับเต็ม** (3600+ บรรทัด) อ่านได้จาก `raw/kidbright32iA_updated.md`
