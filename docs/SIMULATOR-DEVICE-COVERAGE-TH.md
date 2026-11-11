# การขยาย Vibe Coding ให้ครอบคลุม Local Simulator

วันที่ 26 ตุลาคม 2026 — เปลี่ยนเฉพาะการสร้าง/ตรวจ firmware–simulator, คำแนะนำให้ AI, UI สถานะ และชุดทดสอบที่เกี่ยวข้อง ไม่เชื่อม Simulator API ภายนอก ไม่เรียกโมเดลจริง ไม่ Flash บอร์ด

## ข้อค้นพบและสิ่งที่แก้

ก่อนงานนี้ runtime รองรับหลายอุปกรณ์แล้ว แต่มีตัวสร้าง C/JS จากแผนเดียวกันเฉพาะพัดลม การสร้างซิมอย่างเดียวอาจถูกจับคู่กับ Hello boilerplate ได้ และการตรวจ hash ไม่ใช่การตรวจพฤติกรรมตรงกัน

เพิ่ม `generate_device_program` ให้รับ behavior JSON ชุดเดียวแล้วสร้าง `main/main.c` และ `simulation/main.sim.js` สำหรับ classic-reference KidBright32/ESP32 ผ่าน compiler ภายในเครื่อง ทั้งสองไฟล์ผ่าน schema/range/operation validation ก่อนเสนอ Diff เหมือนเดิม ไม่มีการ Keep/Undo แทนผู้ใช้

เพิ่ม `read_device_program` เพื่ออ่าน behavior/hardware configuration เดิมเวลาแก้ต่อ รักษาส่วนที่ผู้ใช้ไม่ได้สั่งแก้และลดการส่ง driver/font ทั้งก้อนให้โมเดลซ้ำ หาก firmware เป็น custom, ถูกแก้มือ หรือมี source/component เพิ่ม ตัวสร้าง template จะปฏิเสธการแทนที่ ให้ใช้ workflow custom firmware + companion โดยเก็บ source ไว้

ตรวจ Hello/ไฟล์ว่างก่อนสร้าง companion และก่อนรายงานงานเสร็จ โปรแกรมแบบ shared จะตรวจ C และ JS เทียบกับ behavior เดิมซ้ำ รวม pending Diff, metadata revision และก่อน Run Sim โปรแกรม custom แสดงสถานะว่า **ตรวจ revision เท่านั้น** ไม่อ้าง semantic equivalence หรือ Build ผ่าน

พบบั๊กจริงเพิ่มจากเทสต์และแก้:

- Unary `-` ของจำนวนติดลบ เช่น `-(-1)` เดิมปล่อย `--1` ทำให้ Acorn อ่านเป็น decrement ของ literal แล้ว parse ไม่ผ่าน แก้การแบ่ง token และเพิ่ม regression
- OUT/USB เดิมตีความ `Number(NaN) !== 0` ว่าเปิดพอร์ต เพิ่ม finite check ให้หยุดพร้อม error
- ตัวสร้าง buzzer รักษา tone_start ที่เรียกซ้ำใน loop โดยไม่รีเซ็ต PWM timer ซ้ำเมื่อความถี่/volume ไม่เปลี่ยน

## ความสามารถที่ครอบคลุม

| กลุ่ม | การสร้างและทดสอบ |
|---|---|
| LED | bitmap 16 bytes, ตัวเลข/เครื่องหมายลบ/การปัดเศษ, ASCII 95 glyphs รวม `e`, text ทีละตัว, scroll ขวาไปซ้าย, clear/wait/idle |
| เซนเซอร์ | อุณหภูมิรวมค่าติดลบ, relative brightness, เงื่อนไข/threshold |
| อินพุต | SW1/SW2 active-low → logical pressed=1, digital IN1–IN4, edge toggle/ปล่อย/กดค้างตาม behavior ที่ระบุ |
| เอาต์พุต | OUT1, OUT2, USB แยกกันและอ่าน logical state ที่เขียนกลับได้ |
| เสียง | tone/note แบบมีระยะเวลา, tone_start/note_start แบบไม่บล็อก, rest/off, volume/get_volume, เสียงทำงานพร้อม LED/พอร์ต |
| ลอจิก | ตัวแปร, arithmetic/comparison, boolean, strict equality, short circuit, 32-bit shifts, repeat/while/break |
| ข้อผิดพลาด | schema/unsupported/range, runtime value ผิดช่วง, NaN/Infinity, Hello-only, คู่ถูก stamp แต่ behavior ต่างกัน, target ไม่ใช่ ESP32, custom source protection |

