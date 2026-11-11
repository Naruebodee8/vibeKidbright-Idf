# สร้าง JavaScript สำหรับ Simulator อย่างตรวจสอบได้

สร้างไฟล์ซิมควบคู่กับโค้ดบอร์ดอัตโนมัติ ให้ใช้ generate_simulation หลังอ่านและเขียนเฟิร์มแวร์ครบ ตัว tool รับ program_json เป็นสตริง JSON แล้ว IDE แปลงเป็น ES5 ให้ JS-Interpreter ทำงานผ่าน API LED/พอร์ต/เซนเซอร์แบบ Simulator ต้นฉบับ จากนั้นส่งภาพ LED/สถานะไป Unity ไม่ส่ง C/C++ เข้า Unity

ตัว tool เขียนได้เฉพาะ simulation/main.sim.js ใช้ metadata, backup และ Keep/Undo ตามระบบเดิม ไม่แก้ main/, components/, CMake หรือ sdkconfig ห้ามแก้ firmware เพียงเพื่อให้จำลองได้ อ่านไฟล์ firmware ที่เกี่ยวข้องก่อนอธิบายพฤติกรรมเสมอ

## รูปแบบ

ส่ง argument program_json เป็น JSON string ของ object นี้:

~~~json
{"version":1,"steps":[{"op":"repeat","body":[{"op":"text","text":"ABCDEFGHIJKLMNOPQRSTUVWXYZ","ms":500}]}],"unsupported":[]}
~~~

ตัวอย่างนี้แสดง A-Z ทีละตัวทุก 500 ms แล้ววนซ้ำ ใช้ bitmap ฟอนต์ 6×8 ที่ตรวจตรงกับ workers/ht16k33.js ใน ZIP ต้นฉบับ หาก firmware ใช้ฟอนต์หรือการวางภาพต่างกัน ให้ใช้ bitmap ตามคอลัมน์ที่ firmware ส่งจริงแทน เพื่อให้ทั้งสองแสดงภาพเดียวกัน

## คำสั่งทั้งหมด

| op | fields | การทำงาน |
|---|---|---|
| tone | hz: expression ช่วง 20..20000, ms: 1..60000 | เสียง square wave ตามความถี่ รอตาม ms แล้วหยุด |
| note | note: 0..36, duration: 0..4 | โน้ต C4..C7 ตามตารางต้นฉบับ และจังหวะ 120 BPM |
| volume | value: expression ช่วง 0..100 | ตั้งระดับเสียงของโปรแกรม |
| buzzer_off | ไม่มี | หยุดเสียง buzzer |
| delay | ms: 0..60000 | รอเป็นมิลลิวินาที |
| bitmap | columns: array ของ byte 0..255 จำนวน 16 | ภาพ LED 16 คอลัมน์ต่อ 8 แถว |
| number | value: expression | แสดงจำนวนเต็ม -999..9999 ผ่าน helper เดิม |
| char | value: expression ของ ASCII code 32..126 | แสดงตัวอักษรหนึ่งตัว ป้อน 65 คือ A |
| text | text: ASCII 1..64 ตัว, ms | แสดงทีละตัว ไม่เลื่อน |
| scroll | text: ASCII 1..64 ตัว, ms | เลื่อนทีละคอลัมน์ด้วย bitmap + delay |
| output | port: out1/out2/usb, value: expression | เขียนสถานะพอร์ต 0 คือปิด ค่าอื่นคือเปิด |
| set | name: ชื่อตัวแปร, value: expression | ตั้งค่า ตัวแปรที่ประกาศด้วย set มีค่าเริ่ม 0 |
| repeat | count: 1..1000000 หรือ null/ไม่ใส่, body: steps | จำนวนรอบ หรือวนตลอดเมื่อไม่ใส่ count |
| while | condition: expression, body: steps | วนตามเงื่อนไข |
| if | condition: expression, then: steps, else: steps (optional) | เลือกทำตามเงื่อนไข |
| break | ไม่มี | ออกจาก repeat/while ที่ใกล้ที่สุด |

### Expression

