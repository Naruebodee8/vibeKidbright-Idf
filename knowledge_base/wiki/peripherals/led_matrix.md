# LED Matrix 16×8 — HT16K33 Reference

> **Raw source**: `raw/led_16x8_matrix_mapping.md`
> **Related**: [[oled_sh1106]] · [[sensor_guide]] · [[kidbright32_pinout]]

---

## ข้อมูลพื้นฐาน

| Property | Value |
|---|---|
| Driver IC | HT16K33 |
| I2C Address | `0x70` |
| I2C Bus | `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) |
| Resolution | 16 columns × 8 rows |
| Layout | **Interleaved** — Left 8×8 (even addr) + Right 8×8 (odd addr) |
| Y-axis | **Inverted** — Bit 0 (0x01) = Top row, Bit 7 (0x80) = Bottom row |

---

## ⚠️ Critical Rules

### 1. NEVER สร้าง `uint8_t img[16]` ด้วยมือ
ใช้ `rows_to_columns_16x8()` หรือ pre-calculated arrays เสมอ — การคำนวณ interleaving เป็น error-prone มาก

### 2. Init ทีละ command เท่านั้น

```c
// ❌ WRONG — Display stays blank!
uint8_t init_cmds[] = {0x21, 0x81, 0xEF};
i2c_master_write_to_device(I2C_NUM_0, 0x70, init_cmds, sizeof(init_cmds), ...);

// ✅ CORRECT — แยก I2C transaction
uint8_t cmd;
cmd = 0x21; i2c_master_write_to_device(I2C_NUM_0, 0x70, &cmd, 1, pdMS_TO_TICKS(100));
cmd = 0x81; i2c_master_write_to_device(I2C_NUM_0, 0x70, &cmd, 1, pdMS_TO_TICKS(100));
cmd = 0xEF; i2c_master_write_to_device(I2C_NUM_0, 0x70, &cmd, 1, pdMS_TO_TICKS(100));
```

### 3. ห้าม ESP_ERROR_CHECK กับ Data Transfer

```c
// ❌ WRONG — board reboot ถ้า I2C glitch
ESP_ERROR_CHECK(i2c_master_write_to_device(...));

// ✅ CORRECT
esp_err_t err = i2c_master_write_to_device(...);
if (err != ESP_OK) { /* handle gracefully */ }
```

---

## HT16K33 Register Map

| Command | Value | Description |
|---|---|---|
| Oscillator ON | `0x21` | Turn on system oscillator |
| Display ON | `0x81` | Display ON, no blink |
| Brightness MAX | `0xEF` | Maximum brightness |
| RAM Start | `0x00` | Start address for display RAM write |

---

## Buffer Layout (17 bytes: 1 addr + 16 data)

```
buffer[0]  = 0x00          (RAM start address)
buffer[1]  = column_0      (Left Matrix Col 0)
buffer[2]  = column_8      (Right Matrix Col 0)
buffer[3]  = column_1      (Left Matrix Col 1)
buffer[4]  = column_9      (Right Matrix Col 1)
...
buffer[15] = column_7      (Left Matrix Col 7)
buffer[16] = column_15     (Right Matrix Col 7)
```

---

## Core Functions

### rows_to_columns_16x8() — แปลง row data → column buffer
```c
static void rows_to_columns_16x8(const uint16_t row_data[8], uint8_t out_cols[16]) {
    memset(out_cols, 0, 16);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 16; col++) {
            if (row_data[row] & (1 << (15 - col))) {
                out_cols[col] |= (1 << (7 - row));
            }
        }
    }
}
```

### matrix_draw() — เขียนลง hardware
```c
static void matrix_draw(const uint8_t cols[16]) {
    uint8_t buf[17] = {0};
    buf[0] = 0x00;
    for (int c = 0; c < 8; c++) {
        buf[1 + (c * 2)] = cols[c];
        buf[2 + (c * 2)] = cols[c + 8];
    }
    i2c_master_write_to_device(I2C_NUM_0, 0x70, buf, sizeof(buf), pdMS_TO_TICKS(100));
}
```

---

## Pre-calculated Direction Arrows

```c
static const uint8_t img_center[16] = {0x00,0x18,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x18,0x00};
static const uint8_t img_up[16]     = {0x00,0xFF,0x00,0xFE,0x00,0x0C,0x00,0x08,0x08,0x00,0x0C,0x00,0xFE,0x00,0xFF,0x00};
static const uint8_t img_down[16]   = {0x00,0xFF,0x00,0x7F,0x00,0x30,0x00,0x10,0x10,0x00,0x30,0x00,0x7F,0x00,0xFF,0x00};
static const uint8_t img_left[16]   = {0x00,0x18,0x00,0x18,0x18,0x18,0x3C,0x18,0x7E,0x18,0xFF,0x18,0x18,0x00,0x18,0x00};
static const uint8_t img_right[16]  = {0x00,0x18,0x00,0x18,0x18,0xFF,0x18,0x7E,0x18,0x3C,0x18,0x18,0x18,0x00,0x18,0x00};
```

---

## Two-Digit Display

```c
// ใช้ arrays สำเร็จรูปเหล่านี้ — ห้ามสร้าง font เอง!
static const uint16_t DIGIT_0[8] = {0x0E00,0x1100,0x1100,0x1100,0x1100,0x1100,0x1100,0x0E00};
// ... DIGIT_1 ถึง DIGIT_9 (ดูไฟล์ raw สำหรับ arrays ครบชุด)

static void display_two_digits(int tens, int units) {
    uint16_t comb[8];
    for (int i = 0; i < 8; i++) comb[i] = DIGITS[tens][i] | (DIGITS[units][i] >> 8);
    uint8_t cols[16];
    rows_to_columns_16x8(comb, cols);
    matrix_draw(cols);
}
```

> ⚠️ Exception: Formula Kid Car Receiver — ใช้ `kidbright32iA_updated.md` Section 20.3 แทน

---

## See Also

- [[oled_sh1106]] — OLED display (ใช้ I2C_NUM_0 bus เดียวกัน)
- [[sensor_guide]] — Board I2C init order
- [[kidbright32_pinout]] — GPIO ทั้งหมด
- [[formula_kid]] — ข้อยกเว้นสำหรับ Car Receiver
