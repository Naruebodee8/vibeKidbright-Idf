# ADC API Guide — ESP-IDF v5.x

> **หมวด:** Hardware Reference  
> **ไฟล์ดิบ (Raw):** `../raw/kidbright_sensor_guide.md` · `../raw/kidbright32iA_updated.md`  
> **เชื่อมโยง:** [[kidbright32_pinout]] · [[gpio_conflict]] · [[minibike]] · [[skate]]

---

## ❌ Legacy API — ถูกลบใน ESP-IDF v5 (ห้ามใช้เด็ดขาด)

```c
#include "driver/adc.h"           // ❌ BANNED
#include "esp_adc_cal.h"          // ❌ BANNED
adc1_config_width(...)            // ❌ ถูกลบ
adc1_config_channel_atten(...)    // ❌ ถูกลบ
adc1_get_raw(...)                 // ❌ ถูกลบ
esp_adc_cal_characterize(...)     // ❌ ถูกลบ
ADC_ATTEN_DB_11                   // ❌ Deprecated → ใช้ ADC_ATTEN_DB_12
```

---

## ✅ Oneshot API ที่ถูกต้อง (ESP-IDF v5.x)

```c
#include "esp_adc/adc_oneshot.h"     // ✅
#include "esp_adc/adc_cali.h"        // ✅
#include "esp_adc/adc_cali_scheme.h" // ✅
```

### ขั้นตอน 4 ขั้น

```c
// 1. สร้าง unit handle
adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
};
adc_oneshot_unit_handle_t adc_handle;
adc_oneshot_new_unit(&init_config, &adc_handle);

// 2. Config channel (ใช้ ADC_ATTEN_DB_12 ไม่ใช่ DB_11)
adc_oneshot_chan_cfg_t chan_cfg = {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten    = ADC_ATTEN_DB_12,  // ✅ ถูกต้อง
};
adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &chan_cfg);

// 3. อ่านค่า raw
int raw_value;
adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw_value);

// 4. แปลงเป็น mV (optional — ต้อง calibration ก่อน)
int voltage_mv;
adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);
```

---

## ⚠️ struct name ที่ถูกต้อง

```c
// ✅ ถูกต้อง
adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };

// ❌ ผิด — ทำให้คอมไพล์ไม่ผ่าน
adc_oneshot_unit_init_config_t init_config = { ... };
```

---

## Channel ↔ GPIO Mapping (ADC1 — KidBright32)

| ADC Channel | GPIO | ใช้งานบนบอร์ด |
|-------------|------|--------------|
| ADC_CHANNEL_0 | GPIO36 | LDR (ทุกรุ่น) |
| ADC_CHANNEL_3 | GPIO39 | S2 (Formula Kid Controller) |
| ADC_CHANNEL_4 | GPIO32 | IN1 (iA เท่านั้น) |
| ADC_CHANNEL_5 | GPIO33 | IN2 (iA เท่านั้น) |
| ADC_CHANNEL_6 | GPIO34 | IN3 / JS_X Joystick |
| ADC_CHANNEL_7 | GPIO35 | IN4 / JS_Y Joystick |

> ⚠️ **ADC_CHANNEL_6 = GPIO34, ADC_CHANNEL_7 = GPIO35**  
> โค้ดเก่าที่ AI สร้างอาจสลับ channel/pin comment — ให้เชื่อ channel number เสมอ

---

## LM35 Temperature Sensor (ตัวอย่าง: IN1/GPIO32)

```c
// LM35: 10 mV per degree Celsius
int mv = adc_read_mv(ADC_CHANNEL_4, cali_in1);
float temp_c = mv / 10.0f;
ESP_LOGI("SENSOR", "LM35: %.2f C", temp_c);
```

---

**ดูเพิ่มเติม:** [[kidbright32_pinout]] | [[gpio_conflict]] | [[oled]]
