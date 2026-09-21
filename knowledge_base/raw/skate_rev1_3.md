# KidBright Skate Rev 1.3 — Technical Reference & Code Examples

> **Source**: Verified from `Skate_rev1_3.kicad_sch` schematic (KiCad EDA 9.0.6) + physical board photo analysis
> **Board**: Skate V1.3 expansion board for KidBright32 — self-balancing two-wheeled robot
> **PCB Label**: "SKATE V1.3" (confirmed from physical board photo)

---

## 1. Hardware Overview

Skate คือ expansion board ที่ต่อเข้ากับ KidBright32 ผ่าน **J18 (10-pin header)**
เพิ่ม motor driver module, IMU (MPU6050), IR sensors, OLED display และวงจร power management

---

## 1A. Physical Board Layout (Verified from Photo)

```
┌─────────────────────────────────────────────────────┐
│              KidBright32 iA Board (Top)             │
│  [KB CHAR]  [OLED 128x64 SSD1306 - fitted]         │
│  [RESET]    [SWITCH2 button]   [PLUG SRO area]     │
│  [SERVO1 connector - left side]                     │
│  [SERVO2 connector]                                 │
└─────────────────────────────────────────────────────┘
         ↕ J18 10-pin header connection
┌─────────────────────────────────────────────────────┐
│              Skate V1.3 Expansion Board             │
│  [MPU6050 IMU - I2C addr 0x68]                     │
│  [Step-down 5V XL4005]  [Step-down 3.3V]           │
│  [Power switch SW3]  [Battery connector J10]        │
└─────────────────────────────────────────────────────┘
         ↕ Motor signal wires (GPIO18/19, GPIO32/33)
┌─────────────────────────────────────────────────────┐
│         External Motor Driver Module (Red PCB)      │
│  DRV8833 / TB6612FNG Dual H-Bridge                  │
│  [Blue screw terminal L-] [Blue screw terminal L+] │
│  [Blue screw terminal R-] [Blue screw terminal R+] │
│  IN1 IN2 IN3 IN4 wires from Skate board            │
└─────────────────────────────────────────────────────┘
         ↕ Motor wires (Red=+, Black=-)
    [Left Motor]                   [Right Motor]
    Yellow circular chassis with 2 wheels
```

### Physically Confirmed Components (from photo):
| Component | Status | Notes |
|-----------|--------|-------|
| OLED 128x64 SSD1306 | ✅ Installed & connected | J26 connector, I2C 0x3C |
| SWITCH2 button | ✅ Visible, accessible | GPIO14, active LOW |
| SERVO1 connector | ✅ Present (left side) | GPIO17 (TX2) |
| MPU6050 IMU | ✅ On Skate board | J3 header, I2C 0x68 |
| Motor Driver Module | ✅ Red PCB, bottom | External DRV8833/TB6612 |
| Blue screw terminals | ✅ 4x for motor wires | L+, L-, R+, R- |
| Battery connector | ✅ XH2.54 2P | 7.4V 2S Li-ion |
| "SKATE V1.3" label | ✅ Printed on PCB | Confirms hardware version |

### Motor Driver Module (External Red PCB):
- **Type**: DRV8833 or TB6612FNG dual H-bridge (external module mounted on chassis)
- **Input signals from Skate board**: IN1 (GPIO32), IN2 (GPIO33), IN3 (GPIO18), IN4 (GPIO19)
- **Motor terminals**: 4x blue screw terminal blocks
  - **L+ / L-** → Left Motor wires
  - **R+ / R-** → Right Motor wires
- **Power**: 5V from Skate board step-down (XL4005)
- **Wiring convention** (from photo): Red wire = Motor+, Black wire = Motor-

> **NOTE FOR AI**: The motor driver is an **external module** between the Skate board GPIO pins and the motors.
> GPIO18/19/32/33 connect to motor driver INPUT pins. Motor driver OUTPUT pins connect to motor terminals.
> Do NOT drive motors directly from GPIO — always route through the motor driver module.

