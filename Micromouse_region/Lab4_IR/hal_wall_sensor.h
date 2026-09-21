// ═══════════════════════════════════════════════════════════════════════════
//  hal_wall_sensor.h
//  Wall Sensor HAL — Sharp GP2Y0A51SK0F (Analog IR)
//
//  Layer: 1 (Hardware Abstraction)
//  Dependencies: config.h
//
//  Hardware: Sharp GP2Y0A51SK0F × 4
//  Interface: ADC analog (analogReadMilliVolts)
//  ไม่ต้องใช้ I2C — ทำงานอิสระจาก IMU
//
//  Sensor Layout:
//        FRONT
//    ┌─────────┐
//    │ FL   FR │  FL = Front-Left, FR = Front-Right
//    │         │
//  L │         │ R   L = Left, R = Right
//    └─────────┘
//
//  Public API:
//    wall_sensor_init()          — ตั้งค่า ADC resolution + attenuation, return จำนวน pin ที่พร้อม
//    wall_sensor_read(id)        — อ่านเซนเซอร์ 1 ตัว: ADC → mV → LUT → offset → mm
//    wall_sensor_read_next()     — อ่าน round-robin (ใช้ใน loop เพื่อกระจายโหลด)
//    wall_sensor_read_all()      — อ่านทุกตัวพร้อมกัน
//    wall_sensor_get_distance(id)
//    wall_sensor_get_distances(r, fr, fl, l)
//    wall_sensor_is_wall(id)
//    wall_sensor_wall_front/left/right()
//    wall_sensor_get_front_distance()
//    wall_sensor_get_reading(id) — return WallSensorReading_t (มี raw_mv + raw_distance_mm + distance_mm)
//    wall_sensor_get_raw_mv(id)  — ADC mV ดิบ ใช้ตอน calibrate LUT
//    wall_sensor_set/get_threshold(front, side)
//    wall_sensor_print_status()
//
//  หมายเหตุ Sharp GP2Y0A51SK0F:
//    - Spec จริง: supply 4.5–5.5V, range 2–15 cm
//    - ใช้งานที่ 3.3V (out of spec): แรงดัน output ต่ำลงทั้ง curve
//      → ต้อง calibrate SHARP_LUT ใน hal_wall_sensor.h Section 1 ให้ตรง
//    - เพิ่ม SHARP_ADC_SAMPLES เพื่อ average และลด noise จาก ADC
//    - ความสัมพันธ์ mV กับระยะ: ใกล้ → mV สูง, ไกล → mV ต่ำ (inverse, non-linear)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef HAL_WALL_SENSOR_H
#define HAL_WALL_SENSOR_H

#include <Arduino.h>
#include "config.h"

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 1: SHARP LUT (mV → mm)
// │
// │ Typical response curve ที่ 3.3V supply (out-of-spec, ค่าอาจต่างจาก datasheet)
// │ *** ต้อง calibrate กับเซนเซอร์จริงของแต่ละคน ***
// │
// │ วิธี calibrate:
// │   1. วางวัตถุที่ระยะที่วัดได้จริง (เช่น 30mm, 50mm, 80mm, 100mm, 120mm)
// │   2. ส่งคำสั่ง 'v' ใน Serial Monitor เพื่ออ่าน raw mV ของแต่ละเซนเซอร์
// │   3. แก้ไขตาราง SHARP_LUT ด้านล่างนี้ให้ตรงกับค่าที่วัดได้
// │   4. flash โปรแกรมใหม่ แล้วส่งคำสั่ง 'l' เพื่อยืนยัน LUT ที่อัปเดต
// │
// │ ลำดับ: mV มาก→น้อย (วัตถุอยู่ใกล้→ไกล), range ~20..150 mm
// │ หมายเหตุ: ความสัมพันธ์ mV vs ระยะเป็น inverse non-linear ไม่ใช่เส้นตรง
// └───────────────────────────────────────────────────────────────────────────

struct _SharpPt { uint16_t mv; uint16_t mm; };

