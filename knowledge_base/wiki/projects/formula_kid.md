# Formula Kid Controller — ESP-NOW Radio Car

> **หมวด:** Projects  
> **ไฟล์ดิบ (Raw):** `../raw/formula_kid_controller.md`  
> **เชื่อมโยง:** [[kidbright32_pinout]] · [[gpio_conflict]] · [[minibike]] · [[skate]]

---

## ภาพรวม

ระบบวิทยุบังคับรถ **Formula Kid** ใช้โปรโตคอล **ESP-NOW** (Unicast Peer-to-Peer)  
ประกอบด้วย 2 บอร์ด:
- **Controller** (บอร์ดที่ถือในมือ) — KB1.3 หรือ KB1.5G
- **Receiver** (บอร์ดบนรถ) — รับคำสั่งและควบคุมมอเตอร์

---

## ปุ่มบน Controller — S1, S2

> ⚠️ **CRITICAL:** Formula Kid ใช้ **S1=GPIO36, S2=GPIO39** — ไม่ใช่ SW1/SW2 ปุ่มบนบอร์ด

| สวิตช์ | GPIO | ข้อมูล |
|--------|------|-------|
| **S1** | GPIO36 (VP) | ADC1_CH0 · Input-only · ไม่มี internal pull |
| **S2** | GPIO39 (VN) | ADC1_CH3 · Input-only · ไม่มี internal pull |

### Logic Level

| สถานะ | ระดับสัญญาณ | gpio_get_level() |
|-------|------------|-----------------|
| ปล่อยปุ่ม | HIGH | 1 |
| กดปุ่ม | LOW | 0 |

---

## กฎการใช้งาน S1, S2 (MANDATORY)

1. **Input-only** — GPIO36/39 ห้ามกำหนดเป็น output เด็ดขาด
2. **ห้ามใช้ pull-up ใน code** — บอร์ดมี external pull-up แล้ว ห้าม `GPIO_PULLUP_ENABLE`
3. **ห้ามใช้ interrupt** — เมื่อใช้ ESP-NOW ร่วมกัน ให้ใช้ **polling เท่านั้น**
4. **Active LOW** — กด = LOW (0), ปล่อย = HIGH (1)

### GPIO Config ที่ถูกต้อง (ESP-IDF v5.x)

```c
#include "driver/gpio.h"

gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_36) | (1ULL << GPIO_NUM_39),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,    // ✅ บอร์ดมี external pull-up แล้ว
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,      // ✅ ห้าม interrupt บน GPIO36/39
};
gpio_config(&io_conf);
```

---

## ESP-NOW Packet Structure

```c
typedef struct {
    int16_t steering;   // -100 ถึง +100 (ซ้าย/ขวา)
    int16_t throttle;   // -100 ถึง +100 (ถอย/หน้า)
    uint8_t buttons;    // bitmask: bit0=S1, bit1=S2
} formula_packet_t;
```

---

## ลำดับ I2C Init บน Controller (KB1.3/KB1.5G)

```c
i2c_init_bus0();   // LED Matrix (0x70) — ไม่มี KXTJ3
i2c_init_bus1();   // LM73 (0x4D)
// ไม่ต้อง adc_init_all() เพราะ GPIO36/39 ใช้ gpio_get_level() ไม่ใช่ ADC
```

---

## See Also

- [[kidbright32_pinout]] — Pinout ทั้งหมด
- [[kidbright32_reference]] — Developer Reference ฉบับเต็ม
- [[gpio_conflict]] — GPIO conflict table
- [[adc_api_guide]] — ADC Oneshot API
- [[minibike]] — ESP-NOW Radio Control อีกรูปแบบ
- [[led_matrix]] — LED Matrix บน Receiver board