### Chassis & Mechanical:
- **Platform**: Yellow circular disc
- **Drive**: 2-wheel differential drive (left + right)
- **Balance axis**: Perpendicular to wheel axle (pitch axis)
- **Center of mass**: Components stacked vertically on circular platform

### J18 Connector (Skate <-> KidBright32)
| Pin | Net | หมายเหตุ |
|-----|-----|---------|
| 1 | VN (GPIO39) | Battery voltage monitor |
| 2 | GND | |
| 3 | +3V3 | |
| 4 | SCL0 (GPIO22) | I2C_NUM_0 SCL |
| 5 | SDA0 (GPIO21) | I2C_NUM_0 SDA |
| 6 | IO18 | Left Motor IN3_MOT |
| 7 | IO19 | Left Motor IN4_MOT |
| 8 | IO23 | General purpose |
| 9 | GND | |
| 10 | +3V3 | |

---

## 2. GPIO Pin Mapping (VERIFIED FROM SCHEMATIC)

| GPIO | Net Label | Component | Use |
|------|-----------|-----------|-----|
| GPIO18 | IN3_MOT | J8/J9 | Left Motor control A |
| GPIO19 | IN4_MOT | J8/J9 | Left Motor control B |
| GPIO32 | IN1 | J1/J2 | Right Motor control A |
| GPIO33 | IN2 | J1/J2 | Right Motor control B |
| GPIO34 | IN3 / IN3_SEN | J9 | IR-R sensor (Right IR) |
| GPIO35 | IN4 / IN4_SEN | J8 | IR-L sensor (Left IR) |
| GPIO21 | SDA0 | J18 pin5 | I2C_NUM_0 SDA (LED Matrix + MPU6050) |
| GPIO22 | SCL0 | J18 pin4 | I2C_NUM_0 SCL (LED Matrix + MPU6050) |
| GPIO39/VN | VN | J18 pin1 | Battery voltage monitor (ADC) |
| GPIO36/VP | VP | - | Analog input VP |
| GPIO25 | USB_SW | - | USB Switch control |
| GPIO26 | OUT1 | - | Output 1 (KidBright on-board) |
| GPIO27 | OUT2 | - | Output 2 (KidBright on-board) |
| GPIO14 | SW2 | - | SW2 button (active LOW) |
| GPIO16 | SW1 | - | SW1 button (active LOW) |
| GPIO4 | SDA1 | - | I2C_NUM_1 SDA (LM73 temp) |
| GPIO5 | SCL1 | - | I2C_NUM_1 SCL (LM73 temp) |
| GPIO17 | SERVO2 | J24 | Servo 2 output |
| GPIO23 | IO23 | - | General purpose / WIFI |
| GPIO12 | IOT | - | IoT indicator |
| GPIO13 | PWM | BZ1 | PWM / Buzzer (shared) |

---

## 3. Power System

- **Battery**: 7.4V 2S Li-ion -> J10 (XH2.54 2P)
- **5V Rail**: Step-down XL4005 (J13) — motor logic
- **3.3V Rail**: Step-down (J29) — ESP32 + sensors
- **Motor Power Enable**: SW3 + Q1 (2N7002H MOSFET) -> net `+5V_STP`
- **Battery voltage divider**: R5=100kOhm, R6=20kOhm -> GPIO39/VN

### Battery Voltage Formula
```
Vbat = ADC_raw * (3.3 / 4095.0) * ((100 + 20) / 20)
     = ADC_raw * 0.009671f   // approx, for 12-bit ADC
```

---

## 4. I2C Bus Architecture

```
I2C_NUM_0 (SDA=GPIO21, SCL=GPIO22):
  +-- 0x70  HT16K33  -- LED Matrix 16x8 (on KidBright32)
  +-- 0x68  MPU6050  -- 6-axis IMU (on Skate board, J3)
  +-- 0x3C  SSD1306  -- OLED 128x64 (optional, J26)

I2C_NUM_1 (SDA=GPIO4, SCL=GPIO5):
  +-- 0x4D  LM73     -- Temperature sensor (on KidBright32 iA)
```