- ตัวเลข finite หรือ boolean เช่น 25, true
- ตัวแปร: {"kind":"var","name":"n"}
- ค่าอุปกรณ์: {"kind":"sensor","name":"temperature"} รองรับ temperature, light, s1, s2, input1..input4, out1, out2, usb
- คำนวณ/เปรียบเทียบ: {"kind":"binary","operator":">","left":{"kind":"sensor","name":"temperature"},"right":35}
- unary: {"kind":"unary","operator":"!","value":{"kind":"sensor","name":"s1"}}
- Binary รองรับ + - * / % < <= > >= == != === !== && || & | ^ << >> >>>; unary รองรับ ! ~ + -
- แปลง pdMS_TO_TICKS(500) ใน firmware เป็น delay ms=500 ไม่คัดลอก macro C ลงในไฟล์ JS

ตัวอย่างอุณหภูมิ:

~~~json
{"version":1,"steps":[{"op":"repeat","body":[{"op":"number","value":{"kind":"sensor","name":"temperature"}},{"op":"delay","ms":250}]}]}
~~~

ตัวอย่างปุ่มควบคุมพอร์ตและตัวแปร:

~~~json
{"version":1,"steps":[{"op":"set","name":"n","value":0},{"op":"repeat","body":[{"op":"output","port":"out2","value":{"kind":"sensor","name":"s1"}},{"op":"number","value":{"kind":"var","name":"n"}},{"op":"set","name":"n","value":{"kind":"binary","operator":"+","left":{"kind":"var","name":"n"},"right":1}},{"op":"delay","ms":500}]}]}
~~~

## ขอบเขตความถูกต้อง

Compiler ตรวจชนิด field, ชื่อตัวแปร, op/อุปกรณ์, ขนาดภาพ, เวลา, ความลึกและขนาดโปรแกรม ก่อนสร้าง JS และเพิ่มจุด yield ในลูปเพื่อให้ Stop ตอบสนองได้ ปฏิเสธ delay-only placeholder/โปรแกรมที่ไม่มีคำสั่งแสดงผลหรือเขียนพอร์ต ใช้เฉพาะ API ที่ runtime มีอยู่จริง

นี่ไม่ใช่ compiler C/C++ ทั่วไป: AI ยังต้องจับคู่พฤติกรรมของ firmware กับคำสั่งที่รองรับ Compiler และ hash ไม่พิสูจน์ว่าทุกเส้นทางของสองโปรแกรมมี semantic equivalence หากมี Wi-Fi, FreeRTOS หลาย task, servo, อุปกรณ์เสริม หรือความสามารถที่ซิมไม่มี ให้ใส่ชื่อใน unsupported เช่น ["Wi-Fi MQTT"] แล้วแจ้งผู้ใช้ ห้ามตัดออกเงียบ ๆ แล้วอ้างว่าจำลองครบ

ภาษาไทย/Unicode ไม่อยู่ในฟอนต์ printable ASCII ชุดนี้ หากต้องการรูปอักษรเฉพาะ ให้ใช้ bitmap 16 bytes ที่กำหนดและตรวจสอบจริง กรณี expression ให้ codepoint/ค่าตัวเลขนอกช่วงที่ runtime รองรับ จะมี runtime error แสดงต่อผู้ใช้

การซ่อมไฟล์ซิมอย่างเดียว: อ่าน firmware เดิม สร้าง program_json ให้ตรง แล้วเรียก generate_simulation ไม่เขียน main.c ซ้ำ กด Keep เฉพาะไฟล์จำลอง จากนั้น Run Sim ระบบตรวจ revision อีกครั้ง การทดสอบบอร์ดจริงต้อง Build/Flash แยกต่างหาก

AI ต้องใช้ generate_simulation สำหรับไฟล์ companion การเรียก write_file ตรงมาที่ simulation/main.sim.js จะคืน error ก่อนเขียนไฟล์ เพื่อให้การสร้างซิมผ่าน compiler ที่ตรวจรูปแบบเสมอ โค้ดร่างในแผง Simulator ยังทดลอง ES5 API ที่รองรับได้


## Buzzer ในเครื่อง

tone สร้าง sound.tone(hz), vTaskDelay(ms), sound.off() เป็น extension ในเครื่องสำหรับเสียง PWM ของ ESP-IDF ไม่ใช่ Unity API ของต้นฉบับ ส่วน note สร้าง sound.note(note), sound.rest(duration) ตาม API ต้นฉบับ note 0=C4 (261 Hz), 9=A4 (440 Hz), 36=C7 (2093 Hz); duration 0/1/2/3/4 คือ 2000/1000/500/250/125 ms ที่ 120 BPM ใช้ volume ก่อน tone/note เพื่อกำหนดระดับเสียง

