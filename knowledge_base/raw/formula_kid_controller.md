# Formula Kid Controller — Plugin Rules & Hardware Reference
> Plugin สำหรับ **KidBrightIDE / KBIDE** · บอร์ด **KB1.3 (V1.5 Rev 3.1)** และ **KB1.5G (V1.5 Rev 3.1G)**
> ใช้โปรโตคอล **ESP-NOW** สื่อสารแบบ Unicast ระหว่าง Controller (บอร์ดถือ) และ Receiver (บอร์ดรถ)

---

## ส่วนที่ 1: ฮาร์ดแวร์ — สวิตช์ S1, S2 บนบอร์ด KidBright32 (KB1.3/KB1.5G)

### ข้อมูล GPIO ของ S1 และ S2

> ⚠️ **CRITICAL — Formula Kid Controller ใช้ S1=GPIO36, S2=GPIO39 ไม่ใช่ SW1/SW2 ปุ่มบนบอร์ด**

| สวิตช์ | GPIO (ESP32) | อ้างอิง | ข้อจำกัดสำคัญ |
|--------|-------------|---------|--------------|
| **S1** | **GPIO36 (VP)** | ADC1_CH0 | Input-only · ไม่มี internal pull-up/pull-down |
| **S2** | **GPIO39 (VN)** | ADC1_CH3 | Input-only · ไม่มี internal pull-up/pull-down |

> ⚠️ **GPIO36 และ GPIO39 เป็น input-only pins** — ห้ามกำหนดเป็น output เด็ดขาด
> ⚠️ บอร์ด KidBright32 มีวงจร **pull-up ภายนอก** ให้แล้ว — ห้ามใช้ `INPUT_PULLUP` หรือ `GPIO_PULLUP_ENABLE` ใน code

### พฤติกรรมทางไฟฟ้า (Logic Level)

| สถานะ | ระดับสัญญาณ | ค่า `gpio_get_level()` |
|-------|------------|----------------------|
| ปล่อยปุ่ม (Released) | HIGH | 1 |
| กดปุ่ม (Pressed) | LOW | 0 |

### กฎการใช้งาน S1, S2 (MANDATORY)

1. **Input-only**: GPIO36/39 ห้ามกำหนดเป็น output เด็ดขาด
2. **ห้าม pull-up ใน code**: บอร์ดมี external pull-up แล้ว ห้ามใช้ `GPIO_PULLUP_ENABLE` หรือ `INPUT_PULLUP`
3. **ห้ามใช้ interrupt**: เมื่อใช้ ESP-NOW ร่วมกัน ห้ามใช้ ISR interrupt บน GPIO36/39 เพราะจะเกิด glitch จาก ADC/WiFi — ให้ใช้ **polling** เท่านั้น
4. **Active LOW**: กด = LOW (0), ปล่อย = HIGH (1)
5. **Config ด้วย `GPIO_MODE_INPUT`** พร้อม `GPIO_PULLUP_DISABLE` และ `GPIO_PULLDOWN_DISABLE`

### ตัวอย่าง GPIO Config (ESP-IDF v5.x) — ถูกต้อง

```c
#include "driver/gpio.h"

void s1_s2_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_36) | (1ULL << GPIO_NUM_39),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   // External pull-up on board
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,     // NEVER use interrupt on GPIO36/39 with ESP-NOW
    };
    gpio_config(&io_conf);
}

// Read (0 = pressed, 1 = released)
int s1 = gpio_get_level(GPIO_NUM_36);
int s2 = gpio_get_level(GPIO_NUM_39);
```

---

## ส่วนที่ 2: Plugin Joystick (ฝั่ง Controller) — RC Timing

> 🔑 **CRITICAL DISCOVERY:** Joystick **ไม่ได้ใช้ ADC** บน GPIO36/39 — ใช้วงจร **RC Timing** ผ่าน GPIO คนละชุด!

### GPIO Pins จริง (จาก Plugin generators.js)

| Joystick | แกน | Trigger GPIO (Output) | Capture GPIO (Input+ISR) |
|----------|-----|-----------------------|--------------------------|
| **JS1** | ขึ้น/ลง (Y) | **GPIO26** (OUT1) | **GPIO32** (IN1) |
| **JS2** | ซ้าย/ขวา (X) | **GPIO27** (OUT2) | **GPIO33** (IN2) |

> ⚠️ GPIO36/39 คือ **S1/S2 switches** เท่านั้น — ไม่เกี่ยวกับ Joystick position

### หลักการ RC Timing (จาก Plugin joystick.cpp)

