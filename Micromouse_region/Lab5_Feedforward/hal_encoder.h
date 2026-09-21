// ═══════════════════════════════════════════════════════════════════════════
//  hal_encoder.h
//  Encoder HAL using ESP32-S3 PCNT (Pulse Counter)
//  
//  Layer: 1 (Hardware Abstraction)
//  Dependencies: config.h
//  
//  Hardware: N20 Motor with Quadrature Encoder
//  Control: ESP32-S3 PCNT (Hardware Pulse Counter)
//  
//  Features:
//    - Hardware-based counting (no CPU overhead)
//    - X4 quadrature decoding (4 counts per encoder cycle)
//    - Overflow handling with software extension
//    - Velocity calculation
//    - Distance calculation
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_ENCODER_H
#define HAL_ENCODER_H

#include <Arduino.h>
#include "config.h"
#include "driver/pulse_cnt.h"

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: CONFIGURATION
// └───────────────────────────────────────────────────────────────────────────

// PCNT configuration
#define ENCODER_PCNT_HIGH_LIMIT   10000      // High limit before overflow
#define ENCODER_PCNT_LOW_LIMIT    -10000     // Low limit before overflow
#define ENCODER_GLITCH_FILTER_NS  1000       // Glitch filter (1µs)

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: DATA TYPES
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Encoder identifier
 */
typedef enum {
    ENCODER_LEFT  = 0,
    ENCODER_RIGHT = 1
} EncoderID_t;

/**
 * @brief Single encoder data
 */
typedef struct {
    int32_t count;              // Total count (software extended)
    int32_t count_prev;         // Previous count (for velocity calculation)
    float velocity;             // Current velocity (counts per second)
    float velocity_filtered;    // Filtered velocity (low-pass)
    float distance_mm;          // Total distance traveled (mm)
    int16_t overflow_count;     // Number of overflows
} EncoderChannel_t;

/**
 * @brief Both encoders data
 */
typedef struct {
    EncoderChannel_t left;
    EncoderChannel_t right;
    uint32_t last_update_us;    // Timestamp of last update
    float dt;                   // Delta time (seconds)
    bool initialized;
} EncoderData_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: STATE VARIABLES (C++17 inline = True Singleton)
// └───────────────────────────────────────────────────────────────────────────

inline EncoderData_t _encoder_data = {
    .left = {0},
    .right = {0},
    .last_update_us = 0,
    .dt = 0,
    .initialized = false
};

// PCNT unit handles (inline for single instance)
inline pcnt_unit_handle_t _pcnt_unit_left = NULL;
inline pcnt_unit_handle_t _pcnt_unit_right = NULL;
inline pcnt_channel_handle_t _pcnt_chan_left_a = NULL;
inline pcnt_channel_handle_t _pcnt_chan_left_b = NULL;
inline pcnt_channel_handle_t _pcnt_chan_right_a = NULL;
inline pcnt_channel_handle_t _pcnt_chan_right_b = NULL;

// Velocity filter coefficient (0-1, lower = more filtering)
inline float _velocity_filter_alpha = 0.3f;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: PRIVATE HELPER FUNCTIONS
// │ Note: ISR callbacks ต้องใช้ static เพราะ IRAM_ATTR ไม่รองรับ inline
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Overflow callback for left encoder
 * @note ใช้ static เพราะ IRAM_ATTR ต้องการ fixed memory location
 */
static bool IRAM_ATTR _encoder_overflow_cb_left(pcnt_unit_handle_t unit, 
                                                  const pcnt_watch_event_data_t *edata, 
                                                  void *user_ctx) {
    if (edata->watch_point_value == ENCODER_PCNT_HIGH_LIMIT) {
        _encoder_data.left.overflow_count++;
    } else if (edata->watch_point_value == ENCODER_PCNT_LOW_LIMIT) {
        _encoder_data.left.overflow_count--;
    }
    return false;  // No need to yield
}

/**
 * @brief Overflow callback for right encoder
 * @note ใช้ static เพราะ IRAM_ATTR ต้องการ fixed memory location
 */
