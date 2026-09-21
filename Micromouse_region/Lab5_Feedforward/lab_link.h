// ═══════════════════════════════════════════════════════════════════════════
//  lab_link.h — ใช้สำหรับส่ง serial ได้ทั้ง USB และ BLE โดยไม่ต้องแก้ทุก Serial.print (เนื่องจากเวอร์ชันแรกมีข้อความค่อนข้างเยอะเลยใช้วิธีมาครอบ)
//
//  วิธีใช้ (ใน .ino):
//    #include "ble_debug.h"
//    #include "lab_link.h"          // ← ต้องอยู่ "ท้ายสุด" ของ include ทั้งหมด
//    ... ใน setup(): Serial.begin(115200);  ble_debug_init("MM_LabX");
//
//  หลังจาก include นี้ คำว่า Serial ในโค้ดด้านล่างจะหมายถึงตัวเชื่อม (LL) ที่:
//    - พิมพ์ออก  → ส่งทั้ง USB และ BLE (BLE ส่งทีละบรรทัด)
//    - อ่านเข้า  → รับจาก USB หรือ BLE (คำสั่งเดิมใช้ได้ทั้งสองทาง)
// ═══════════════════════════════════════════════════════════════════════════
#ifndef LAB_LINK_H
#define LAB_LINK_H

#include <Arduino.h>
#include "ble_debug.h"

// เริ่มพอร์ต USB (เรียกก่อน #define Serial จะได้ใช้ Serial ตัวจริง)
inline void _lab_real_begin(uint32_t baud) { Serial.begin(baud); }

class LabLink : public Stream {
public:
    Stream* usb;                 // ตัวจริง (USBCDC หรือ HardwareSerial — ทั้งคู่เป็น Stream)
    explicit LabLink(Stream* s) : usb(s) {}

    void begin(uint32_t baud) { _lab_real_begin(baud); }
    void flush() { usb->flush(); }
    explicit operator bool() const { return true; }   // for `while (!Serial)` — always ready

    // output: USB + BLE (BLE เก็บค่าจนเจอ '\n' ค่อยส่งทั้งบรรทัด) 
    size_t write(uint8_t c) override {
        usb->write(c);
        if (_on < (int)sizeof(_ob) - 1) _ob[_on++] = (char)c;
        if (c == '\n' || _on >= (int)sizeof(_ob) - 1) { _ob[_on] = 0; ble_debug_send(_ob); _on = 0; }
        return 1;
    }
    size_t write(const uint8_t* b, size_t n) override { for (size_t i = 0; i < n; i++) write(b[i]); return n; }

    // input: USB ก่อน ถ้าไม่มีค่อยดึงบรรทัดจาก BLE มาป้อนทีละตัว 
    int available() override {
        if (usb->available()) return usb->available();
        _pull_ble();
        return _ilen - _ipos;
    }
    int read() override {
        if (usb->available()) return usb->read();
        _pull_ble();
        return (_ipos < _ilen) ? (uint8_t)_ib[_ipos++] : -1;
    }
    int peek() override {
        if (usb->available()) return usb->peek();
        _pull_ble();
        return (_ipos < _ilen) ? (uint8_t)_ib[_ipos] : -1;
    }

private:
    char _ob[160]; int _on = 0;          // output line buffer (→ BLE)
    char _ib[72];  int _ilen = 0, _ipos = 0; // input line pulled from BLE

    void _pull_ble() {
        if (_ipos < _ilen) return;        // ยังมีตัวอักษรค้างอยู่
        char line[64];
        if (ble_debug_take_line(line, sizeof(line))) {
            int n = 0;
            while (line[n] && n < (int)sizeof(_ib) - 2) { _ib[n] = line[n]; n++; }
            _ib[n++] = '\n';              // ส่ง newline ปิดท้าย ให้ตัว parser เห็นจบบรรทัด
            _ilen = n; _ipos = 0;
        }
    }
};

inline LabLink LL(&Serial);
#define Serial LL

#endif // LAB_LINK_H