```
1. ตั้ง trig_gpio = HIGH  → discharge capacitor (รอ 10ms)
2. จับเวลา start_ts = esp_timer_get_time()
3. ตั้ง trig_gpio = LOW   → เริ่มชาร์จ capacitor
4. ISR (rising edge บน cap_gpio) บันทึก stop_ts
5. คำนวณ:
   resistance = (stop_ts - start_ts) × RC_FACTOR_5V − R_SERIE
   raw_pos    = (resistance × 200.0 / 10000.0) − 100
```

ค่าคงที่จาก Plugin:
- `R_SERIE = 1000` Ω
- `RC_FACTOR_5V = 9.788075945`
- `CAP_TIMEOUT_US = 500000` (500ms)
- `DISCHARGE_MS = 10`

### Calibration (ตรงกับบล็อก)

```c
// ขั้นตอน calibrate หลัง raw_pos:
pos -= calibrate_release;           // ลบค่า release (JS1=-3, JS2=-3)
if (pos < 0) pos = pos * 100 / abs(calibrate_min - calibrate_release);
if (pos > 0) pos = pos * 100 / abs(calibrate_max - calibrate_release);
pos = clamp(pos, -100, 100);
```

| Joystick | Release | DownMost / LeftMost | UpMost / RightMost |
|----------|---------|---------------------|--------------------|
| JS1 | -3 | -100 | 89 |
| JS2 | -3 | -100 | 90 |

### Dead Zone และ Encode ค่าส่งผ่าน ESP-NOW

| สถานการณ์ | ESPNOW_VALUE ที่ส่ง | ความหมาย |
|-----------|--------------------|---------| 
| JS1 ≥ 10 (เดินหน้า) | `10` ถึง `100` (ค่าบวก) | เดินหน้า — แสดง "U" |
| JS1 ≤ -10 (ถอยหลัง) | `-100` ถึง `-10` (ค่าลบ) | ถอยหลัง — แสดง "D" |
| JS2 ≥ 10 (เลี้ยวขวา) | `410` ถึง `500` (JS2+400) | เลี้ยวขวา — แสดง "R" |
| JS2 ≤ -10 (เลี้ยวซ้าย) | `300` ถึง `399` (JS2+400) | เลี้ยวซ้าย — แสดง "L" |
| ทั้งคู่ dead zone | `999` | หยุด — แสดง "--" |

> **Encoding rule:** ส่งค่า JS1 ตรงๆ (ไม่ invert) · JS2 บวก offset `+400` ก่อนส่ง เพื่อแยกช่วงค่าจาก JS1

---


## ส่วนที่ 3: Plugin DRV8833 Motor Driver (ฝั่งรถ)

ชิปขับมอเตอร์ DRV8833 ควบคุมผ่าน ESP32 GPIO บนบอร์ดรถ

### GPIO Mapping — DRV8833

| สัญญาณ | GPIO | หมายเหตุ |
|--------|------|---------|
| nSLEEP | GPIO23 | Enable chip (HIGH = active) |
| Motor A1 | GPIO18 | |
| Motor A2 | GPIO26 (OUT1) | Active LOW connector |
| Motor B1 | GPIO19 | |
| Motor B2 | GPIO27 (OUT2) | Active LOW connector |

### กฎทิศทางการเคลื่อนที่

| Direction Code | ทิศทาง | Motor A | Motor B |
|---------------|---------|---------|---------|
| 0 | เดินหน้า (Forward) | +speed | +speed |
| 1 | ถอยหลัง (Backward) | -speed | -speed |
| 2 | เลี้ยวซ้าย (Turn Left) | +speed | -speed |
| 3 | เลี้ยวขวา (Turn Right) | -speed | +speed |

### กฎการรับค่า ESP-NOW และสั่งมอเตอร์

| ค่า ESPNOW_VALUE (`int32_t`) | การกระทำ | รายละเอียด & LED |
|-----------------|---------|-----------|
| `999` | **หยุด** | `drv8833.stop()`, แสดง "--" |
| `10` ถึง `199` | **เดินหน้า** (direction=0) | speed = ค่า, แสดง "U" |
| `-10` ถึง `-199` | **ถอยหลัง** (direction=1) | speed = \|ค่า\|, แสดง "D" |
| `300` ถึง `390` | **เลี้ยวซ้าย** (direction=2) | speed = ค่า - 400, แสดง "L" |
| `410` ขึ้นไป | **เลี้ยวขวา** (direction=3) | speed = ค่า - 400, แสดง "R" |