static bool IRAM_ATTR _encoder_overflow_cb_right(pcnt_unit_handle_t unit, 
                                                   const pcnt_watch_event_data_t *edata, 
                                                   void *user_ctx) {
    if (edata->watch_point_value == ENCODER_PCNT_HIGH_LIMIT) {
        _encoder_data.right.overflow_count++;
    } else if (edata->watch_point_value == ENCODER_PCNT_LOW_LIMIT) {
        _encoder_data.right.overflow_count--;
    }
    return false;
}

/**
 * @brief Configure PCNT unit for quadrature encoding
 */
inline esp_err_t _encoder_config_pcnt_unit(pcnt_unit_handle_t *unit,
                                            pcnt_channel_handle_t *chan_a,
                                            pcnt_channel_handle_t *chan_b,
                                            int gpio_a, int gpio_b,
                                            pcnt_watch_cb_t overflow_cb) {
    esp_err_t ret;
    
    // Create PCNT unit
    // Note: ลำดับต้องตรงกับ ESP-IDF: low_limit, high_limit, flags
    pcnt_unit_config_t unit_config = {
        .low_limit = ENCODER_PCNT_LOW_LIMIT,
        .high_limit = ENCODER_PCNT_HIGH_LIMIT,
        .flags = {
            .accum_count = true,  // Enable accumulator
        }
    };
    
    ret = pcnt_new_unit(&unit_config, unit);
    if (ret != ESP_OK) return ret;
    
    // Set glitch filter
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = ENCODER_GLITCH_FILTER_NS,
    };
    pcnt_unit_set_glitch_filter(*unit, &filter_config);
    
    // Create channel A (counts on A edges, direction from B)
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = gpio_a,
        .level_gpio_num = gpio_b,
    };
    ret = pcnt_new_channel(*unit, &chan_a_config, chan_a);
    if (ret != ESP_OK) return ret;
    
    // Create channel B (counts on B edges, direction from A)
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = gpio_b,
        .level_gpio_num = gpio_a,
    };
    ret = pcnt_new_channel(*unit, &chan_b_config, chan_b);
    if (ret != ESP_OK) return ret;
    
    // Configure channel A for X4 quadrature
    // Rising edge of A: if B is low, count up; if B is high, count down
    pcnt_channel_set_edge_action(*chan_a, 
                                  PCNT_CHANNEL_EDGE_ACTION_DECREASE,   // Rising edge, B low
                                  PCNT_CHANNEL_EDGE_ACTION_INCREASE);  // Falling edge, B low
    pcnt_channel_set_level_action(*chan_a,
                                   PCNT_CHANNEL_LEVEL_ACTION_KEEP,      // B low: keep direction
                                   PCNT_CHANNEL_LEVEL_ACTION_INVERSE);  // B high: invert direction
    
    // Configure channel B for X4 quadrature
    pcnt_channel_set_edge_action(*chan_b,
                                  PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                  PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(*chan_b,
                                   PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                   PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    // Add watch points for overflow handling
    pcnt_unit_add_watch_point(*unit, ENCODER_PCNT_HIGH_LIMIT);
    pcnt_unit_add_watch_point(*unit, ENCODER_PCNT_LOW_LIMIT);
    
    // Register overflow callback
    pcnt_event_callbacks_t cbs = {
        .on_reach = overflow_cb,
    };
    pcnt_unit_register_event_callbacks(*unit, &cbs, NULL);
    
    // Enable and start
    pcnt_unit_enable(*unit);
    pcnt_unit_clear_count(*unit);
    pcnt_unit_start(*unit);
    
    return ESP_OK;
}

/**
 * @brief Read extended count (with overflow handling)
 */
inline int32_t _encoder_read_extended_count(pcnt_unit_handle_t unit, int16_t overflow_count) {
    int count;
    pcnt_unit_get_count(unit, &count);
    
    // Combine PCNT count with overflow count
    (void)overflow_count;  // accum_count=true → overflows already accumulated in get_count()
    return count;          // was overflow_count*(HIGH-LOW)+count → double-counted (~2500mm jumps)
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: PUBLIC FUNCTIONS - Initialization
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize encoder HAL
 * @return true if successful, false otherwise
 * 
 * @note This initializes the PCNT peripheral for quadrature decoding.
 *       X4 mode counts 4 edges per encoder cycle for maximum resolution.
 * 
 * @example
 *   if (!encoder_init()) {
 *       Serial.println("Encoder init failed!");
 *       while(1);
 *   }
 */
bool encoder_init(void) {
    esp_err_t ret;
    
    #if DEBUG_SERIAL
    Serial.println("[ENCODER] Initializing...");
    #endif
    
    // Initialize left encoder
    ret = _encoder_config_pcnt_unit(&_pcnt_unit_left,
                                     &_pcnt_chan_left_a,
                                     &_pcnt_chan_left_b,
                                     PIN_ENC_L_A,
                                     PIN_ENC_L_B,
                                     _encoder_overflow_cb_left);
    if (ret != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[ENCODER] Left encoder init failed: %d\n", ret);
        #endif
        return false;
    }
    
    // Initialize right encoder
    ret = _encoder_config_pcnt_unit(&_pcnt_unit_right,
                                     &_pcnt_chan_right_a,
                                     &_pcnt_chan_right_b,
                                     PIN_ENC_R_A,
                                     PIN_ENC_R_B,
                                     _encoder_overflow_cb_right);
    if (ret != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[ENCODER] Right encoder init failed: %d\n", ret);
        #endif
        return false;
    }
    
    // Initialize data
    memset(&_encoder_data.left, 0, sizeof(EncoderChannel_t));
    memset(&_encoder_data.right, 0, sizeof(EncoderChannel_t));
    _encoder_data.last_update_us = micros();
    _encoder_data.initialized = true;
    
    #if DEBUG_SERIAL
    Serial.println("[ENCODER] Initialized successfully");
    Serial.printf("[ENCODER] Counts per rev: %d\n", COUNTS_PER_REV);
    Serial.printf("[ENCODER] mm per count: %.4f\n", MM_PER_COUNT);
    #endif
    
    return true;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PUBLIC FUNCTIONS - Update & Read
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Update encoder readings
 * 
 * @note Call this at the beginning of each control loop iteration.
 *       It reads counts, calculates velocity, and updates distance.
 */
void encoder_update(void) {
    if (!_encoder_data.initialized) return;
    
    // Calculate delta time
    uint32_t now_us = micros();
    uint32_t dt_us = now_us - _encoder_data.last_update_us;
    _encoder_data.last_update_us = now_us;
    _encoder_data.dt = dt_us / 1000000.0f;  // Convert to seconds
    
    // Avoid division by zero
    if (_encoder_data.dt < 0.0001f) _encoder_data.dt = 0.001f;
    
    // Store previous counts
    _encoder_data.left.count_prev = _encoder_data.left.count;
    _encoder_data.right.count_prev = _encoder_data.right.count;
    
    // Read current counts
    _encoder_data.left.count = _encoder_read_extended_count(_pcnt_unit_left, 
                                                             _encoder_data.left.overflow_count);
    _encoder_data.right.count = _encoder_read_extended_count(_pcnt_unit_right, 
                                                              _encoder_data.right.overflow_count);
    
    // Calculate velocity (counts per second)
    int32_t delta_left = _encoder_data.left.count - _encoder_data.left.count_prev;
    int32_t delta_right = _encoder_data.right.count - _encoder_data.right.count_prev;
    
    _encoder_data.left.velocity = delta_left / _encoder_data.dt;
    _encoder_data.right.velocity = delta_right / _encoder_data.dt;
    
    // Apply low-pass filter to velocity
    _encoder_data.left.velocity_filtered = _velocity_filter_alpha * _encoder_data.left.velocity +
                                            (1.0f - _velocity_filter_alpha) * _encoder_data.left.velocity_filtered;
    _encoder_data.right.velocity_filtered = _velocity_filter_alpha * _encoder_data.right.velocity +
                                             (1.0f - _velocity_filter_alpha) * _encoder_data.right.velocity_filtered;
    
    // Calculate distance
    _encoder_data.left.distance_mm = _encoder_data.left.count * MM_PER_COUNT;
    _encoder_data.right.distance_mm = _encoder_data.right.count * MM_PER_COUNT;
    
    #if DEBUG_ENCODERS
    Serial.printf("[ENC] L:%6d R:%6d | Vel L:%.1f R:%.1f\n",
                  _encoder_data.left.count, _encoder_data.right.count,
                  _encoder_data.left.velocity, _encoder_data.right.velocity);
    #endif
}

/**
 * @brief Get encoder count
 * @param encoder ENCODER_LEFT or ENCODER_RIGHT
 * @return Current count value
 */
int32_t encoder_get_count(EncoderID_t encoder) {
    if (encoder == ENCODER_LEFT) {
        return _encoder_data.left.count;
    } else {
        return _encoder_data.right.count;
    }
}

/**
 * @brief Get both encoder counts
 * @param left  Pointer to store left count
 * @param right Pointer to store right count
 */
void encoder_get_counts(int32_t* left, int32_t* right) {
    if (left != NULL) *left = _encoder_data.left.count;
    if (right != NULL) *right = _encoder_data.right.count;
}

/**
 * @brief Get encoder velocity (counts per second)
 * @param encoder ENCODER_LEFT or ENCODER_RIGHT
 * @param filtered true for filtered value, false for raw
 * @return Velocity in counts per second
 */
float encoder_get_velocity(EncoderID_t encoder, bool filtered) {
    if (encoder == ENCODER_LEFT) {
        return filtered ? _encoder_data.left.velocity_filtered : _encoder_data.left.velocity;
    } else {
        return filtered ? _encoder_data.right.velocity_filtered : _encoder_data.right.velocity;
    }
}

/**
 * @brief Get encoder velocity in mm/s
 * @param encoder ENCODER_LEFT or ENCODER_RIGHT
 * @param filtered true for filtered value
 * @return Velocity in mm/s
 */
float encoder_get_velocity_mm(EncoderID_t encoder, bool filtered) {
    return encoder_get_velocity(encoder, filtered) * MM_PER_COUNT;
}

/**
 * @brief Get both encoder velocities in mm/s
 * @param left  Pointer to store left velocity
 * @param right Pointer to store right velocity
 * @param filtered true for filtered values
 */
void encoder_get_velocities_mm(float* left, float* right, bool filtered) {
    if (left != NULL) *left = encoder_get_velocity_mm(ENCODER_LEFT, filtered);
    if (right != NULL) *right = encoder_get_velocity_mm(ENCODER_RIGHT, filtered);
}

/**
 * @brief Get encoder distance in mm
 * @param encoder ENCODER_LEFT or ENCODER_RIGHT
 * @return Distance in mm
 */
float encoder_get_distance_mm(EncoderID_t encoder) {
    if (encoder == ENCODER_LEFT) {
        return _encoder_data.left.distance_mm;
    } else {
        return _encoder_data.right.distance_mm;
    }
}

/**
 * @brief Get both encoder distances
 * @param left  Pointer to store left distance
 * @param right Pointer to store right distance
 */
void encoder_get_distances_mm(float* left, float* right) {
    if (left != NULL) *left = _encoder_data.left.distance_mm;
    if (right != NULL) *right = _encoder_data.right.distance_mm;
}

/**
 * @brief Get average distance (robot travel distance)
 * @return Average distance in mm
 */
float encoder_get_average_distance_mm(void) {
    return (_encoder_data.left.distance_mm + _encoder_data.right.distance_mm) / 2.0f;
}

/**
 * @brief Get average velocity
 * @param filtered true for filtered value
 * @return Average velocity in mm/s
 */
float encoder_get_average_velocity_mm(bool filtered) {
    float left = encoder_get_velocity_mm(ENCODER_LEFT, filtered);
    float right = encoder_get_velocity_mm(ENCODER_RIGHT, filtered);
    return (left + right) / 2.0f;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: PUBLIC FUNCTIONS - Reset & Config
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Reset encoder counts to zero
 * @param encoder ENCODER_LEFT, ENCODER_RIGHT, or -1 for both
 */
void encoder_reset(int encoder) {
    if (encoder == ENCODER_LEFT || encoder < 0) {
        pcnt_unit_clear_count(_pcnt_unit_left);
        _encoder_data.left.count = 0;
        _encoder_data.left.count_prev = 0;
        _encoder_data.left.distance_mm = 0;
        _encoder_data.left.overflow_count = 0;
        _encoder_data.left.velocity = 0;
        _encoder_data.left.velocity_filtered = 0;
    }
    
    if (encoder == ENCODER_RIGHT || encoder < 0) {
        pcnt_unit_clear_count(_pcnt_unit_right);
        _encoder_data.right.count = 0;
        _encoder_data.right.count_prev = 0;
        _encoder_data.right.distance_mm = 0;
        _encoder_data.right.overflow_count = 0;
        _encoder_data.right.velocity = 0;
        _encoder_data.right.velocity_filtered = 0;
    }
    
    #if DEBUG_ENCODERS
    Serial.println("[ENCODER] Reset");
    #endif
}

/**
 * @brief Set velocity filter coefficient
 * @param alpha Filter coefficient (0-1, lower = more filtering)
 */
void encoder_set_filter(float alpha) {
    _velocity_filter_alpha = CONSTRAIN(alpha, 0.0f, 1.0f);
}

/**
 * @brief Get encoder data structure
 * @return EncoderData_t with all encoder information
 */
EncoderData_t encoder_get_data(void) {
    return _encoder_data;
}

/**
 * @brief Get delta time from last update
 * @return Delta time in seconds
 */
float encoder_get_dt(void) {
    return _encoder_data.dt;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: PUBLIC FUNCTIONS - Utility
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Convert counts to distance in mm
 * @param counts Encoder counts
 * @return Distance in mm
 */
float encoder_counts_to_mm(int32_t counts) {
    return counts * MM_PER_COUNT;
}

/**
 * @brief Convert distance in mm to counts
 * @param mm Distance in mm
 * @return Encoder counts
 */
int32_t encoder_mm_to_counts(float mm) {
    return (int32_t)(mm / MM_PER_COUNT);
}

/**
 * @brief Convert counts to wheel revolutions
 * @param counts Encoder counts
 * @return Number of revolutions
 */
float encoder_counts_to_revolutions(int32_t counts) {
    return (float)counts / COUNTS_PER_REV;
}

/**
 * @brief Calculate angular velocity difference (for rotation rate)
 * @param filtered true for filtered values
 * @return Difference in mm/s (positive = turning right)
 */
float encoder_get_velocity_difference(bool filtered) {
    float left = encoder_get_velocity_mm(ENCODER_LEFT, filtered);
    float right = encoder_get_velocity_mm(ENCODER_RIGHT, filtered);
    return right - left;
}

/**
 * @brief Print encoder status to Serial
 */
void encoder_print_status(void) {
    #if DEBUG_SERIAL
    Serial.println("────────────────────────────────────────────────────");
    Serial.println("              ENCODER STATUS");
    Serial.println("────────────────────────────────────────────────────");
    Serial.printf("Left Count:     %d\n", _encoder_data.left.count);
    Serial.printf("Right Count:    %d\n", _encoder_data.right.count);
    Serial.printf("Left Velocity:  %.1f counts/s (%.1f mm/s)\n", 
                  _encoder_data.left.velocity_filtered,
                  _encoder_data.left.velocity_filtered * MM_PER_COUNT);
    Serial.printf("Right Velocity: %.1f counts/s (%.1f mm/s)\n",
                  _encoder_data.right.velocity_filtered,
                  _encoder_data.right.velocity_filtered * MM_PER_COUNT);
    Serial.printf("Left Distance:  %.1f mm (%.2f rev)\n",
                  _encoder_data.left.distance_mm,
                  encoder_counts_to_revolutions(_encoder_data.left.count));
    Serial.printf("Right Distance: %.1f mm (%.2f rev)\n",
                  _encoder_data.right.distance_mm,
                  encoder_counts_to_revolutions(_encoder_data.right.count));
    Serial.printf("Delta Time:     %.3f ms\n", _encoder_data.dt * 1000);
    Serial.println("────────────────────────────────────────────────────");
    #endif
}

#endif // HAL_ENCODER_H