static constexpr _SharpPt SHARP_LUT[] = {
    {2500,  20},
    {2000,  30},
    {1600,  40},
    {1300,  50},
    {1100,  60},
    { 950,  70},
    { 850,  80},
    { 780,  90},
    { 720, 100},
    { 650, 110},
    { 600, 120},
    { 520, 130},
    { 470, 140},
    { 420, 150},
};
static constexpr int SHARP_LUT_SIZE = sizeof(SHARP_LUT) / sizeof(SHARP_LUT[0]);

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 2: CONSTANTS & ENUMS
// └───────────────────────────────────────────────────────────────────────────

typedef enum {
    WALL_SENSOR_RIGHT       = 0,
    WALL_SENSOR_FRONT_RIGHT = 1,
    WALL_SENSOR_FRONT_LEFT  = 2,
    WALL_SENSOR_LEFT        = 3,
    WALL_SENSOR_COUNT       = 4
} WallSensorID_t;

typedef enum {
    IR_STATUS_OK        = 0,   // อ่านได้ปกติ
    IR_STATUS_NO_PIN    = 1,   // Pin ไม่ได้ตั้งค่า (= -1)
    IR_STATUS_OUT_RANGE = 2,   // ไม่มีวัตถุในระยะ หรือ ไกลเกินไป
} IRSensorStatus_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 3: DATA TYPES
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief ข้อมูลเซนเซอร์ตัวเดียว
 * raw_mv          = ADC millivolts (ก่อน LUT) — ใช้ตอน calibrate
 * raw_distance_mm = LUT mm ก่อนหักลบ offset (255 = out of range)
 * distance_mm     = ระยะจริงหลัง offset (255 = invalid)
 */
typedef struct {
    uint16_t raw_mv;           // ADC mV (เฉลี่ย SHARP_ADC_SAMPLES ครั้ง)
    uint8_t  raw_distance_mm;  // LUT mm ก่อน offset
    uint8_t  distance_mm;      // ระยะ mm หลัง offset
    uint8_t  status;           // IRSensorStatus_t
    bool     valid;            // true = วัตถุอยู่ในระยะ
    bool     wall_detected;    // true = ระยะ < threshold (มีกำแพง)
    uint32_t timestamp_ms;
} WallSensorReading_t;

/**
 * @brief สถานะทุกเซนเซอร์รวมกัน
 */
typedef struct {
    WallSensorReading_t sensor[WALL_SENSOR_COUNT];
    uint8_t  current_sensor;   // ใช้ใน round-robin
    bool     initialized;
    uint32_t read_count;
    uint32_t error_count;      // จำนวนครั้งที่อ่านได้ out-of-range
} WallSensorState_t;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 4: STATE VARIABLES
// └───────────────────────────────────────────────────────────────────────────

inline WallSensorState_t _wall_state = {
    .sensor         = {{0}},
    .current_sensor = 0,
    .initialized    = false,
    .read_count     = 0,
    .error_count    = 0
};

// Pin mapping ตาม WallSensorID_t
// index 0=RIGHT, 1=FRONT_RIGHT, 2=FRONT_LEFT, 3=LEFT
inline const int _wall_pins[WALL_SENSOR_COUNT] = {
    PIN_IR_RIGHT,    // WALL_SENSOR_RIGHT       = 0
    PIN_IR_FRONT_R,  // WALL_SENSOR_FRONT_RIGHT  = 1
    PIN_IR_FRONT_L,  // WALL_SENSOR_FRONT_LEFT   = 2
    PIN_IR_LEFT,     // WALL_SENSOR_LEFT         = 3
};

// Offset (mm) ต่อ sensor
inline const int16_t _wall_offsets[WALL_SENSOR_COUNT] = {
    WALL_OFFSET_R,
    WALL_OFFSET_FR,
    WALL_OFFSET_FL,
    WALL_OFFSET_L,
};

