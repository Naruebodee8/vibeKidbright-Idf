# โปรแกรมจำลอง KidBright ใน IDE (local-only)

เมื่อสร้าง/แก้ firmware ที่ใช้ LED matrix, อุณหภูมิ, แสง, ปุ่ม หรือ OUT/USB ให้สร้างไฟล์ `simulation/main.sim.js` ควบคู่กันผ่าน `write_file` หลังเขียน firmware ครบ ใช้ ES5 และ device API ที่รองรับเท่านั้น ไฟล์นี้เป็นโปรแกรมอีกชุด ไม่ใช่ผลแปลง C/C++ และไม่ได้รับรองว่าบอร์ดจริงทำงานตรงกับผลจำลอง

## อุณหภูมิแสดงบน LED 16×8

```javascript
while (1) {
  matrix.show_number(lm73_1.get());
  vTaskDelay(250);
}
```

`matrix.show_number` เป็น helper ของ IDE รันฝั่ง browser แล้วส่ง bitmap ให้ Unity ไม่ใช่ API ดั้งเดิมของ Unity ปัดจำนวนเต็มด้วย Math.round และแสดง -999 ถึง 9999; ไม่รับ NaN/Infinity/ข้อความ หน้าจอใช้ฟอนต์ 3×5 จัดกลาง LED 16×8 รองรับเครื่องหมายลบ ยังไม่แสดงทศนิยม

ผู้ใช้กด Run Sim ในแชทหรือเปิด TOOLS → KidBright Simulator ระบบอ่านไฟล์ประจำโปรเจกต์โดยอัตโนมัติ แล้วกด Run Simulator ครั้งเดียว ระบบสั่งเริ่ม Unity และรอ callback จริง ไม่ต้องกด Play ในภาพบอร์ดซ้ำ ถ้าเริ่มไม่ได้ภายใน 15 วินาทีจะแจ้ง error; Stop ยกเลิกการเริ่มงานที่ยังรอได้ เลือกแหล่งอุณหภูมิเป็นค่าทดสอบใน IDE (ค่าเริ่ม 25, ช่วงทดสอบ -100..150) หรือเซนเซอร์ใน Unity ช่วงค่าทดสอบเป็นข้อจำกัด UI ไม่ใช่การรับรองช่วง/ความแม่นยำเซนเซอร์จริง

## API ที่ใช้ได้

- `vTaskDelay(ms)` 0..60000 และ `portTICK_RATE_MS = 1`
- `lm73_1.get()`, `ldr.get()`
- `button12.is_sw1_pressed()`, `is_sw2_pressed()`, `is_sw1_released()`, `is_sw2_released()`, `sw1_get()`, `sw2_get()`
- `ports.output1_write(v)`, `output2_write(v)`, `usbsw_write(v)` และ read ของแต่ละช่อง
- `ports.input1_read()` ถึง `input4_read()`
- `ht16k33.show(buf)` รับสตริง 16 ไบต์หนึ่งคอลัมน์ต่อไบต์, `idle()`, `wait_idle()`
- `matrix.show_number(value)` ตามข้างต้น

ไม่รองรับฟังก์ชันที่ผู้ใช้ประกาศเอง, dynamic calls, Math/String calls, modules/imports, eval, browser/Node/network, ht16k33.scroll() โดยตรง, Wi-Fi, tasks หรือปลั๊กอิน ถ้างานเกินขอบเขตให้แจ้งส่วนที่จำลองไม่ได้ ห้ามสร้างตัวอย่างหลอกว่าแทนพฤติกรรม firmware ทั้งหมดได้

## การจับคู่รุ่นโค้ดและ Diff

`write_file` ใส่ metadata comment บรรทัดแรกของ `simulation/main.sim.js` ให้อัตโนมัติ ประกอบด้วย hash ของเนื้อหา simulation และไฟล์ project-owned ใน main/components พร้อม CMake/config ที่รองรับ โดยอ่านเนื้อหาใหม่จาก pending Diff เมื่อมี ห้าม AI สร้างหรือแก้ hash เอง เขียน firmware ก่อน และ simulation เป็นไฟล์สุดท้าย

ถ้าเป็นไฟล์เดิมต้องตรวจ Diff และกด Keep/Undo ทั้งสองส่วนก่อน Run ถ้าแก้ firmware/headers/build inputs ภายหลัง การ Run โหมดโปรเจกต์จะปฏิเสธไฟล์จำลองเก่า ต้องให้ AI อัปเดต companion ใหม่ ห้ามเพียงเปลี่ยน metadata เพื่ออ้างว่า logic ตรงกัน Hash ตรวจรุ่นของไฟล์ ไม่ใช่การพิสูจน์ semantic equivalence และไม่ได้ครอบคลุม source ใน SDK/managed dependencies

