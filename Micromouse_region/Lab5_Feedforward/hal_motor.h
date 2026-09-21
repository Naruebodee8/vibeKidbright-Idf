// ═══════════════════════════════════════════════════════════════════════════
//  hal_motor.h
//  Motor Control HAL using ESP32-S3 MCPWM
//  
//  Layer: 1 (Hardware Abstraction)
//  Dependencies: config.h
//  
//  Hardware: DRV8833 Dual H-Bridge Driver
//  Control: ESP32-S3 MCPWM (Motor Control PWM)
//  
//  Features:
//    - Dual motor control (left/right)
//    - PWM frequency: 20 kHz (ultrasonic, no audible noise)
//    - Resolution: 10-bit (0-1023)
//    - Forward/Backward/Brake/Coast modes
//    - Battery voltage compensation (optional)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_MOTOR_H
#define HAL_MOTOR_H

#include <Arduino.h>
#include "config.h"
#include "driver/mcpwm_prelude.h"

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: DATA TYPES
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Motor identifier
 */
typedef enum {
    MOTOR_LEFT  = 0,
    MOTOR_RIGHT = 1,
    MOTOR_BOTH  = 2
} MotorID_t;

/**
 * @brief Motor state
 */
typedef enum {
    MOTOR_STATE_COAST,      // ปล่อยให้หมุนอิสระ (IN1=LOW, IN2=LOW)
    MOTOR_STATE_BRAKE,      // เบรค (IN1=HIGH, IN2=HIGH)
    MOTOR_STATE_FORWARD,    // หมุนไปข้างหน้า
    MOTOR_STATE_BACKWARD    // หมุนถอยหลัง
} MotorState_t;

/**
 * @brief Motor data structure
 */
typedef struct {
    int16_t pwm_left;           // PWM value (-1023 to +1023)
    int16_t pwm_right;          // PWM value (-1023 to +1023)
    MotorState_t state_left;    // Current state
    MotorState_t state_right;   // Current state
    bool enabled;               // Motors enabled flag
    float voltage_scale;        // Battery voltage compensation factor
} MotorData_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: STATE VARIABLES (C++17 inline = True Singleton)
// └───────────────────────────────────────────────────────────────────────────

inline MotorData_t _motor_data = {
    .pwm_left = 0,
    .pwm_right = 0,
    .state_left = MOTOR_STATE_COAST,
    .state_right = MOTOR_STATE_COAST,
    .enabled = false,
    .voltage_scale = 1.0f
};

// MCPWM handles (inline for single instance across all translation units)
inline mcpwm_timer_handle_t _mcpwm_timer = NULL;
inline mcpwm_oper_handle_t _mcpwm_oper_left = NULL;
inline mcpwm_oper_handle_t _mcpwm_oper_right = NULL;
inline mcpwm_cmpr_handle_t _mcpwm_cmp_l1 = NULL;
inline mcpwm_cmpr_handle_t _mcpwm_cmp_l2 = NULL;
inline mcpwm_cmpr_handle_t _mcpwm_cmp_r1 = NULL;
inline mcpwm_cmpr_handle_t _mcpwm_cmp_r2 = NULL;
inline mcpwm_gen_handle_t _mcpwm_gen_l1 = NULL;
inline mcpwm_gen_handle_t _mcpwm_gen_l2 = NULL;
inline mcpwm_gen_handle_t _mcpwm_gen_r1 = NULL;
inline mcpwm_gen_handle_t _mcpwm_gen_r2 = NULL;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: PRIVATE HELPER FUNCTIONS (inline)
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Configure generator actions for PWM output
 */
inline void _motor_config_generator(mcpwm_gen_handle_t gen, mcpwm_cmpr_handle_t cmp) {
    // Set high on timer empty (start of period)
    mcpwm_generator_set_action_on_timer_event(gen,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    
    // Set low on compare threshold
    mcpwm_generator_set_action_on_compare_event(gen,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, cmp, MCPWM_GEN_ACTION_LOW));
}

/**
 * @brief Apply PWM to single motor
 */
