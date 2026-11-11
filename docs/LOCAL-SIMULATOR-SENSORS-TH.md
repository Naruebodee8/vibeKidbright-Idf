# เซ็นเซอร์ใน Local Simulator: วิธีลองและผลตรวจ

อัปเดต 27 ตุลาคม 2026 — แก้ในโปรเจกต์หลัก vibeKidbright รุ่นพัฒนา 3.7.1 ทดลองในเครื่อง ไม่มีการเชื่อม Simulator API ภายนอก

## ลองได้ทันทีโดยไม่เรียก AI

### Digital: อุปกรณ์ 6 ตัวในภาพ

1. เปิด Run Simulator แล้วกด Stop ถ้ากำลังรันอยู่
2. เลือกตัวอย่าง **Digital IN1 → 0/1 บน LED** ในแถบซ้าย
3. ในภาพ Unity เปิด **Sensor/เซ็นเซอร์** เลือกอุปกรณ์ แล้วลากตัวอุปกรณ์ไปวางในกรอบ **IN1 สีเหลือง Digital** ให้เห็นการต่อสาย
4. กด Run Simulator แล้วเลื่อนตัวปรับค่าที่ปรากฏข้างอุปกรณ์ เลขบนจอ LED ต้องเปลี่ยน 0/1
5. ต้องให้ช่องในโค้ดตรงกับช่องที่ต่อ หากเลือก IN2 ใช้ ports.input2_read() หรือให้ AI สร้างสำหรับ IN2

### Analog: เลือกอุปกรณ์คนละรายการ

1. กด Stop แล้วเลือกตัวอย่าง **Analog IN1 → ค่าดิบบน LED**
2. เปิด **Plugins/ปลั๊กอิน** ใน Unity เลือกความชื้น/แรง/น้ำฝนที่มีคำว่า **(Analog)**
3. ลากบริเวณหัวต่อของอุปกรณ์ไปวางในกรอบ **IN1 สีฟ้า Analog** จนต่อสายสำเร็จ แล้วกด Run
4. เลื่อนตัวปรับค่าของอุปกรณ์ เลข LED ต้องเปลี่ยนตามค่าดิบ 0..4095 รุ่น Digital ในเมนู Sensor ใช้แทนรายการ Analog นี้ไม่ได้

ตัวอย่างเหล่านี้เป็นโค้ดร่างแยกจากเฟิร์มแวร์ การเลือก/รันไม่ได้เขียน main.c หรือ Keep/Undo ให้ผู้ใช้ หากจะ Run โหมดโปรเจกต์ต้องสร้างคู่ไฟล์และตรวจรับตาม workflow เดิม

## พรอมพ์สั้น

> สร้างโปรเจกต์ใหม่สำหรับ KidBright32 อ่าน Digital IN1 แล้วแสดง 0 หรือ 1 บน LED Matrix ทุก 100 มิลลิวินาที พร้อมไฟล์ซิม

สำหรับ Analog ที่จะ Build ลงบอร์ดด้วย ต้องยืนยันรุ่นและวงจร IN ก่อน ตัวอย่างเมื่อยืนยันจริงแล้ว:

> สร้างโปรเจกต์ KidBright32 V1.5 ที่ยืนยันแล้วว่าช่อง IN เป็นวงจร bypass รองรับ Analog อ่านค่าดิบ Analog IN1 แสดงบน LED ทุก 100 มิลลิวินาที พร้อมไฟล์ซิม

หากไม่ทราบรุ่น/การประกอบ ให้ลองตัวอย่าง Analog ในหน้าซิมก่อน และให้ AI สอบถามก่อนสร้าง driver บอร์ด ห้ามอนุมานว่าบอร์ดทุกตัวอ่าน ADC ผ่าน IN ได้

## อุปกรณ์ที่ทดสอบผ่านจาก Unity จริง

ทดสอบ compiled IDE + Unity build จาก ZIP ด้วยเมาส์จริง ลากอุปกรณ์เข้าช่อง IN1 แล้วเลื่อน slider ตรวจ callback และเลขบน LED โดยไม่แทนค่าที่เซ็นเซอร์ส่ง Native IPC ของ IDE/AI/บอร์ดยังเป็น fixture และไม่ได้ต่อเซ็นเซอร์จริง