> ⚠️ **CRITICAL FIX:** ค่าที่ส่งจากฝั่ง Controller (KBIDE block) มีขนาด 4-byte Integer (`int32_t`)
> ฝั่ง Receiver **ต้องใช้** `int32_t esp_value;` ใน Struct
> **ห้ามใช้ `float` เด็ดขาด** เพราะจะทำให้การแปลงค่าลบกลายเป็น `-nan` และ 999 กลายเป็น `0.00`

---

## ส่วนที่ 4: กฎการใช้ S1, S2 เพื่อส่งคำสั่งพิเศษผ่าน ESP-NOW

เนื่องจาก JS1 และ JS2 ใช้ช่วงค่า `-100` ถึง `500` (รวม `999`) ไปแล้ว ค่าที่ส่งสำหรับ S1/S2 ต้องไม่ซ้ำกับช่วงเหล่านี้

### ตัวอย่างค่าที่แนะนำสำหรับ S1, S2

| สถานะ | ESPNOW_VALUE ที่แนะนำ | ฟังก์ชัน |
|-------|----------------------|---------|
| กด **S1** เท่านั้น | `-200` | ฟังก์ชัน A (เช่น boost / เร็วพิเศษ) |
| กด **S2** เท่านั้น | `-300` | ฟังก์ชัน B (เช่น honk / เสียงแตร) |
| กด **S1 + S2** พร้อมกัน | `-400` | ฟังก์ชัน C (เช่น reset / สลับ mode) |

### Priority Order การส่งค่า (ตามบล็อก)

```
1. JS1 ≥ 10 หรือ JS1 ≤ -10 → ส่ง JS1 ตรงๆ  (JS1 มีความสำคัญกว่า JS2)
2. JS2 ≥ 10 หรือ JS2 ≤ -10 → ส่ง JS2 + 400
3. ทั้งคู่ dead zone (-10 < JS < 10) → ส่ง 999 (stop)
```

> ⚠️ **JS1 มีความสำคัญสูงกว่า JS2 เสมอ** — ถ้า JS1 เคลื่อน ค่า JS2 จะถูกละเว้น
> ℹ️ S1/S2 (GPIO36/39) เป็นวงจรแยกต่างหาก — ถ้าต้องการใช้ในโปรเจกต์นี้ให้ encode เป็นค่าพิเศษที่ไม่ชนช่วง JS

---

## ส่วนที่ 5: กฎการสื่อสาร ESP-NOW

### พารามิเตอร์การส่ง

| พารามิเตอร์ | ค่า | หมายเหตุ |
|------------|-----|---------|
| โปรโตคอล | ESP-NOW | ไม่ต้องใช้ Router |
| โหมด | Unicast | ส่งไปยัง MAC Address เฉพาะของบอร์ดรถ |
| ชนิดข้อมูล | `int32_t` (Integer 4-byte) | ส่งตัวเลขจำนวนเต็มหนึ่งตัวต่อครั้ง (ห้ามใช้ float) |
| อัตราการส่ง | ทุก 500ms | `vTaskDelay(pdMS_TO_TICKS(500))` |

### กฎสำคัญ ESP-NOW (MANDATORY)

1. **ห้ามใช้ IoT (WiFi) พร้อมกับ ESP-NOW**: ถ้า SSID/Password ถูกตั้งไว้ ESP-NOW จะส่งได้แค่ครั้งแรกครั้งเดียว ต้องลบ SSID/Password ออกจาก config ก่อนใช้ ESP-NOW
2. **MAC Address ต้องถูกต้อง**: ต้อง hardcode MAC Address ของบอร์ดรถที่ต้องการส่งถึง
3. **ส่งแบบ polling**: ไม่ใช้ interrupt บน GPIO36/39
4. **ค่าที่ส่งเป็น int เดียว**: ไม่ใช่ struct หรือ array (ตามโปรโตคอล Formula Kid)

### ⚠️ BREAKING API CHANGE — ESP-IDF v5.5+ Send Callback

> **`esp_now_register_send_cb` callback signature เปลี่ยนใน ESP-IDF v5.5+**
> ใช้ signature เก่าจะ compile error: `incompatible pointer type`

```c
// ❌ WRONG — ESP-IDF ≤ v5.4 (compile error บน v5.5+)
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) { }

// ✅ CORRECT — ESP-IDF v5.5+ (ALWAYS use this)
static void espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI("ESPNOW", "Send OK");
    } else {
        ESP_LOGW("ESPNOW", "Send FAIL");
    }
    // ถ้าต้องการ MAC address ของ peer → tx_info->peer_addr
}

// ⚠️ ESP-IDF v5.5+ RECEIVE CALLBACK SIGNATURE
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // recv_info->src_addr, recv_info->des_addr
}

// Registration ยังเหมือนเดิม:
ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
```

