# สร้าง firmware และ Simulator จากพฤติกรรมเดียวกัน

ใช้ `generate_device_program` สำหรับโปรเจกต์ **KidBright32/ESP32 classic reference ใหม่** ที่สร้างไฟล์ Simulator คู่กับเฟิร์มแวร์อัตโนมัติ ตัว tool รับ `program_json` schema เดียวกับ `generate_simulation` แล้วสร้าง `main/main.c` กับ `simulation/main.sim.js` จาก program เดียวกันผ่าน compiler ในเครื่อง ไม่ใช่ให้โมเดลแปล C เป็น JavaScript และไม่ใช่การรับรองบอร์ดจริง

ต้อง `create_project_workspace` ให้สำเร็จก่อนสำหรับคำขอสร้างใหม่ โปรเจกต์เก่าที่เป็น custom, มี source/component เพิ่ม หรือไฟล์ generated ที่แก้มือแล้ว จะถูกปฏิเสธการแทนด้วย template เพื่อรักษาโค้ดเดิม ให้ใช้ read_file/write_file สำหรับ firmware ที่ผู้ใช้สั่งแก้ และ generate_simulation สำหรับ companion การซ่อมซิมอย่างเดียว **ห้ามเปลี่ยน firmware เพื่อทำให้ซิมผ่าน**

## ความสามารถที่ใช้ร่วมกันได้

| งาน | schema / ความหมาย |
|---|---|
| LED bitmap | bitmap: columns 16 bytes; byte ต่อคอลัมน์ bit 0 อยู่ล่างตาม reference Unity |
| ตัวเลข | number: value expression, Math.round เป็นจำนวนเต็ม -999..9999; ไม่ใช่ font 5x7 เดิมของ custom firmware |
| ตัวอักษร/ข้อความ | char: ASCII code 32..126, text: แสดงทีละตัว, scroll: เลื่อนขวาไปซ้าย รองรับตัวพิมพ์ใหญ่/เล็ก ตัวเลข punctuation |
| ล้าง/รอจอ | clear, matrix_wait; sensor matrix_idle คืน true เพราะจอแบบ synchronous ในโปรไฟล์นี้ |
| อุณหภูมิ | sensor temperature, LM73 signed MSB-first /128 บน SDA4/SCL5; default address 0x4D เลือก 0x49 ได้ตามหลักฐานบอร์ด |
| แสง | sensor light: relative brightness 0..100 ตาม ADC endpoints ที่กำหนด ไม่ใช่ lux; ต้องสอบเทียบกับบอร์ดจริง |
| ปุ่ม | sensor s1/s2: 1=กด, 0=ปล่อย ทั้ง C และ JS; GPIO16/14 active LOW เฉพาะ classic reference |
| IN1–IN4 | sensor input1..input4: digital 0/1; GPIO32/33/34/35; analog1..4 ต้องใช้โปรไฟล์ kidbright32_v15_analog_bypass ที่ยืนยันทาง IN แล้ว |
| OUT1/OUT2/USB | output port out1/out2/usb และอ่านค่าที่เขียนผ่าน sensor ชื่อเดียวกัน; เป็นคนละช่อง ไม่ย้ายพัดลม USB ไป OUT1 |
| Buzzer แบบมีระยะเวลา | tone: hz expression 20..20000 และ ms 1..60000; note: 0..36 กับ duration 0..4 ที่120 BPM |
| เสียงพร้อมจอ/พอร์ต | tone_start หรือ note_start เริ่มเสียงโดยไม่รอ; ทำ LED/พอร์ตต่อได้ แล้ว buzzer_off หรือ rest เพื่อรอจบและหยุดเสียง |
| ระดับเสียง | volume: value 0..100; sensor volume อ่านค่าของโปรแกรม; volume/mute ในหน้า IDE เป็นการควบคุมเสียงในเครื่องแยกต่างหาก |
| การคำนวณ/ลอจิก | set, if/else, repeat, while, break, arithmetic/comparison/boolean/32-bit shifts ตาม schema; &&/\|\| short circuit และ === แยก boolean กับ number |

อ่านรูปแบบ JSON รายละเอียดใน `simulator_compiler.md` input schema ที่ไม่รองรับจะคืน error ไม่ถูกตัดออกเงียบ ๆ ทุก loop มี yield เพื่อให้ Stop ทำงาน; simulator มีขีดจำกัดเวลา/จำนวน step ตาม runtime เดิม