CRITICAL: init I2C_NUM_0 only ONCE — all devices (LED Matrix, MPU6050, OLED) share this bus.
NEVER call i2c_driver_install() twice on I2C_NUM_0. It returns ESP_ERR_INVALID_STATE.

---

## 5. Code Examples (ESP-IDF v5.x, Pure C)

### 5.1 CMakeLists.txt for Skate Project

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(skate_balance)
```

```cmake
# main/CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_timer
)
```

---

### 5.2 Motor Control (LEDC PWM)

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// ============================================================
// MOTOR GPIO (from Skate_rev1_3 schematic)
// ============================================================
#define MOTOR_L_A   GPIO_NUM_18   // Left  Motor IN3_MOT
#define MOTOR_L_B   GPIO_NUM_19   // Left  Motor IN4_MOT
#define MOTOR_R_A   GPIO_NUM_32   // Right Motor IN1
#define MOTOR_R_B   GPIO_NUM_33   // Right Motor IN2

// ============================================================
// LEDC channels
// ============================================================
#define CH_L_A      LEDC_CHANNEL_0
#define CH_L_B      LEDC_CHANNEL_1
#define CH_R_A      LEDC_CHANNEL_2
#define CH_R_B      LEDC_CHANNEL_3
#define MOTOR_TIMER LEDC_TIMER_0
#define MOTOR_BITS  LEDC_TIMER_10_BIT   // duty: 0-1023
#define MOTOR_FREQ  20000               // 20 kHz

static void motor_ledc_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = MOTOR_BITS,
        .timer_num       = MOTOR_TIMER,
        .freq_hz         = MOTOR_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch[] = {
        { .gpio_num = MOTOR_L_A, .channel = CH_L_A,
          .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = MOTOR_TIMER,
          .duty = 0, .hpoint = 0 },
        { .gpio_num = MOTOR_L_B, .channel = CH_L_B,
          .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = MOTOR_TIMER,
          .duty = 0, .hpoint = 0 },
        { .gpio_num = MOTOR_R_A, .channel = CH_R_A,
          .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = MOTOR_TIMER,
          .duty = 0, .hpoint = 0 },
        { .gpio_num = MOTOR_R_B, .channel = CH_R_B,
          .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = MOTOR_TIMER,
          .duty = 0, .hpoint = 0 },
    };
    for (int i = 0; i < 4; i++) ledc_channel_config(&ch[i]);
}

// duty: 0-1023, direction: 1=forward, -1=backward, 0=stop (brake)
static void motor_set(ledc_channel_t ch_a, ledc_channel_t ch_b,
                      int duty, int direction) {
    if (duty < 0) duty = -duty;
    if (duty > 1023) duty = 1023;

    if (direction > 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_a, (uint32_t)duty);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_b, 0);
    } else if (direction < 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_a, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_b, (uint32_t)duty);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_a, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_b, 0);
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_b);
}

static inline void motor_left(int duty, int dir)  { motor_set(CH_L_A, CH_L_B, duty, dir); }
static inline void motor_right(int duty, int dir) { motor_set(CH_R_A, CH_R_B, duty, dir); }
static inline void motors_stop(void) { motor_left(0, 0); motor_right(0, 0); }
```

---

### 5.3 I2C Init + MPU6050 Read