### ตัวอย่าง Verified Controller Loop & LED Matrix (ESP-IDF v5.x)

> **CRITICAL LED MATRIX 180° ROTATION**: The 16x8 LED Matrix on Formula Kid is physically rotated 180 degrees.
> `cols[0]` is physical Left, `cols[15]` is physical Right.
> `Bit 7` is physical Top, `Bit 0` is physical Bottom.

```c
// LED Matrix — Letter characters (Corrected mapping for 180° rotated display)
static const uint8_t img_up[16]    = {0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0x00, 0x00}; // U
static const uint8_t img_down[16]  = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7E, 0x3C, 0x00, 0x00, 0x00}; // D
static const uint8_t img_left[16]  = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00}; // L
static const uint8_t img_right[16] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x90, 0x90, 0x98, 0x94, 0x62, 0x01, 0x00, 0x00, 0x00, 0x00}; // R
static const uint8_t img_stop[16]  = {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00}; // --

// DO NOT swap panels in matrix_draw. Keep it like this:
static void matrix_draw(const uint8_t cols[16]) {
    uint8_t buf[17] = {0};
    buf[0] = 0x00;
    for (int c = 0; c < 8; c++) {
        buf[1 + (c * 2)] = cols[c];         // Left screen col
        buf[2 + (c * 2)] = cols[c + 8];     // Right screen col
    }
    i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

// Main loop (polling every 300ms)
while (1) {
    rc_timing_event_t event;
    int64_t current_time = esp_timer_get_time();

    // Trigger JS1
    read_rc_timing(JS1_TRIG_GPIO, JS1_CAP_GPIO, &js1_start_time);
    vTaskDelay(pdMS_TO_TICKS(1)); // Allow ISR to trigger
    if (xQueueReceive(s_rc_timing_queue, &event, pdMS_TO_TICKS(CAP_TIMEOUT_US / 1000)) == pdTRUE && event.gpio_num == JS1_CAP_GPIO) {
        js1_pos = calculate_joystick_position(event.duration - js1_start_time, -3, -100, 89);
    }

    // Trigger JS2
    read_rc_timing(JS2_TRIG_GPIO, JS2_CAP_GPIO, &js2_start_time);
    vTaskDelay(pdMS_TO_TICKS(1));
    if (xQueueReceive(s_rc_timing_queue, &event, pdMS_TO_TICKS(CAP_TIMEOUT_US / 1000)) == pdTRUE && event.gpio_num == JS2_CAP_GPIO) {
        js2_pos = calculate_joystick_position(event.duration - js2_start_time, -3, -100, 90);
    }

    int32_t espnow_value = 999;
    const uint8_t *current_matrix_pattern = img_stop;

    if (js1_pos >= JS_DEAD_ZONE) {
        espnow_value = js1_pos;
        current_matrix_pattern = img_up;
    } else if (js1_pos <= -JS_DEAD_ZONE) {
        espnow_value = js1_pos;
        current_matrix_pattern = img_down;
    } else if (js2_pos >= JS_DEAD_ZONE) {
        espnow_value = js2_pos + 400;
        current_matrix_pattern = img_right;
    } else if (js2_pos <= -JS_DEAD_ZONE) {
        espnow_value = js2_pos + 400;
        current_matrix_pattern = img_left;
    } else {
        espnow_value = 999;
        current_matrix_pattern = img_stop;
    }

    // Update Display and Send ESP-NOW if changed...
    // ...
    
    vTaskDelay(pdMS_TO_TICKS(300));
}
```

---

## ส่วนที่ 6: ความแตกต่างระหว่าง KB1.3 (Rev 3.1) และ KB1.5G (Rev 3.1G)

> ⚠️ **CRITICAL:** Formula Kid Controller รองรับทั้ง KB1.3 และ KB1.5G
> **ทั้งสองรุ่นนี้ไม่มี KXTJ3 Accelerometer** — ห้าม init KXTJ3 สำหรับบอร์ดทั้งสอง

| Feature | KB1.3 (Rev 3.1) | KB1.5G (Rev 3.1G) |
|---------|----------------|-------------------|
| S1 (Formula Kid) | **GPIO36** | **GPIO36** |
| S2 (Formula Kid) | **GPIO39** | **GPIO39** |
| SW1 ปุ่มบนบอร์ด | GPIO16 | GPIO16 |
| **SW2 ปุ่มบนบอร์ด** | **GPIO14** | **GPIO14** |
| KXTJ3 Accelerometer | ❌ ไม่มี | ❌ ไม่มี |
| USB Connector | Micro-USB | Micro-USB |