ห้ามเรียก build/flash หรือ Simulator API ภายนอกเพียงเพราะผู้ใช้ขอจำลอง การจำลองครั้งนี้ทำภายในเครื่องเท่านั้น


## ตัวสร้าง JavaScript ที่ตรวจรูปแบบได้

อ่าน simulator_compiler.md และใช้ generate_simulation เป็นหลัก IDE แปลงคำสั่ง LED/พอร์ต/เซนเซอร์ เงื่อนไข ลูป ตัวแปรและคำนวณ เป็น ES5 ผ่าน API เดิม มี text/char/scroll ที่สร้างเป็น bitmap โดยใช้ฟอนต์จาก ZIP ตรวจว่าพฤติกรรมตรงกับ firmware ไม่เขียน firmware ทับเมื่อซ่อมไฟล์ซิมอย่างเดียว หากมีส่วนที่ไม่รองรับให้แจ้งผ่าน unsupported ห้ามแทนด้วยลูปรอเวลาเปล่า


## เสียง buzzer

รองรับ sound.note(0..36), sound.rest(0..4), sound.off(), sound.get_volume(), sound.set_volume(0..100) ตาม host ต้นฉบับ จังหวะ rest ใช้ 120 BPM คงที่ รองรับ sound.tone(hz) ช่วง 20..20000 เป็น extension ของซิมเรา ใช้ generate_simulation ด้วย op tone/note/volume/buzzer_off ตาม simulator_compiler.md แทนการส่ง raw companion

เสียงถูกสร้างด้วย Web Audio ในหน้า IDE จากเหตุการณ์ BUZZER ของ runtime ไม่ต้องเชื่อม API ภายนอกหรือให้ Unity อ่าน C/C++ โดยตรง ตัวอย่างเสียงแบบร่างที่ลองได้:

~~~javascript
sound.set_volume(30);
sound.note(0); sound.rest(2);
sound.note(4); sound.rest(2);
sound.note(7); sound.rest(2);
sound.off();
~~~

เสียง buzzer ตาม Hz:

~~~javascript
sound.tone(1000);
vTaskDelay(200);
sound.off();
~~~

sound.note/tone เปิดเสียงค้างจน off/rest ครบเวลา หรือโปรแกรมจบ Stop/Reset/ปิดหน้าซิมต้องหยุดเสียงทุกครั้ง เสียงเป็น square wave ช่องเดียว ไม่จำลอง PWM duty ที่ปรับได้หรือระบบเสียงทุกชนิด ห้ามอ้างว่ารองรับ DAC/PCM/ไฟล์เพลง/หลายช่องเสียง


## SW1 ควบคุมพัดลม: ต้องระบุช่องจริง

- พัดลมอุปกรณ์สำเร็จรูปของ Unity ที่มีหัว USB ใช้ช่อง **USB** (`ports.usbsw_write`) ไม่ใช่ OUT1 หรือ OUT2 การลากอุปกรณ์ใน Unity ยังไม่ได้ส่ง inventory/ภาพให้ AI อัตโนมัติ
- ระบุช่องในข้อความ หรือใช้แผนที่บันทึกไว้ของโปรเจกต์เดิม เช่น `ให้ SW1 สลับเปิดปิดพัดลม USB พร้อม log` ถ้าไม่ทราบช่องให้ถามผู้ใช้ ห้ามเดา OUT1
- `generate_fan_control` เป็น recipe เฉพาะ SW1 สลับเปิด/ปิดพัดลม มี stable-sample debounce และ Fan ON/OFF logs โดยสร้าง `main/main.c` และสร้าง companion จาก IR ชุดเดียวกันโดยอัตโนมัติเมื่อสร้างหรือแก้พฤติกรรมที่รองรับ
- ใช้ recipe นี้เมื่อครอบคลุมคำขอทั้งหมดเท่านั้น ถ้ามีการทำงานอื่นอยู่แล้วต้องอ่านและรักษาไว้ ใช้การเขียนโค้ดและ `generate_simulation` ตามปกติพร้อมตรวจช่อง ห้ามทับพฤติกรรมเดิมเพื่อให้ผ่าน validator

| ช่อง | GPIO ของ classic KidBright อ้างอิง | Simulator |
|---|---:|---|
| USB power switch | 25 | `ports.usbsw_write(0/1)` |
| OUT1 MOSFET gate | 26 | `ports.output1_write(0/1)` |
| OUT2 MOSFET gate | 27 | `ports.output2_write(0/1)` |

Simulator ใช้ 1=เปิด, 0=ปิด ส่วน GPIO ต้องแปลงตามวงจร: recipe classic ใช้ USB active LOW และ OUT gate HIGH ตาม reference classic; **ต้องยืนยันรุ่น/วงจรบอร์ดจริงก่อนใช้กับรุ่นอื่น** USB power switch ไม่ใช่ USB host data API

