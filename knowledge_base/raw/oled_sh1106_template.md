# OLED SH1106 & LED Matrix Full Code Templates

> ⚠️ **AI INSTRUCTION:** Read this file via `read_kb_file("oled_sh1106_template.md")` whenever the user requests OLED display or joystick calibration code. Use all templates verbatim.

---

## 1. SH1106 OLED Display Template

- Address = 0x3C, I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22)
- 1024-byte framebuffer `s_oled_fb[1024]`
- SH1106 requires column offset 2: send lower col `0x02`, higher col `0x10` before each page
- Character grid: col×8 pixels wide, row×8 pixels tall (8×8 font)

### Pin Mapping (KidBright32 iA/V1.6) — Verified on hardware
| Signal | GPIO | ADC Channel | Notes |
|--------|------|-------------|-------|
| JS_X_CHAN (Left-Right) | GPIO34 (IN3) | ADC_CHANNEL_6 | ADC_UNIT_1 — confirmed working |
| JS_Y_CHAN (Up-Down) | GPIO35 (IN4) | ADC_CHANNEL_7 | ADC_UNIT_1 — confirmed working |
| SW1 (Cal button) | GPIO16 | — | Active LOW, internal pull-up OK |
| SW2 (Cancel button) | GPIO14 | — | Active LOW, internal pull-up OK |

> ⚠️ **ADC Channel Truth:** `ADC_CHANNEL_6 = GPIO34`, `ADC_CHANNEL_7 = GPIO35`. Old comments in AI-generated code were swapped — always trust the channel number, not the comment.

### Complete C Code Template

```c
#define I2C_PORT_OLED  I2C_NUM_0
#define OLED_SDA_GPIO  GPIO_NUM_21
#define OLED_SCL_GPIO  GPIO_NUM_22
#define OLED_ADDR      0x3C

// Joystick ADC channels (KidBright32 iA/V1.6) — Verified on hardware
#define JS_X_CHAN      ADC_CHANNEL_6   // GPIO34 (IN3) → Left-Right ✅ confirmed
#define JS_Y_CHAN      ADC_CHANNEL_7   // GPIO35 (IN4) → Up-Down   ✅ confirmed
#define JS_ADC_UNIT    ADC_UNIT_1

// On-board buttons
#define BTN_SW1_GPIO   GPIO_NUM_16     // SW1 — step through calibration
#define BTN_SW2_GPIO   GPIO_NUM_14     // SW2 — cancel / escape
// ⚠️ NEVER use GPIO_NUM_17 for SW2 — it does not exist on this board!

static uint8_t s_oled_fb[1024];

// --- I2C Bus 0 init ---
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

// --- SH1106 low-level ---
static esp_err_t sh1106_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_PORT_OLED, OLED_ADDR, buf, 2, pdMS_TO_TICKS(50));
}

static esp_err_t sh1106_init(void) {
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0xAD, 0x8B, // DC-DC ON
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
        0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (size_t i = 0; i < sizeof(cmds); i++) {
        if (sh1106_write_cmd(cmds[i]) != ESP_OK) return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t sh1106_update(void) {
    uint8_t buf[130];
    for (uint8_t page = 0; page < 8; page++) {
        sh1106_write_cmd(0xB0 + page);
        sh1106_write_cmd(0x02); // col offset 2 for SH1106
        sh1106_write_cmd(0x10);
        buf[0] = 0x40;
        memcpy(buf + 1, s_oled_fb + page * 128, 128);
        esp_err_t err = i2c_master_write_to_device(I2C_PORT_OLED, OLED_ADDR, buf, 129, pdMS_TO_TICKS(50));
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static void oled_clear(void) { memset(s_oled_fb, 0, 1024); }

static void oled_draw_pixel(int x, int y, int on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = x + (y / 8) * 128;
    if (on) s_oled_fb[idx] |=  (1 << (y % 8));
    else    s_oled_fb[idx] &= ~(1 << (y % 8));
}

static void oled_print(int col, int row, const char *str, int on) {
    int x = col * 8;
    int y = row * 8;
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 127) c = ' ';
        int fi = c - 32;
        for (int r = 0; r < 8; r++) {
            uint8_t line = font8x8[fi][r];
            for (int cl = 0; cl < 8; cl++) {
                if (line & (1 << (7 - cl))) oled_draw_pixel(x + cl, y + r, on);
            }
        }
        x += 8;
    }
}
```

### ADC Oneshot Init (for Joystick)
```c
static adc_oneshot_unit_handle_t s_adc_handle;

static esp_err_t adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = JS_ADC_UNIT };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (err != ESP_OK) return err;
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(s_adc_handle, JS_X_CHAN, &chan_config);
    if (err != ESP_OK) return err;
    return adc_oneshot_config_channel(s_adc_handle, JS_Y_CHAN, &chan_config);
}
```