> ⚠️ S1/S2 ของ Formula Kid Controller (GPIO36/39) ไม่เกี่ยวกับ SW1/SW2 ปุ่มบนบอร์ด (GPIO16/14) — เป็นคนละวงจรกัน

---

## ส่วนที่ 7: สรุปกฎ Quick Reference

### DO ✅

- ใช้ `gpio_get_level(GPIO_NUM_36)` และ `gpio_get_level(GPIO_NUM_39)` สำหรับ S1, S2
- Config GPIO36/39 ด้วย `GPIO_MODE_INPUT`, `GPIO_PULLUP_DISABLE`, `GPIO_PULLDOWN_DISABLE`, `GPIO_INTR_DISABLE`
- ใช้ polling loop + delay ไม่ใช่ interrupt
- ส่ง ESP-NOW ทุก 500ms
- ตรวจสอบ JS1 ก่อน JS2 ใน priority order (JS1 override JS2)
- Encode: JS1 ส่งตรงๆ (บวก/ลบตาม direction), JS2+400 สำหรับเลี้ยว, 999 สำหรับ stop
- แสดง LED: "U"=เดินหน้า, "D"=ถอยหลัง, "R"=ขวา, "L"=ซ้าย, "--"=หยุด
- **CMakeLists.txt (CRITICAL):** เมื่อสร้างโปรเจกต์ใหม่ ในไฟล์ `main/CMakeLists.txt` ห้ามใส่ `esp_now` ในบรรทัด `PRIV_REQUIRES` เพราะ ESP-IDF v5 ย้ายไปรวมไว้ใน `esp_wifi` แล้ว ให้ใช้แค่: `PRIV_REQUIRES driver esp_timer esp_wifi nvs_flash`

### DON'T ❌

- ❌ ห้ามใช้ `GPIO_PULLUP_ENABLE` บน GPIO36/39
- ❌ ห้ามตั้ง GPIO36/39 เป็น output
- ❌ ห้ามใช้ ISR interrupt บน GPIO36/39 เมื่อใช้ ESP-NOW
- ❌ ห้ามใช้ IoT WiFi (SSID/Password) พร้อมกับ ESP-NOW
- ❌ ห้ามสับสน S1/S2 (GPIO36/39) กับ SW1/SW2 ปุ่มบนบอร์ด (GPIO16/14)
- ❌ ห้าม init KXTJ3 Accelerometer สำหรับ KB1.3 / KB1.5G
- ❌ ห้ามใช้ค่า ESPNOW_VALUE ที่ชนกับช่วง JS1 (-100~100), JS2+400 (300~500), หรือ 999
- ❌ **ห้ามประกาศ `img_left[]` หรือ `img_right[]` ถ้าไม่ได้ใช้งาน** — ESP-IDF v5.x treats unused `static const` arrays as **build errors** (`-Werror=unused-const-variable=`). หากโปรเจกต์ Controller อ่านแค่ JS1 (เดินหน้า/ถอยหลัง) ให้ประกาศเฉพาะ `img_up`, `img_down`, `img_stop` เท่านั้น
- ❌ **ห้ามใส่ `esp_now` ใน `PRIV_REQUIRES` ของ `main/CMakeLists.txt`** — ใน ESP-IDF v5.x คอมโพเนนต์ `esp_now` ถูกรวมไว้ใน `esp_wifi` แล้ว การใส่ `esp_now` จะทำให้ CMake ฟ้อง `Failed to resolve component 'esp_now'`
- ❌ **ห้ามใช้ `wifi_tx_info_t` เป็น parameter ของ ESP-NOW send callback** — ใน ESP-IDF v5.4.x ยังคงใช้ `const uint8_t *mac_addr` อยู่: `static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)`

---

## ส่วนที่ 8: Verified Sender Code Reference (ESP-IDF v5.4.3)

> ⚠️ **AI INSTRUCTION (MANDATORY):** ก่อนเขียนโค้ด Formula Kid Controller Sender ทุกครั้ง คุณ **MUST** อ่านและใช้โครงสร้างโค้ดด้านล่างนี้เป็นแม่แบบ ห้ามแต่งโครงสร้างขึ้นมาเองเด็ดขาด

### โครงสร้างสำคัญ (Key Patterns — DO NOT DEVIATE)