```c
// ============================================================
// I2C (shared bus: LED Matrix 0x70 + MPU6050 0x68)
// ============================================================
#define I2C_PORT     I2C_NUM_0
#define I2C_SDA      GPIO_NUM_21
#define I2C_SCL      GPIO_NUM_22
#define I2C_FREQ_HZ  400000

#define MPU6050_ADDR    0x68
#define MPU_REG_PWR     0x6B    // Power Management 1
#define MPU_REG_ACCEL   0x3B    // ACCEL_XOUT_H (first of 14 bytes)

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
} mpu_data_t;

// Call ONCE — DO NOT call again for MPU6050 or OLED on same bus
static void i2c_bus0_init(void) {
    i2c_config_t conf = {0};
    conf.mode             = I2C_MODE_MASTER;
    conf.sda_io_num       = I2C_SDA;
    conf.scl_io_num       = I2C_SCL;
    conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FREQ_HZ;
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
}

static void mpu6050_init(void) {
    uint8_t buf[2] = { MPU_REG_PWR, 0x00 };  // wake up: clear sleep bit
    esp_err_t ret = i2c_master_write_to_device(
        I2C_PORT, MPU6050_ADDR, buf, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        // Wiring check: SDA=GPIO21, SCL=GPIO22, 4.7k pull-up to 3.3V
    }
}

// Read 14 bytes starting from ACCEL_XOUT_H
// Layout: AX_H, AX_L, AY_H, AY_L, AZ_H, AZ_L, TEMP_H, TEMP_L,
//         GX_H, GX_L, GY_H, GY_L, GZ_H, GZ_L
static esp_err_t mpu6050_read(mpu_data_t *d) {
    uint8_t reg = MPU_REG_ACCEL;
    uint8_t raw[14] = {0};
    esp_err_t ret = i2c_master_write_read_device(
        I2C_PORT, MPU6050_ADDR,
        &reg, 1,
        raw, 14,
        pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    d->ax = (int16_t)((raw[0]  << 8) | raw[1]);
    d->ay = (int16_t)((raw[2]  << 8) | raw[3]);
    d->az = (int16_t)((raw[4]  << 8) | raw[5]);
    // raw[6..7] = temperature (ignored)
    d->gx = (int16_t)((raw[8]  << 8) | raw[9]);
    d->gy = (int16_t)((raw[10] << 8) | raw[11]);
    d->gz = (int16_t)((raw[12] << 8) | raw[13]);
    return ESP_OK;
}
```

---

### 5.4 Complementary Filter (Pitch Angle)

```c
// MPU6050 default scale factors
#define ACCEL_SCALE  16384.0f   // +/-2g  -> LSB/g
#define GYRO_SCALE   131.0f     // +/-250 deg/s -> LSB/(deg/s)
#define ALPHA        0.98f      // complementary weight (gyro trust)

static float g_pitch = 0.0f;

// Call at fixed dt seconds (e.g. 0.005s for 200 Hz)
static void angle_update(const mpu_data_t *d, float dt) {
    float ax_g = d->ax / ACCEL_SCALE;
    float ay_g = d->ay / ACCEL_SCALE;
    float az_g = d->az / ACCEL_SCALE;

    // Pitch from accelerometer (tilt around X axis)
    float accel_pitch = atan2f(ax_g, sqrtf(ay_g * ay_g + az_g * az_g))
                        * (180.0f / (float)M_PI);

    // Pitch rate from gyroscope (Gy axis)
    float gyro_rate = d->gy / GYRO_SCALE;

    // Complementary filter: trust gyro 98%, accel 2%
    g_pitch = ALPHA * (g_pitch + gyro_rate * dt)
            + (1.0f - ALPHA) * accel_pitch;
}
```

---

### 5.5 PID Controller

```c
typedef struct {
    float kp, ki, kd;
    float setpoint;
    float integral;
    float prev_error;
    float integral_limit;
} pid_t;

static void pid_init(pid_t *p, float kp, float ki, float kd, float setpoint) {
    p->kp             = kp;
    p->ki             = ki;
    p->kd             = kd;
    p->setpoint       = setpoint;
    p->integral       = 0.0f;
    p->prev_error     = 0.0f;
    p->integral_limit = 300.0f;  // anti-windup clamp
}

// Returns motor output: positive=forward, negative=backward
static float pid_compute(pid_t *p, float measured, float dt) {
    float error = p->setpoint - measured;

    p->integral += error * dt;
    if (p->integral >  p->integral_limit) p->integral =  p->integral_limit;
    if (p->integral < -p->integral_limit) p->integral = -p->integral_limit;

    float derivative = (error - p->prev_error) / dt;
    p->prev_error = error;

    return (p->kp * error) + (p->ki * p->integral) + (p->kd * derivative);
}
```