| รายการ | เมนูและโหมด | ค่าที่เห็นบน LED |
|---|---|---|
| ความชื้น | Sensor, Digital | 0 → 1 |
| รายการแสง/แรง (ชื่อ Unity: ForceSensor) | Sensor, Digital | 0 → 1 |
| PIR ตรวจการเคลื่อนไหว | Sensor, Digital | 0 → 1 |
| ตรวจสิ่งกีดขวาง | Sensor, Digital | 0 → 1 |
| ตรวจเส้นขาวดำ | Sensor, Digital | 0 → 1 |
| แก๊สและควัน | Sensor, Digital | 0 → 1 |
| ความชื้น (Analog) | Plugins, Analog | 0 → 4095 |
| แรง (Analog) | Plugins, Analog | 0 → 4095 |
| น้ำฝน (Analog) | Plugins, Analog | 0 → 4095 |

หลักฐานที่สร้างซ้ำได้: scripts/probe-sensor-accessories-ui.mjs → src-tauri/target/sensor-accessories-verification.json และภาพ *-before.png/*-after.png ภายใต้ target/ การทดสอบนี้ไม่ยืนยันความแม่นยำหรือหน่วยกายภาพของเซ็นเซอร์

## สิ่งที่เปลี่ยน

- เพิ่ม API เดิม kb_specio.input1_read()..input4_read() อ่าน TextAnalogInputN ที่ Unity Call_AnalogInputN อัปเดต แยกจาก Digital TextInputN และ LDR บนบอร์ด ตรวจค่าจำกัด 0..4095 และตัดเศษทศนิยม
- เพิ่ม sensor analog1..analog4 ใน IR ที่สร้าง C/JS ร่วมกัน รวมเงื่อนไข/การคำนวณตาม schema เดิม ไม่ใช่ตัวแปลง C/C++ ทั่วไป
- เพิ่ม firmware profile kidbright32_v15_analog_bypass สำหรับ ADC1 GPIO32/33/34/35 ช่อง4/5/6/7 เมื่อยืนยัน input assembly เท่านั้น ใช้ ADC1 handle เดียวร่วมกับ LDR ช่อง0 และปฏิเสธการอ้าง Analog/Digital เป็นคนละอินพุตบน IN ช่องเดียวกัน
- รักษา template C เดิมสำหรับโปรแกรมที่ไม่ใช้ Analog และเพิ่ม variant แยกสำหรับ Analog เพื่อไม่ให้โปรเจกต์เก่าถูกมองว่าแก้ firmware เอง ตรวจ SHA-256 ของ C และ JS เดิม 19 โปรแกรมแล้วตรงทุก byte
- อัปเดตคำสั่ง AI, ชื่อ feature ในแชท, ตัวอย่างหน้าซิม และ KB ทั้งต้นฉบับกับไฟล์ที่ bundle

อ้างอิงจาก ZIP เดิม: app/public/js/simulator/simulator.js ส่วน Analog callback และ interpreter init; kidbright32/kb_specio.js; blockly/kidbright/generators-specio.js; UnityBuild/index.html Call_InputN / Call_AnalogInputN หลักฐานทาง IN ของบอร์ดอยู่ใน knowledge_base/kidbright32_v13.md, kidbright32_v15.md และ kidbright32_versions.md

## ผลตรวจ

| การตรวจ | ผล |
|---|---|
| cargo test --locked | 132 ผ่าน ไม่มี failed/ignored |
| cargo check --locked | ผ่าน |
| npx tsc --noEmit | ผ่าน |
| Vitest frontend | 139 ผ่าน ใน 18 ไฟล์ |
| node --test scripts/test-simulator.mjs | 29 ผ่าน รวม Analog ทุกช่อง, เปลี่ยนค่าระหว่างรัน, Stop/retry, ค่าผิดรูปแบบ/นอกช่วง |
| node scripts/test-device-program.mjs | 23 โปรแกรม: รัน C จริงด้วย hardware fixture เทียบ JS-Interpreter จริง ผ่าน |
| golden C/JS เดิม | 19 โปรแกรมไม่เปลี่ยนทุก byte |
| npm run build:release | ผ่าน รวม hotfix/public/bundle checks |
| node scripts/probe-device-program-ui.mjs | ผ่านทั้งชุดเดิมและเลือกตัวอย่าง Analog 2048→4095, Digital 0→1 ผ่าน callback Unity จริง โดยค่ากระตุ้นของ probe นี้เป็น fixture; ไม่เขียน firmware |
| node scripts/probe-sensor-accessories-ui.mjs | ผ่านทั้ง 9 รายการจากเมาส์และอุปกรณ์ Unity จริง ไม่มี sensor-value fixture/คำขอภายนอก |
| ESP-IDF 5.5.4 build แยก | ผ่าน 2 โปรแกรม: Analog ครบ4ช่อง+LDR; Analog+Digitalคนละช่อง+Buzzer+USB/OUT2 |

ชุดทดสอบ fan เดิมเคย timeout ในรอบที่รันพร้อม headless Unity หลายตัว รอบแยกผ่านครบ 29 ข้อโดยไม่ได้เปลี่ยน timeout หรือลอจิก fan

## ข้อจำกัดที่ยังเหลือ

- ไม่ได้เรียกโมเดล AI จริงเพื่อยืนยันการเลือก tool/ความเข้าใจพรอมพ์ และไม่ได้ทดสอบ rate limit
- ไม่ได้แฟลชบอร์ด/วัดแรงดัน/ต่อเซ็นเซอร์จริง หรือทดสอบ macOS/Linux การ Build ผ่านไม่ยืนยัน hardware wiring/ADC accuracy
- V1.3 มี digital buffer; V1.5 บาง assembly ต้องตรวจ U7/U8=N/A และทาง bypass จริง โปรไฟล์นี้ไม่ได้ทำให้บอร์ดที่มี buffer อ่าน Analog ได้
- ค่าดิบไม่ใช่ความชื้น %, lux, แรง หรือแก๊ส ppm ต้องมี calibration/active polarity ของอุปกรณ์จริงก่อนใช้หน่วยเหล่านั้น
- เมนูอื่นนอกจาก 9 รายการนี้ไม่ได้ทดสอบครบ การมีโมเดลอยู่ใน Unity ไม่ยืนยันว่ามี API ในตัวสร้างโค้ด เซอร์โว/PWM/Wi-Fi/FreeRTOS หลาย task ยังไม่เพิ่มในงานนี้
- โปรเจกต์ custom C ยังใช้การจับคู่ revision เป็นหลัก ไม่มีการพิสูจน์ว่า C ที่ AI เขียนเองตรงกับ JS ทุกบรรทัด โปรแกรมที่สร้างผ่าน shared IR เท่านั้นที่มีการสร้างพฤติกรรมร่วมกัน
- template Analog แยกไว้เพื่อรักษาความเข้ากันได้เดิม การแก้ driver ในอนาคตต้องตรวจทั้งสอง variant และทดสอบ golden/SDK ซ้ำตามส่วนที่เปลี่ยน

## ไฟล์ที่แก้รอบนี้

simulator-assets/runtime.js, simulator-assets/runtime-core.js; src-tauri/src/simulation_compiler.rs, device_program.rs, simulation_project.rs และ device_program_analog_idf.c (ใหม่); src/SimulatorPanel.tsx, src/AiChat.tsx; scripts/test-simulator.mjs, test-device-program.mjs, probe-device-program-ui.mjs และ probe-sensor-accessories-ui.mjs (ใหม่); simulator-tests/device-program-cases.json และ device-program-legacy-sha256.json (ใหม่); knowledge_base/ และ resources/knowledge_base/ เฉพาะ simulator_programs.md, simulator_compiler.md, simulator_paired_programs.md, support_scope.md; เอกสารนี้และ LOCAL-SIMULATOR-3.7.1-TH.md

ไม่ได้เพิ่ม dependency หรือแก้ Cargo.toml/Cargo.lock/package.json ในรอบนี้ ไม่เปลี่ยนโค้ด Build/Flash และไม่แก้เฟิร์มแวร์โปรเจกต์ผู้ใช้ ตรวจ hash main/main.c, main/CMakeLists.txt, CMakeLists.txt และ sdkconfig.defaults ของโปรเจกต์ที่เปิดระหว่างทดสอบ SDK แล้วยังตรงเดิม