## ขั้นตอนการใช้งานของ AI

1. แยกคำขอสร้างใหม่/แก้เดิมจากข้อความจริง ระบุ board และช่องพัดลมจาก context; อย่าเดาจากภาพหรือการลาก Unity ที่ไม่ได้ถูกส่งให้โมเดล
2. สร้าง workspace ใหม่เมื่อผู้ใช้ขอ จากนั้นกำหนด **พฤติกรรมครบทั้งงาน** เป็น program_json รวม input/เงื่อนไข/ลำดับ/เวลา/พอร์ต/เสียง
3. เรียก generate_device_program ครั้งเดียว ทั้ง C และ JS จะถูกตรวจและสร้างผ่าน Diff pipeline เดิม หากมี Diff ให้ผู้ใช้ตรวจ Keep/Undo; ห้ามกดแทน
4. การแก้ต่อโปรแกรมที่สร้างด้วย tool นี้: เรียก `read_device_program` เพื่ออ่าน program และ hardware configuration เดิม รักษา step ที่ไม่ได้ขอแก้ แล้วส่ง program ฉบับเต็มที่ปรับแล้วกลับเข้า generate_device_program ไม่ต้องคัดลอก driver/font ทั้งก้อนผ่านโมเดลซ้ำ
5. หากพบ custom/edited firmware ให้รักษา source นั้นไว้ อย่าสลับไปสร้าง template ใหม่เพื่อหลบ error; การเปลี่ยนเป็น custom workflow ต้องรักษาโค้ดที่แก้มือและไม่อ้างว่าเป็น shared-behavior verified pair อีก
6. แจ้ง “สร้างคู่จากพฤติกรรมเดียวกันแล้ว ยังไม่ได้ Build/ทดสอบบอร์ดจริง” ผลตรวจ revision อย่างเดียวของ custom firmware ต้องระบุว่ายังไม่ยืนยัน semantic equivalence ห้ามอ้างว่าผ่าน Build/Flash จากการสร้างไฟล์

## ตัวอย่าง JSON

อุณหภูมิขึ้นจอทุก 250ms:

~~~json
{"version":1,"steps":[{"op":"repeat","body":[{"op":"number","value":{"kind":"sensor","name":"temperature"}},{"op":"delay","ms":250}]}],"unsupported":[]}
~~~

แสดง A–Z เลื่อนขวาไปซ้าย:

~~~json
{"version":1,"steps":[{"op":"repeat","body":[{"op":"scroll","text":"ABCDEFGHIJKLMNOPQRSTUVWXYZ","ms":80}]}],"unsupported":[]}
~~~

เสียง 1000Hz พร้อมแสดงข้อความ แล้วหยุด:

~~~json
{"version":1,"steps":[{"op":"tone_start","hz":1000},{"op":"text","text":"Hello e","ms":300},{"op":"buzzer_off"}],"unsupported":[]}
~~~

hardware_json เป็น JSON string optional:

~~~json
{"profile":"classic_reference","lm73_address":77,"light_bright":0,"light_dark":4095}
~~~

นี่คือ reference configuration ไม่ใช่ผลตรวจชิป/สอบเทียบจริง LM73 อนุญาต 0x49/0x4D เท่านั้น ADC endpoints ต้องอยู่ 0..4095 และไม่เท่ากัน ค่า default แสงใช้ full raw range ซึ่งต้องสอบเทียบก่อนเทียบหน่วย/threshold กับของจริง

## ขอบเขต firmware ที่ต้องแจ้ง

