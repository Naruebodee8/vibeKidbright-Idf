// ═══════════════════════════════════════════════════════════════════════════
//  hal_encoder.h (FIXED VELOCITY CALCULATION)
//  Fixes: Added time gating to prevent zero-velocity issues on fast loops
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_ENCODER_H
#define HAL_ENCODER_H

#include <Arduino.h>
#include "config.h"
#include "driver/pulse_cnt.h"

// ---------------------------------------------------------------------------
// SECTION 1: CONFIGURATION
// ---------------------------------------------------------------------------
#define ENCODER_PCNT_HIGH_LIMIT   10000      
#define ENCODER_PCNT_LOW_LIMIT    -10000     
#define ENCODER_GLITCH_FILTER_NS  1000       
#define VELOCITY_UPDATE_INTERVAL_US 10000    // ★ FIX: Update velocity every 10ms (100Hz)

// ---------------------------------------------------------------------------
// SECTION 2: DATA TYPES
// ---------------------------------------------------------------------------
typedef enum { ENCODER_LEFT = 0, ENCODER_RIGHT = 1 } EncoderID_t;

typedef struct {
    int32_t count;              
    int32_t count_prev;         
    float velocity;             
    float velocity_filtered;    
    float distance_mm;          
    int16_t overflow_count;     
} EncoderChannel_t;

typedef struct {
    EncoderChannel_t left;
    EncoderChannel_t right;
    uint32_t last_update_us;    
    float dt;                   
    bool initialized;
} EncoderData_t;

// ---------------------------------------------------------------------------
// SECTION 3: STATE VARIABLES
// ---------------------------------------------------------------------------
inline EncoderData_t _encoder_data = { {0}, {0}, 0, 0, false };

inline pcnt_unit_handle_t _pcnt_unit_left = NULL;
inline pcnt_unit_handle_t _pcnt_unit_right = NULL;
inline pcnt_channel_handle_t _pcnt_chan_left_a = NULL;
inline pcnt_channel_handle_t _pcnt_chan_left_b = NULL;
inline pcnt_channel_handle_t _pcnt_chan_right_a = NULL;
inline pcnt_channel_handle_t _pcnt_chan_right_b = NULL;

inline float _velocity_filter_alpha = 0.5f; // Increased filter responsiveness

// ---------------------------------------------------------------------------
// SECTION 4: PRIVATE HELPER FUNCTIONS
// ---------------------------------------------------------------------------
static bool IRAM_ATTR _encoder_overflow_cb_left(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx) {
    if (edata->watch_point_value == ENCODER_PCNT_HIGH_LIMIT) _encoder_data.left.overflow_count++;
    else if (edata->watch_point_value == ENCODER_PCNT_LOW_LIMIT) _encoder_data.left.overflow_count--;
    return false;
}

static bool IRAM_ATTR _encoder_overflow_cb_right(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx) {
    if (edata->watch_point_value == ENCODER_PCNT_HIGH_LIMIT) _encoder_data.right.overflow_count++;
    else if (edata->watch_point_value == ENCODER_PCNT_LOW_LIMIT) _encoder_data.right.overflow_count--;
    return false;
}