**1. RC Timing ISR — ใช้ struct ส่งผ่าน Queue (ไม่ใช่ global variable)**
```c
typedef struct {
    int     gpio_num;
    int64_t duration;   // stop_time จาก esp_timer_get_time() ณ จุดที่ Rising Edge เกิด
} rc_timing_event_t;

static QueueHandle_t s_rc_timing_queue;  // xQueueCreate(10, sizeof(rc_timing_event_t))

static IRAM_ATTR void rc_timing_isr_handler(void *arg) {
    int gpio_num = (int)arg;
    int64_t stop_time = esp_timer_get_time();
    rc_timing_event_t event = { .gpio_num = gpio_num, .duration = stop_time };
    xQueueSendFromISR(s_rc_timing_queue, &event, NULL);
}
```

**2. Discharge + Measure Sequence (ต้องทำตามลำดับนี้เสมอ)**
```c
// Step 1: Discharge capacitor
gpio_intr_disable(cap_gpio);
gpio_set_level(trig_gpio, 1);
esp_rom_delay_us(DISCHARGE_MS * 1000);  // DISCHARGE_MS = 10ms → ใช้ us เสมอ

// Step 2: Start charge + timestamp
gpio_set_level(trig_gpio, 0);
int64_t js_start_time = esp_timer_get_time();
gpio_intr_enable(cap_gpio);
```

**3. Flush Queue ก่อนวัดแต่ละแกน (ป้องกัน stale event)**
```c
// Flush stale events before measuring JS1
while (xQueueReceive(s_rc_timing_queue, &event, 0) == pdTRUE) { }
// ... trigger JS1 ...
// Wait for ISR result
if (xQueueReceive(s_rc_timing_queue, &event, pdMS_TO_TICKS(CAP_TIMEOUT_US / 1000)) == pdTRUE
    && event.gpio_num == JS1_CAP_GPIO) {
    js1_pos = calculate_joystick_position(event.duration - js1_start_time, -3, -100, 89);
}
// Flush again before measuring JS2
while (xQueueReceive(s_rc_timing_queue, &event, 0) == pdTRUE) { }
// ... trigger JS2 ...
```

**4. Calibration Constants (ห้ามเปลี่ยน)**
```c
#define R_SERIE      1000.0f
#define RC_FACTOR_5V 9.788075945f
#define CAP_TIMEOUT_US 500000   // 500ms

// JS1 Y-axis: release=-3, cal_min=-100, cal_max=89
// JS2 X-axis: release=-3, cal_min=-100, cal_max=90
int calculate_joystick_position(int64_t duration, int release, int min_cal, int max_cal) {
    float resistance = (float)duration * RC_FACTOR_5V - R_SERIE;
    int raw_pos = (int)(resistance * 200.0f / 10000.0f) - 100;
    int pos = raw_pos - release;
    if (pos < 0) pos = (int)((float)pos * 100.0f / (float)abs(min_cal - release));
    else         pos = (int)((float)pos * 100.0f / (float)abs(max_cal - release));
    if (pos >  100) pos =  100;
    if (pos < -100) pos = -100;
    return pos;
}
```

**5. Priority Logic + Smart ESP-NOW Send (ส่งเฉพาะเมื่อจำเป็น)**
```c
// Priority: JS1 > JS2 > Stop
if      (js1_pos >= JS1_DEAD_ZONE)  { espnow_value = js1_pos;        img = img_up;    }
else if (js1_pos <= -JS1_DEAD_ZONE) { espnow_value = js1_pos;        img = img_down;  }
else if (js2_pos >= JS2_DEAD_ZONE)  { espnow_value = js2_pos + 400;  img = img_right; }
else if (js2_pos <= -JS2_DEAD_ZONE) { espnow_value = js2_pos + 400;  img = img_left;  }
else                                 { espnow_value = 999;            img = img_stop;  }

// Update LED only on pattern change
int direction_changed = (img != prev_img);
if (direction_changed) { matrix_draw(img); prev_img = img; }

// Send ESP-NOW only on direction change OR significant value change while moving
int value_delta = abs((int)espnow_value - (int)prev_espnow_value);
int moving = (img != img_stop);
if (direction_changed || (moving && value_delta > 5)) {
    esp_now_send(s_broadcast_mac, (uint8_t *)&espnow_value, sizeof(espnow_value));
    prev_espnow_value = espnow_value;
}
```

**6. ESP-NOW Send Callback (ใช้ `const uint8_t *mac_addr` เสมอใน ESP-IDF v5.4)**
```c
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    (void)mac_addr;
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "ESP-NOW send failed");
    }
}
```