inline uint8_t _wall_threshold_front = WALL_THRESHOLD_FRONT;
inline uint8_t _wall_threshold_side  = WALL_THRESHOLD_SIDE;

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 5: PRIVATE HELPERS
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief แปลง ADC millivolts → ระยะ mm ผ่าน SHARP_LUT
 *
 * Sharp GP2Y0A51SK0F มี curve แบบ inverse non-linear:
 *   ยิ่งใกล้  → output voltage สูง (mV มาก)
 *   ยิ่งไกล  → output voltage ต่ำ (mV น้อย)
 *   ไกลเกิน ~15cm → voltage ต่ำมากจนอ่านไม่ออก (return 255)
 *
 * LUT ใช้ linear interpolation ระหว่าง breakpoint เพื่อความเร็ว
 * (ไม่ใช้สมการ curve fitting เพราะ curve เปลี่ยนตาม supply voltage)
 *
 * @param mv  ADC millivolts ที่ผ่านการ average แล้ว
 * @return    ระยะ mm (~20–150), หรือ 255 ถ้า out of range / ไม่มีวัตถุ
 */
inline uint8_t _sharp_mv_to_mm(uint16_t mv) {
    // mV ต่ำกว่า entry สุดท้าย = วัตถุอยู่ไกลเกิน range หรือไม่มีวัตถุ
    if (mv < SHARP_LUT[SHARP_LUT_SIZE - 1].mv) return 255;

    // mV สูงกว่า entry แรก = วัตถุชิดเกินไป → clamp ที่ระยะต่ำสุด
    if (mv >= SHARP_LUT[0].mv) return (uint8_t)SHARP_LUT[0].mm;

    // หา segment ที่ mv อยู่ระหว่าง [i] และ [i+1] แล้ว interpolate เชิงเส้น
    for (int i = 0; i + 1 < SHARP_LUT_SIZE; i++) {
        if (mv <= SHARP_LUT[i].mv && mv >= SHARP_LUT[i + 1].mv) {
            // t = สัดส่วนที่ mv อยู่ระหว่าง SHARP_LUT[i] และ SHARP_LUT[i+1]
            float t  = (float)(SHARP_LUT[i].mv - mv) /
                       (float)(SHARP_LUT[i].mv - SHARP_LUT[i + 1].mv);
            float mm = (float)SHARP_LUT[i].mm +
                       t * ((float)SHARP_LUT[i + 1].mm - (float)SHARP_LUT[i].mm);
            return (uint8_t)(mm + 0.5f);  // round to nearest mm
        }
    }
    return 255;
}

/**
 * @brief อ่าน ADC millivolts และ average หลายครั้งเพื่อลด noise
 *
 * ESP32 ADC มี noise และ nonlinearity สูงกว่า external ADC ทั่วไป
 * การ average SHARP_ADC_SAMPLES ครั้งช่วยลด random noise ได้
 * (~1/√N ของ noise → 4 samples ลด noise ~50%)
 *
 * @param pin  GPIO ADC pin number
 * @return     mV เฉลี่ย (0–3300 mV สำหรับ ADC_11db attenuation)
 */