ตัวอย่างกด SW1 ให้บี๊บ 1000 Hz 200 ms แล้วรอปล่อยปุ่ม ใช้ตัวแปรเพื่อไม่เล่นซ้ำขณะกดค้าง:

~~~json
{"version":1,"steps":[{"op":"set","name":"pressed","value":false},{"op":"repeat","body":[{"op":"if","condition":{"kind":"binary","operator":"&&","left":{"kind":"sensor","name":"s1"},"right":{"kind":"unary","operator":"!","value":{"kind":"var","name":"pressed"}}},"then":[{"op":"tone","hz":1000,"ms":200}]},{"op":"set","name":"pressed","value":{"kind":"sensor","name":"s1"}},{"op":"delay","ms":20}]}],"unsupported":[]}
~~~

ตัวอย่างทดสอบโน้ตและระดับเสียง:

~~~json
{"version":1,"steps":[{"op":"volume","value":30},{"op":"note","note":0,"duration":2},{"op":"note","note":4,"duration":2},{"op":"note","note":7,"duration":2},{"op":"buzzer_off"}],"unsupported":[]}
~~~

Runtime ส่งสถานะ BUZZER ไปหน้า IDE ผ่าน protocol local ที่ตรวจ origin/session/run ID แล้ว IDE ใช้ Web Audio เล่นเสียงหลังคลิก Run ผู้ใช้ mute หรือปรับระดับเสียงรวมได้โดยไม่เปลี่ยน logic ของโปรแกรม Stop, Reset, error, จบโปรแกรม, ปิดหน้าซิม และ unmount ต้องหยุดเสียง ปุ่ม mute ไม่ได้ส่งกลับไปแก้ firmware

ซิมรองรับ buzzer แบบ monophonic square wave ช่วง 20..20000 Hz ไม่จำลอง duty cycle ที่เปลี่ยนได้, DAC/PCM/MP3, ลำโพงหลายช่อง, timbre ของ buzzer จริง หรือ audio peripherals อื่น หาก firmware ใช้ความสามารถเหล่านั้นให้ระบุใน unsupported อย่าตัดออกเงียบ ๆ และอย่าเปลี่ยนโค้ดบอร์ดเพียงเพื่อให้เข้ากับซิม firmware ยังคงใช้ LEDC/GPIO ของบอร์ดรุ่นนั้นตาม KB เขียน firmware ก่อน สร้าง companion หลัง และไม่ Build/Flash เพียงเพื่อทดสอบ local


## ครอบคลุมอุปกรณ์ด้วย program คู่ C/JS

โปรเจกต์ใหม่ที่รองรับใช้ `generate_device_program` และอ่าน `simulator_paired_programs.md` การแก้ต่อใช้ `read_device_program` ส่วน custom firmware/ซ่อมซิมอย่างเดียวใช้ `generate_simulation` โดยคง source เดิม อย่าปล่อย Hello boilerplate แล้วรายงานว่าสร้าง firmware ตามคำขอแล้ว

เพิ่ม operation ที่ใช้ API runtime เดิม: `tone_start{hz}` / `note_start{note}` เริ่มเสียงโดยไม่บล็อก; `rest{duration}` รอแล้วหยุดเสียง; `clear{}`; `matrix_wait{}` และ sensor `volume` / `matrix_idle` จึงทำเสียงพร้อม LED/พอร์ตได้โดยไม่แกล้งตัดระยะเวลารอออก รายละเอียด range/ขอบเขตเท่ากับ API เดิม


## External sensors: original Analog and Digital ports

