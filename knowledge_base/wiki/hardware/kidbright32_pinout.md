# KidBright32 — Pinout & Board Variants

> **หมวด:** Hardware Reference  
> **ไฟล์ดิบ (Raw):** `../raw/kidbright_sensor_guide.md` · `../raw/kidbright32iA_updated.md`  
> **เชื่อมโยง:** [[adc_api_guide]] · [[gpio_conflict]] · [[led_matrix]] · [[oled]] · [[formula_kid]] · [[minibike]] · [[skate]]

---

## บอร์ดที่รองรับ (Board Variants)

| รุ่น | ชื่อ | ผู้ผลิต | ความแตกต่างสำคัญ |
|------|------|---------|-----------------|
| V1.5 Rev 3.1 | KidBright32 Standard | NECTEC | ไม่มี Accelerometer · IN1–IN4 ไม่รองรับ ADC |
| V1.5 Rev 3.1G | KidBright32 OEM | Gravitech | เหมือน Rev 3.1 — ยืนยัน I2C scan Apr 2026 |
| V1.5 iA | KidBright32 iA | INEX | **มี KXTJ3 Accelerometer** · IN1–IN4 รองรับ ADC |
| V1.6 | KidBright32 V1.6 | Gravitech | คล้าย iA แต่ไม่มี KXTJ3 |

> ⚠️ **SW2 = GPIO14 สำหรับทุกบอร์ด** — ตรวจสอบ PCB silkscreen ก่อนเสมอ

---

## Sensor Map — On-board (ทุกรุ่น)

| เซนเซอร์ / อุปกรณ์ | โปรโตคอล | Bus / Pin | I2C Address |
|-------------------|----------|-----------|-------------|
| LDR (แสง) | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (อุณหภูมิ) | I2C | I2C_NUM_1 · SDA=GPIO4 · SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1 · SDA=GPIO4 · SCL=GPIO5 | 0x6F |
| HT16K33 (LED Matrix) | I2C | I2C_NUM_0 · SDA=GPIO21 · SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| **SW2 Button** | GPIO | **GPIO14** | — |
| USB Host | GPIO | GPIO25 (Active LOW) | — |

### เพิ่มเติม: iA เท่านั้น

| เซนเซอร์ | โปรโตคอล | Bus / Pin | I2C Address |
|---------|----------|-----------|-------------|
| KXTJ3 (Accelerometer) | I2C | I2C_NUM_0 · SDA=GPIO21 · SCL=GPIO22 | 0x0E |

---

## External JST Ports

| Port | GPIO | Mode | หมายเหตุ |
|------|------|------|---------|
| IN1 | GPIO32 | Digital / ADC1_CH4 / Touch | iA เท่านั้นสำหรับ ADC |
| IN2 | GPIO33 | Digital / ADC1_CH5 / Touch | iA เท่านั้นสำหรับ ADC |
| IN3 | GPIO34 | Input-only / ADC1_CH6 | ไม่มี pull resistor |
| IN4 | GPIO35 | Input-only / ADC1_CH7 | ไม่มี pull resistor |
| OUT1 | GPIO26 | Digital / DAC2 | — |
| OUT2 | GPIO27 | Digital | — |

---

## I2C Initialization Order (สำคัญมาก)

```c
// ลำดับที่ถูกต้อง — ห้ามสลับ
i2c_init_bus0();   // I2C_NUM_0: LED Matrix (0x70) + KXTJ3 (0x0E) [iA]
i2c_init_bus1();   // I2C_NUM_1: LM73 (0x4D)
adc_init_all();    // ADC1: LDR (GPIO36) + IN1-IN4 [iA เท่านั้น]
```

> ⚠️ i2c_driver_install() เรียกได้แค่ครั้งเดียวต่อ port — เรียก 2 ครั้งจะเกิด ESP_ERR_INVALID_STATE

---

## Crystal Frequency — CRITICAL

> บอร์ด KidBright ส่วนใหญ่ใช้ **26 MHz XTAL** (ต่างจาก Dev Kit ทั่วไปที่ 40 MHz)
> หากไม่ตั้งค่านี้ Serial Monitor จะแสดงตัวอักษรขยะ

```sdkconfig
CONFIG_XTAL_FREQ=26
CONFIG_XTAL_FREQ_26=y
```

---

## See Also

- [[kidbright32_reference]] — ประวัติรุ่นและภาพรวมทุกรุ่น
- [[sensor_guide]] — Sensor code + I2C init order
- [[adc_api_guide]] — ADC Oneshot API
- [[gpio_conflict]] — GPIO conflict table
- [[led_matrix]] — LED Matrix HT16K33
- [[oled_sh1106]] — OLED SH1106 display