---

### 5.6 Full Balance Loop (app_main + FreeRTOS Task)

```c
static const char *TAG = "SKATE";

// Tuning parameters — adjust for your robot
#define PID_KP       25.0f
#define PID_KI        0.5f
#define PID_KD        0.8f
#define BALANCE_SETPOINT  0.0f   // target tilt angle (degrees)
#define FALL_ANGLE   35.0f       // cut motors if tilted > 35 deg

// Control loop: 200 Hz
#define LOOP_HZ      200
#define LOOP_US      (1000000 / LOOP_HZ)
#define LOOP_DT      (1.0f / LOOP_HZ)

static void balance_task(void *arg) {
    pid_t pid = {0};
    pid_init(&pid, PID_KP, PID_KI, PID_KD, BALANCE_SETPOINT);

    mpu_data_t imu = {0};
    int64_t last_us = esp_timer_get_time();

    while (1) {
        int64_t now = esp_timer_get_time();
        float dt = (float)(now - last_us) / 1e6f;
        last_us = now;
        if (dt <= 0.0f || dt > 0.1f) dt = LOOP_DT; // safety clamp

        if (mpu6050_read(&imu) != ESP_OK) {
            motors_stop();
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        angle_update(&imu, dt);

        // Safety: cut power if robot has fallen
        if (g_pitch > FALL_ANGLE || g_pitch < -FALL_ANGLE) {
            motors_stop();
            ESP_LOGW(TAG, "FALLEN pitch=%.1f", g_pitch);
            pid.integral = 0.0f;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        float output = pid_compute(&pid, g_pitch, dt);
        int duty = (int)fabsf(output);
        if (duty > 1023) duty = 1023;
        int dir = (output >= 0.0f) ? 1 : -1;

        motor_left(duty, dir);
        motor_right(duty, dir);

        // Wait for remainder of 5 ms slot
        int64_t elapsed = esp_timer_get_time() - now;
        if (elapsed < LOOP_US) {
            vTaskDelay(pdMS_TO_TICKS((LOOP_US - elapsed) / 1000));
        }
    }
}

void app_main(void) {
    // 1. I2C_NUM_0 init ONCE (shared: LED Matrix 0x70 + MPU6050 0x68)
    i2c_bus0_init();

    // 2. Wake up MPU6050
    mpu6050_init();

    // 3. Init motor PWM
    motor_ledc_init();

    // 4. Sensor settle time
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_LOGI(TAG, "Skate balance started (200 Hz PID loop)");

    // 5. Balance task pinned to core 1 (core 0 for WiFi/BT if needed)
    xTaskCreatePinnedToCore(balance_task, "balance", 4096, NULL, 5, NULL, 1);
}
```

---

### 5.7 Battery Voltage Monitor (GPIO39/VN)

```c
// Voltage divider: R5=100k, R6=20k
// Vbat = Vadc * (100+20)/20 = Vadc * 6
#define BAT_ADC_UNIT    ADC_UNIT_1
#define BAT_ADC_CHAN    ADC_CHANNEL_3   // GPIO39/VN (input-only pin)
#define BAT_DIVIDER     6.0f

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

static void bat_adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = BAT_ADC_UNIT };
    adc_oneshot_new_unit(&init_cfg, &s_adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,        // ALWAYS use DB_12, never DB_11
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHAN, &chan_cfg);
}

static float bat_read_voltage(void) {
    int raw = 0;
    adc_oneshot_read(s_adc_handle, BAT_ADC_CHAN, &raw);
    float vadc = (float)raw * 3.3f / 4095.0f;
    return vadc * BAT_DIVIDER;  // actual battery voltage in V
}
```

