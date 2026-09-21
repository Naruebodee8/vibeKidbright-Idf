// ═══════════════════════════════════════════════════════════════════════════
//  hal_imu.h
//  IMU HAL for MPU6050 (GY-521 Module)
//  
//  Layer: 1 (Hardware Abstraction)
//  Dependencies: config.h, Wire.h
//  
//  Hardware: GY-521 Module (MPU6050)
//  Interface: I2C Bus 0 (GPIO 8, 9)
//  
//  Features:
//    - Gyroscope (±250, ±500, ±1000, ±2000 °/s)
//    - Accelerometer (±2, ±4, ±8, ±16 g)
//    - Temperature sensor
//    - Configurable Digital Low Pass Filter (DLPF)
//    - Calibration support
//    - Heading (yaw) integration
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_IMU_H
#define HAL_IMU_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: MPU6050 REGISTERS
// └───────────────────────────────────────────────────────────────────────────

#define MPU6050_ADDR            0x68    // Default I2C address (AD0 = GND)
#define MPU6050_ADDR_ALT        0x69    // Alternative address (AD0 = VCC)

// Configuration registers
#define MPU6050_REG_SMPLRT_DIV  0x19    // Sample Rate Divider
#define MPU6050_REG_CONFIG      0x1A    // Configuration (DLPF)
#define MPU6050_REG_GYRO_CONFIG 0x1B    // Gyroscope Configuration
#define MPU6050_REG_ACCEL_CONFIG 0x1C   // Accelerometer Configuration
#define MPU6050_REG_FIFO_EN     0x23    // FIFO Enable
#define MPU6050_REG_INT_PIN_CFG 0x37    // INT Pin Configuration
#define MPU6050_REG_INT_ENABLE  0x38    // Interrupt Enable
#define MPU6050_REG_INT_STATUS  0x3A    // Interrupt Status

// Data registers
#define MPU6050_REG_ACCEL_XOUT_H 0x3B   // Accelerometer X High
#define MPU6050_REG_ACCEL_XOUT_L 0x3C   // Accelerometer X Low
#define MPU6050_REG_ACCEL_YOUT_H 0x3D
#define MPU6050_REG_ACCEL_YOUT_L 0x3E
#define MPU6050_REG_ACCEL_ZOUT_H 0x3F
#define MPU6050_REG_ACCEL_ZOUT_L 0x40
#define MPU6050_REG_TEMP_OUT_H  0x41    // Temperature High
#define MPU6050_REG_TEMP_OUT_L  0x42    // Temperature Low
#define MPU6050_REG_GYRO_XOUT_H 0x43    // Gyroscope X High
#define MPU6050_REG_GYRO_XOUT_L 0x44    // Gyroscope X Low
#define MPU6050_REG_GYRO_YOUT_H 0x45
#define MPU6050_REG_GYRO_YOUT_L 0x46
#define MPU6050_REG_GYRO_ZOUT_H 0x47
#define MPU6050_REG_GYRO_ZOUT_L 0x48

// Power management
#define MPU6050_REG_PWR_MGMT_1  0x6B    // Power Management 1
#define MPU6050_REG_PWR_MGMT_2  0x6C    // Power Management 2
#define MPU6050_REG_WHO_AM_I    0x75    // Device ID (should return 0x68)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: CONFIGURATION CONSTANTS
// └───────────────────────────────────────────────────────────────────────────

// Gyroscope Full Scale Range
typedef enum {
    GYRO_FS_250DPS  = 0,    // ±250 °/s  (131 LSB/°/s)
    GYRO_FS_500DPS  = 1,    // ±500 °/s  (65.5 LSB/°/s)
    GYRO_FS_1000DPS = 2,    // ±1000 °/s (32.8 LSB/°/s)
    GYRO_FS_2000DPS = 3     // ±2000 °/s (16.4 LSB/°/s)
} GyroFullScale_t;

// Accelerometer Full Scale Range
typedef enum {
    ACCEL_FS_2G  = 0,       // ±2g  (16384 LSB/g)
    ACCEL_FS_4G  = 1,       // ±4g  (8192 LSB/g)
    ACCEL_FS_8G  = 2,       // ±8g  (4096 LSB/g)
    ACCEL_FS_16G = 3        // ±16g (2048 LSB/g)
} AccelFullScale_t;

