# OLED SH1106 — Template & Usage Guide

> **Raw source**: `raw/oled_sh1106_template.md`
> **Related**: [[led_matrix]] · [[sensor_guide]] · [[kidbright32_pinout]]

---

## ข้อมูลพื้นฐาน

| Property | Value |
|---|---|
| Controller | SH1106 |
| I2C Address | `0x3C` |
| I2C Bus | `I2C_NUM_0` (SDA=GPIO21, SCL=GPIO22) |
| Resolution | 128 × 64 pixels |
| Framebuffer | `uint8_t s_oled_fb[1024]` (8 pages × 128 bytes) |
| Column offset | **+2** (SH1106 quirk — ต่างจาก SSD1306) |

---

## ⚠️ SH1106 vs SSD1306

SH1106 **ต้องส่ง column offset = 2** ก่อนเขียนแต่ละ page:
```c
sh1106_write_cmd(0xB0 + page); // Set page
sh1106_write_cmd(0x02);        // ← Lower column address (+2 offset)
sh1106_write_cmd(0x10);        // Higher column address
```

---

## Pin Mapping (KidBright32 iA / V1.6)

| Signal | GPIO | ADC Channel | หมายเหตุ |
|--------|------|-------------|---------|
| JS_X (Left-Right) | GPIO34 (IN3) | ADC_CHANNEL_6 | ✅ Verified |
| JS_Y (Up-Down) | GPIO35 (IN4) | ADC_CHANNEL_7 | ✅ Verified |
| SW1 (Cal button) | GPIO16 | — | Active LOW |
| SW2 (Cancel) | GPIO14 | — | Active LOW |

> ⚠️ `ADC_CHANNEL_6 = GPIO34`, `ADC_CHANNEL_7 = GPIO35` — ห้ามสลับ!
> ❌ **ห้ามใช้ GPIO_NUM_17 สำหรับ SW2** — ขานี้ไม่มีบนบอร์ด!

---

## Defines

```c
#define I2C_PORT_OLED  I2C_NUM_0
#define OLED_SDA_GPIO  GPIO_NUM_21
#define OLED_SCL_GPIO  GPIO_NUM_22
#define OLED_ADDR      0x3C

#define JS_X_CHAN      ADC_CHANNEL_6   // GPIO34 (IN3) → Left-Right
#define JS_Y_CHAN      ADC_CHANNEL_7   // GPIO35 (IN4) → Up-Down
#define JS_ADC_UNIT    ADC_UNIT_1

#define BTN_SW1_GPIO   GPIO_NUM_16
#define BTN_SW2_GPIO   GPIO_NUM_14
```

---

## Initialization Templates

### I2C Bus Init
```c
static esp_err_t i2c_bus0_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_GPIO,
        .scl_io_num = OLED_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_PORT_OLED, &conf);
    return i2c_driver_install(I2C_PORT_OLED, conf.mode, 0, 0, 0);
}
```

### SH1106 Init Sequence
```c
static esp_err_t sh1106_init(void) {
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0xAD, 0x8B,  // DC-DC ON
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
        0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (size_t i = 0; i < sizeof(cmds); i++) {
        if (sh1106_write_cmd(cmds[i]) != ESP_OK) return ESP_FAIL;
    }
    return ESP_OK;
}
```

### sh1106_update() — Flush Framebuffer
```c
static esp_err_t sh1106_update(void) {
    uint8_t buf[130];
    for (uint8_t page = 0; page < 8; page++) {
        sh1106_write_cmd(0xB0 + page);
        sh1106_write_cmd(0x02); // col offset 2 ← SH1106 specific!
        sh1106_write_cmd(0x10);
        buf[0] = 0x40;
        memcpy(buf + 1, s_oled_fb + page * 128, 128);
        i2c_master_write_to_device(I2C_PORT_OLED, OLED_ADDR, buf, 129, pdMS_TO_TICKS(50));
    }
    return ESP_OK;
}
```

### Drawing Functions
```c
static void oled_clear(void) { memset(s_oled_fb, 0, 1024); }

static void oled_draw_pixel(int x, int y, int on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = x + (y / 8) * 128;
    if (on) s_oled_fb[idx] |=  (1 << (y % 8));
    else    s_oled_fb[idx] &= ~(1 << (y % 8));
}

// col/row เป็น character grid (8×8 font)
static void oled_print(int col, int row, const char *str, int on);
```

---

## ADC Oneshot Init (สำหรับ Joystick)

```c
static esp_err_t adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = JS_ADC_UNIT };
    adc_oneshot_new_unit(&init_config, &s_adc_handle);
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(s_adc_handle, JS_X_CHAN, &chan_config);
    return adc_oneshot_config_channel(s_adc_handle, JS_Y_CHAN, &chan_config);
}
```

---

## CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_timer nvs_flash esp_adc
)
```

---

## See Also

- [[led_matrix]] — LED Matrix 16×8 (HT16K33) — ใช้ I2C_NUM_0 bus เดียวกัน
- [[sensor_guide]] — Joystick ADC channel mapping
- [[kidbright32_pinout]] — GPIO ทั้งหมด