**7. Dead Zone ที่แนะนำ**
```c
#define JS1_DEAD_ZONE 10   // JS1 (Y-axis) พักใกล้ 0
#define JS2_DEAD_ZONE 20   // JS2 (X-axis) พักประมาณ ~25 ต้องใช้ zone ใหญ่กว่า
```

**8. GPIO Config — CAP (Input+ISR) และ TRIG (Output) แยกกัน**
```c
// CAP pins: Input, no pull, rising edge ISR
gpio_config_t cap_conf = {
    .pin_bit_mask = (1ULL << JS1_CAP_GPIO) | (1ULL << JS2_CAP_GPIO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_POSEDGE,
};
gpio_config(&cap_conf);

// TRIG pins: Output
gpio_config_t trig_conf = {
    .pin_bit_mask = (1ULL << JS1_TRIG_GPIO) | (1ULL << JS2_TRIG_GPIO),
    .mode = GPIO_MODE_OUTPUT,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&trig_conf);

gpio_install_isr_service(0);
gpio_isr_handler_add(JS1_CAP_GPIO, rc_timing_isr_handler, (void *)JS1_CAP_GPIO);
gpio_isr_handler_add(JS2_CAP_GPIO, rc_timing_isr_handler, (void *)JS2_CAP_GPIO);
```

**9. Joystick Calibration Mode (Optional but Recommended)**
หากผู้ใช้ต้องการฟังก์ชัน Calibration ให้เพิ่มโค้ดนี้ก่อน `app_main`:
```c
// --- Scroll a signed integer value on LED Matrix ---
static void matrix_show_bar(int value) {
  uint8_t buf[17] = {0};
  buf[0] = 0x00;
  if (value > 0) {
    int bars = (value * 8) / 100;
    if (bars > 8) bars = 8;
    for (int c = 0; c < bars; c++) buf[1 + ((8 + c) * 2 - 15)] = 0xFF;
    for (int c = 8; c < 8 + bars && c < 16; c++) buf[2 + ((c - 8) * 2)] = 0x3C;
  } else if (value < 0) {
    int bars = ((-value) * 8) / 100;
    if (bars > 8) bars = 8;
    for (int c = 0; c < bars; c++) buf[1 + ((7 - c) * 2)] = 0x3C;
  } else {
    buf[15] = 0x18; buf[16] = 0x18;
  }
  i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

// Helper: อ่าน raw_pos แบบไม่ผ่าน calibration
static int read_raw_pos(gpio_num_t trig_gpio, gpio_num_t cap_gpio, int cap_gpio_num) {
  rc_timing_event_t event;
  while (xQueueReceive(s_rc_timing_queue, &event, 0) == pdTRUE) {}
  gpio_intr_disable((gpio_num_t)cap_gpio_num);
  gpio_set_level(trig_gpio, 1);
  esp_rom_delay_us(DISCHARGE_MS * 1000);
  gpio_set_level(trig_gpio, 0);
  int64_t start_time = esp_timer_get_time();
  gpio_intr_enable((gpio_num_t)cap_gpio_num);
  if (xQueueReceive(s_rc_timing_queue, &event, pdMS_TO_TICKS(500)) == pdTRUE && event.gpio_num == cap_gpio_num) {
    float resistance = (float)(event.duration - start_time) * RC_FACTOR_5V - R_SERIE;
    return (int)(resistance * 200.0f / 10000.0f) - 100;
  }
  return 0;
}

#define CALIB_DURATION_S 15
static void joystick_calibrate(void) {
  ESP_LOGI(TAG, "=== JOYSTICK CALIBRATION MODE (%ds) ===", CALIB_DURATION_S);
  int js1_min = 9999, js1_max = -9999, js1_rel_sum = 0;
  int js2_min = 9999, js2_max = -9999, js2_rel_sum = 0;
  for (int i = 0; i < 5; i++) {
    js1_rel_sum += read_raw_pos(JS1_TRIG_GPIO, JS1_CAP_GPIO, JS1_CAP_GPIO);
    js2_rel_sum += read_raw_pos(JS2_TRIG_GPIO, JS2_CAP_GPIO, JS2_CAP_GPIO);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  int js1_release = js1_rel_sum / 5, js2_release = js2_rel_sum / 5;
  ESP_LOGI(TAG, "Release: JS1=%d, JS2=%d", js1_release, js2_release);

  int64_t end_time = esp_timer_get_time() + (int64_t)CALIB_DURATION_S * 1000000;
  while (esp_timer_get_time() < end_time) {
    int js1 = read_raw_pos(JS1_TRIG_GPIO, JS1_CAP_GPIO, JS1_CAP_GPIO);
    int js2 = read_raw_pos(JS2_TRIG_GPIO, JS2_CAP_GPIO, JS2_CAP_GPIO);
    if (js1 < js1_min) js1_min = js1; if (js1 > js1_max) js1_max = js1;
    if (js2 < js2_min) js2_min = js2; if (js2 > js2_max) js2_max = js2;
    matrix_show_bar(js2);
    vTaskDelay(pdMS_TO_TICKS(80));
  }
  matrix_draw(img_stop);
  ESP_LOGI(TAG, "=== CALIBRATION RESULT ===");
  ESP_LOGI(TAG, "JS1: release=%d, min=%d, max=%d", js1_release, js1_min, js1_max);
  ESP_LOGI(TAG, "JS2: release=%d, min=%d, max=%d", js2_release, js2_min, js2_max);
}
```
*การใช้งาน:* เรียก `joystick_calibrate();` ก่อนเข้า `while(1)` ใน `app_main`