inline void _motor_apply_pwm(MotorID_t motor, int16_t pwm) {
    // Constrain PWM value
    pwm = CONSTRAIN(pwm, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
    
    // Apply direction compensation
    if (motor == MOTOR_LEFT) {
        pwm *= MOTOR_L_DIRECTION;
    } else if (motor == MOTOR_RIGHT) {
        pwm *= MOTOR_R_DIRECTION;
    }
    
    // NOTE: battery-voltage compensation lives in motors_controller.h (PWM = volts/Vbat).
    // We do NOT multiply by voltage_scale here — that would double-compensate.
    int16_t scaled_pwm = pwm;
    scaled_pwm = CONSTRAIN(scaled_pwm, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
    
    // Calculate duty cycles
    uint32_t duty_in1 = 0;
    uint32_t duty_in2 = 0;
    MotorState_t state = MOTOR_STATE_COAST;
    
    if (scaled_pwm > 0) {
        // Forward: IN1=PWM, IN2=0
        duty_in1 = (uint32_t)scaled_pwm;
        duty_in2 = 0;
        state = MOTOR_STATE_FORWARD;
    } else if (scaled_pwm < 0) {
        // Backward: IN1=0, IN2=PWM
        duty_in1 = 0;
        duty_in2 = (uint32_t)(-scaled_pwm);
        state = MOTOR_STATE_BACKWARD;
    } else {
        // Coast: IN1=0, IN2=0
        duty_in1 = 0;
        duty_in2 = 0;
        state = MOTOR_STATE_COAST;
    }
    
    // Apply to hardware
    if (motor == MOTOR_LEFT) {
        mcpwm_comparator_set_compare_value(_mcpwm_cmp_l1, duty_in1);
        mcpwm_comparator_set_compare_value(_mcpwm_cmp_l2, duty_in2);
        _motor_data.pwm_left = pwm;
        _motor_data.state_left = state;
    } else if (motor == MOTOR_RIGHT) {
        mcpwm_comparator_set_compare_value(_mcpwm_cmp_r1, duty_in1);
        mcpwm_comparator_set_compare_value(_mcpwm_cmp_r2, duty_in2);
        _motor_data.pwm_right = pwm;
        _motor_data.state_right = state;
    }
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: PUBLIC FUNCTIONS - Initialization
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize motor control HAL
 * @return true if successful, false otherwise
 * 
 * @note This function initializes the MCPWM peripheral for motor control.
 *       It must be called once during setup before using other motor functions.
 * 
 * @example
 *   if (!motor_init()) {
 *       Serial.println("Motor init failed!");
 *       while(1);
 *   }
 */
bool motor_init(void) {
    esp_err_t ret;
    
    #if DEBUG_SERIAL
    Serial.println("[MOTOR] Initializing...");
    #endif
    
    // ─────────────────────────────────────────────────────────────────────
    // Create MCPWM Timer
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MOTOR_PWM_FREQ * MOTOR_PWM_MAX,  // Timer resolution
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = MOTOR_PWM_MAX,  // 1023 ticks per period
    };
    
    ret = mcpwm_new_timer(&timer_config, &_mcpwm_timer);
    if (ret != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[MOTOR] Timer create failed: %d\n", ret);
        #endif
        return false;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Create MCPWM Operators (one per motor)
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_operator_config_t oper_config = {
        .group_id = 0,
    };
    
    ret = mcpwm_new_operator(&oper_config, &_mcpwm_oper_left);
    if (ret != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[MOTOR] Operator L create failed: %d\n", ret);
        #endif
        return false;
    }
    
    ret = mcpwm_new_operator(&oper_config, &_mcpwm_oper_right);
    if (ret != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[MOTOR] Operator R create failed: %d\n", ret);
        #endif
        return false;
    }
    
    // Connect operators to timer
    mcpwm_operator_connect_timer(_mcpwm_oper_left, _mcpwm_timer);
    mcpwm_operator_connect_timer(_mcpwm_oper_right, _mcpwm_timer);
    
    // ─────────────────────────────────────────────────────────────────────
    // Create Comparators (2 per motor for IN1/IN2)
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_comparator_config_t cmp_config = {
        .flags = {
            .update_cmp_on_tez = true,  // Update on timer zero
        }
    };
    
    // Left motor comparators
    ret = mcpwm_new_comparator(_mcpwm_oper_left, &cmp_config, &_mcpwm_cmp_l1);
    if (ret != ESP_OK) return false;
    ret = mcpwm_new_comparator(_mcpwm_oper_left, &cmp_config, &_mcpwm_cmp_l2);
    if (ret != ESP_OK) return false;
    
    // Right motor comparators
    ret = mcpwm_new_comparator(_mcpwm_oper_right, &cmp_config, &_mcpwm_cmp_r1);
    if (ret != ESP_OK) return false;
    ret = mcpwm_new_comparator(_mcpwm_oper_right, &cmp_config, &_mcpwm_cmp_r2);
    if (ret != ESP_OK) return false;
    
    // ─────────────────────────────────────────────────────────────────────
    // Create Generators and bind to GPIO
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_generator_config_t gen_config = {};
    
    // Left motor IN1
    gen_config.gen_gpio_num = PIN_MOTOR_L_IN1;
    ret = mcpwm_new_generator(_mcpwm_oper_left, &gen_config, &_mcpwm_gen_l1);
    if (ret != ESP_OK) return false;
    
    // Left motor IN2
    gen_config.gen_gpio_num = PIN_MOTOR_L_IN2;
    ret = mcpwm_new_generator(_mcpwm_oper_left, &gen_config, &_mcpwm_gen_l2);
    if (ret != ESP_OK) return false;
    
    // Right motor IN1
    gen_config.gen_gpio_num = PIN_MOTOR_R_IN1;
    ret = mcpwm_new_generator(_mcpwm_oper_right, &gen_config, &_mcpwm_gen_r1);
    if (ret != ESP_OK) return false;
    
    // Right motor IN2
    gen_config.gen_gpio_num = PIN_MOTOR_R_IN2;
    ret = mcpwm_new_generator(_mcpwm_oper_right, &gen_config, &_mcpwm_gen_r2);
    if (ret != ESP_OK) return false;
    
    // ─────────────────────────────────────────────────────────────────────
    // Configure generator actions
    // ─────────────────────────────────────────────────────────────────────
    _motor_config_generator(_mcpwm_gen_l1, _mcpwm_cmp_l1);
    _motor_config_generator(_mcpwm_gen_l2, _mcpwm_cmp_l2);
    _motor_config_generator(_mcpwm_gen_r1, _mcpwm_cmp_r1);
    _motor_config_generator(_mcpwm_gen_r2, _mcpwm_cmp_r2);
    
    // ─────────────────────────────────────────────────────────────────────
    // Set initial compare values (motors stopped)
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_l1, 0);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_l2, 0);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_r1, 0);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_r2, 0);
    
    // ─────────────────────────────────────────────────────────────────────
    // Enable and start timer
    // ─────────────────────────────────────────────────────────────────────
    mcpwm_timer_enable(_mcpwm_timer);
    mcpwm_timer_start_stop(_mcpwm_timer, MCPWM_TIMER_START_NO_STOP);
    
    _motor_data.enabled = true;
    
    #if DEBUG_SERIAL
    Serial.println("[MOTOR] Initialized successfully");
    Serial.printf("[MOTOR] PWM Freq: %d Hz, Resolution: %d-bit\n", 
                  MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    #endif
    
    return true;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: PUBLIC FUNCTIONS - Motor Control
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Set motor speed
 * @param left  Left motor PWM (-1023 to +1023, negative=backward)
 * @param right Right motor PWM (-1023 to +1023, negative=backward)
 * 
 * @note Positive values = forward, Negative values = backward, 0 = coast
 * 
 * @example
 *   motor_set_speed(500, 500);   // Forward at ~50% speed
 *   motor_set_speed(-500, -500); // Backward at ~50% speed
 *   motor_set_speed(500, -500);  // Spin right (pivot turn)
 *   motor_set_speed(0, 0);       // Stop (coast)
 */
void motor_set_speed(int16_t left, int16_t right) {
    if (!_motor_data.enabled) return;
    
    _motor_apply_pwm(MOTOR_LEFT, left);
    _motor_apply_pwm(MOTOR_RIGHT, right);
    
    #if DEBUG_MOTORS
    Serial.printf("[MOTOR] Set L:%d R:%d\n", left, right);
    #endif
}

/**
 * @brief Set single motor speed
 * @param motor Motor ID (MOTOR_LEFT or MOTOR_RIGHT)
 * @param pwm   PWM value (-1023 to +1023)
 */
void motor_set_single(MotorID_t motor, int16_t pwm) {
    if (!_motor_data.enabled) return;
    
    if (motor == MOTOR_LEFT) {
        _motor_apply_pwm(MOTOR_LEFT, pwm);
    } else if (motor == MOTOR_RIGHT) {
        _motor_apply_pwm(MOTOR_RIGHT, pwm);
    } else if (motor == MOTOR_BOTH) {
        _motor_apply_pwm(MOTOR_LEFT, pwm);
        _motor_apply_pwm(MOTOR_RIGHT, pwm);
    }
}

/**
 * @brief Stop motors (coast mode - free spinning)
 * 
 * @note Motors will gradually slow down due to friction.
 *       Use motor_brake() for immediate stop.
 */
void motor_stop(void) {
    motor_set_speed(0, 0);
    
    #if DEBUG_MOTORS
    Serial.println("[MOTOR] Stop (coast)");
    #endif
}

/**
 * @brief Brake motors (active braking)
 * 
 * @note This applies regenerative braking by shorting motor terminals.
 *       Motors will stop quickly but may cause voltage spike.
 */
void motor_brake(void) {
    if (!_motor_data.enabled) return;
    
    // Set both IN1 and IN2 high for braking
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_l1, MOTOR_PWM_MAX);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_l2, MOTOR_PWM_MAX);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_r1, MOTOR_PWM_MAX);
    mcpwm_comparator_set_compare_value(_mcpwm_cmp_r2, MOTOR_PWM_MAX);
    
    _motor_data.pwm_left = 0;
    _motor_data.pwm_right = 0;
    _motor_data.state_left = MOTOR_STATE_BRAKE;
    _motor_data.state_right = MOTOR_STATE_BRAKE;
    
    #if DEBUG_MOTORS
    Serial.println("[MOTOR] Brake");
    #endif
}