- Digital IN1–4: IR sensors input1..input4 → ports.input1_read()..input4_read(), logical 0/1.
- Analog IN1–4: IR sensors analog1..analog4 → original kb_specio.input1_read()..input4_read(), reading TextAnalogInputN populated by Unity Call_AnalogInputN. Raw finite 0..4095, truncated to integer like the original host. Keep Analog and Digital separate; external LDR is not the on-board ldr.get().
- Raw values do not establish moisture %, lux, gas ppm or real sensor accuracy. Display raw first, then apply explicitly supplied calibration/threshold and active polarity. Never invent physical units or a detection polarity.
- Paired analog firmware requires hardware_json {"profile":"kidbright32_v15_analog_bypass"}, after the user confirms V1.5 input assembly has U7/U8 absent and the 0Ω bypass path. Read kidbright32_v15.md/kidbright32_versions.md. Do not assume ADC on V1.3 buffered IN connectors or an unknown revision. Simulator-only draft examples are available without choosing physical hardware. For unknown boards ask before generating analog board drivers, preserve existing firmware.
- This profile uses one ADC1 unit shared by LDR GPIO36/channel0 and IN1–4 GPIO32/33/34/35/channels4/5/6/7, raw 12-bit readings. Different channels may mix analog/digital; a single physical IN cannot be two independent ports. GPIO34/35 have no internal pulls. Board wiring and calibration still need physical verification; do not auto-flash.
- The simulator panel includes Analog IN1 → raw LED and Digital IN1 → 0/1 LED examples. Stop, connect the intended accessory/channel/mode, run, then change its simulated stimulus and check the display. A model in the menu alone is not proof of a working sensor.


### เลือกอุปกรณ์ใน Unity ให้ตรงโหมด
Digital 6 รายการแรกในเมนู Sensor/เซ็นเซอร์ (ความชื้น, รายการวัดแสง/แรง, PIR, สิ่งกีดขวาง, ตรวจเส้น, แก๊ส/ควัน) ให้ลากตัวอุปกรณ์ไปช่อง IN1 สีเหลือง Digital ขณะ Stop แล้ว Run และเลื่อนตัวปรับค่าของอุปกรณ์ จึงอ่านด้วย sensor input1 ได้ ส่วน Analog ให้เปิด Plugins/ปลั๊กอิน เลือกความชื้น/แรง/น้ำฝนที่มีคำว่า (Analog) แล้วลากหัวต่อเข้า IN1 สีฟ้า จึงอ่านด้วย sensor analog1 ได้ ไม่ใช้ตัว Digital แทน Analog ชื่อ ForceSensor ใน Unity ตรงกับรายการที่ UI ไทยบางหน้าระบุว่า วัดระดับแสง จึงไม่อนุมานสเกล lux/แรงจากชื่อเมนู

## รูปแบบ argument และการลองใหม่เมื่อ JSON ไม่ครบ

`program_json` เป็น **สตริง JSON ของโปรแกรมครบชุด** ทุก object ใน `steps` และภายใน `body` / `then` / `else` ต้องมี `op` เสมอ ส่วน expression ใช้ `kind` อยู่ใน `value` หรือ `condition` อย่าใส่ sensor/binary expression เปล่า ๆ เป็น statement ตัวอย่างการแปลงค่าดิบ Analog IN1 เป็นระดับความชื้นจำลอง 0–100:

~~~json
{"version":1,"steps":[{"op":"repeat","body":[{"op":"number","value":{"kind":"binary","operator":"/","left":{"kind":"binary","operator":"*","left":{"kind":"sensor","name":"analog1"},"right":100},"right":4095}},{"op":"delay","ms":100}]}],"unsupported":[]}
~~~

ตัวอย่างเป็นสเกลจำลองตามคำขอ ไม่ใช่ความชื้นจริงที่สอบเทียบแล้ว ใช้ `generate_simulation` หลัง firmware ที่ตรงกัน หรือ `generate_device_program` สำหรับโครงการที่เข้าเงื่อนไขและยืนยัน hardware profile แล้ว ข้อจำกัด board assembly/Analog ข้างต้นยังมีผล การแก้ JSON ไม่ใช่การอนุญาตให้สมมติโปรไฟล์บอร์ด

หากพบ `missing field op` ให้แก้ตำแหน่ง JSON ที่ error ระบุ แล้วส่งโปรแกรม **ครบชุด** ให้ tool เดิมอีกครั้ง รักษาความสามารถที่ผู้ใช้ขอทั้งหมด ห้ามเติม op โดยเดาหรือแก้เป็นการเขียน `.sim.js` ตรงเพื่อข้ามตัวตรวจสอบ ข้อมูลที่ไม่ผ่าน compiler ถูกปฏิเสธก่อนสร้างไฟล์ หากครั้งถัดไปสำเร็จ UI จะระบุว่าลองใหม่สำเร็จและเก็บข้อผิดพลาดเดิมไว้; หากยังผิดต้องรายงานตามจริง ไม่อ้างว่าจบงาน