---

### 5.8 IR Sensor (GPIO34=IR-R, GPIO35=IR-L)

```c
#define IR_RIGHT    GPIO_NUM_34   // IN3_SEN net
#define IR_LEFT     GPIO_NUM_35   // IN4_SEN net

static void ir_gpio_init(void) {
    gpio_config_t io = {0};
    io.intr_type    = GPIO_INTR_DISABLE;
    io.mode         = GPIO_MODE_INPUT;
    io.pin_bit_mask = (1ULL << IR_RIGHT) | (1ULL << IR_LEFT);
    io.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io);
}

// Returns 0 = obstacle detected, 1 = clear (active-LOW sensor)
static inline int ir_right(void) { return gpio_get_level(IR_RIGHT); }
static inline int ir_left(void)  { return gpio_get_level(IR_LEFT);  }
```

---

### 5.9 HT16K33 LED Matrix (shared I2C_NUM_0 with MPU6050)

```c
// DO NOT re-init I2C_NUM_0 here -- already done in i2c_bus0_init()
#define MATRIX_ADDR  0x70

static void matrix_init(void) {
    uint8_t cmds[] = { 0x21, 0x81, 0xEF };  // Osc ON, Display ON, Max brightness
    for (int i = 0; i < 3; i++) {
        i2c_master_write_to_device(I2C_PORT, MATRIX_ADDR,
                                   &cmds[i], 1, pdMS_TO_TICKS(10));
    }
}

// KidBright32 iA interleaved layout:
// cols[0..7]  -> even bytes (left screen)
// cols[8..15] -> odd bytes  (right screen)
static void matrix_draw(const uint8_t cols[16]) {
    uint8_t buf[17] = {0};
    buf[0] = 0x00;  // register pointer
    for (int c = 0; c < 8; c++) {
        buf[1 + c * 2] = cols[c];       // even index -> left screen
        buf[2 + c * 2] = cols[c + 8];   // odd index  -> right screen
    }
    i2c_master_write_to_device(I2C_PORT, MATRIX_ADDR, buf, 17, pdMS_TO_TICKS(100));
}

// Convert row-based pattern to column layout and display
static void rows_to_columns_16x8(const uint16_t row_data[8], uint8_t out[16]) {
    memset(out, 0, 16);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 16; col++) {
            if (row_data[row] & (1 << (15 - col))) {
                out[col] |= (1 << (7 - row));
            }
        }
    }
}
```

---

## 6. Common Build Errors & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `ESP_ERR_INVALID_STATE` on I2C | `i2c_driver_install()` called twice | Init I2C_NUM_0 ONCE only |
| MPU6050 reads all 0xFF | No pull-up or wrong wiring | SDA=GPIO21, SCL=GPIO22, 4.7k pull-up to 3.3V |
| Motor spins one way only | Wrong LEDC channel/GPIO | Left=GPIO18/19, Right=GPIO32/33 |
| Robot falls without trying to balance | PID Kp too low | Start Kp=20 and increase |
| `implicit declaration of i2c_master_write_read_device` | Missing include | Add `#include "driver/i2c.h"` |
| `undefined reference` to esp_timer | Missing PRIV_REQUIRES | Add `driver esp_timer` in main/CMakeLists.txt |
| Oscillation/vibration | Kp too high or Kd too low | Reduce Kp by 20%, increase Kd |

---

## 7. PID Tuning Guide

1. **Start**: Kp=20, Ki=0, Kd=0 — test upright balance
2. **Kp too low**: Robot slowly falls over — increase Kp
3. **Kp too high**: Robot oscillates/vibrates — reduce Kp, add Kd
4. **Add Kd=0.5**: Reduces oscillation — increase by 0.1 until stable
5. **Add Ki=0.1**: Corrects steady lean — increase slowly, watch for windup
6. **SETPOINT**: Adjust +/-2 degrees to compensate for physical imbalance