ตัวตรวจช่องรองรับ `gpio_set_level` และ alias แบบง่ายใน `main/main.c` พร้อมตรวจ API ของ companion ไม่ใช่ตัวแปล C/C++ หรือหลักฐานว่า custom driver ทุกแบบทำงานจริง ถ้าตรวจไม่ได้ให้รายงานข้อจำกัด ห้ามรายงาน Build/Flash ผ่านโดยไม่มีผลรันจริง


## ครอบคลุมอุปกรณ์ด้วย program คู่ C/JS

โปรเจกต์ใหม่ที่รองรับใช้ `generate_device_program` และอ่าน `simulator_paired_programs.md` การแก้ต่อใช้ `read_device_program` ส่วน custom firmware/ซ่อมซิมอย่างเดียวใช้ `generate_simulation` โดยคง source เดิม อย่าปล่อย Hello boilerplate แล้วรายงานว่าสร้าง firmware ตามคำขอแล้ว

เพิ่ม operation ที่ใช้ API runtime เดิม: `tone_start{hz}` / `note_start{note}` เริ่มเสียงโดยไม่บล็อก; `rest{duration}` รอแล้วหยุดเสียง; `clear{}`; `matrix_wait{}` และ sensor `volume` / `matrix_idle` จึงทำเสียงพร้อม LED/พอร์ตได้โดยไม่แกล้งตัดระยะเวลารอออก รายละเอียด range/ขอบเขตเท่ากับ API เดิม


### Firmware-only outcome for unsupported simulator capabilities
After writing complete real firmware for unsupported behavior (servo/PWM, Wi-Fi, Bluetooth, RTC, accelerometer, parallel tasks, networking), call report_simulation_unsupported with capabilities from its schema. It is read-only and returns firmware available, simulationReady=false, Build/physical board unchecked. Explain this partial outcome and pin/revision uncertainty. Do not invent a servo API, use OUT on/off as servo angle control, or generate a dummy .sim.js. The declaration is tied to the current/proposed firmware revision and invalidated by later code generation; re-report after edits. Supported code still requires a valid paired companion. Never replace existing/custom firmware to fit the simulator.


## External sensors: original Analog and Digital ports

- Digital IN1–4: IR sensors input1..input4 → ports.input1_read()..input4_read(), logical 0/1.
- Analog IN1–4: IR sensors analog1..analog4 → original kb_specio.input1_read()..input4_read(), reading TextAnalogInputN populated by Unity Call_AnalogInputN. Raw finite 0..4095, truncated to integer like the original host. Keep Analog and Digital separate; external LDR is not the on-board ldr.get().
- Raw values do not establish moisture %, lux, gas ppm or real sensor accuracy. Display raw first, then apply explicitly supplied calibration/threshold and active polarity. Never invent physical units or a detection polarity.
- Paired analog firmware requires hardware_json {"profile":"kidbright32_v15_analog_bypass"}, after the user confirms V1.5 input assembly has U7/U8 absent and the 0Ω bypass path. Read kidbright32_v15.md/kidbright32_versions.md. Do not assume ADC on V1.3 buffered IN connectors or an unknown revision. Simulator-only draft examples are available without choosing physical hardware. For unknown boards ask before generating analog board drivers, preserve existing firmware.
- This profile uses one ADC1 unit shared by LDR GPIO36/channel0 and IN1–4 GPIO32/33/34/35/channels4/5/6/7, raw 12-bit readings. Different channels may mix analog/digital; a single physical IN cannot be two independent ports. GPIO34/35 have no internal pulls. Board wiring and calibration still need physical verification; do not auto-flash.
- The simulator panel includes Analog IN1 → raw LED and Digital IN1 → 0/1 LED examples. Stop, connect the intended accessory/channel/mode, run, then change its simulated stimulus and check the display. A model in the menu alone is not proof of a working sensor.


### เลือกอุปกรณ์ใน Unity ให้ตรงโหมด
Digital 6 รายการแรกในเมนู Sensor/เซ็นเซอร์ (ความชื้น, รายการวัดแสง/แรง, PIR, สิ่งกีดขวาง, ตรวจเส้น, แก๊ส/ควัน) ให้ลากตัวอุปกรณ์ไปช่อง IN1 สีเหลือง Digital ขณะ Stop แล้ว Run และเลื่อนตัวปรับค่าของอุปกรณ์ จึงอ่านด้วย sensor input1 ได้ ส่วน Analog ให้เปิด Plugins/ปลั๊กอิน เลือกความชื้น/แรง/น้ำฝนที่มีคำว่า (Analog) แล้วลากหัวต่อเข้า IN1 สีฟ้า จึงอ่านด้วย sensor analog1 ได้ ไม่ใช้ตัว Digital แทน Analog ชื่อ ForceSensor ใน Unity ตรงกับรายการที่ UI ไทยบางหน้าระบุว่า วัดระดับแสง จึงไม่อนุมานสเกล lux/แรงจากชื่อเมนู