// Digital Low Pass Filter Configuration
typedef enum {
    DLPF_260HZ = 0,         // Accel: 260Hz, Gyro: 256Hz, Delay: 0ms
    DLPF_184HZ = 1,         // Accel: 184Hz, Gyro: 188Hz, Delay: 2ms
    DLPF_94HZ  = 2,         // Accel: 94Hz,  Gyro: 98Hz,  Delay: 3ms
    DLPF_44HZ  = 3,         // Accel: 44Hz,  Gyro: 42Hz,  Delay: 4.9ms
    DLPF_21HZ  = 4,         // Accel: 21Hz,  Gyro: 20Hz,  Delay: 8.5ms
    DLPF_10HZ  = 5,         // Accel: 10Hz,  Gyro: 10Hz,  Delay: 13.8ms
    DLPF_5HZ   = 6          // Accel: 5Hz,   Gyro: 5Hz,   Delay: 19ms
} DLPFConfig_t;

// Scale factors (LSB per unit)
const float GYRO_SCALE_FACTOR[] = {131.0f, 65.5f, 32.8f, 16.4f};     // LSB/(°/s)
const float ACCEL_SCALE_FACTOR[] = {16384.0f, 8192.0f, 4096.0f, 2048.0f}; // LSB/g

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: DATA TYPES
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Raw IMU data (16-bit signed integers)
 */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temp;
} IMURawData_t;

/**
 * @brief Processed IMU data (floating point, physical units)
 */
typedef struct {
    // Accelerometer (g)
    float accel_x;
    float accel_y;
    float accel_z;
    
    // Gyroscope (°/s)
    float gyro_x;
    float gyro_y;
    float gyro_z;
    
    // Temperature (°C)
    float temperature;
    
    // Integrated heading (°)
    float heading;
    
    // Timestamp
    uint32_t timestamp_us;
    float dt;               // Delta time in seconds
} IMUData_t;

/**
 * @brief IMU calibration offsets
 */
typedef struct {
    float gyro_offset_x;
    float gyro_offset_y;
    float gyro_offset_z;
    float accel_offset_x;
    float accel_offset_y;
    float accel_offset_z;
    bool calibrated;
} IMUCalibration_t;

/**
 * @brief IMU state
 */
typedef struct {
    IMURawData_t raw;
    IMUData_t data;
    IMUCalibration_t calibration;
    GyroFullScale_t gyro_fs;
    AccelFullScale_t accel_fs;
    float gyro_scale;       // Current scale factor
    float accel_scale;      // Current scale factor
    bool initialized;
    uint32_t last_update_us;
    uint32_t read_count;
    uint32_t error_count;
} IMUState_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: STATE VARIABLES (C++17 inline)
// └───────────────────────────────────────────────────────────────────────────

inline IMUState_t _imu_state = {
    .raw = {0},
    .data = {0},
    .calibration = {
        .gyro_offset_x = GYRO_OFFSET_X,
        .gyro_offset_y = GYRO_OFFSET_Y,
        .gyro_offset_z = GYRO_OFFSET_Z,
        .accel_offset_x = ACCEL_OFFSET_X,
        .accel_offset_y = ACCEL_OFFSET_Y,
        .accel_offset_z = ACCEL_OFFSET_Z,
        .calibrated = false
    },
    .gyro_fs = GYRO_FS_250DPS,
    .accel_fs = ACCEL_FS_2G,
    .gyro_scale = 131.0f,
    .accel_scale = 16384.0f,
    .initialized = false,
    .last_update_us = 0,
    .read_count = 0,
    .error_count = 0
};

inline TwoWire* _imu_wire = &Wire;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: PRIVATE HELPER FUNCTIONS
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Write single byte to MPU6050 register
 */
inline bool _imu_write_reg(uint8_t reg, uint8_t value) {
    _imu_wire->beginTransmission(MPU6050_ADDR);
    _imu_wire->write(reg);
    _imu_wire->write(value);
    return (_imu_wire->endTransmission() == 0);
}

/**
 * @brief Read single byte from MPU6050 register
 */
inline uint8_t _imu_read_reg(uint8_t reg) {
    _imu_wire->beginTransmission(MPU6050_ADDR);
    _imu_wire->write(reg);
    _imu_wire->endTransmission(false);
    _imu_wire->requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)1);
    return _imu_wire->read();
}