- C ใช้ ESP-IDF 5.5 new I2C master/ADC oneshot/LEDC และ init เฉพาะอุปกรณ์ที่ถูกใช้ ไม่ปน legacy/new I2C ภายในโปรแกรม ไม่ย้าย LM73 ไปบัสจอ GPIO21/22
- Matrix raw bytes interleave `[col0,col8,col1,col9,...]` โดยไม่ reverse ซ้ำ เพราะ shared reference font มี bit orientation ตรงกับ JS แล้ว สำหรับ custom firmware ใช้ font/mapping ของจริงต่อไป
- USB25 active LOW, OUT26/27 gate HIGH, buzzer13 เป็น **classic reference** ไม่อนุมาน revision/ไดรเวอร์ของ iA/iP/บอร์ดอื่นหรือ µAI จากชื่อ generic; µAI target ไม่ผ่าน paired template นี้
- I2C/ADC/LEDC failure ต้องแจ้ง error และหยุดอย่างชัดเจน ไม่แทนการอ่านเซนเซอร์ผิดพลาดด้วยค่า0 ค่าคำนวณ NaN/Infinity ห้ามใช้เปิด OUT/USB
- Sound เป็น monophonic square wave เสียง C ใช้ PWM volume โดยประมาณ ไม่ใช่การสอบเทียบความดังหรือรูปคลื่นเท่ากับ WebAudio การตั้งเวลาเกิดข้อจำกัดตาม FreeRTOS ticks/JS event loop
- ไม่มี Wi-Fi/Cloud, RTC, servo/motor PWM speed, accelerometer, DAC/PCM/MP3 หรือ parallel FreeRTOS task ใน shared schema งานที่ต้อง custom ESP_LOGI messages/driver อื่นนอกจาก port logs ของ template ให้ใช้ custom firmware workflow และทำ companion เฉพาะพฤติกรรมที่รองรับอย่างเปิดเผย
- ยังต้อง Build และทดสอบทางกายภาพแยกกัน ตัว compiler ไม่เรียกโมเดล/API ภายนอก ไม่ Flash และไม่ติดตั้ง dependency


## External sensors: original Analog and Digital ports

- Digital IN1–4: IR sensors input1..input4 → ports.input1_read()..input4_read(), logical 0/1.
- Analog IN1–4: IR sensors analog1..analog4 → original kb_specio.input1_read()..input4_read(), reading TextAnalogInputN populated by Unity Call_AnalogInputN. Raw finite 0..4095, truncated to integer like the original host. Keep Analog and Digital separate; external LDR is not the on-board ldr.get().
- Raw values do not establish moisture %, lux, gas ppm or real sensor accuracy. Display raw first, then apply explicitly supplied calibration/threshold and active polarity. Never invent physical units or a detection polarity.
- Paired analog firmware requires hardware_json {"profile":"kidbright32_v15_analog_bypass"}, after the user confirms V1.5 input assembly has U7/U8 absent and the 0Ω bypass path. Read kidbright32_v15.md/kidbright32_versions.md. Do not assume ADC on V1.3 buffered IN connectors or an unknown revision. Simulator-only draft examples are available without choosing physical hardware. For unknown boards ask before generating analog board drivers, preserve existing firmware.
- This profile uses one ADC1 unit shared by LDR GPIO36/channel0 and IN1–4 GPIO32/33/34/35/channels4/5/6/7, raw 12-bit readings. Different channels may mix analog/digital; a single physical IN cannot be two independent ports. GPIO34/35 have no internal pulls. Board wiring and calibration still need physical verification; do not auto-flash.
- The simulator panel includes Analog IN1 → raw LED and Digital IN1 → 0/1 LED examples. Stop, connect the intended accessory/channel/mode, run, then change its simulated stimulus and check the display. A model in the menu alone is not proof of a working sensor.


### เลือกอุปกรณ์ใน Unity ให้ตรงโหมด
Digital 6 รายการแรกในเมนู Sensor/เซ็นเซอร์ (ความชื้น, รายการวัดแสง/แรง, PIR, สิ่งกีดขวาง, ตรวจเส้น, แก๊ส/ควัน) ให้ลากตัวอุปกรณ์ไปช่อง IN1 สีเหลือง Digital ขณะ Stop แล้ว Run และเลื่อนตัวปรับค่าของอุปกรณ์ จึงอ่านด้วย sensor input1 ได้ ส่วน Analog ให้เปิด Plugins/ปลั๊กอิน เลือกความชื้น/แรง/น้ำฝนที่มีคำว่า (Analog) แล้วลากหัวต่อเข้า IN1 สีฟ้า จึงอ่านด้วย sensor analog1 ได้ ไม่ใช้ตัว Digital แทน Analog ชื่อ ForceSensor ใน Unity ตรงกับรายการที่ UI ไทยบางหน้าระบุว่า วัดระดับแสง จึงไม่อนุมานสเกล lux/แรงจากชื่อเมนู
