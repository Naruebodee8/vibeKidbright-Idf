# Micromouse — BLE Wireless Testing

> **Raw source**: `raw/micromouse_ble_testing.md`
> **Related**: [[micromouse]] · [[micromouse_architecture]]

---

## ทำไมต้องไร้สาย

หลาย Lab หุ่นต้องเคลื่อนที่จริง — สาย USB จะลาก/รั้งทำให้ผลเพี้ยน
**Lab4 (IR) ห้ามใช้ WiFi** เพราะ IR อยู่บน ADC2 ที่ WiFi ล็อกไว้ จึงต้องใช้ BLE เท่านั้น

---

## วิธีเชื่อมต่อ

### A. คอมพิวเตอร์ — mm_ui.py (แนะนำ)
```bash
cd Micromouse/tools
pip install -r requirements.txt    # ครั้งแรกครั้งเดียว
python mm_ui.py
```
1. กด **Scan** → เห็นรายชื่อหุ่น (เช่น `MM_Lab4_IR_3F2A9C`)
2. เลือกหุ่นตัวเอง → กด **Connect**
3. สถานะ: 🟢 เขียว = ต่ออยู่ · 🔴 แดง = หลุด
4. เลือก **Tab ของ Lab** → กดคำสั่ง
5. ผล + telemetry + ช่องพิมพ์อยู่ที่ **แถบล่างถาวร**

> ทุกหุ่นชื่อไม่ซ้ำกัน (มีรหัสชิป) → ในห้อง 20 ตัวเลือกของตัวเองได้ตรง

### B. มือถือ — BLE UART App
- **nRF Connect** หรือ **Serial Bluetooth Terminal** (โหมด BLE)
- สแกนหาตามชื่อ → Nordic UART Service (NUS) → พิมพ์คำสั่ง

> เปิด Serial Monitor (USB) คู่กันได้ — output ออกทั้งสองทางพร้อมกัน

---

## BLE Name ต่อ Lab

| Lab | BLE Name | หมายเหตุ |
|---|---|---|
| Lab1 | `MM_Lab1_Motor` | |
| Lab2 | `MM_Lab2_Enc` | |
| Lab3 | `MM_Lab3_IMU` | |
| Lab4 | `MM_Lab4_IR` | ⚠️ WiFi ปิดถาวร |
| Lab5 | `MM_Lab5_FF` | |
| Lab6 | `MM_Lab6_PID` | |
| Integration | `MM_robot` | |

---

## คำสั่งต่อ Lab

### Lab1 — Motor
| คำสั่ง | ผล |
|---|---|
| `f/b` | เดินหน้า/ถอย |
| `a/d` | เลี้ยวซ้าย/ขวา |
| `w/s` | เพิ่ม/ลด PWM |
| `l/r` | ทดสอบมอเตอร์ทีละข้าง |
| `m` | หา PWM_MIN อัตโนมัติ |
| `v` | อ่าน Vbat |
| `c` | calibrate battery ratio (กรอกค่ามัลติมิเตอร์) |
| `i` | เช็คปุ่ม START/MODE |
| `x` | หยุด |

### Lab2 — Encoder
| คำสั่ง | ผล |
|---|---|
| กรอกระยะเป้าหมาย | หุ่นขับไปแล้วหยุด (วัดระยะจริง) |
| `k` | กรอกระยะจริง → คำนวณ CPR ที่ควรเป็น |
| `r` | reset + หมุนล้อมือ 1 รอบ หา CPR เบื้องต้น |
| `e` | กรอก CPR ดูระยะ |

### Lab3 — IMU
| คำสั่ง | ผล |
|---|---|
| `c` | calibrate gyro offset (~2 วิ วางนิ่ง) |
| `r` | reset heading |
| `s` | ค่าสด (หมุนหุ่นดู heading เปลี่ยน) |

### Lab4 — IR (BLE เท่านั้น)
| คำสั่ง | ผล |
|---|---|
| `s` | อ่านต่อเนื่อง (mm) |
| `r` | อ่านครั้งเดียว |
| `v` | raw mV |
| `t` | ตั้ง threshold (พิมพ์ `60 80` = front เว้น side) |
| `l` | ดู LUT |

### Lab5 — Feedforward
| คำสั่ง | ผล |
|---|---|
| `r` | sweep อัตโนมัติ (2→5V) → กราฟ + Km/Tm |
| `ff <V>` | step เดียว |
| `clear plot` | ล้างกราฟ |
| `v` | Vbat |
| `x` | หยุด |

**Output:** `FF_SPEED_L/R`, `FF_BIAS_L/R`, `FF_ACC`, **Km**, **Tm** (ส่งต่อ Lab6)

### Lab6 — PID
| คำสั่ง | ผล |
|---|---|
| `gaincalc <Km> <Tm>` | คำนวณ Kp/Kd อัตโนมัติ |
| `f` / `g` | FF+PD / FF อย่างเดียว |
| `turn 90` / `turn -90` / `turn 180` | ทดสอบหมุน |
| `dist <mm>` | ตั้งระยะวิ่ง (เช่น `dist 400`) |
| `p/o k/m` | Kp/Kd tuning |
| `d` | dump CSV |

---

## BLE Protocol (NUS Line Protocol)

```
Telemetry: "T,fwd,rot,heading,lPwm,rPwm,vbat"
Diagnostic: "D,F,fl,fr,l,r,turn_err,set_err"
Pose: "P,x,y,heading"
Log: "L,message"
```

---

## See Also

- [[micromouse]] — Lab guide ทีละขั้นตอน
- [[micromouse_architecture]] — Flowchart และ Sequence diagram
