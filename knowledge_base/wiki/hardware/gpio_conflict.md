# GPIO Conflict Table — KidBright32

> **หมวด:** Hardware Reference  
> **ไฟล์ดิบ (Raw):** `../raw/kidbright_sensor_guide.md`  
> **เชื่อมโยง:** [[kidbright32_pinout]] · [[adc_api_guide]] · [[formula_kid]] · [[skate]]

---

## หลักการ: GPIO ที่ต้องระวัง

บอร์ด KidBright32 ใช้ GPIO ร่วมกันระหว่างวงจรบนบอร์ดและพอร์ตภายนอก  
**ก่อนใช้ GPIO ใดก็ตาม ตรวจตารางนี้ก่อนเสมอ**

---

## V1.5 Rev 3.1 (NECTEC Standard)

| GPIO | ฟังก์ชันที่ถูกจองแล้ว | ใช้งานอื่นได้ไหม |
|------|----------------------|----------------|
| GPIO2 | Wi-Fi LED | ❌ ห้ามใช้งานอื่น |
| GPIO4 | BT LED **หรือ** LM73 SDA | ⚠️ เลือกได้แค่อย่างเดียว |
| GPIO13 | Passive Buzzer (LEDC/PWM) | ❌ ต้องใช้ PWM เสมอ |
| GPIO14 | **SW2 Button** | ❌ ห้ามใช้งานอื่น |
| GPIO16 | SW1 Button | ❌ ห้ามใช้งานอื่น |
| GPIO25 | USB Host (Active LOW) | ❌ อย่าใช้งานอื่น |
| GPIO36 | LDR ADC (Input-only) | ⚠️ Input-only ไม่มี pull |

---

## V1.5 Rev 3.1G (Gravitech OEM)

| GPIO | ฟังก์ชันที่ถูกจองแล้ว | ใช้งานอื่นได้ไหม |
|------|----------------------|----------------|
| GPIO2 | Wi-Fi LED | ❌ ห้ามใช้งานอื่น |
| GPIO4 | BT LED **หรือ** LM73 SDA | ⚠️ เลือกได้แค่อย่างเดียว |
| GPIO13 | Passive Buzzer (LEDC/PWM) | ❌ ต้องใช้ PWM เสมอ |
| GPIO14 | **SW2 Button** | ❌ ห้ามใช้งานอื่น |
| GPIO16 | SW1 Button | ❌ ห้ามใช้งานอื่น |
| GPIO25 | USB Host (Active LOW) | ❌ อย่าใช้งานอื่น |
| GPIO36 | LDR ADC (Input-only) | ⚠️ Input-only ไม่มี pull |

---

## V1.5 iA / V1.6 (Shared)

| GPIO | ฟังก์ชันที่ถูกจองแล้ว | ใช้งานอื่นได้ไหม |
|------|----------------------|----------------|
| GPIO2 | Wi-Fi LED | ❌ ห้ามใช้งานอื่น |
| GPIO4 | BT LED **หรือ** LM73 SDA | ⚠️ เลือกได้แค่อย่างเดียว |
| GPIO14 | **SW2 Button** | ❌ ห้ามใช้งานอื่น |
| GPIO16 | SW1 Button **หรือ** SERVO1 | ⚠️ เลือกได้แค่อย่างเดียว |
| GPIO36 | LDR ADC (Input-only) | ⚠️ Input-only ไม่มี pull |

---

## Formula Kid Controller — GPIO พิเศษ

> ดูรายละเอียดเพิ่มเติมที่ [[formula_kid]]

| GPIO | ฟังก์ชัน | ข้อจำกัด |
|------|---------|---------|
| GPIO36 | **S1 บน Controller (ไม่ใช่ LDR)** | Input-only · ไม่มี pull · ห้ามใช้ interrupt |
| GPIO39 | **S2 บน Controller** | Input-only · ไม่มี pull · ห้ามใช้ interrupt |

> ⚠️ Formula Kid Controller ใช้ S1=GPIO36 S2=GPIO39 **ไม่ใช่** SW1/SW2 ปุ่มบนบอร์ด

---

## กฎสรุป

1. GPIO36, GPIO39 = Input-only เสมอ — ห้ามกำหนดเป็น output
2. ถ้าใช้ ESP-NOW ห้ามใช้ interrupt บน GPIO36/39 — ใช้ polling เท่านั้น
3. GPIO14 = SW2 ทุกบอร์ดโดยไม่มีข้อยกเว้น
4. GPIO2 = Wi-Fi LED อย่าแตะ

---

**ดูเพิ่มเติม:** [[kidbright32_pinout]] | [[adc_api_guide]] | [[formula_kid]]