/**
 * @brief Read multiple bytes from MPU6050
 */
inline bool _imu_read_bytes(uint8_t reg, uint8_t* buffer, uint8_t length) {
    _imu_wire->beginTransmission(MPU6050_ADDR);
    _imu_wire->write(reg);
    if (_imu_wire->endTransmission(false) != 0) {
        return false;
    }
    
    uint8_t received = _imu_wire->requestFrom((uint8_t)MPU6050_ADDR, length);
    if (received != length) {
        return false;
    }
    
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = _imu_wire->read();
    }
    return true;
}

/**
 * @brief Convert raw gyro to degrees per second
 */
inline float _imu_raw_to_dps(int16_t raw, float offset) {
    return (raw - offset) / _imu_state.gyro_scale;
}

/**
 * @brief Convert raw accel to g
 */
inline float _imu_raw_to_g(int16_t raw, float offset) {
    return (raw - offset) / _imu_state.accel_scale;
}

/**
 * @brief Convert raw temperature to Celsius
 * Formula: Temp(°C) = raw/340 + 36.53
 */
inline float _imu_raw_to_celsius(int16_t raw) {
    return (raw / 340.0f) + 36.53f;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PUBLIC FUNCTIONS - Initialization
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize IMU (MPU6050)
 * @param gyro_fs Gyroscope full scale range (default: ±250°/s)
 * @param accel_fs Accelerometer full scale range (default: ±2g)
 * @param dlpf Digital Low Pass Filter config (default: 44Hz)
 * @return true if successful
 */
inline bool imu_init(GyroFullScale_t gyro_fs = GYRO_FS_250DPS, 
                     AccelFullScale_t accel_fs = ACCEL_FS_2G,
                     DLPFConfig_t dlpf = DLPF_44HZ) {
    
    #if DEBUG_SERIAL
    Serial.println("[IMU] Initializing MPU6050...");
    #endif
    
    // Initialize I2C
    _imu_wire = &Wire;
    _imu_wire->begin(PIN_I2C0_SDA, PIN_I2C0_SCL, I2C0_FREQ);
    
    // Check WHO_AM_I register
    uint8_t whoami = _imu_read_reg(MPU6050_REG_WHO_AM_I);
    if (whoami != 0x68 && whoami != 0x72) {  // 0x72 for some clones
        #if DEBUG_SERIAL
        Serial.printf("[IMU] ERROR: WHO_AM_I = 0x%02X (expected 0x68)\n", whoami);
        #endif
        return false;
    }
    
    #if DEBUG_SERIAL
    Serial.printf("[IMU] WHO_AM_I = 0x%02X (OK)\n", whoami);
    #endif
    
    // Wake up MPU6050 (clear sleep bit, use PLL with Gyro X as clock source)
    if (!_imu_write_reg(MPU6050_REG_PWR_MGMT_1, 0x01)) {
        #if DEBUG_SERIAL
        Serial.println("[IMU] ERROR: Failed to wake up");
        #endif
        return false;
    }
    delay(100);  // Wait for PLL to stabilize
    
    // Configure sample rate divider (Sample Rate = 1kHz / (1 + divider))
    // For 1kHz sample rate, divider = 0
    _imu_write_reg(MPU6050_REG_SMPLRT_DIV, 0x00);
    
    // Configure DLPF
    _imu_write_reg(MPU6050_REG_CONFIG, dlpf);
    
    // Configure Gyroscope
    _imu_write_reg(MPU6050_REG_GYRO_CONFIG, gyro_fs << 3);
    _imu_state.gyro_fs = gyro_fs;
    _imu_state.gyro_scale = GYRO_SCALE_FACTOR[gyro_fs];
    
    // Configure Accelerometer
    _imu_write_reg(MPU6050_REG_ACCEL_CONFIG, accel_fs << 3);
    _imu_state.accel_fs = accel_fs;
    _imu_state.accel_scale = ACCEL_SCALE_FACTOR[accel_fs];
    
    // Initialize state
    _imu_state.initialized = true;
    _imu_state.last_update_us = micros();
    _imu_state.data.heading = 0;
    
    #if DEBUG_SERIAL
    Serial.printf("[IMU] Gyro: ±%d°/s, Accel: ±%dg, DLPF: %d\n",
                  250 << gyro_fs, 2 << accel_fs, dlpf);
    Serial.println("[IMU] Initialized successfully");
    #endif
    
    return true;
}

/**
 * @brief Initialize IMU with default settings
 */
inline bool imu_init_default(void) {
    return imu_init(GYRO_FS_250DPS, ACCEL_FS_2G, DLPF_44HZ);
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: PUBLIC FUNCTIONS - Data Reading
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Read all sensor data from MPU6050
 * @return true if successful
 * 
 * @note Call this at the beginning of each control loop iteration.
 *       Reads accel (6 bytes) + temp (2 bytes) + gyro (6 bytes) = 14 bytes
 */
inline bool imu_update(void) {
    if (!_imu_state.initialized) return false;
    
    // Read all 14 bytes in one transaction (faster than separate reads)
    uint8_t buffer[14];
    if (!_imu_read_bytes(MPU6050_REG_ACCEL_XOUT_H, buffer, 14)) {
        _imu_state.error_count++;
        #if DEBUG_IMU
        Serial.println("[IMU] Read error");
        #endif
        return false;
    }
    
    // Parse raw data (big-endian)
    _imu_state.raw.accel_x = (buffer[0] << 8) | buffer[1];
    _imu_state.raw.accel_y = (buffer[2] << 8) | buffer[3];
    _imu_state.raw.accel_z = (buffer[4] << 8) | buffer[5];
    _imu_state.raw.temp    = (buffer[6] << 8) | buffer[7];
    _imu_state.raw.gyro_x  = (buffer[8] << 8) | buffer[9];
    _imu_state.raw.gyro_y  = (buffer[10] << 8) | buffer[11];
    _imu_state.raw.gyro_z  = (buffer[12] << 8) | buffer[13];
    
    // Calculate delta time
    uint32_t now_us = micros();
    uint32_t dt_us = now_us - _imu_state.last_update_us;
    _imu_state.last_update_us = now_us;
    _imu_state.data.dt = dt_us / 1000000.0f;
    _imu_state.data.timestamp_us = now_us;
    
    // Limit dt to reasonable range (prevent huge jumps on first read)
    if (_imu_state.data.dt > 0.1f) _imu_state.data.dt = 0.001f;
    
    // Convert to physical units with calibration offset
    _imu_state.data.accel_x = _imu_raw_to_g(_imu_state.raw.accel_x, 
                                             _imu_state.calibration.accel_offset_x);
    _imu_state.data.accel_y = _imu_raw_to_g(_imu_state.raw.accel_y, 
                                             _imu_state.calibration.accel_offset_y);
    _imu_state.data.accel_z = _imu_raw_to_g(_imu_state.raw.accel_z, 
                                             _imu_state.calibration.accel_offset_z);
    
    _imu_state.data.gyro_x = _imu_raw_to_dps(_imu_state.raw.gyro_x, 
                                              _imu_state.calibration.gyro_offset_x);
    _imu_state.data.gyro_y = _imu_raw_to_dps(_imu_state.raw.gyro_y, 
                                              _imu_state.calibration.gyro_offset_y);
    _imu_state.data.gyro_z = _imu_raw_to_dps(_imu_state.raw.gyro_z, 
                                              _imu_state.calibration.gyro_offset_z);
    
    _imu_state.data.temperature = _imu_raw_to_celsius(_imu_state.raw.temp);
    
    // Integrate heading (yaw angle)
    // Note: This is simple integration, will drift over time
    _imu_state.data.heading += _imu_state.data.gyro_z * _imu_state.data.dt;
    
    // Normalize heading to -180 to +180
    while (_imu_state.data.heading > 180.0f) _imu_state.data.heading -= 360.0f;
    while (_imu_state.data.heading < -180.0f) _imu_state.data.heading += 360.0f;
    
    _imu_state.read_count++;
    
    #if DEBUG_IMU
    Serial.printf("[IMU] Gyro Z: %.2f°/s, Heading: %.1f°\n", 
                  _imu_state.data.gyro_z, _imu_state.data.heading);
    #endif
    
    return true;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: PUBLIC FUNCTIONS - Data Access
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Get processed IMU data
 */
inline IMUData_t imu_get_data(void) {
    return _imu_state.data;
}

/**
 * @brief Get raw IMU data
 */
inline IMURawData_t imu_get_raw(void) {
    return _imu_state.raw;
}

/**
 * @brief Get gyroscope Z (yaw rate) in degrees per second
 */
inline float imu_get_gyro_z(void) {
    return _imu_state.data.gyro_z;
}

/**
 * @brief Get all gyroscope values
 */
inline void imu_get_gyro(float* x, float* y, float* z) {
    if (x) *x = _imu_state.data.gyro_x;
    if (y) *y = _imu_state.data.gyro_y;
    if (z) *z = _imu_state.data.gyro_z;
}

/**
 * @brief Get all accelerometer values
 */
inline void imu_get_accel(float* x, float* y, float* z) {
    if (x) *x = _imu_state.data.accel_x;
    if (y) *y = _imu_state.data.accel_y;
    if (z) *z = _imu_state.data.accel_z;
}

/**
 * @brief Get integrated heading (yaw angle) in degrees
 */
inline float imu_get_heading(void) {
    return _imu_state.data.heading;
}

/**
 * @brief Get temperature in Celsius
 */
inline float imu_get_temperature(void) {
    return _imu_state.data.temperature;
}

/**
 * @brief Get delta time from last update
 */
inline float imu_get_dt(void) {
    return _imu_state.data.dt;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 9: PUBLIC FUNCTIONS - Calibration
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Reset heading to zero
 */
inline void imu_reset_heading(void) {
    _imu_state.data.heading = 0;
}

/**
 * @brief Set heading to specific value
 */
inline void imu_set_heading(float heading) {
    _imu_state.data.heading = heading;
}

/**
 * @brief Calibrate gyroscope (robot must be stationary)
 * @param samples Number of samples to average (default: 500)
 * @return true if successful
 * 
 * @note Keep robot completely still during calibration.
 *       Takes about 0.5-1 second with default samples.
 */
inline bool imu_calibrate_gyro(uint16_t samples = 500) {
    if (!_imu_state.initialized) return false;
    
    #if DEBUG_SERIAL
    Serial.println("[IMU] Calibrating gyroscope... Keep robot still!");
    #endif
    
    float sum_x = 0, sum_y = 0, sum_z = 0;
    
    for (uint16_t i = 0; i < samples; i++) {
        // Read raw gyro data
        uint8_t buffer[6];
        if (!_imu_read_bytes(MPU6050_REG_GYRO_XOUT_H, buffer, 6)) {
            #if DEBUG_SERIAL
            Serial.println("[IMU] Calibration read error");
            #endif
            return false;
        }
        
        int16_t gx = (buffer[0] << 8) | buffer[1];
        int16_t gy = (buffer[2] << 8) | buffer[3];
        int16_t gz = (buffer[4] << 8) | buffer[5];
        
        sum_x += gx;
        sum_y += gy;
        sum_z += gz;
        
        delay(1);  // 1ms between samples
    }
    
    // Calculate average offset (in raw units)
    _imu_state.calibration.gyro_offset_x = sum_x / samples;
    _imu_state.calibration.gyro_offset_y = sum_y / samples;
    _imu_state.calibration.gyro_offset_z = sum_z / samples;
    _imu_state.calibration.calibrated = true;
    
    #if DEBUG_SERIAL
    Serial.println("[IMU] Gyroscope calibration complete");
    Serial.printf("[IMU] Offsets: X=%.1f, Y=%.1f, Z=%.1f (raw)\n",
                  _imu_state.calibration.gyro_offset_x,
                  _imu_state.calibration.gyro_offset_y,
                  _imu_state.calibration.gyro_offset_z);
    #endif
    
    return true;
}

/**
 * @brief Calibrate accelerometer (robot must be on a level surface)
 * @param samples Number of samples to average (default: 500)
 */
inline bool imu_calibrate_accel(uint16_t samples = 500) {
    if (!_imu_state.initialized) return false;
    
    #if DEBUG_SERIAL
    Serial.println("[IMU] Calibrating accelerometer... Keep robot level and still!");
    #endif
    
    float sum_x = 0, sum_y = 0, sum_z = 0;
    
    for (uint16_t i = 0; i < samples; i++) {
        uint8_t buffer[6];
        if (!_imu_read_bytes(MPU6050_REG_ACCEL_XOUT_H, buffer, 6)) return false;
        
        int16_t ax = (buffer[0] << 8) | buffer[1];
        int16_t ay = (buffer[2] << 8) | buffer[3];
        int16_t az = (buffer[4] << 8) | buffer[5];
        
        sum_x += ax;
        sum_y += ay;
        // หักลบแรงโน้มถ่วงโลกออก (1g) ในแกน Z 
        // สำหรับช่วง +/- 2g ค่า 1g คือ 16384 LSB
        sum_z += (az - 16384); 
        
        delay(1);
    }
    
    _imu_state.calibration.accel_offset_x = sum_x / samples;
    _imu_state.calibration.accel_offset_y = sum_y / samples;
    _imu_state.calibration.accel_offset_z = sum_z / samples;
    
    return true;
}

/**
 * @brief Get current accelerometer calibration offsets
 */
inline void imu_get_accel_offset(float* offset_x, float* offset_y, float* offset_z) {
    if (offset_x) *offset_x = _imu_state.calibration.accel_offset_x;
    if (offset_y) *offset_y = _imu_state.calibration.accel_offset_y;
    if (offset_z) *offset_z = _imu_state.calibration.accel_offset_z;
}

/**
 * @brief Set gyroscope calibration offsets manually
 */
inline void imu_set_gyro_offset(float offset_x, float offset_y, float offset_z) {
    _imu_state.calibration.gyro_offset_x = offset_x;
    _imu_state.calibration.gyro_offset_y = offset_y;
    _imu_state.calibration.gyro_offset_z = offset_z;
    _imu_state.calibration.calibrated = true;
}

/**
 * @brief Get current gyroscope calibration offsets
 */
inline void imu_get_gyro_offset(float* offset_x, float* offset_y, float* offset_z) {
    if (offset_x) *offset_x = _imu_state.calibration.gyro_offset_x;
    if (offset_y) *offset_y = _imu_state.calibration.gyro_offset_y;
    if (offset_z) *offset_z = _imu_state.calibration.gyro_offset_z;
}

/**
 * @brief Check if IMU is calibrated
 */
inline bool imu_is_calibrated(void) {
    return _imu_state.calibration.calibrated;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 10: PUBLIC FUNCTIONS - Utility
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Check if IMU is initialized
 */
inline bool imu_is_initialized(void) {
    return _imu_state.initialized;
}

/**
 * @brief Get read count (for debugging)
 */
inline uint32_t imu_get_read_count(void) {
    return _imu_state.read_count;
}

/**
 * @brief Get error count (for debugging)
 */
inline uint32_t imu_get_error_count(void) {
    return _imu_state.error_count;
}

/**
 * @brief Print IMU status to Serial
 */
inline void imu_print_status(void) {
    #if DEBUG_SERIAL
    Serial.println("────────────────────────────────────────────────────────");
    Serial.println("                   IMU STATUS");
    Serial.println("────────────────────────────────────────────────────────");
    Serial.printf("Initialized:  %s\n", _imu_state.initialized ? "Yes" : "No");
    Serial.printf("Calibrated:   %s\n", _imu_state.calibration.calibrated ? "Yes" : "No");
    Serial.println();
    Serial.printf("Gyro X:       %.2f °/s\n", _imu_state.data.gyro_x);
    Serial.printf("Gyro Y:       %.2f °/s\n", _imu_state.data.gyro_y);
    Serial.printf("Gyro Z:       %.2f °/s\n", _imu_state.data.gyro_z);
    Serial.printf("Heading:      %.1f °\n", _imu_state.data.heading);
    Serial.println();
    Serial.printf("Accel X:      %.3f g\n", _imu_state.data.accel_x);
    Serial.printf("Accel Y:      %.3f g\n", _imu_state.data.accel_y);
    Serial.printf("Accel Z:      %.3f g\n", _imu_state.data.accel_z);
    Serial.println();
    Serial.printf("Temperature:  %.1f °C\n", _imu_state.data.temperature);
    Serial.printf("Delta Time:   %.3f ms\n", _imu_state.data.dt * 1000);
    Serial.printf("Read Count:   %lu\n", _imu_state.read_count);
    Serial.printf("Error Count:  %lu\n", _imu_state.error_count);
    Serial.println("────────────────────────────────────────────────────────");
    #endif
}

/**
 * @brief Get Wire instance (for I2C bus sharing)
 */
inline TwoWire* imu_get_wire(void) {
    return _imu_wire;
}

#endif // HAL_IMU_H
