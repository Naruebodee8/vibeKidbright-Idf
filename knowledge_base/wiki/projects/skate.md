# KidBright Skate Rev 1.3 — Technical Reference

> **Raw source**: `raw/skate_rev1_3.md`
> **Related**: [[kidbright32_pinout]] · [[sensor_guide]] · [[gpio_conflict]] · [[led_matrix]]

---

## ภาพรวม Hardware

**Skate V1.3** คือ expansion board ที่ต่อเข้ากับ KidBright32 ผ่าน J18 (10-pin header)
เพิ่ม motor driver, IMU (MPU6050), IR sensors, OLED display และวงจร power management

**ประเภท**: Two-wheeled self-balancing robot บนแพลตฟอร์มวงกลมสีเหลือง

---

## Physical Layout

```
┌─────────────────────────────────────────┐
│     KidBright32 iA Board (Top)          │
│  [OLED 128x64 SSD1306]  [SWITCH2]      │
│  [SERVO1]  [SERVO2]                     │
└─────────────────────────────────────────┘
         ↕ J18 10-pin header
┌─────────────────────────────────────────┐
│     Skate V1.3 Expansion Board          │
│  [MPU6050 IMU 0x68]                    │
│  [XL4005 5V step-down]  [SW3]  [J10]  │
└─────────────────────────────────────────┘
         ↕ GPIO18/19, GPIO32/33
┌─────────────────────────────────────────┐
│     External Motor Driver (Red PCB)     │
│     DRV8833 / TB6612FNG H-Bridge        │
└─────────────────────────────────────────┘
```

---

## GPIO Pin Mapping (Verified from Schematic)

| GPIO | Net Label | หน้าที่ |
|------|-----------|---------|
| GPIO18 | IN3_MOT | Left Motor control A |
| GPIO19 | IN4_MOT | Left Motor control B |
| GPIO32 | IN1 | Right Motor control A |
| GPIO33 | IN2 | Right Motor control B |
| GPIO34 | IN3_SEN | IR-R sensor (Right IR) |
| GPIO35 | IN4_SEN | IR-L sensor (Left IR) |
| GPIO21 | SDA0 | I2C_NUM_0 SDA |
| GPIO22 | SCL0 | I2C_NUM_0 SCL |
| GPIO39/VN | VN | Battery voltage monitor (ADC) |
| GPIO14 | SW2 | SW2 button (Active LOW) |
| GPIO16 | SW1 | SW1 button (Active LOW) |
| GPIO4 | SDA1 | I2C_NUM_1 SDA |
| GPIO5 | SCL1 | I2C_NUM_1 SCL |
| GPIO17 | SERVO2 | Servo 2 |
| GPIO13 | PWM | Buzzer (LEDC/PWM) |

---

## I2C Bus Architecture

```
I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22):
  0x70  HT16K33  — LED Matrix 16x8
  0x68  MPU6050  — 6-axis IMU (on Skate board)
  0x3C  SSD1306  — OLED 128x64 (optional)

I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5):
  0x4D  LM73     — Temperature sensor
```

> ⚠️ CRITICAL: `i2c_driver_install()` เรียกได้ **ครั้งเดียวต่อ port** เท่านั้น!
> ทุก device บน I2C_NUM_0 ใช้ bus เดียวกัน — ห้าม install ซ้ำ

---

## Power System

| ส่วน | รายละเอียด |
|---|---|
| Battery | 7.4V 2S Li-ion → J10 (XH2.54 2P) |
| 5V Rail | XL4005 step-down (motor logic) |
| 3.3V Rail | step-down (ESP32 + sensors) |
| Motor Enable | SW3 + Q1 (2N7002H MOSFET) |

### Battery Voltage Formula
```c
// GPIO39/VN, R5=100kΩ, R6=20kΩ
float Vbat = adc_raw * (3.3f / 4095.0f) * ((100.0f + 20.0f) / 20.0f);
// ≈ adc_raw * 0.009671f
```

---

## Motor Control (LEDC PWM)

```c
// Motor GPIO
#define MOTOR_L_A   GPIO_NUM_18   // Left  Motor IN3_MOT
#define MOTOR_L_B   GPIO_NUM_19   // Left  Motor IN4_MOT
#define MOTOR_R_A   GPIO_NUM_32   // Right Motor IN1
#define MOTOR_R_B   GPIO_NUM_33   // Right Motor IN2

// LEDC config
#define MOTOR_BITS  LEDC_TIMER_10_BIT   // duty: 0–1023
#define MOTOR_FREQ  20000               // 20 kHz

// direction: +1=forward, -1=backward, 0=brake
static void motor_set(ledc_channel_t ch_a, ledc_channel_t ch_b,
                      int duty, int direction);
```

---

## CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_timer
)
```

---

## Self-Balancing (PID Overview)

Skate ใช้ **MPU6050** วัด pitch angle แล้วควบคุม motor ด้วย PID:

```
Sensor: MPU6050 (I2C 0x68)
  → pitch angle (complementary filter หรือ Kalman)
  → PID output → motor duty
```

ค่า PID ต้องจูนตามน้ำหนักและการจัดวาง CG ของหุ่นแต่ละตัว

---

## สิ่งที่ควรระวัง

1. **ห้ามต่อมอเตอร์ตรงจาก GPIO** — ต้องผ่าน Motor Driver Module เสมอ
2. GPIO34, GPIO35 = **Input-only** (ไม่มี pull resistor ภายใน) — ต้องใช้ pull-up ภายนอก
3. GPIO18, GPIO19 ใช้กับ I2S ด้วย — ห้ามใช้ I2S ถ้าใช้ motor

---

## See Also

- [[kidbright32_pinout]] — Pinout ทั้งหมด
- [[sensor_guide]] — I2C init order
- [[gpio_conflict]] — GPIO conflict table
- [[led_matrix]] — LED Matrix บน I2C_NUM_0 เดียวกัน