inline esp_err_t _encoder_config_pcnt_unit(pcnt_unit_handle_t *unit, pcnt_channel_handle_t *chan_a, pcnt_channel_handle_t *chan_b, int gpio_a, int gpio_b, pcnt_watch_cb_t overflow_cb) {
    esp_err_t ret;
    pcnt_unit_config_t unit_config = { .low_limit = ENCODER_PCNT_LOW_LIMIT, .high_limit = ENCODER_PCNT_HIGH_LIMIT, .flags = { .accum_count = true } };
    ret = pcnt_new_unit(&unit_config, unit); if (ret != ESP_OK) return ret;
    
    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = ENCODER_GLITCH_FILTER_NS };
    pcnt_unit_set_glitch_filter(*unit, &filter_config);
    
    pcnt_chan_config_t chan_a_config = { .edge_gpio_num = gpio_a, .level_gpio_num = gpio_b };
    ret = pcnt_new_channel(*unit, &chan_a_config, chan_a); if (ret != ESP_OK) return ret;
    
    pcnt_chan_config_t chan_b_config = { .edge_gpio_num = gpio_b, .level_gpio_num = gpio_a };
    ret = pcnt_new_channel(*unit, &chan_b_config, chan_b); if (ret != ESP_OK) return ret;
    
    pcnt_channel_set_edge_action(*chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(*chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(*chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(*chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    pcnt_unit_add_watch_point(*unit, ENCODER_PCNT_HIGH_LIMIT);
    pcnt_unit_add_watch_point(*unit, ENCODER_PCNT_LOW_LIMIT);
    pcnt_event_callbacks_t cbs = { .on_reach = overflow_cb };
    pcnt_unit_register_event_callbacks(*unit, &cbs, NULL);
    
    pcnt_unit_enable(*unit);
    pcnt_unit_clear_count(*unit);
    pcnt_unit_start(*unit);
    return ESP_OK;
}

inline int32_t _encoder_read_extended_count(pcnt_unit_handle_t unit, int16_t overflow_count) {
    (void)overflow_count;   // unused: accum_count=true accumulates overflows inside get_count()
    int count;
    pcnt_unit_get_count(unit, &count);
    // accum_count=true → get_count() already returns the FULL accumulated value.
    // The old "+ overflow_count*(HIGH-LOW)" double-counted every overflow and turned
    // a single spurious overflow event into a ~2500mm jump. Return the value as-is.
    return count;
}

// ---------------------------------------------------------------------------
// SECTION 5: PUBLIC FUNCTIONS
// ---------------------------------------------------------------------------

inline bool encoder_init(void) {
    esp_err_t ret;
    ret = _encoder_config_pcnt_unit(&_pcnt_unit_left, &_pcnt_chan_left_a, &_pcnt_chan_left_b, PIN_ENC_L_A, PIN_ENC_L_B, _encoder_overflow_cb_left);
    if (ret != ESP_OK) return false;
    ret = _encoder_config_pcnt_unit(&_pcnt_unit_right, &_pcnt_chan_right_a, &_pcnt_chan_right_b, PIN_ENC_R_A, PIN_ENC_R_B, _encoder_overflow_cb_right);
    if (ret != ESP_OK) return false;
    
    memset(&_encoder_data.left, 0, sizeof(EncoderChannel_t));
    memset(&_encoder_data.right, 0, sizeof(EncoderChannel_t));
    _encoder_data.last_update_us = micros();
    _encoder_data.initialized = true;
    return true;
}

inline void encoder_update(void) {
    if (!_encoder_data.initialized) return;
    
    // Always update raw counts (for distance)
    _encoder_data.left.count = _encoder_read_extended_count(_pcnt_unit_left, _encoder_data.left.overflow_count);
    _encoder_data.right.count = _encoder_read_extended_count(_pcnt_unit_right, _encoder_data.right.overflow_count);
    
    _encoder_data.left.distance_mm = _encoder_data.left.count * MM_PER_COUNT;
    _encoder_data.right.distance_mm = _encoder_data.right.count * MM_PER_COUNT;

    // ★ FIX: Update velocity ONLY if enough time has passed (10ms)
    uint32_t now_us = micros();
    if (now_us - _encoder_data.last_update_us >= VELOCITY_UPDATE_INTERVAL_US) {
        float dt = (now_us - _encoder_data.last_update_us) / 1000000.0f;
        _encoder_data.last_update_us = now_us;
        _encoder_data.dt = dt;

        int32_t delta_l = _encoder_data.left.count - _encoder_data.left.count_prev;
        int32_t delta_r = _encoder_data.right.count - _encoder_data.right.count_prev;
        
        _encoder_data.left.count_prev = _encoder_data.left.count;
        _encoder_data.right.count_prev = _encoder_data.right.count;
        
        float raw_vel_l = delta_l / dt;
        float raw_vel_r = delta_r / dt;

        // Reject implausible spikes (>2000 mm/s = ~15500 counts/s for this robot)
        const float MAX_VEL_COUNTS = 2000.0f / MM_PER_COUNT;
        if (fabsf(raw_vel_l) > MAX_VEL_COUNTS) raw_vel_l = _encoder_data.left.velocity;
        if (fabsf(raw_vel_r) > MAX_VEL_COUNTS) raw_vel_r = _encoder_data.right.velocity;

        // Apply filter
        _encoder_data.left.velocity = raw_vel_l;
        _encoder_data.left.velocity_filtered = _velocity_filter_alpha * raw_vel_l + (1.0f - _velocity_filter_alpha) * _encoder_data.left.velocity_filtered;

        _encoder_data.right.velocity = raw_vel_r;
        _encoder_data.right.velocity_filtered = _velocity_filter_alpha * raw_vel_r + (1.0f - _velocity_filter_alpha) * _encoder_data.right.velocity_filtered;
    }
}

// Getters
inline int32_t encoder_get_count(EncoderID_t encoder) {
    return (encoder == ENCODER_LEFT) ? _encoder_data.left.count : _encoder_data.right.count;
}

inline float encoder_get_velocity(EncoderID_t encoder, bool filtered) {
    if (encoder == ENCODER_LEFT) return filtered ? _encoder_data.left.velocity_filtered : _encoder_data.left.velocity;
    return filtered ? _encoder_data.right.velocity_filtered : _encoder_data.right.velocity;
}

inline float encoder_get_velocity_mm(EncoderID_t encoder, bool filtered) {
    return encoder_get_velocity(encoder, filtered) * MM_PER_COUNT;
}

inline float encoder_get_distance_mm(EncoderID_t encoder) {
    return (encoder == ENCODER_LEFT) ? _encoder_data.left.distance_mm : _encoder_data.right.distance_mm;
}

// Control
inline void encoder_reset(int encoder) {
    if (encoder == ENCODER_LEFT || encoder < 0) {
        pcnt_unit_clear_count(_pcnt_unit_left);
        _encoder_data.left.count = 0;
        _encoder_data.left.count_prev = 0;
        _encoder_data.left.overflow_count = 0;
        _encoder_data.left.distance_mm = 0;
        _encoder_data.left.velocity = 0;
        _encoder_data.left.velocity_filtered = 0;
    }
    if (encoder == ENCODER_RIGHT || encoder < 0) {
        pcnt_unit_clear_count(_pcnt_unit_right);
        _encoder_data.right.count = 0;
        _encoder_data.right.count_prev = 0;
        _encoder_data.right.overflow_count = 0;
        _encoder_data.right.distance_mm = 0;
        _encoder_data.right.velocity = 0;
        _encoder_data.right.velocity_filtered = 0;
    }
    // Sync velocity timer so first update after reset uses a fresh dt
    _encoder_data.last_update_us = micros();
}

#endif // HAL_ENCODER_H