เพิ่ม operation ตาม API ที่ runtime มีอยู่แล้ว: tone_start, note_start, rest, clear, matrix_wait และ sensor volume/matrix_idle ไม่เพิ่ม API Cloud/USB host/servo โดยอนุมาน

## ขอบเขตฮาร์ดแวร์

Template ใช้ **classic_reference** และ ESP-IDF 5.5: matrix GPIO21/22 address0x70; LM73 GPIO4/5 address0x4D หรือ0x49ตาม configuration; LDR ADC1_CH0 GPIO36; SW1/2 GPIO16/14; digital IN GPIO32–35; OUT26/27; USB25 active LOW; buzzer13. Init เฉพาะ resource ที่ program ใช้ ไม่ผสม legacy/new I2C

การ Build ผ่านไม่รับรอง wiring/PCB revision/ชนิดเซนเซอร์/USB polarity ของบอร์ดอื่น ไม่สร้าง iA/iP/µAI profile โดยอัตโนมัติ คง catalog เดิมไว้ ค่าแสง default เป็นการ map raw0..4095แบบ reference ไม่ใช่ค่าที่สอบเทียบหรือ lux ระดับเสียง PWM เป็นการประมาณ ไม่ใช่ acoustic calibration และเวลาขึ้นกับ tick/event-loop

Custom firmware ที่ใช้ font/driver อื่นต้องคงของเดิมและทำ companion ตาม source นั้น ไม่มีตัวแปล C/C++ ทั่วไป ไม่อ้างว่าจำลอง Wi-Fi, RTC, accelerometer, servo/motor speed PWM, analog IN, DAC/PCM/MP3 หรือ parallel FreeRTOS task ได้ งานนอก shared schema รวม custom log/driver ต้องอธิบายส่วนที่ไม่รองรับและใช้ custom workflow ไม่ตัดงานทิ้งเงียบ ๆ

## ผลตรวจจริง

- `cargo test --locked`: **124 passed**, 0 failed, 0 ignored
- `cargo check --locked`: ผ่าน
- `npx tsc --noEmit`: ผ่าน
- Frontend: **137 passed**, 18 ไฟล์ รวม board context, สถานะ shared/revision-only, save/Build/Flash และ Diff regression เดิม
- Runtime/JS-Interpreter: **26 passed** รวมเสียงพร้อมจอ/พอร์ตและ nonfinite output
- `npm run build:release`: ผ่าน รวม hotfix/public/bundle checks
- Cross backend: **19 กรณี** รัน C ที่ generator สร้างด้วย host GCC จริง เทียบกับ JS ที่รันผ่าน reference JS-Interpreter เทียบ frame bytes, sensor reads, outputs, tone/volume, delays/control flow รวม **222 operator boundary checks** จาก JavaScript oracle และ 5 error cases ที่ต้องล้มเหลวตามคาด รายงาน `src-tauri/target/device-program-tests/report.json`
- ESP-IDF v5.5.4: Build ผ่าน exit0 ทั้ง **6 ชุด**: ทุก peripheral ร่วมกัน, temperature+matrix, ASCII/text/clear, สองปุ่ม+ทุก digital IN/OUT, light threshold, buzzer notes/limits ใช้ `D:\kb_device_program_coverage_probe` แยกต่างหาก ตรวจ hash ยืนยัน source/config/companion ของ `D:\kb_fan_toggle_sw1` ไม่ถูกแก้
- Browser integration: compiled IDE + **Unity จริง** รับ source จาก shared compiler กด SW1/SW2 ด้วย mouse input, temperature25→42, distinct outputs, light/digital-input fixtures, toneพร้อมLED และ Stop ปิดเสียง ผ่าน ไม่มี page errors/external page requests รายงาน `src-tauri/target/device-program-ui-verification.json` รูป `device-program-live.png`
- WebAudio regression ผ่าน probe เดิม: ตรวจ audio/volume/mute/Stop/reset/close ด้วยเสียงสัญญาณใน browser รายงาน `src-tauri/target/buzzer-ui-verification.json`

Browser tests ใช้ native IPC และ physical-sensor fixtures; Unity, interpreter, compiled UI และ WebAudio เป็นของจริง ไม่ใช่การ Flash/อ่าน sensor จากบอร์ด ไม่ได้เรียก Gemini/OpenAI จริงเพื่อวัดการเลือก tool หรือคุณภาพภาษาของโมเดล