### Button ISR Setup (SW1/SW2)
```c
static QueueHandle_t s_btn_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t pin = (uint32_t)arg;
    xQueueSendFromISR(s_btn_evt_queue, &pin, NULL);
}

static void button_init(void) {
    s_btn_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_SW1_GPIO) | (1ULL << BTN_SW2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,   // Active LOW — trigger on press
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_SW1_GPIO, gpio_isr_handler, (void *)BTN_SW1_GPIO);
    gpio_isr_handler_add(BTN_SW2_GPIO, gpio_isr_handler, (void *)BTN_SW2_GPIO);
}
```

### CMakeLists.txt for OLED + Joystick + NVS
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_timer nvs_flash esp_adc
)
```

---

## 2. LED Matrix 16×8 Full Template (KidBright32 iA/V1.6)

> ⚠️ Exception: DO NOT use for Formula Kid CAR Receiver — use `kidbright32iA_updated.md` Section 20.3 instead.

- Chip: HT16K33, I2C_NUM_0, Address = 0x70
- **Interleaved layout:** Left screen cols → even indexes, Right screen cols → odd indexes
- **Y-axis inverted:** Bit 0 (0x01) = Top row, Bit 7 (0x80) = Bottom row
- **NEVER create `uint8_t img[16]` manually** — use `rows_to_columns_16x8()` or the pre-calculated arrays below

### rows_to_columns_16x8() Helper
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

### Pre-calculated Direction Arrows
```c
static const uint8_t img_center[16] = {0x00,0x18,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x18,0x00};
static const uint8_t img_up[16]     = {0x00,0xFF,0x00,0xFE,0x00,0x0C,0x00,0x08,0x08,0x00,0x0C,0x00,0xFE,0x00,0xFF,0x00};
static const uint8_t img_down[16]   = {0x00,0xFF,0x00,0x7F,0x00,0x30,0x00,0x10,0x10,0x00,0x30,0x00,0x7F,0x00,0xFF,0x00};
static const uint8_t img_left[16]   = {0x00,0x18,0x00,0x18,0x18,0x18,0x3C,0x18,0x7E,0x18,0xFF,0x18,0x18,0x00,0x18,0x00};
static const uint8_t img_right[16]  = {0x00,0x18,0x00,0x18,0x18,0xFF,0x18,0x7E,0x18,0x3C,0x18,0x18,0x18,0x00,0x18,0x00};
```

### matrix_draw() — Always use this to write to hardware
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

### Two-Digit Display (MANDATORY — use exact arrays, NOT self-invented fonts)
```c
static const uint16_t DIGIT_0[8] = {0x0E00,0x1100,0x1100,0x1100,0x1100,0x1100,0x1100,0x0E00};
static const uint16_t DIGIT_1[8] = {0x0200,0x0600,0x0A00,0x0200,0x0200,0x0200,0x0200,0x1F00};
static const uint16_t DIGIT_2[8] = {0x0E00,0x1100,0x0100,0x0200,0x0400,0x0800,0x1000,0x1F00};
static const uint16_t DIGIT_3[8] = {0x0E00,0x1100,0x0100,0x0600,0x0100,0x0100,0x1100,0x0E00};
static const uint16_t DIGIT_4[8] = {0x0200,0x0600,0x0A00,0x1200,0x1F00,0x0200,0x0200,0x0200};
static const uint16_t DIGIT_5[8] = {0x1F00,0x1000,0x1E00,0x0100,0x0100,0x0100,0x1100,0x0E00};
static const uint16_t DIGIT_6[8] = {0x0E00,0x1100,0x1000,0x1E00,0x1100,0x1100,0x1100,0x0E00};
static const uint16_t DIGIT_7[8] = {0x1F00,0x0100,0x0200,0x0400,0x0400,0x0400,0x0400,0x0400};
static const uint16_t DIGIT_8[8] = {0x0E00,0x1100,0x1100,0x0E00,0x1100,0x1100,0x1100,0x0E00};
static const uint16_t DIGIT_9[8] = {0x0E00,0x1100,0x1100,0x0F00,0x0100,0x0100,0x1100,0x0E00};
static const uint16_t *DIGITS[10] = {DIGIT_0,DIGIT_1,DIGIT_2,DIGIT_3,DIGIT_4,DIGIT_5,DIGIT_6,DIGIT_7,DIGIT_8,DIGIT_9};

static void display_two_digits(int tens, int units) {
    uint16_t comb[8];
    for (int i = 0; i < 8; i++) comb[i] = DIGITS[tens][i] | (DIGITS[units][i] >> 8);
    uint8_t cols[16];
    rows_to_columns_16x8(comb, cols);
    matrix_draw(cols);
}
```