---

## 8. Wiring Checklist

### MPU6050 (on Skate board J3 — already mounted)
- [ ] MPU6050 VCC -> 3.3V (NOT 5V)
- [ ] MPU6050 SDA -> GPIO21 (with 4.7k pull-up to 3.3V)
- [ ] MPU6050 SCL -> GPIO22 (with 4.7k pull-up to 3.3V)
- [ ] MPU6050 AD0 -> GND (for address 0x68)

### Motor Driver Module (External Red PCB with blue screw terminals)
- [ ] Motor Driver IN1 -> Skate board IN1 (GPIO32) — Right Motor A
- [ ] Motor Driver IN2 -> Skate board IN2 (GPIO33) — Right Motor B
- [ ] Motor Driver IN3 -> Skate board IN3_MOT (GPIO18) — Left Motor A
- [ ] Motor Driver IN4 -> Skate board IN4_MOT (GPIO19) — Left Motor B
- [ ] Motor Driver VCC -> 5V from Skate step-down (XL4005)
- [ ] Motor Driver GND -> GND (common ground)
- [ ] Left Motor wires -> L+ / L- screw terminals (Red=+, Black=-)
- [ ] Right Motor wires -> R+ / R- screw terminals (Red=+, Black=-)

### Power
- [ ] Battery 7.4V -> J10 XH2.54 2P connector
- [ ] Battery polarity: verify + and - before connecting!

### OLED (confirmed fitted in photo)
- [ ] OLED J26 connector already wired to I2C_NUM_0 (GPIO21/22)
- [ ] I2C address: 0x3C (typical) — scan if display not showing

---

## 9. SERVO Control Code (GPIO16=SW1, GPIO17=SERVO2 confirmed from photo)

```c
// SERVO1 uses GPIO17 (TX2) via LEDC PWM — 50Hz for standard servo
// Standard servo: 1ms pulse = 0 deg, 2ms pulse = 180 deg
// Period = 20ms (50Hz), duty resolution LEDC_TIMER_14_BIT

#include "driver/ledc.h"

#define SERVO_PIN       GPIO_NUM_17   // SERVO2 on schematic (matches SERVO1 connector in photo)
#define SERVO_TIMER     LEDC_TIMER_1
#define SERVO_CHANNEL   LEDC_CHANNEL_4
#define SERVO_FREQ_HZ   50
#define SERVO_BITS      LEDC_TIMER_14_BIT   // 0-16383
// For 14-bit at 50Hz: 1ms = ~819, 1.5ms = ~1229, 2ms = ~1639

static void servo_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_BITS,
        .timer_num       = SERVO_TIMER,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = SERVO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = SERVO_CHANNEL,
        .timer_sel  = SERVO_TIMER,
        .duty       = 1229,  // 90 degrees center
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
}

// angle_deg: 0 to 180
static void servo_set_angle(int angle_deg) {
    if (angle_deg < 0)   angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;
    // Map 0-180 deg -> 819-1639 duty (1ms to 2ms pulse)
    uint32_t duty = 819 + (uint32_t)(angle_deg * (1639 - 819) / 180);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
}
```

---

## 10. OLED Display Init (I2C shared with MPU6050 — DO NOT re-init bus)

> OLED is confirmed present and wired from photo. Uses I2C_NUM_0 addr 0x3C.
> NEVER call i2c_driver_install() again after i2c_bus0_init().