---

## ส่วนที่ 10: LED Matrix Text Scrolling (Verified)
หากผู้ใช้ต้องการแสดงตัวเลข หรือข้อความ (เช่น เลื่อนแสดงค่า Joystick) บนจอ LED Matrix ของ KidBright **ห้าม** เขียนโค้ดสลับซ้าย-ขวาเอง และ **ต้อง** Reverse Bits ของ Font เสมอ เนื่องจากจอ KidBright กลับหัว (Bit 7 คือ Top, Bit 0 คือ Bottom)

**ให้ใช้ Code Pattern นึ้เท่านั้น:**
```c
// Font 5x8 สำหรับตัวเลขและข้อความ
static const uint8_t font5x8[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0: '0'
    {0x00,0x42,0x7F,0x40,0x00}, // 1: '1'
    {0x42,0x61,0x51,0x49,0x46}, // 2: '2'
    {0x21,0x41,0x45,0x4B,0x31}, // 3: '3'
    {0x18,0x14,0x12,0x7F,0x10}, // 4: '4'
    {0x27,0x45,0x45,0x45,0x39}, // 5: '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // 6: '6'
    {0x01,0x71,0x09,0x05,0x03}, // 7: '7'
    {0x36,0x49,0x49,0x49,0x36}, // 8: '8'
    {0x06,0x49,0x49,0x29,0x1E}, // 9: '9'
    {0x00,0x36,0x36,0x00,0x00}, // 10: ':'
    {0x00,0x08,0x08,0x08,0x00}, // 11: '-'
    {0x00,0x00,0x00,0x00,0x00}, // 12: ' '
    {0x20,0x40,0x41,0x3F,0x01}, // 13: 'J'
    {0x46,0x49,0x49,0x49,0x31}, // 14: 'S'
};

// ฟังก์ชันกลับด้าน Bit (เนื่องจากจอ KidBright บิต 0 คือด้านล่าง)
static uint8_t reverse_bits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static uint8_t get_font_col(char c, int col) {
    int idx = 12; // default space
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c == ':') idx = 10;
    else if (c == '-') idx = 11;
    else if (c == ' ') idx = 12;
    else if (c == 'J') idx = 13;
    else if (c == 'S') idx = 14;
    
    if (col < 5) return reverse_bits(font5x8[idx][col]);
    return 0x00;
}

// Scroll ข้อความบนจอ (เช่น matrix_scroll_text("JS2 100"))
static void matrix_scroll_text(const char *text) {
    int text_len = strlen(text);
    int total_width = text_len * 6; // 5px char + 1px space
    int scroll_start = 16;
    int scroll_end = -total_width;

    for (int x = scroll_start; x > scroll_end; x--) {
        uint8_t buf[17] = {0};
        buf[0] = 0x00;
        for (int col = 0; col < 16; col++) {
            int char_x = col - x; 
            uint8_t pixel = 0x00;
            if (char_x >= 0 && char_x < total_width) {
                int char_idx  = char_x / 6;
                int char_col  = char_x % 6;
                if (char_idx < text_len) {
                    pixel = get_font_col(text[char_idx], char_col);
                }
            }
            // แปลง Column ลง Buffer (Left = col 0-7, Right = col 8-15)
            if (col < 8) buf[1 + col * 2] = pixel;
            else         buf[2 + (col - 8) * 2] = pixel;
        }
        i2c_master_write_to_device(I2C_NUM_0, HT16K33_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(60)); // Scroll speed
    }
}
```