inline uint16_t _sharp_read_mv(int pin) {
    uint32_t sum = 0;
    for (int i = 0; i < SHARP_ADC_SAMPLES; i++) {
        sum += (uint32_t)analogReadMilliVolts(pin);
    }
    return (uint16_t)(sum / SHARP_ADC_SAMPLES);
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 6: PUBLIC FUNCTIONS — Initialization
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief ตั้งค่า ADC pins สำหรับ Sharp IR sensors
 * @return จำนวน pin ที่ configured (0-4)
 *
 * @note Sharp เป็น analog — ไม่ต้องการ I2C และไม่ต้องการ hal_imu.h
 */
inline uint8_t wall_sensor_init(void) {
#if DEBUG_SERIAL
    Serial.println("[IR] Initializing Sharp GP2Y0A51SK0F sensors...");
#endif

    analogReadResolution(12);  // 12-bit ADC (0-4095)

    uint8_t active = 0;
    for (int i = 0; i < WALL_SENSOR_COUNT; i++) {
        if (_wall_pins[i] >= 0) {
            pinMode(_wall_pins[i], INPUT);
            analogSetPinAttenuation(_wall_pins[i], ADC_11db);
            _wall_state.sensor[i].status       = IR_STATUS_OK;
            _wall_state.sensor[i].distance_mm  = 255;
            _wall_state.sensor[i].valid        = false;
            active++;
#if DEBUG_SERIAL
            Serial.printf("[IR]   Sensor %d (GPIO %2d): ready\n", i, _wall_pins[i]);
#endif
        } else {
            _wall_state.sensor[i].status       = IR_STATUS_NO_PIN;
            _wall_state.sensor[i].distance_mm  = 255;
            _wall_state.sensor[i].valid        = false;
#if DEBUG_SERIAL
            Serial.printf("[IR]   Sensor %d: not configured (pin = -1)\n", i);
#endif
        }
    }

    _wall_state.initialized    = (active > 0);
    _wall_state.current_sensor = 0;

#if DEBUG_SERIAL
    Serial.printf("[IR] Initialized %d/%d sensors\n", active, WALL_SENSOR_COUNT);
    if (SHARP_SUPPLY_MV < 4500) {
        Serial.printf("[IR] WARN: GP2Y0A51SK0F datasheet ต้องการ 4.5-5.5V, ใช้งานที่ %dmV\n",
                      SHARP_SUPPLY_MV);
        Serial.println("[IR]       LUT ต้อง calibrate กับเซนเซอร์จริง ดูคำสั่ง 'v' และ 'l'");
    }
#endif

    return active;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 7: PUBLIC FUNCTIONS — Reading
// └───────────────────────────────────────────────────────────────────────────

/**
 * @brief อ่านเซนเซอร์ Sharp IR 1 ตัว (blocking, ~0.2ms ต่อ sensor สำหรับ 4 samples)
 *
 * ขั้นตอนการแปลงสัญญาณ:
 *   GPIO (analog) → mV (ADC average) → mm (LUT interpolation) → mm (offset correction)
 *
 * @param sensor_id  ID ของเซนเซอร์ (WallSensorID_t)
 * @return distance_mm หลังหักลบ offset (255 = ไม่มีวัตถุในระยะ หรือ pin ไม่ได้ตั้งค่า)
 */
inline uint8_t wall_sensor_read(WallSensorID_t sensor_id) {
    if (sensor_id >= WALL_SENSOR_COUNT) return 255;
    if (_wall_state.sensor[sensor_id].status == IR_STATUS_NO_PIN) return 255;

    // ขั้นที่ 1: อ่าน ADC → mV (average หลายครั้งเพื่อลด noise)
    uint16_t mv = _sharp_read_mv(_wall_pins[sensor_id]);

    // ขั้นที่ 2: แปลง mV → mm ผ่าน LUT
    // raw_mm = 255 หมายถึง mV ต่ำเกินไป = ไม่มีวัตถุหรืออยู่ไกลเกิน range
    uint8_t  raw_mm   = _sharp_mv_to_mm(mv);
    bool     in_range = (raw_mm != 255);

    // ขั้นที่ 3: ลบ offset เพื่อชดเชยตำแหน่งติดตั้งเซนเซอร์บนตัวหุ่นยนต์
    int16_t corrected = in_range
                        ? (int16_t)raw_mm - _wall_offsets[sensor_id]
                        : 255;
    if (corrected < 0)   corrected = 0;    // ป้องกันค่าติดลบ (offset ใหญ่เกินระยะ)
    if (corrected > 254) corrected = 254;  // clamp สูงสุด (255 สงวนไว้สำหรับ invalid)

    // บันทึกผลทุก field ลงใน state
    _wall_state.sensor[sensor_id].raw_mv          = mv;
    _wall_state.sensor[sensor_id].raw_distance_mm = raw_mm;      // mm ก่อน offset
    _wall_state.sensor[sensor_id].distance_mm     = in_range ? (uint8_t)corrected : 255;
    _wall_state.sensor[sensor_id].valid           = in_range;
    _wall_state.sensor[sensor_id].status          = in_range ? IR_STATUS_OK : IR_STATUS_OUT_RANGE;
    _wall_state.sensor[sensor_id].timestamp_ms    = millis();

    // ขั้นที่ 4: เปรียบเทียบกับ threshold เพื่อตัดสินว่า "มีกำแพง" หรือไม่
    // ใช้ threshold แยกสำหรับด้านหน้า (ใกล้กว่า) กับด้านข้าง
    uint8_t threshold = (sensor_id == WALL_SENSOR_FRONT_LEFT ||
                         sensor_id == WALL_SENSOR_FRONT_RIGHT)
                        ? _wall_threshold_front : _wall_threshold_side;
    _wall_state.sensor[sensor_id].wall_detected = in_range && ((uint8_t)corrected < threshold);

    _wall_state.read_count++;
    if (!in_range) _wall_state.error_count++;

    return _wall_state.sensor[sensor_id].distance_mm;
}

/**
 * @brief อ่าน 1 ตัวแบบ round-robin (เรียกใน loop เพื่อกระจายโหลด)
 * @return ID ของเซนเซอร์ที่เพิ่งอ่าน
 */
inline WallSensorID_t wall_sensor_read_next(void) {
    WallSensorID_t id = (WallSensorID_t)_wall_state.current_sensor;

    for (int tries = 0; tries < WALL_SENSOR_COUNT; tries++) {
        if (_wall_state.sensor[id].status != IR_STATUS_NO_PIN) {
            wall_sensor_read(id);
            break;
        }
        id = (WallSensorID_t)((id + 1) % WALL_SENSOR_COUNT);
    }

    _wall_state.current_sensor = (_wall_state.current_sensor + 1) % WALL_SENSOR_COUNT;
    return id;
}

/**
 * @brief อ่านทุกตัวพร้อมกัน (blocking ~1ms รวม 4 sensors × 4 ADC samples)
 * @note ใช้ wall_sensor_read_next() ใน loop เพื่อกระจายการอ่านออกไปแทน
 */
inline void wall_sensor_read_all(void) {
    for (int i = 0; i < WALL_SENSOR_COUNT; i++) {
        wall_sensor_read((WallSensorID_t)i);
    }
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 8: PUBLIC FUNCTIONS — Data Access
// └───────────────────────────────────────────────────────────────────────────

inline uint8_t wall_sensor_get_distance(WallSensorID_t sensor_id) {
    if (sensor_id >= WALL_SENSOR_COUNT) return 255;
    return _wall_state.sensor[sensor_id].distance_mm;
}

inline void wall_sensor_get_distances(uint8_t* right, uint8_t* front_right,
                                      uint8_t* front_left, uint8_t* left) {
    if (right)       *right       = _wall_state.sensor[WALL_SENSOR_RIGHT].distance_mm;
    if (front_right) *front_right = _wall_state.sensor[WALL_SENSOR_FRONT_RIGHT].distance_mm;
    if (front_left)  *front_left  = _wall_state.sensor[WALL_SENSOR_FRONT_LEFT].distance_mm;
    if (left)        *left        = _wall_state.sensor[WALL_SENSOR_LEFT].distance_mm;
}

inline bool wall_sensor_is_wall(WallSensorID_t sensor_id) {
    if (sensor_id >= WALL_SENSOR_COUNT) return false;
    return _wall_state.sensor[sensor_id].wall_detected;
}

inline bool wall_sensor_wall_front(void) {
    return _wall_state.sensor[WALL_SENSOR_FRONT_LEFT].wall_detected ||
           _wall_state.sensor[WALL_SENSOR_FRONT_RIGHT].wall_detected;
}

inline bool wall_sensor_wall_left(void) {
    return _wall_state.sensor[WALL_SENSOR_LEFT].wall_detected;
}

inline bool wall_sensor_wall_right(void) {
    return _wall_state.sensor[WALL_SENSOR_RIGHT].wall_detected;
}

/**
 * @brief ระยะด้านหน้า (เฉลี่ย FL กับ FR)
 * @note ถ้าตัวใดตัวหนึ่ง out-of-range จะใช้ค่าที่อ่านได้ตัวเดียว
 */
inline uint8_t wall_sensor_get_front_distance(void) {
    uint8_t fl = _wall_state.sensor[WALL_SENSOR_FRONT_LEFT].distance_mm;
    uint8_t fr = _wall_state.sensor[WALL_SENSOR_FRONT_RIGHT].distance_mm;
    if (fl == 255 && fr == 255) return 255;
    if (fl == 255) return fr;
    if (fr == 255) return fl;
    return (fl + fr) / 2;
}

inline WallSensorReading_t wall_sensor_get_reading(WallSensorID_t sensor_id) {
    if (sensor_id >= WALL_SENSOR_COUNT) {
        WallSensorReading_t empty = {0, 255, 255, IR_STATUS_NO_PIN, false, false, 0};
        return empty;
    }
    return _wall_state.sensor[sensor_id];
}

/**
 * @brief ดึง raw ADC mV — ใช้ตอน calibrate LUT
 */
inline uint16_t wall_sensor_get_raw_mv(WallSensorID_t sensor_id) {
    if (sensor_id >= WALL_SENSOR_COUNT) return 0;
    return _wall_state.sensor[sensor_id].raw_mv;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 9: PUBLIC FUNCTIONS — Configuration
// └───────────────────────────────────────────────────────────────────────────

inline void wall_sensor_set_threshold(uint8_t front_mm, uint8_t side_mm) {
    _wall_threshold_front = front_mm;
    _wall_threshold_side  = side_mm;
}

inline void wall_sensor_get_threshold(uint8_t* front_mm, uint8_t* side_mm) {
    if (front_mm) *front_mm = _wall_threshold_front;
    if (side_mm)  *side_mm  = _wall_threshold_side;
}

// ┌───────────────────────────────────────────────────────────────────────────
// │ SECTION 10: PUBLIC FUNCTIONS — Utility
// └───────────────────────────────────────────────────────────────────────────

inline bool wall_sensor_is_initialized(void) {
    return _wall_state.initialized;
}

inline uint8_t wall_sensor_get_active_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < WALL_SENSOR_COUNT; i++) {
        if (_wall_state.sensor[i].status != IR_STATUS_NO_PIN) count++;
    }
    return count;
}

inline uint32_t wall_sensor_get_read_count(void)  { return _wall_state.read_count; }
inline uint32_t wall_sensor_get_error_count(void) { return _wall_state.error_count; }

/**
 * @brief แสดงสถานะทุก sensor ใน Serial
 * แสดง: GPIO pin, raw mV, raw mm (ก่อน offset), corrected mm, status, wall
 */
inline void wall_sensor_print_status(void) {
#if DEBUG_SERIAL
    const char* names[] = {"Right ", "FrontR", "FrontL", "Left  "};

    Serial.println("────────────────────────────────────────────────────────────");
    Serial.println("              SHARP IR SENSOR STATUS");
    Serial.println("────────────────────────────────────────────────────────────");
    Serial.printf("Initialized: %s  |  Active: %d/%d  |  Supply: %dmV\n",
                  _wall_state.initialized ? "Yes" : "No",
                  wall_sensor_get_active_count(), WALL_SENSOR_COUNT,
                  SHARP_SUPPLY_MV);
    Serial.println();

    Serial.println("Sensor   GPIO   mV     Raw    Dist    Status    Wall");
    Serial.println("──────   ────   ────   ────   ────    ──────    ────");

    for (int i = 0; i < WALL_SENSOR_COUNT; i++) {
        if (_wall_state.sensor[i].status == IR_STATUS_NO_PIN) {
            Serial.printf("%s   --     ----   ---    ---     NO PIN    --\n", names[i]);
        } else {
            const char* status_str =
                (_wall_state.sensor[i].status == IR_STATUS_OK)        ? "OK    " :
                (_wall_state.sensor[i].status == IR_STATUS_OUT_RANGE) ? "OOR   " : "???   ";
            Serial.printf("%s   %2d     %4d   %3d    %3dmm   %s    %s\n",
                          names[i],
                          _wall_pins[i],
                          _wall_state.sensor[i].raw_mv,
                          _wall_state.sensor[i].raw_distance_mm,
                          _wall_state.sensor[i].distance_mm,
                          status_str,
                          _wall_state.sensor[i].wall_detected ? "YES" : "NO");
        }
    }

    Serial.println();
    Serial.printf("Threshold  Front: %3d mm  |  Side: %3d mm\n",
                  _wall_threshold_front, _wall_threshold_side);
    Serial.printf("Reads: %lu  |  Out-of-Range: %lu\n",
                  _wall_state.read_count, _wall_state.error_count);
    Serial.println("────────────────────────────────────────────────────────────");
#endif
}

#endif // HAL_WALL_SENSOR_H