/**
 * @brief Enable/Disable motors
 * @param enable true to enable, false to disable
 */
void motor_enable(bool enable) {
    _motor_data.enabled = enable;
    
    if (!enable) {
        motor_stop();
    }
    
    #if DEBUG_MOTORS
    Serial.printf("[MOTOR] %s\n", enable ? "Enabled" : "Disabled");
    #endif
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PUBLIC FUNCTIONS - Utility
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Set voltage compensation factor
 * @param battery_voltage Current battery voltage in mV
 * @param nominal_voltage Nominal battery voltage in mV (default 7400 for 2S LiPo)
 * 
 * @note This compensates for battery voltage drop to maintain consistent speed.
 *       scale = nominal_voltage / battery_voltage
 */
void motor_set_voltage_compensation(uint16_t battery_voltage, uint16_t nominal_voltage) {
    if (battery_voltage < BATTERY_CRITICAL_MV) {
        _motor_data.voltage_scale = 1.0f;  // Don't compensate at very low voltage
        return;
    }
    
    if (nominal_voltage == 0) {
        nominal_voltage = BATTERY_NOMINAL_MV;
    }
    
    _motor_data.voltage_scale = (float)nominal_voltage / (float)battery_voltage;
    
    // Limit compensation to prevent over-driving
    _motor_data.voltage_scale = CONSTRAIN(_motor_data.voltage_scale, 0.8f, 1.2f);
    
    #if DEBUG_MOTORS
    Serial.printf("[MOTOR] Voltage compensation: %.2f (bat=%dmV)\n", 
                  _motor_data.voltage_scale, battery_voltage);
    #endif
}

/**
 * @brief Get current motor data
 * @return MotorData_t structure with current motor state
 */
MotorData_t motor_get_data(void) {
    return _motor_data;
}

/**
 * @brief Get current PWM values
 * @param left  Pointer to store left motor PWM
 * @param right Pointer to store right motor PWM
 */
void motor_get_pwm(int16_t* left, int16_t* right) {
    if (left != NULL) *left = _motor_data.pwm_left;
    if (right != NULL) *right = _motor_data.pwm_right;
}

/**
 * @brief Check if motors are enabled
 * @return true if motors are enabled
 */
bool motor_is_enabled(void) {
    return _motor_data.enabled;
}

/**
 * @brief Print motor status to Serial
 */
void motor_print_status(void) {
    #if DEBUG_SERIAL
    const char* state_names[] = {"COAST", "BRAKE", "FWD", "BWD"};
    
    Serial.println("────────────────────────────────────");
    Serial.println("       MOTOR STATUS");
    Serial.println("────────────────────────────────────");
    Serial.printf("Enabled:    %s\n", _motor_data.enabled ? "YES" : "NO");
    Serial.printf("Left PWM:   %d (%s)\n", _motor_data.pwm_left, state_names[_motor_data.state_left]);
    Serial.printf("Right PWM:  %d (%s)\n", _motor_data.pwm_right, state_names[_motor_data.state_right]);
    Serial.printf("V-Scale:    %.2f\n", _motor_data.voltage_scale);
    Serial.println("────────────────────────────────────");
    #endif
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: CONVENIENCE FUNCTIONS
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief Move forward at specified speed
 * @param speed PWM value (0 to 1023)
 */
void motor_forward(int16_t speed) {
    speed = ABS(speed);
    motor_set_speed(speed, speed);
}

/**
 * @brief Move backward at specified speed
 * @param speed PWM value (0 to 1023)
 */
void motor_backward(int16_t speed) {
    speed = ABS(speed);
    motor_set_speed(-speed, -speed);
}

/**
 * @brief Turn left (pivot on left wheel)
 * @param speed PWM value (0 to 1023)
 */
void motor_turn_left(int16_t speed) {
    speed = ABS(speed);
    motor_set_speed(-speed, speed);
}

/**
 * @brief Turn right (pivot on right wheel)
 * @param speed PWM value (0 to 1023)
 */
void motor_turn_right(int16_t speed) {
    speed = ABS(speed);
    motor_set_speed(speed, -speed);
}

/**
 * @brief Arc turn (differential speed)
 * @param left_speed  Left motor PWM
 * @param right_speed Right motor PWM
 */
void motor_arc_turn(int16_t left_speed, int16_t right_speed) {
    motor_set_speed(left_speed, right_speed);
}

#endif // HAL_MOTOR_H