```c
// Minimal SSD1306 init — sends commands directly via I2C
// For full graphics, use install_idf_library("nopnop2002/esp-idf-ssd1306")

#define OLED_ADDR  0x3C
#define OLED_CMD   0x00  // Co=0, D/C#=0 (command)
#define OLED_DATA  0x40  // Co=0, D/C#=1 (data)

static void oled_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = { OLED_CMD, cmd };
    i2c_master_write_to_device(I2C_PORT, OLED_ADDR, buf, 2, pdMS_TO_TICKS(10));
}

static void oled_init(void) {
    // Standard SSD1306 init sequence
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x20); oled_send_cmd(0x00); // Horizontal addressing
    oled_send_cmd(0xB0); // Page start address
    oled_send_cmd(0xC8); // COM scan direction: remapped
    oled_send_cmd(0x00); // Lower column start address
    oled_send_cmd(0x10); // Upper column start address
    oled_send_cmd(0x40); // Display start line
    oled_send_cmd(0x81); oled_send_cmd(0xFF); // Contrast = max
    oled_send_cmd(0xA1); // Segment remap
    oled_send_cmd(0xA6); // Normal display (not inverted)
    oled_send_cmd(0xA8); oled_send_cmd(0x3F); // Multiplex ratio = 64
    oled_send_cmd(0xA4); // Output follows RAM
    oled_send_cmd(0xD3); oled_send_cmd(0x00); // Display offset = 0
    oled_send_cmd(0xD5); oled_send_cmd(0xF0); // Clock divide ratio
    oled_send_cmd(0xD9); oled_send_cmd(0x22); // Pre-charge period
    oled_send_cmd(0xDA); oled_send_cmd(0x12); // COM pins hardware config
    oled_send_cmd(0xDB); oled_send_cmd(0x20); // VCOMH deselect level
    oled_send_cmd(0x8D); oled_send_cmd(0x14); // Charge pump ON
    oled_send_cmd(0xAF); // Display ON
}

// Clear entire display (fill with 0x00)
static void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page);  // Set page
        oled_send_cmd(0x00);          // Col low nibble
        oled_send_cmd(0x10);          // Col high nibble
        uint8_t buf[129];
        buf[0] = OLED_DATA;
        memset(buf + 1, 0x00, 128);
        i2c_master_write_to_device(I2C_PORT, OLED_ADDR, buf, 129, pdMS_TO_TICKS(50));
    }
}
```

> **TIP**: For full text rendering on OLED, use the `esp-idf-ssd1306` component:
> ```cmake
> # install_idf_library("nopnop2002/esp-idf-ssd1306")
> ```

---

## 11. SWITCH2 Input (GPIO14 — Confirmed visible in photo)

```c
#define SW2_PIN     GPIO_NUM_14   // SWITCH2 — active LOW
#define SW1_PIN     GPIO_NUM_16   // SW1 — active LOW

static void buttons_init(void) {
    gpio_config_t io = {0};
    io.intr_type    = GPIO_INTR_DISABLE;
    io.mode         = GPIO_MODE_INPUT;
    io.pin_bit_mask = (1ULL << SW2_PIN) | (1ULL << SW1_PIN);
    io.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io);
}

// Returns true when button is pressed (active LOW)
static inline bool sw2_pressed(void) { return gpio_get_level(SW2_PIN) == 0; }
static inline bool sw1_pressed(void) { return gpio_get_level(SW1_PIN) == 0; }
```

---

## 12. Photo-Confirmed Hardware Notes

Based on physical board inspection:

1. **OLED is pre-wired** — no additional wiring needed if using J26 connector
2. **Motor driver is EXTERNAL** (red module, NOT integrated on Skate PCB) — signal wires must be connected
3. **MPU6050 header J3** — chip/module plugs into 8-pin header on Skate board
4. **SWITCH2 is on KidBright board** (not Skate board) — GPIO14 accessible from KidBright
5. **SERVO1 connector** is on the LEFT side of KidBright board — physically labeled
6. **Battery connector** (XH2.54) on Skate board — verify polarity before plugging
7. **Power switch SW3** controls motor 5V rail (+5V_STP) — must be ON for motors to work
8. **Circular yellow chassis** — MPU6050 pitch axis is perpendicular to wheel axle

