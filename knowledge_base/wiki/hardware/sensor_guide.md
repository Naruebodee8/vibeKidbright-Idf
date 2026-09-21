# Sensor Guide — KidBright32 iA (ESP-IDF v5.x)

> **Raw source**: `raw/kidbright_sensor_guide.md`
> **Related**: [[kidbright32_pinout]] · [[adc_api_guide]] · [[gpio_conflict]] · [[kidbright32_reference]]

---

## ⚠️ กฎเหล็กก่อนเริ่ม

| ข้อ | รายละเอียด |
|---|---|
| ESP-IDF version | **v5.x เท่านั้น** |
| ADC Legacy API | ❌ ถูกลบแล้ว — ห้ามใช้ `driver/adc.h`, `esp_adc_cal.h` |
| Atten ที่ถูกต้อง | ✅ `ADC_ATTEN_DB_12` (DB_11 = deprecated) |

---

## Board Coverage

| บอร์ด | ย่อ | Accelerometer | ADC IN1-IN4 |
|---|---|---|---|
| V1.5 Rev 3.1 (NECTEC Standard) | Rev 3.1 | ❌ | ❌ |
| V1.5 Rev 3.1G (Gravitech OEM) | Rev 3.1G | ❌ | ❌ |
| V1.5 iA (INEX) | iA | ✅ KXTJ3 | ✅ |
| V1.6 (Gravitech) | V1.6 | ✅ | ✅ |

> ⚠️ **SW2 = GPIO14** สำหรับ **ทุกบอร์ด** — ตรวจ PCB silkscreen ก่อนเสมอ

---

## On-board Sensor Map

### V1.5 Rev 3.1 / Rev 3.1G

| Sensor | Protocol | Bus/Pin | Address |
|--------|----------|---------|---------|
| LDR | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| RTC MCP794xx | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x6F |
| HT16K33 (LED Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |
| Passive Buzzer | GPIO/PWM | GPIO13 (LEDC) | — |
| SW1 Button | GPIO | GPIO16 | — |
| **SW2 Button** | GPIO | **GPIO14** | — |
| USB Host | GPIO | GPIO25 (Active LOW) | — |

> 📋 I2C Scan (Rev 3.1G, Apr 17 2026): `I2C_NUM_1: 0x4D + 0x6F` · `I2C_NUM_0: 0x70`

### V1.5 iA (INEX) — เพิ่ม KXTJ3 + ADC IN1-IN4

| Sensor | Protocol | Bus/Pin | Address |
|--------|----------|---------|---------|
| LDR | ADC | GPIO36 / ADC1_CH0 | — |
| LM73 (Temp) | I2C | I2C_NUM_1, SDA=GPIO4, SCL=GPIO5 | 0x4D |
| **KXTJ3 (Accel)** | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x0E |
| HT16K33 (LED Matrix) | I2C | I2C_NUM_0, SDA=GPIO21, SCL=GPIO22 | 0x70 |

---

## External JST Ports

| Port | GPIO | Mode |
|------|------|------|
| IN1 | GPIO32 | Digital / ADC1_CH4 / Touch |
| IN2 | GPIO33 | Digital / ADC1_CH5 / Touch |
| IN3 | GPIO34 | Input-only / ADC1_CH6 (no pull) |
| IN4 | GPIO35 | Input-only / ADC1_CH7 (no pull) |
| OUT1 | GPIO26 | Digital / DAC2 |
| OUT2 | GPIO27 | Digital |

---

## ADC API ที่ถูกต้อง (v5.x)

```c
// ✅ Headers ที่ใช้ได้
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// ขั้นตอน
adc_oneshot_new_unit(...)           // 1. Create unit
adc_oneshot_config_channel(...)     // 2. Config channel (ใช้ ADC_ATTEN_DB_12)
adc_oneshot_read(...)               // 3. Read raw
adc_cali_raw_to_voltage(...)        // 4. Convert to mV
```

### ตัวอย่าง: อ่าน LM35 บน IN1 (GPIO32)
```c
int mv = adc_read_mv(ADC_CHANNEL_4, cali_in1);
float temp_c = mv / 10.0f; // LM35: 10mV per °C
```

---

## I2C Init Order (กฎทอง)

> `i2c_driver_install()` เรียกได้แค่ **ครั้งเดียวต่อ port** — เรียกซ้ำ = `ESP_ERR_INVALID_STATE`

### iA Board
```
1. i2c_init_bus0() → I2C_NUM_0: LED Matrix (0x70) + KXTJ3 (0x0E)
2. i2c_init_bus1() → I2C_NUM_1: LM73 (0x4D)
3. adc_init_all()  → ADC1: LDR + IN1..IN4
```

### Rev 3.1 / Rev 3.1G
```
1. i2c_init_bus0() → I2C_NUM_0: LED Matrix (0x70) เท่านั้น
2. i2c_init_bus1() → I2C_NUM_1: LM73 (0x4D)
3. adc_init_all()  → ADC1: LDR (GPIO36) เท่านั้น
```

---

## See Also

- [[gpio_conflict]] — รายการ GPIO ที่ conflict กัน
- [[adc_api_guide]] — ADC Oneshot API รายละเอียด
- [[kidbright32_pinout]] — Pinout ครบทุก GPIO
- [[kidbright32_reference]] — Developer Reference ฉบับเต็ม