## ไฟล์ที่เปลี่ยนในรอบนี้

- ใหม่: `src-tauri/src/device_program.rs`, `device_program_core.c`, `device_program_idf.c`, `examples/generate-device-program.rs`
- Backend: `simulation_compiler.rs`, `simulation_font.rs` (แชร์ข้อมูลเดิม), `simulation_project.rs`, `hardware_contract.rs`, `ai_chat.rs`, `ai/providers.rs`, `ai/tools.rs`, `ai/workflow.rs`, `lib.rs`
- Frontend: `src/App.tsx` ส่ง board context, `src/AiChat.tsx` ส่ง context/แสดงระดับการตรวจ และ `src/test/aiHardware.test.tsx`
- Tests: `simulator-tests/device-program-cases.json`, `scripts/test-device-program.mjs`, `scripts/probe-device-program-ui.mjs`, `scripts/test-simulator.mjs` และ unit tests ใน Rust
- KB source+bundle: `simulator_paired_programs.md` ใหม่, `simulator_programs.md`, `simulator_compiler.md`, `support_scope.md`
- `package.json` เพิ่มคำสั่ง `test:device-program`; คู่มือ Local Simulator และรายงานนี้
- ไม่เพิ่ม dependency และไม่เปลี่ยน Cargo.toml/Cargo.lock/package-lock.json ในงานรอบนี้

## วิธีลอง

เปิดแอปจาก source ล่าสุด เลือก KidBright32 เปิด “สร้างไฟล์ Simulator คู่กับเฟิร์มแวร์” แล้วเริ่มด้วยคำขอเช่น:

> สร้างโปรเจกต์ KidBright32 ให้แสดง A–Z เลื่อนจากขวาไปซ้าย พร้อมไฟล์ซิม

> สร้างโปรเจกต์ KidBright32 แสดงอุณหภูมิบน LED ทุก 250 มิลลิวินาที พร้อมซิม

> สร้างโปรเจกต์ KidBright32 ให้ SW1 เปิดเสียง 1000Hz ขณะกด และแสดงอุณหภูมิบน LED พร้อมซิม

ตรวจทุก Diff แล้ว Keep เฉพาะที่ยอมรับก่อน Run Sim หากแก้โปรเจกต์ custom เดิม ระบบจะรักษา firmware เดิมและไม่อ้างว่าสร้างคู่จาก shared program

การรันเทสต์ซ้ำ: `npm run test:device-program` ต้องมี Cargo และ host GCC ใน PATH; browser probes ใช้ `VIBE_PROBE_TOOLS` ไปยัง isolated playwright-core และ Chromium ที่ติดตั้งไว้แล้ว ไม่มีขั้นติดตั้งหรือดาวน์โหลดอัตโนมัติ

## สิ่งที่ยังไม่ได้ทดสอบ/ความเสี่ยงที่เหลือ

ยังไม่ได้วัดคุณภาพ prompt ด้วย live model, Flash/ทดสอบไฟฟ้าบนบอร์ดจริง, สอบเทียบ LDR, ยืนยันชนิด/address ของ sensor หรือ polarity บน PCB ผู้ใช้ทุก revision, วัด physical buzzer waveform/volume และทดสอบ Linux/macOS ในงานนี้ ไม่รับรอง arbitrary custom C/C++ หรือพฤติกรรม/ปลั๊กอินนอกขอบเขต schema การแก้ C เองภายหลังต้องเก็บงานไว้และตรวจคู่ใหม่ ห้ามใช้การ stamp metadata แทนการพิสูจน์พฤติกรรม


ตรวจ schema ของฟังก์ชันไม่มี argument สำหรับ Google ให้เว้น parameters พร้อม unit check ตาม [Google FunctionDeclaration reference](https://googleapis.github.io/js-genai/release_docs/interfaces/types.FunctionDeclaration.html) โดยไม่ได้ส่ง live GenerateContent request เพื่อยืนยันคุณภาพโมเดลหรือ quota

ค่าคงที่ของ char/number ที่ผิดชนิดหรือเกินช่วงถูกปฏิเสธก่อนสร้างไฟล์ ส่วนค่าที่คำนวณ/อ่านขณะรันมี runtime guards และเคสทดสอบ error แยกต่างหาก
