# Local Simulator บน Classic 3.7.1

ฐานงาน: tag `v3.7.1`, commit `402116994f1fdc6eae40572c099772ce137186fd` บน branch `feat/local-simulator-3.7.1` ไม่เปลี่ยนเลขรุ่น และยังไม่เผยแพร่ release

## บั๊กที่ยืนยันและแก้ก่อนเพิ่มซิม

1. `handleBuildFlash` ไม่รอ Save และ `saveAllFiles` จับ error แล้วไม่ส่งต่อ แก้ให้ lifecycle รอการเขียนไฟล์ dirty สำเร็จทั้งหมดก่อนเรียกคำสั่งเดิม ถ้า Save ล้มเหลวต้องรายงาน failed และไม่ Build/Flash มี ref กันกดซ้ำตั้งแต่ช่วง Save
2. ตัวตรวจ hotfix ใช้ regex ที่ต้องเป็น LF ทำให้ clone บน Windows ที่มี CRLF ไม่ผ่าน แม้ Cargo.lock เป็น 3.7.1 จริง แก้ normalize เฉพาะข้อความที่อ่านมาตรวจ โดยยังตรวจเลขรุ่นและเงื่อนไขเดิมครบ
3. ระหว่างตรวจ compiled UI พบ Monaco worker ใช้ data-URL bootstrap ที่ resolve relative ESM imports ไม่ได้ก่อนเปิดซิม เพิ่มการกำหนด bundled workers ใน `src/main.tsx` ตาม public exports ของ Monaco 0.56 โดยคง CSP และ dependency รุ่นเดิม

การ Save ใช้ snapshot ของเนื้อหาตอนเริ่ม Save หากพิมพ์เพิ่มระหว่างรอ write จะยังแสดง dirty ของข้อความใหม่ ไม่ทำเครื่องหมายว่าไฟล์ใหม่ถูกบันทึกแล้วทั้งที่เขียนเพียง snapshot เก่า

## วิธีใช้

เปิดจาก root ของโปรเจกต์นี้:

```powershell
npm.cmd ci
npm.cmd run tauri dev
```

ถ้ามี dev server ของโปรเจกต์เก่าเปิดที่ 1420 ให้ปิด instance เก่าก่อน เมื่อยังไม่มี SDK ให้เลือก **ใช้ Simulator ก่อน** หรือ **ติดตั้งสำหรับลงบอร์ด** ระบบไม่เริ่มดาวน์โหลด SDK อัตโนมัติ ปุ่ม Build & Flash ต้องรอ SDK พร้อม และยังใช้คำสั่ง ESP-IDF ของรุ่น 3.7.1 เดิม

- ทดลองซิม: กด **Run Simulator** ที่แถบเครื่องมือ เลือกตัวอย่าง แล้วกด **Run Simulator** ภายในแผง มี Stop/Reset/ปิดแผง ไม่ต้อง Flash หรือเชื่อม API เว็บภายนอก
- ใช้กับโปรเจกต์: ในแชทติ๊ก **สร้างไฟล์ Simulator คู่กับเฟิร์มแวร์** แล้วขอ AI เขียนพฤติกรรมที่รองรับ ระบบสร้าง `simulation/main.sim.js` ควบคู่กับเฟิร์มแวร์ กด Keep สำหรับไฟล์ที่เสนอแก้ให้ครบก่อน Run
- ตัวเลือกสร้าง companion ปิดเป็นค่าเริ่มต้น งาน AI เฟิร์มแวร์เดิมจึงไม่ได้รับคำสั่งให้เพิ่มไฟล์ซิมโดยอัตโนมัติ
- ทดสอบอุณหภูมิ: เลือกตัวอย่างอุณหภูมิ เปลี่ยนค่าทดสอบระหว่าง Run แล้วดูตัวเลขบน LED หรือเลือกอ่านจากเซนเซอร์ของ Unity
- ลงบอร์ด: ปิดแผงซิม เลือกพอร์ตบอร์ด แล้วกด **Build & Flash** ระบบ Save ก่อนใช้คำสั่ง ESP-IDF เดิม

ตัวอย่างคำขอเมื่อเปิดตัวเลือก companion: “ใช้บอร์ด KidBright32 ตามรุ่นในโปรเจกต์ อ่านอุณหภูมิแล้วแสดงจำนวนเต็มบน LED Matrix ทุก 250 ms พร้อมสร้าง simulation/main.sim.js ให้จำลองพฤติกรรมเดียวกัน” ระบุรุ่นบอร์ดและอ่านหลักฐานฮาร์ดแวร์ก่อนเลือก pin/driver

## ขอบเขตและการแยกส่วน

- เฟิร์มแวร์ยังเป็น ESP-IDF C/C++ ส่วนซิมเป็น ES5 สำหรับ interpreter ไม่ใช่การจำลอง binary ESP32 หรือแปลง C/C++ อัตโนมัติ
- รองรับปุ่ม S1/S2, แสง, อุณหภูมิ, OUT1/OUT2/USB, input1–4, bitmap LED 16×8 และ helper แสดงจำนวนเต็ม รองรับ text/scroll ผ่านตัวสร้าง bitmap เพิ่มเติม รองรับ buzzer แบบ square wave ในเครื่องแล้ว (ดูหัวข้อ Local buzzer ด้านล่าง) ยังไม่รองรับ Wi-Fi/FreeRTOS tasks หรือทุกอุปกรณ์ของบอร์ดจริง
- Unity assets เก็บใน `simulator-assets/` และฝังใน Rust ด้วย `include_bytes!` โหลดเมื่อเปิดซิม ไม่ใส่ JS/WASM ใน KB bundle และไม่ดาวน์โหลด Unity จากเว็บตอนใช้งาน
- `simulator.rs` เปิด HTTP บน loopback พอร์ตชั่วคราว ใช้ session/Host validation และเสิร์ฟเฉพาะ assets ที่กำหนด ไม่มี route อ่าน workspace หรือสั่ง shell
- แผงซิมใช้ iframe คนละ origin กับ IDE และสื่อสารผ่าน origin/source/session/run ID ที่ตรวจสอบ แก้ CSP เพิ่มเฉพาะ frame ของ loopback พร้อมปิด object ไม่ปิด CSP ทั้งชุด
- `simulation_project.rs` จับคู่ revision ด้วย SHA-256 ของ companion และ firmware inputs ตรวจ pending diff/บันทึกไฟล์ก่อน Run และปฏิเสธไฟล์ที่ firmware เปลี่ยนหลังจับคู่
- `simulation_paths.rs` จำกัด path ให้อยู่ใน workspace รวม canonicalization/symlink/junction ตรวจปลายทาง companion ซ้ำก่อน Keep/Undo การตรวจเฉพาะส่วนซิมนี้ไม่ใช่การรับรองความปลอดภัยทุก AI tool ของรุ่นเดิม
- ระบบ Build/Flash backend, `esp_idf.rs`, `toolchain.rs`, การเลือกพอร์ตและการตั้งค่า driver ของ Windows ไม่ได้ย้ายจาก prototype มาทับรุ่นหลัก

## ตรวจซ้ำ

```powershell
npm.cmd test -- --reporter=dot --maxWorkers=2
npm.cmd run test:simulator
npx.cmd tsc --noEmit
npm.cmd run build:release
cargo test --locked --manifest-path src-tauri/Cargo.toml
cargo check --locked --manifest-path src-tauri/Cargo.toml
```

`scripts/probe-simulator-ui.mjs` ทดสอบ compiled UI และ Unity จริงภายใต้ CSP โดยใช้ native/AI/hardware IPC fixture แยก browser profile ต้องกำหนด `VIBE_PROBE_TOOLS` ไปยังโฟลเดอร์เครื่องมือทดสอบที่ติดตั้ง `playwright-core` แล้วรัน `node scripts/probe-simulator-ui.mjs` ไม่เพิ่ม playwright เป็น dependency ของแอป ผล JSON อยู่ใน `src-tauri/target/simulator-ui-verification.json`

การตรวจนี้ไม่เปิด serial port ไม่เรียกโมเดล AI ภายนอก ไม่ติดตั้ง SDK จริง และไม่รัน installer ทับแอปผู้ใช้ ต้องทดสอบ Flash/Boot/Serial กับบอร์ดจริงอีกครั้งก่อนเผยแพร่ รวมทั้ง installer และ macOS/Linux การจับคู่ hash ตรวจ revision แต่ไม่พิสูจน์ว่าโปรแกรม JS และเฟิร์มแวร์มีพฤติกรรมเท่ากันทุกกรณี และการตรวจ path ยังไม่ใช่ filesystem sandbox ที่ป้องกันการสลับ path โดย process ภายนอกได้แบบ atomic

## ผลตรวจรอบนี้

- Frontend: 13 files / 117 เทสต์ผ่าน รวม Save failure, dirty snapshot, กด Build ซ้ำ และการสลับ Simulator/Build
- Rust: 97 เทสต์ผ่าน ไม่มี ignored รวม HTTP host ของ Rust จริง, Thai paths, Windows junction, revision pairing และ Keep/Undo destination guard
- Interpreter/adapter/compiler: 18 เทสต์ผ่าน
- `cargo check --locked`, TypeScript และ `npm run build:release` ผ่าน ยังมี warning ของ Vite เรื่องขนาด bundle และ static/dynamic imports ตาม baseline
- Compiled UI + Unity จริง: เข้า Simulator โดยไม่มี SDK และไม่มีการเรียกดาวน์โหลด → Save ล้มเหลวต้องไม่ Build → Save สำเร็จใช้ args เดิม → ซิมแสดงอุณหภูมิ 25→42 → Stop/Close → Build อีกครั้ง → เปิด `.sim.js` พร้อม JavaScript worker → ตรวจ AI opt-in false/true ผ่าน ไม่มี page error หรือ external request
- Probe ใช้ IPC fixture สำหรับ native/AI/hardware จึงไม่ใช่หลักฐาน Flash ลงบอร์ด หรือ end-to-end AI ผ่านโมเดลจริง ตรวจส่วน native ด้วย Rust tests แยกต่างหาก ไม่ได้รันแอป native เต็มตัวเพื่อหลีกเลี่ยง startup migration/auto-repair ของผู้ใช้

## ไฟล์ที่เปลี่ยน

- บั๊ก: `src/saveProject.ts`, `src/buildFlash.ts`, `src/App.tsx`, `scripts/check-hotfix.mjs`, `src/main.tsx`
- ซิม: `src/SimulatorPanel.tsx`, `src/simulator/SimulatorAdapter.ts`, `src-tauri/src/simulator.rs`, `src-tauri/src/simulation_paths.rs`, `src-tauri/src/simulation_project.rs`, `simulator-assets/`
- จุดเชื่อม: `src/AiChat.tsx`, `src-tauri/src/ai_chat.rs`, `src-tauri/src/ai/tools.rs`, `src-tauri/src/lib.rs`, `src-tauri/tauri.conf.json`
- ความรู้: `knowledge_base/simulator_programs.md` และสำเนาเดียวกันใน `resources/knowledge_base/`
- ดาวน์โหลด: src/ToolchainSetup.tsx, src-tauri/src/toolchain.rs, src-tauri/src/toolchain_download_progress.rs, src/test/toolchainActivation.test.tsx, scripts/probe-hotfix-ui.mjs
- ตรวจซ้ำ: `src/test/saveProject.test.ts`, `src/test/simulatorIntegration.test.tsx`, `scripts/test-simulator.mjs`, `scripts/smoke-simulator.mjs`, `scripts/probe-simulator-ui.mjs`, `package.json` และเอกสารนี้
- Cargo ส่วนซิม: เพิ่ม direct dependency `sha2 = 0.10.9` ซึ่งมีอยู่แล้วใน lock เปลี่ยน lock เฉพาะ dependency list ของแพ็กเกจแอป ไม่เปลี่ยนรุ่น transitive dependencies และไม่เปลี่ยน `package-lock.json`

ส่วน ESP-IDF เปลี่ยนเฉพาะการค้นหา SDK ให้ใช้ตำแหน่งที่บันทึกไว้ พร้อม regression test ของ resolver ไม่เปลี่ยนคำสั่ง Build/Flash ส่วน toolchain เพิ่มความคืบหน้าและตรวจพื้นที่ตามรายละเอียดด้านล่าง และคืน generated schemas ที่ไม่มีเนื้อหาเปลี่ยนให้เป็นสถานะเดิมแล้ว


## ปรับการดาวน์โหลด SDK หลังพบว่ารอนาน

- SDK สำหรับบอร์ดมาจาก GitHub Releases สองไฟล์: frameworks.zip ประมาณ 286.6 MiB และ tools.zip ประมาณ 1889.8 MiB รวมประมาณ 2.3 GB ก่อนแตกไฟล์ ไม่ใช่ขนาด Unity ซึ่งมี assets อยู่ในแอปแล้ว
- เดิมแต่ละ download thread ส่งความเร็ว/ETA ของไฟล์ตัวเองเข้ากล่องเดียวกัน ทำให้เวลาเหลือสลับไปมา และกรณีไม่ทราบขนาดรวมใช้ modulo จน progress วนกลับ แก้ให้ใช้ byte counter และตัวจับเวลาร่วม ส่งความเร็ว/ETA รวมทุก 3 วินาที เมื่อไม่ทราบขนาดจะแสดงว่ากำลังคำนวณ ไม่สร้างเปอร์เซ็นต์วนกลับ
- ตัด HEAD ที่เรียงรอก่อนเริ่ม GET ออก ใช้ Content-Length จาก GET ของแต่ละไฟล์แทน ช่วงดาวน์โหลดยังใช้ 0–49% และต้องแตกไฟล์/ตรวจ SDK ก่อนแสดงเสร็จ ไม่เปลี่ยน URL, staging, extraction หรือ install lock
- เมื่อเปิดแอปที่ยังไม่มี SDK จะแสดงตัวเลือกติดตั้งสำหรับลงบอร์ด หรือใช้ Simulator ก่อน ไม่มี countdown เริ่มดาวน์โหลดอัตโนมัติ หากเลือกข้าม ปุ่ม Setup / Repair ESP-IDF ยังใช้เริ่มติดตั้งภายหลังได้
- เทสต์ใหม่: Rust รวม byte/ความเร็ว/ETA ของทั้งสองไฟล์ รวมกรณีไม่ทราบขนาดและ progress ไม่ถอยหลัง; frontend ไม่ดาวน์โหลดเมื่อรอหรือกดข้าม และเริ่มติดตั้งครั้งเดียวเมื่อกดติดตั้ง พร้อมเปิด Build โดยไม่ต้อง reload
- Browser probes ใช้ compiled UI และ Unity จริง ตรวจใช้ Simulator โดยไม่มี SDK โดยไม่เรียก download และตรวจเลือกติดตั้งผ่าน IPC fixture จนปุ่ม Build พร้อมใช้งาน
- ตรวจพบพื้นที่ C: ต่ำกว่า 1 GB ระหว่างคอมไพล์ จึงคืนพื้นที่จาก incremental cache ที่สร้างสำหรับการทดสอบ เก็บ SDK และ ZIP ที่โหลดค้างไว้ทั้งหมด การติดตั้งจริงยังต้องมีพื้นที่สำหรับทั้ง ZIP และไฟล์ที่แตกแล้ว
- ยังไม่ได้เพิ่ม resume: ถ้าปิดระหว่างดาวน์โหลด การติดตั้งรอบต่อไปยังเริ่มใหม่เมื่อผู้ใช้เลือกติดตั้ง ไม่ได้ทดสอบดาวน์โหลด SDK หลาย GB/แตกไฟล์จริงรอบนี้ และไม่ได้วัดว่าเครือข่ายเร็วขึ้น ความเร็วจริงขึ้นกับเครือข่าย ดิสก์ และเซิร์ฟเวอร์


## ตรวจพื้นที่และเลือกไดรฟ์ติดตั้ง SDK

### พฤติกรรม

- หน้าเริ่มติดตั้งและ Setup / Repair ESP-IDF แสดงพื้นที่ว่างและตำแหน่งติดตั้ง ให้เลือกไดรฟ์หรือโฟลเดอร์อื่นได้ก่อนโหลด หากตำแหน่งเดิมไม่ผ่าน จะเลือกตำแหน่งแนะนำที่ผ่านให้ พร้อมแจ้งผู้ใช้
- Windows ตรวจไดรฟ์ภายในที่ต่ออยู่ด้วย GetLogicalDrives/GetDriveTypeW และพื้นที่ที่ผู้ใช้มีสิทธิ์ใช้ด้วย GetDiskFreeSpaceExW ไม่ใช้ cmd หรือ PowerShell สแกนไดรฟ์ ส่วน Unix ใช้ statvfs และเลือกโฟลเดอร์บน filesystem อื่นได้
- เกณฑ์เริ่มต้นเป็นพื้นที่ว่างสำรอง 12 GiB สำหรับ ZIP + SDK ที่แตกแล้ว + พื้นที่ทำงาน เป็นค่าประเมินเผื่อไว้ ไม่ใช่ขนาด SDK ที่วัดจาก Release โดยตรง เมื่อดาวน์โหลดครบจะอ่านขนาด uncompressed ของทุก ZIP แล้วตรวจพื้นที่ก่อนแตกไฟล์อีกครั้ง พร้อมเผื่อ 512 MiB
- ถ้าไม่พบตำแหน่งที่พื้นที่พอหรือเขียนได้ จะปิดปุ่มติดตั้งและแจ้งให้คืนพื้นที่หรือเลือกไดรฟ์อื่น การอ่านพื้นที่ล้มเหลวจะไม่ถือว่าพื้นที่พอ ยังใช้ Simulator ก่อนได้
- ตรวจพื้นที่ซ้ำตอนกดติดตั้งและใน backend ก่อนลบ staging หรือเริ่ม network จึงไม่อาศัยข้อมูลพื้นที่เก่าที่ UI แสดงอย่างเดียว
- ZIP, staging, SDK และ backup อยู่บนไดรฟ์ปลายทางเดียวกัน โดยไดรฟ์อื่นใช้โฟลเดอร์ .vibekidbright-toolchain/<account-id>/toolchain แยกต่อผู้ใช้ รับเฉพาะตำแหน่งที่โปรแกรมจัดการ และปฏิเสธ symlink/junction ใน managed directories ก่อน cleanup
- บันทึกตำแหน่งหลังติดตั้งสำเร็จเท่านั้น ลง toolchain-location.json ใน AppData ด้วย temporary file + atomic persist เมื่อเปิดแอปใหม่ ทั้งปุ่ม Build/Flash และ AI build ใช้ resolver เดียวกันที่อ่านตำแหน่งนี้ SDK เดิมที่พร้อมอยู่แล้วไม่ต้องดาวน์โหลดซ้ำหรือย้ายอัตโนมัติ
- แม้ติดตั้งบนไดรฟ์อื่น ยังต้องมีพื้นที่ AppData อย่างน้อย 1 MiB และเขียนไฟล์ตั้งค่าได้ จะตรวจเงื่อนไขนี้ก่อนโหลด

### ผลตรวจและขอบเขต

- Rust เพิ่ม tests การเลือกไดรฟ์สำรอง/เต็มทุกไดรฟ์ เกณฑ์พื้นที่ ขนาด ZIP จริง การบันทึกและอ่านตำแหน่งหลังเปิดใหม่ path ภาษาไทยมีช่องว่าง การคง setting เดิมเมื่อค่าใหม่ไม่ผ่าน การอ่านไดรฟ์จริง และปฏิเสธ Windows junction
- Frontend เพิ่ม tests เลือกสำรองและเลือกเอง เต็มทั้งคู่ ตรวจซ้ำก่อนโหลด อ่านพื้นที่ไม่ได้ และ countdown ที่ไม่วนติดตั้งเมื่อพื้นที่ไม่พอ
- Compiled UI probe ตรวจเต็มทั้งคู่แล้วไม่โหลด → คืนพื้นที่ผ่าน fixture → เลือกไดรฟ์ D → ส่งตำแหน่งที่เลือกให้ installer และเปิด Build ได้โดยไม่ reload ส่วน Unity probe ยังตรวจใช้ Simulator โดยไม่มี SDK ผ่าน
- ไม่ดาวน์โหลด SDK เต็มชุด ไม่ทดสอบติดตั้ง/Flash ลงบอร์ดจริงในรอบนี้ และยังไม่ทดสอบ native runtime บน macOS/Linux ไม่เพิ่ม resume การตรวจพื้นที่ไม่ใช่การจองพื้นที่ ถ้าแอปอื่นใช้พื้นที่เพิ่มระหว่างติดตั้ง I/O ยังอาจล้มเหลวและจะแสดง error
- การเลือกไดรฟ์ครอบคลุม SDK และไฟล์ชั่วคราวของการติดตั้งนี้ การ build ของโปรเจกต์และ cache คอมไพล์ยังใช้พื้นที่ตามตำแหน่งของโปรเจกต์

ไฟล์รอบพื้นที่: src/ToolchainStorage.tsx, src/ToolchainSetup.tsx, src/App.tsx, src-tauri/src/toolchain_storage.rs, src-tauri/src/toolchain.rs, src-tauri/src/esp_idf.rs, src-tauri/src/lib.rs, src/test/toolchainStorage.test.tsx, src/test/toolchainActivation.test.tsx, scripts/probe-hotfix-ui.mjs, scripts/probe-simulator-ui.mjs และเอกสารนี้

Cargo รอบพื้นที่: เพิ่ม direct dependencies แบบแยก OS ได้แก่ windows-sys 0.61.2 (Win32_Foundation, Win32_Storage_FileSystem) และ libc 0.2.182; ย้าย tempfile 3 จาก dev-dependencies มา runtime ทั้งหมดมีอยู่แล้วใน lock ไม่เปลี่ยนเวอร์ชัน transitive dependencies

อ้างอิง API ของ Windows: [GetDiskFreeSpaceExW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdiskfreespaceexw), [GetLogicalDrives](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getlogicaldrives), [GetDriveTypeW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypew)


## ตรวจการสร้างไฟล์ซิมจาก Vibe Code ตาม ZIP ต้นฉบับ

ตรวจ simulator-main.zip ในเครื่อง SHA-256 cab5aa612c80f0128323c0713d35d4213bf775c2f730306b3abcae903622d332 และเทียบ worker ht16k33.js กับไฟล์ที่แตกไว้แบบ byte-for-byte ผลตรงกัน ระบบต้นฉบับ preprocess โค้ด Blockly ให้ JS-Interpreter รัน แล้ว worker แปลงคอลัมน์ LED เป็น x/y ก่อนส่ง DOM/MessageLED ให้ Unity ไม่พบหลักฐานว่าตัว Unity แปลง C/C++ ทั้งโปรแกรมเอง

### บั๊กที่ยืนยัน

ไฟล์ simulation/main.sim.js ของตัวอย่าง matrix_az_demo มีเพียง while(1) และ vTaskDelay(500) ไม่มีคำสั่งแสดงผล ขณะที่ firmware มี glyph A-Z ครบ ระบบเดิมตรวจเพียงเนื้อหาไม่ว่างและ revision hashes จึงรับไฟล์ที่ไม่มีพฤติกรรมจริงได้

### สิ่งที่เพิ่ม

- generate_simulation รับ program_json ที่อธิบาย device behavior แบบมีชนิดข้อมูล แล้ว compiler ฝั่ง Rust สร้าง ES5 ที่ใช้เฉพาะ API ของ runtime เดิม: LED bitmap, เซนเซอร์, ปุ่ม, OUT/USB, delay และ helper ตัวเลข
- รองรับคำนวณ ตัวแปร เงื่อนไข repeat/while/break ตัวอักษร printable ASCII ข้อความทีละตัวและข้อความเลื่อน โดยแปลงอักษรเป็น bitmap ด้วย font bytes จาก ZIP เดิม เพิ่ม yield ในลูปเพื่อให้ Stop ตอบสนองได้แม้ไม่ระบุ delay
- ไม่รัน code ของผู้ใช้ใน host JavaScript ไม่เรียก shell ไม่เพิ่ม dependencies ไม่แก้ Unity binaries ตัว compiler เป็น pure function และผลลัพธ์ถูกส่งเข้า JS-Interpreter เดิม
- AI ใช้ตัวสร้างนี้เขียนได้เฉพาะ simulation/main.sim.js ผ่าน metadata/backup/Diff workflow เดิม การเรียก write_file ตรงสำหรับ companion จะคืน error ก่อนเขียน เพื่อไม่ให้ข้ามการตรวจชนิด/API/ขนาดโดยส่ง JS ที่แต่งเอง
- ปฏิเสธ delay-only placeholder ทั้งตอน stamp และ load รวมการตรวจ output call ใน AST ฝั่ง runtime ของโหมดโปรเจกต์ เพื่อให้ไฟล์เดิมที่จับคู่ hash แล้วแต่ไม่มีพฤติกรรมถูกรายงานด้วย
- สั่ง AI อ่าน firmware จริง จับคู่ทั้งภาพ เวลา ลำดับลูปและเงื่อนไข และแจ้ง unsupported capabilities อย่างชัดเจน กรณี compiler พบ error จะไม่สร้างไฟล์ซิมใหม่ ไม่ลดความสามารถ firmware เพื่อให้ซิมผ่าน

### ซ่อมตัวอย่างปัจจุบัน

สร้างภาพ 26 ตัวจาก FONT_5X7_UPPERCASE ใน main/main.c จริง โดยใช้ reverse_bits และตำแหน่งคอลัมน์เดียวกับ firmware แล้วส่งผ่าน bitmap operation ของ compiler วน A-Z ทุก 500 ms

ทดสอบ candidate กับ Unity จริงก่อนบันทึก จากนั้นสำรอง main.sim.js เดิมเป็น main.sim.js.before-0a1d14d0.bak และบันทึก companion ใหม่แบบ atomic ตรวจ SHA-256 ของ main/main.c, main/CMakeLists.txt, CMakeLists.txt และ sdkconfig.defaults ก่อน/หลัง ตรงกันทั้งหมด ค่า main/main.c คือ a8033712ea61bfffd3cd7a67a9d75569cc3a5147fdf48f27b013fc4db3b826ef

เปิดโปรเจกต์เดิม กด Stop หากกำลังรัน แล้วโหลดใหม่จากโปรเจกต์และกด Run Simulator ส่วนตัวสร้าง AI ใหม่ต้องใช้แอปที่เปิดจากซอร์สล่าสุด

### หลักฐานทดสอบ

- Rust 92 tests ผ่าน รวม compiler schema/ขอบเขต, ทุก sensor/output mapping, การจับคู่ไฟล์และคง firmware bytes, ปฏิเสธ placeholder เดิม และ tool schema ของ OpenAI-compatible/Google
- Golden fixtures 8 โปรแกรมสร้างจาก compiler จริง Rust tests ตรวจ source ตรงกับ fixtures ทุกครั้ง และ Node tests นำ JS ชุดเดียวกันเข้า parser/interpreter จริง
- Interpreter/adapter/compiler 18 tests ผ่าน รวม A-Z 26 ภาพตามลำดับและวนกลับ A, Stop ไม่มี frame ต่อ, อุณหภูมิเปลี่ยนพร้อมเงื่อนไข USB, ปุ่ม/IN1/IN4, ตัวแปร, while/break, dynamic ASCII, scroll, bitmap และลูปไม่มี delay
- Frontend 108 tests ผ่าน; cargo check --locked, npx tsc --noEmit และ build:release ผ่าน
- Compiled UI + Unity จริงใช้ candidate ของ matrix_az_demo ผ่าน A-Z และวนกลับ A พร้อม regression เดิม Save failure/Build args/อุณหภูมิ/Stop/Close/AI opt-in ไม่มี page error หรือ external request มีภาพหลักฐาน alphabet-A.png และ alphabet-Z.png ใน src-tauri/target

### ข้อจำกัดที่ยังต้องรักษา

ตัวสร้างนี้แปลง device-behavior IR เป็น JS ไม่ใช่ parser/compiler ของ C/C++ ทุกคำสั่ง AI ยังเป็นผู้จับคู่พฤติกรรมจาก firmware การตรวจชนิด/hash/static output ไม่ใช่ proof ว่าทุก branch ให้ผลเทียบเท่า firmware หรือว่า branch นั้นจะถูกเรียกจริง การทดสอบครอบคลุมชุดกรณีที่ระบุ ไม่ได้พิสูจน์ทุกโปรแกรมที่ AI อาจสร้าง

Wi-Fi/โปรโตคอลเครือข่าย servo อุปกรณ์ที่ไม่อยู่ใน bridge และ FreeRTOS หลาย task ยังต้องแจ้งว่าไม่รองรับ ไม่สร้างซิมว่างมาทดแทน ภาษาไทยต้องมี bitmap ที่ตรวจเอง ฟอนต์ข้อความชุดนี้รองรับ ASCII 32..126 เท่านั้น ค่าที่คำนวณนอกช่วง LED/ASCII อาจเกิด runtime error ซึ่งแสดงใน UI

รอบนี้ไม่ได้เรียกโมเดล AI จริง/ทุก provider ไม่ Flash หรือทดสอบ firmware ของโปรเจกต์บนบอร์ดจริง ไม่ทดสอบ native runtime บน macOS/Linux และไม่เชื่อม Simulator API ภายนอก

ไฟล์รอบนี้: src-tauri/src/simulation_compiler.rs, simulation_font.rs, simulation_validation.rs, simulation_project.rs, src-tauri/src/ai/tools.rs, providers.rs, workflow.rs, src-tauri/src/lib.rs, src-tauri/examples/compile-simulation.rs, simulator-assets/runtime-core.js, runtime.js, NOTICE.md, src/SimulatorPanel.tsx, scripts/test-simulator.mjs, scripts/probe-simulator-ui.mjs, simulator-tests/compiler-cases.json, knowledge_base/simulator_compiler.md และ simulator_programs.md พร้อมสำเนาที่ตรงกันใน resources/knowledge_base และเอกสารนี้ ไม่เปลี่ยน Cargo dependencies รอบตัวสร้างซิม


## Keep แล้ว Run Sim ยังแจ้งว่ามี Diff ค้าง

### สาเหตุที่ตรวจยืนยันจากสถานะจริง

รอบนี้ไฟล์ simulation/main.sim.js ถูก Keep แล้ว แต่ main/main.c ยังอยู่ใน pending diff และเนื้อหาในข้อเสนอนั้นมี hash ตรงกับ firmware revision ที่บันทึกในไฟล์ซิม ขณะที่ main.c บนดิสก์ตอนเริ่มตรวจยังเป็นรุ่นก่อนหน้า จึงเป็นการบล็อกที่ถูกต้อง แต่ UI เดิมไม่แสดงชื่อไฟล์ค้าง ทำให้ไม่ชัดว่ารับไฟล์ใดแล้วหรือยังเหลือไฟล์ใด

ก่อนแก้ backend ได้สำรอง proposal ที่ยังอยู่ในหน่วยความจำไว้ใน .pending-review-before-keep-fix.json และ localStorage ของ IDE รวม hash ของไฟล์ต้นฉบับ ไม่เรียก Keep/Undo แทนผู้ใช้ พบ probe 4 รูปแบบพาธที่อ้างถึง proposal main.c รุ่นเดียวกัน ไม่ใช่ 4 proposal ต่างรุ่น หลังการตรวจรับล่าสุด ไฟล์ firmware ทั้ง 4 ไฟล์บนดิสก์ตรงกับ metadata ของซิมแล้ว

### การแก้ไข

- หน้าซิมแสดงรายชื่อไฟล์ที่ยังค้าง เช่น main/main.c พร้อมปุ่มตรวจ Diff ที่เปิดแท็บตรงไฟล์นั้นและตรวจ pending ใหม่ ผู้ใช้ยังต้องกด Keep หรือ Undo เอง ไม่มีปุ่มที่รับทุกไฟล์อัตโนมัติ
- ข้อความ error ฝั่ง Rust ระบุชื่อไฟล์ที่บล็อก Run และมี list_pending_diffs สำหรับรายการใน workspace ที่มีผลกับการจำลอง ปรับสีข้อความให้ใช้กับ theme สว่าง/มืดได้
- check/accept/reject/propose ใช้ canonical file identity ร่วมกัน ป้องกันความต่างระหว่าง Windows path ปกติกับ verbatim path และไม่เทียบเพียงชื่อไฟล์ท้ายพาธข้ามโปรเจกต์
- Keep ตรวจว่า proposal ยังเป็นเนื้อหารุ่นที่เปิดตรวจอยู่ เขียน temporary file + sync + atomic persist และคง permissions เดิม ก่อนล้าง pending หลังเขียนสำเร็จ ถ้าเขียนไม่ได้หรือรุ่นเปลี่ยนจะเก็บข้อเสนอไว้และแจ้ง error
- กันการกด Keep/Undo ซ้ำระหว่างคำสั่งทำงาน รวมการป้องกันผลตรวจเก่าทับสถานะใหม่ และส่ง ai-diff-resolved เพื่อให้หน้าซิมตรวจ revision ใหม่หลังรับ/ปฏิเสธไฟล์
- กู้คืนเฉพาะ pending state จาก snapshot รอบแก้ ไม่เขียน firmware หรืออนุมัติ proposal ตรวจ workspace, base hash และ proposal ที่ใหม่กว่าก่อนกู้คืน เมื่อผู้ใช้ตัดสินใจผ่าน Keep/Undo จะล้างรายการ recovery ของไฟล์นั้น การกู้คืนนี้ไม่ใช่การบันทึกทุก pending diff แบบถาวรในทุก session

### ผลตรวจ

- Frontend 117 tests / 13 files ผ่าน รวมรายชื่อไฟล์ค้าง เปิดตรวจตรงไฟล์ ไม่เรียก accept/reject อัตโนมัติ event refresh กัน Keep ซ้ำ และ snapshot recovery
- Rust 97 tests ผ่าน รวม Windows canonical aliases, write/atomic replace failure แล้ว pending ยังอยู่, stale review rejection, แยกโปรเจกต์ชื่อไฟล์ซ้ำ และกู้คืน proposal โดยไฟล์บนดิสก์ไม่เปลี่ยน
- cargo test --locked, cargo check --locked, npx tsc --noEmit และ build:release ผ่าน
- Browser probe ใช้ compiled IDE + Unity จริง และ native/AI/hardware IPC fixture ทดสอบ pending main.c → ปุ่มตรวจ Diff → ผู้ใช้กด Keep → เปิด Run ได้ ต่อด้วย Save failure/Build arguments/อุณหภูมิ/ตัวอักษร A-Z และ Stop/Close เดิม
- ไม่รัน Build/Flash ลงบอร์ดหรือเรียกโมเดล AI จริง ไม่กดรับ/ทิ้ง Diff ของโปรเจกต์จริงผ่านเครื่องมือทดสอบ และยังไม่ได้ทดสอบ native runtime บน macOS/Linux

ไฟล์รอบ review: src-tauri/src/pending_diff.rs, src-tauri/src/ai_chat.rs, src-tauri/src/ai/tools.rs, src-tauri/src/lib.rs, src-tauri/src/simulation_project.rs, src/CodeEditor.tsx, src/SimulatorPanel.tsx, src/App.tsx, src/utils.ts, src/pendingReviewRecovery.ts, src/test/codeEditorDiff.test.tsx, src/test/simulatorReviews.test.tsx, src/test/pendingReviewRecovery.test.ts, scripts/probe-simulator-ui.mjs และเอกสารนี้ ไม่เพิ่ม Cargo/npm dependencies รอบนี้


## แก้คำสั่งสร้างโปรเจกต์ใหม่กลับไปแก้ไฟล์โปรเจกต์เดิม

ตรวจยืนยันจากโค้ด: AiChat.tsx แทรก CURRENT PROJECT STATE ที่มีข้อความ do NOT create a new project ไว้ใน user message ขณะที่ requests_new_project ตรวจข้อความรวมและปฏิเสธทันทีเมื่อพบ do not create ทำให้ fresh_creation เป็น false ทั้งที่ผู้ใช้ขอสร้างใหม่จริง create_project_workspace จึงถูกบล็อกและเครื่องมือเขียนไฟล์เดิมยังใช้ได้ นอกจากนี้ตัวตรวจเดิมปฏิเสธคำอธิบาย/โปรเจกต์เดิมทุกตำแหน่ง ทำให้ข้อความอย่าง สร้างโปรเจกต์ใหม่โดยไม่แก้โปรเจกต์เดิม ผิดด้วย

การแก้:
- ส่ง userRequest ที่ผู้ใช้พิมพ์แยกจากบริบทที่แอปเติมเข้ามา และใช้ค่านี้ตรวจเจตนา หาก frontend เก่าไม่ส่งค่านี้ ให้ลอกเฉพาะ wrapper ของแอปก่อนตรวจ
- รองรับคำว่าโปรเจกต์หลายรูปสะกด แยกคำสั่งสร้างจริงจากคำถาม/ตัวอย่าง/โค้ดอ้างอิง และดูเฉพาะข้อความผู้ใช้ล่าสุด
- เมื่อขอสร้างใหม่ บล็อกเครื่องมือโปรเจกต์จนกว่า create_project_workspace จะสร้างโฟลเดอร์ใหม่สำเร็จ ถ้ายกเลิกหรือชื่อซ้ำ ไม่เปลี่ยนไปใช้โปรเจกต์เดิม หากจบโดยยังไม่ได้สร้าง workspace ให้แจ้ง error
- read_file, write_file, list_files และ diff_file ตรวจ relative path ภายใน workspace ที่เลือกแล้ว ปฏิเสธ absolute path และ ../ ที่ชี้กลับโปรเจกต์เดิม ไม่อ้างว่าการตรวจนี้ sandbox คำสั่ง shell ทุกชนิด
- แก้การปิด Monaco DiffEditor ที่ทำให้ TextModel ถูก dispose ก่อน widget โดยเก็บ model ไว้จน widget dispose แล้วจึงคืน model ทดสอบทั้ง lifecycle และ compiled browser

เทสต์ Rust เพิ่ม 5 ข้อครอบคลุม wrapper จริง, คำสั่งสร้างพร้อมรักษาโปรเจกต์เดิม/อธิบายโค้ด/รูปสะกดภาษาไทย, การปฏิเสธ/อ้างอิง/ข้อความล่าสุด, โฟลเดอร์ชื่อซ้ำและการเขียนข้ามกลับโปรเจกต์เดิม และ error เมื่อสร้างไม่สำเร็จ ใช้โฟลเดอร์ชั่วคราว ตรวจไฟล์โปรเจกต์เดิม byte ต่อ byte ไม่เปลี่ยน Frontend probe ตรวจ userRequest เป็นข้อความที่พิมพ์จริงไม่ใช่ wrapper

ผลตรวจสุดท้ายรอบนี้: cargo test --locked ผ่าน 102 tests, cargo check --locked ผ่าน, frontend ผ่าน 118 tests ใน 13 files, npx tsc --noEmit ผ่าน ไม่มี Cargo/npm dependency เพิ่มในรอบ review และ new-project fix

ยังไม่ได้เรียกโมเดลจริง/ทดสอบ native folder picker กับทุก provider, ไม่ Flash บอร์ดจริง และไม่ทดสอบ native runtime บน macOS/Linux การจำแนกภาษาธรรมชาติยังเป็น heuristic ภาษาไทย/อังกฤษ ไม่รับประกันรูปประโยคทุกแบบ

ไฟล์ส่วน new-project: src/AiChat.tsx, src-tauri/src/ai_chat.rs, src-tauri/src/ai/workflow.rs, src-tauri/src/ai/tools.rs และเอกสารนี้ ส่วน Monaco lifecycle อยู่ src/CodeEditor.tsx และ src/test/codeEditorDiff.test.tsx; scripts/probe-simulator-ui.mjs เพิ่มการตรวจ transport ข้อความผู้ใช้

วิธีลอง: เปิดแอปจาก source ล่าสุด เปิดโปรเจกต์เดิมไว้ แล้วพิมพ์ สร้างโปรเจกต์ใหม่ชื่อ matrix_az_new ให้แสดง A-Z โดยไม่แก้โปรเจกต์เดิม เมื่อกล่องเลือกตำแหน่งปรากฏ ให้เลือกโฟลเดอร์แม่ที่ต้องการ ระบบต้องสร้างโฟลเดอร์ลูก matrix_az_new และเปลี่ยน workspace ไปที่นั้น ก่อนให้ AI เขียนโค้ด หากยกเลิกหรือโฟลเดอร์ชื่อนั้นมีอยู่แล้ว ต้องแจ้งปัญหาและไม่เขียนโปรเจกต์เดิม


ผล browser รอบสุดท้ายหลังแก้ Monaco: build:release ผ่าน และ compiled IDE + Unity/Monaco จริงผ่าน Keep -> Run Sim, อุณหภูมิ 25/42, fixture A-Z ครบ 26 ตัวและวนกลับ A, Stop/Close/Build regression, userRequest transport ไม่มี page error และไม่มี external request จำลองเฉพาะ native/provider/hardware IPC รายงานอยู่ src-tauri/target/simulator-ui-verification.json

มีการลองตั้ง VIBE_PROBE_COMPANION เป็นไฟล์ matrix_az_demo ปัจจุบันด้วย แต่ probe รุ่นนี้คาด ht16k33.show ด้วย literal 26 ภาพ ขณะที่ไฟล์ปัจจุบันเป็น scroll array จึงหยุดที่ assertion จำนวน 0 != 26 ก่อนทดสอบลำดับไฟล์นั้น ผลนี้ไม่ใช่หลักฐานว่า runtime ของไฟล์ดังกล่าวเสีย การรันที่ผ่านทั้งหมดข้างต้นใช้ golden alphabet fixture และยังไม่ได้ยืนยันลำดับ frame ของไฟล์ scroll ปัจจุบันทั้งโปรแกรมใน browser รอบนี้


## เห็นเฉพาะโค้ดซิม แต่ editor main.c ยังค้างที่ Hello (SW1 Pressed)

ตรวจโปรเจกต์ D:/kb_sw1_scroll_text แบบอ่านอย่างเดียว: main/main.c มีโค้ด 5,644 ไบต์สำหรับอ่าน SW1, debounce, เขียน LED Matrix ผ่าน I2C และ ESP_LOGI(TAG, "SW1 Pressed") แล้ว ไม่ใช่ไฟล์ boilerplate Hello ขณะตรวจ SHA-256 ของ main/main.c เป็น b53907279b951af442755a71aafa9b7a3f200c15579dbbd604dfdcfd94998742 และ hash ของ firmware ทั้ง 4 ไฟล์ตรงกับ metadata ใน simulation/main.sim.js ข้อมูลนี้ยืนยันว่าฝั่ง firmware ถูกเขียนลงดิสก์แล้ว แต่ไม่รับรองว่า firmware ทำงานถูกต้องบนบอร์ดทุกกรณี

บั๊กที่พิสูจน์จากโค้ดและการทดสอบ: App.tsx ใช้การแทน backslash อย่างเดียวเมื่อรับ file-modified และใช้ตัวเปรียบเทียบอีกชุดเมื่อรับ ai-diff-pending จึงไม่รู้จักพาธ Windows ปกติและ canonical ที่มี prefix //?/ ว่าเป็นไฟล์เดียวกัน ในขณะที่ backend ใช้ canonical path ใน pending diff และลด prefix ตอน emit หลัง Keep ทำให้พลาดการรีโหลดหรือเปิด tab ซ้ำได้ ตัวอ่าน activeFile ใช้ normPath อีกแบบหนึ่งและอาจยังเลือก tab เก่า จึงเห็น Hello ทั้งที่ไฟล์จริงถูก Keep แล้ว

ก่อนแก้: browser ที่ใช้ Monaco/Unity จริงและจำลอง native IPC กด Keep สำเร็จแต่ editor ยังแสดง void app_main(void) {} (editor_shows_kept_firmware=false, page_errors=[]) และ regression App tests ล้มเหลวตรงบั๊ก 4 ข้อจาก 5 ข้อ ได้แก่ canonical -> ปกติ, ปกติ -> canonical, ตัวพิมพ์ต่างกัน และ tab ซ้ำ ส่วนกรณีไฟล์ชื่อเดียวกันอีกโปรเจกต์ผ่าน

แก้เฉพาะ App.tsx ให้ file-modified, ai-diff-pending และ reloadFile ใช้ normPath ตัวเดียวกัน รออ่านไฟล์หลัง event ก่อนรีเฟรชรายการ และใช้ tab เดิมเมื่อ proposal อ้างพาธ alias พร้อมกันเพิ่มการกัน tab ซ้ำใน state updater ไม่แก้ backend หรือโค้ด firmware/sim ของผู้ใช้ ไม่เรียก Keep/Undo แทนผู้ใช้ ไม่เรียก Gemini และไม่ Flash

ไฟล์รอบนี้: src/App.tsx, src/test/firmwareReload.test.tsx, scripts/probe-firmware-reload.mjs และเอกสารนี้ เทสต์ใหม่ใช้เนื้อหาเก่า/ใหม่ต่างกันจริง ไม่ตรวจแค่ว่า banner หาย รายงานใน terminal ว่าเจอ rate limit เป็นข้อมูลอีกส่วน ยังไม่มี error response/timestamp ของคำขอครบพอให้โยงเป็นสาเหตุที่ editor แสดง Hello รอบนี้


ผลหลังแก้ firmware reload: frontend 123 tests ใน 14 files ผ่าน รวม regression ใหม่ทั้ง 5 ข้อ, npx tsc --noEmit ผ่าน, npm run build:release ผ่าน และ git diff --check ผ่าน Browser regression ตัวเดิมที่เคยล้มเหลวตรวจ native_file_was_kept=true และ editor_shows_kept_firmware=true แล้ว ไม่มี page error การทดสอบ compiled IDE/Unity เดิมผ่าน Keep/Run Sim, อุณหภูมิ, A-Z, Stop/Close และการคงคำสั่ง Build/Flash เดิม ใช้ native/provider/hardware fixture ทั้งสอง browser tests

ไม่ได้ทดสอบด้วยการเรียก Gemini จริงหรือ compile/flash firmware SW1 บนบอร์ดจริง และไม่ได้รัน Rust tests ใหม่ในรอบนี้เพราะไม่แก้ Rust/dependencies ผล Rust 102 tests และ cargo check --locked ที่ผ่านก่อนหน้ายังเป็นผลจากรอบก่อน ไม่ใช่การทดสอบ firmware ที่ AI สร้างบนฮาร์ดแวร์

สำหรับแท็บที่ค้างอยู่ก่อนแก้ ให้ปิดและเปิด main.c ใหม่จากรายการไฟล์เพื่อโหลดเนื้อหาจากดิสก์ การแก้ UI นี้ไม่ได้ให้ AI เขียนทับ main/main.c หรือ simulation/main.sim.js รอบตรวจปัจจุบันไฟล์ firmware ยังมี hash b53907279b951af442755a71aafa9b7a3f200c15579dbbd604dfdcfd94998742 ตรงกับตอนเริ่มตรวจ


## Build ล้มเหลว Cannot load build/project_description.json (SW1)

ตรวจจาก D:/kb_sw1_scroll_text/build จริง พบ CMakeCache.txt และ CMakeFiles แต่ไม่มี project_description.json และ build.ninja ซอร์ส ESP-IDF ที่ติดตั้งจริง v5.5.4 ใน tools/idf_py_actions/tools.py: ensure_build_directory ข้ามการ configure หาก cache ยังใช้ได้ จากนั้น _set_build_context เปิด project_description.json ทันทีจึง FatalError และ exit code 2 ก่อน compiler เริ่ม ทำสำเนา cache ลง temporary project แล้วสั่ง idf.py build ได้ error เดียวกันและ exit code 2 ยืนยันแล้ว

ตัวตรวจ is_build_dir_stale เดิมดู IDF/Python path แต่ไม่พบกรณี cache อ่านได้ทั้งที่ configure ยังไม่ครบ ข้อความนี้ไม่ใช่ compiler error ของ main.c และไม่เกี่ยวกับการอ่านโค้ดโดย Unity จากข้อมูล log ที่เหลือยังยืนยันไม่ได้ว่าเหตุใดการ configure ครั้งแรกจึงหยุดกลางทาง

เพิ่ม recover_idf_build_args ตรวจเฉพาะคำสั่ง build/all/flash เมื่อมี CMakeCache.txt แต่ project_description.json หาย/อ่านไม่ได้/JSON เสีย/ไม่ใช่ object หรือไม่มี generated build.ninja/Makefile ให้เติม reconfigure หน้าคำสั่งเดิมพร้อม log อธิบาย ใช้กับ run_shell_command ที่เรียก idf.py และ build_firmware_with_toolchain ไม่ลบ build ทั้งโฟลเดอร์สำหรับกรณีนี้ จึงเก็บ log/object เดิม การตรวจ cache ต่างเครื่องเดิมยังคงอยู่ ไม่บังคับ configure โปรเจกต์ใหม่ซ้ำ เพราะ idf.py ทำเอง และไม่เพิ่ม reconfigure เมื่อคำสั่งระบุไว้แล้ว

Rust regression เพิ่ม 5 tests: metadata หายและคง cache/source bytes, metadata JSON เสีย, metadata มีแต่ไม่มี build files, Ninja/Make build ปกติไม่ถูกสั่ง reconfigure ทุกครั้ง และ fresh/explicit reconfigure/non-build commands ไม่ถูกเปลี่ยน ทดสอบว่า build flash -p COM6 คง arguments เดิมเมื่อเติม reconfigure

ก่อนแก้ backend สำรองสถานะ pending review ที่เกี่ยวกับ firmware/sim ไว้ .pending-review-before-build-recovery.json ผลพบ 0 รายการ ไม่มีการกด Keep/Undo แทนผู้ใช้ โมดูล capture ชั่วคราวถูกถอดแล้ว

ทดสอบซอร์สจริงบนสำเนา temporary ด้วย ESP-IDF v5.5.4: reconfigure ผ่านและ build ผ่าน ได้ kb_sw1_scroll_text.bin ขนาด 0x2d490 ไบต์ ไม่รัน flash ขั้นต่อไปทดสอบ reconfigure build กับโปรเจกต์ D:/kb_sw1_scroll_text โดยตรง พร้อมเทียบ hash ของ CMakeLists.txt, main/CMakeLists.txt, main/main.c, sdkconfig.defaults และ simulation/main.sim.js ก่อน/หลังเพื่อป้องกันเขียนทับโค้ด

ไฟล์รอบ build recovery: src-tauri/src/esp_idf.rs, src-tauri/src/toolchain.rs และเอกสารนี้ ไม่เพิ่ม Cargo/npm dependencies ไม่แก้ firmware/sim sources


ผลสุดท้ายรอบ Build recovery: cargo test --locked ผ่าน 107 tests (เพิ่ม 5 tests), cargo check --locked ผ่าน, npx tsc --noEmit ผ่าน และ git diff --check ผ่าน ไม่รัน frontend 123 tests/Monaco browser ใหม่ในรอบนี้เพราะไม่เปลี่ยนลอจิก frontend ผล frontend รอบก่อนยังเป็นผลรอบ firmware reload

โปรเจกต์จริง D:/kb_sw1_scroll_text รัน idf.py reconfigure build สำเร็จ exit code 0 ได้ build/project_description.json, build/build.ninja และ build/kb_sw1_scroll_text.bin ขนาด 0x2d490 ไบต์ บิลด์หลัก 1026 ขั้นและ bootloader 118 ขั้นผ่าน ยืนยัน hash ก่อน/หลังของ firmware inputs ทั้ง 4 ไฟล์และ simulation/main.sim.js ไม่เปลี่ยน ไม่มีการรัน flash/erase-flash หรือเปิดพอร์ต COM6 ผ่านคำสั่งทดสอบ

ยังไม่ได้ทดสอบการ Flash/ปุ่ม/LED บนบอร์ดจริง หรือ native build recovery บน macOS/Linux และยังระบุไม่ได้ว่าอะไรทำให้การ configure ครั้งแรกถูกขัดจังหวะ ไฟล์ log ที่มีไม่แสดงสาเหตุแรกครบ การแก้ครั้งนี้ป้องกันสถานะ cache ไม่สมบูรณ์ทำให้ Build ล้มเหลวซ้ำ และเก็บหลักฐานเดิมไว้


## Local buzzer: ทดสอบเสียงตามต้นฉบับก่อนเชื่อม API

เพิ่มเสียงออกลำโพง/หูฟังของเครื่องผ่าน Web Audio โดยไม่เปิดพอร์ต serial, Build/Flash, เรียกโมเดล AI หรือ Simulator API ภายนอก การเพิ่มนี้ไม่แก้ firmware ของผู้ใช้ และไม่เปลี่ยน Unity binary

### วิธีลองโดยไม่ใช้ AI

1. เปิดแอปจาก source ล่าสุดด้วย npm.cmd run tauri dev ในโฟลเดอร์โปรเจกต์
2. กด Run Simulator รอ Unity พร้อม แล้วเลือกตัวอย่าง Buzzer บี๊บ 1000 Hz กด Run Simulator เสียงดัง 5 วินาทีแล้วหยุด
3. ลองตัวอย่าง Buzzer โน้ต C–E–G หรือ ปุ่ม SW1 → Buzzer ตัวอย่าง SW1 ดัง 200 ms หนึ่งครั้งต่อการกดและรอปล่อยปุ่ม
4. ทดลองปิดเสียง/เปิดเสียงและแถบระดับเสียงในแผง ใช้ Stop/Reset/ปิดหน้าซิมเพื่อหยุดเสียงได้ทันที

AudioContext สร้าง/เปิดใน click handler ก่อนรอ native/iframe IPC เพื่อให้ได้ user activation ถ้าระบบไม่รองรับหรือเบราว์เซอร์ไม่เปิดเสียง จะแสดงสาเหตุพร้อมปุ่มเปิดเสียงอีกครั้ง ไม่ใช้ autoplay bypass ในตัวแอป อ้างอิง [Chrome autoplay/Web Audio policy](https://developer.chrome.com/blog/autoplay/#web-audio)

### ขอบเขตและเส้นทางข้อมูล

- เทียบโน้ตกับไฟล์ sound.js และ wrapper sound ใน simulator.js จาก ZIP ต้นฉบับ: note 0..36 คือ C4..C7, A4=440 Hz และ square wave เสียงเดียว; rest 0..4 คือ 2000/1000/500/250/125 ms ที่ 120 BPM รวม sound.off/get_volume/set_volume (0..100)
- sound.tone(hz) ช่วง 20..20000 เป็น extension ในเครื่องสำหรับจำลองความถี่ PWM/LEDC ของ ESP-IDF ไม่อ้างว่าเป็น API เดิมของ Unity ส่วนระดับเสียงรวมในแอปเป็นตัวคูณเพิ่มเติมจาก volume ของโปรแกรม และจำกัด gain สูงสุดไว้ 0.2
- ตัวสร้าง program_json เพิ่ม tone{hz,ms}, note{note,duration}, volume{value}, buzzer_off ตรวจช่วงค่าคงที่และตรวจค่าที่คำนวณขณะรัน รองรับ audio-only companion ผ่าน completeness guard โดยยังปฏิเสธ placeholder ที่มีแค่ delay/ปรับ volume และคงการจับคู่ hash/Keep/Undo เดิม
- Runtime ควบคุมเวลาใน JS-Interpreter แล้วส่ง BUZZER {playing,frequency,volume} ที่มี sessionId/runId ให้หน้า IDE adapter ตรวจ origin/source/session ตามเดิม แล้ว renderer รับเฉพาะ run ปัจจุบัน เสียงเป็นสัญญาณ local ไม่ส่ง source code เข้า endpoint ของ Unity หรือ cloud
- Parent renderer เก็บ oscillator เดียวต่อครั้ง เปลี่ยนความถี่/ระดับเสียงได้ เมื่อ mute/level 0 จะ stop/disconnect node; Stop/Reset/error/completed/close จะล้าง run ownership และไม่ให้ BUZZER เก่ากลับมาเล่น Unmount ปิด AudioContext และคืน graph
- เมื่อเชื่อม API ในอนาคต ให้เพิ่ม adapter แปลงเหตุการณ์อุปกรณ์ตาม contract ที่ทีม Simulator ยืนยันได้ ส่วนรุ่นปัจจุบันไม่มีการเรียก API จริง และไม่ได้สมมติชื่อ Unity sound endpoint ที่ไม่พบในซอร์ส

### การสร้างผ่าน AI

อัปเดต system instructions และ KB simulator_compiler.md/simulator_programs.md ทั้ง knowledge_base และ resources/knowledge_base ให้ generate_simulation สร้างเสียงคู่กับ firmware เมื่อเปิดตัวเลือก Simulator โดยยังต้องอ่าน/เขียน firmware จริงก่อน ไม่เขียน main.c ทับเพียงเพื่อซ่อม companion และต้องระบุอุปกรณ์/ลักษณะเสียงที่ไม่รองรับผ่าน unsupported

พรอมต์ลอง: สร้างโปรเจกต์ใหม่สำหรับ KidBright32 เมื่อกด SW1 ให้ buzzer ดัง 1000 Hz นาน 200 มิลลิวินาทีหนึ่งครั้งต่อการกด พร้อมสร้างไฟล์ซิมให้กด Run Sim ทดสอบได้

### ผลตรวจ

- cargo test --locked: 112 tests ผ่าน รวม compiler/ช่วงค่า/ชื่อ sound ถูกสงวน/completeness ของ audio-only และยืนยัน firmware bytes ไม่เปลี่ยน
- cargo check --locked, npx tsc --noEmit และ npm run build:release ผ่าน ไม่มี Cargo/npm dependencies เพิ่มในรอบ buzzer
- Frontend 133 tests ใน 16 files ผ่าน รวม lazy AudioContext, mute/volume, Stop แล้ว event เก่ากลับมาไม่ได้, dispose, blocked autoplay/missing API, invalid payload, failed Stop ACK, Reset/close และ React StrictMode
- Node interpreter/adapter/compiler 24 tests ผ่าน รวมโน้ตทั้ง 37 ค่า/จังหวะเดิม, note/rest จริง, SW1/ช่วง Hz, runtime errors, Stop/Reset/Dispose และ golden fixtures ที่สร้างด้วย Rust compiler จริง 3 โปรแกรมใหม่
- Browser probe ใช้ compiled UI + Unity + Web Audio จริง ไม่ใส่ autoplay bypass และวัด PCM ด้วย AnalyserNode: สั่ง 1000 Hz วัดได้ 996.09375 Hz (ตาม FFT bin resolution), peak 0.047701; mute/Stop peak 0 และ active oscillator 0; C–E–G = 261/329/391 Hz; SW1 signal → เสียงจริง, volume/Reset/close ปิด context ผ่าน ไม่มี page error/external request รายงาน src-tauri/target/buzzer-ui-verification.json
- Browser regression เดิมผ่าน Keep, LED A-Z/วนกลับ A, อุณหภูมิ, Save failure และคง arguments Build/Flash เดิม โดยใช้ native/provider/hardware fixture เท่านั้น ไม่ Flash ฮาร์ดแวร์

### สิ่งที่ยังไม่ได้ทดสอบและข้อจำกัด

ยังไม่ได้เรียก Gemini/provider จริงเพื่อสร้าง firmware+companion รอบใหม่ ไม่ Flash/เทียบเสียงกับ buzzer บนบอร์ดจริง ไม่ยืนยันคุณภาพเสียงผ่านลำโพงของผู้ใช้จากการฟังจริง และไม่ทดสอบ native WebView ทุก OS ผล browser ใช้ Chromium ในเครื่องและวัดสัญญาณดิจิทัล สัญญาณ SW1 ใน browser probe ใส่ผ่าน DOM bridge ที่ Unity ใช้ และมี runtime condition tests เพิ่มเติม ยังไม่ใช่การทดสอบคลิกภาพ 3D ทุกตำแหน่ง

รองรับ buzzer monophonic square wave เท่านั้น ไม่จำลอง duty cycle ที่ปรับได้, DAC/PCM/MP3, หลายเสียงพร้อมกัน หรือ timbre ของฮาร์ดแวร์ การจับคู่ IR กับพฤติกรรม ESP-IDF ยังทำโดย AI ไม่ใช่การแปล C/C++ ทุกโปรแกรม ความสามารถ Wi-Fi/FreeRTOS หลาย task/servo/อุปกรณ์อื่นยังต้องระบุว่าไม่รองรับ จึงยังไม่อ้างว่าทดสอบทุกด้านของบอร์ดครบแล้ว

ไฟล์รอบ buzzer: src/SimulatorPanel.tsx, src/simulator/BuzzerAudio.ts, simulator-assets/runtime-core.js และ runtime.js/NOTICE.md, src-tauri/src/simulation_compiler.rs, simulation_validation.rs, simulation_project.rs, src/test/buzzerAudio.test.ts, buzzerPanel.test.tsx, scripts/test-simulator.mjs, probe-buzzer-ui.mjs, simulator-tests/compiler-cases.json, KB สองไฟล์พร้อมสำเนา resources และเอกสารนี้ สำรอง pending review ก่อนแก้ backend พบ 0 รายการ โมดูล capture ชั่วคราวถูกถอดออกแล้ว


## 26 ตุลาคม 2026: แก้การไวบ์โค้ดพัดลมใช้ช่องไม่ตรงกับ Unity

ยืนยันจากโปรเจกต์ `D:\kb_fan_toggle_sw1`: firmware เดิมเขียน GPIO26 และ companion เขียน OUT1 ในขณะที่พัดลมที่ลากต่อใน Unity ใช้ USB การกด SW1 เดิมทำงานจริง แต่ไปเปลี่ยนคนละช่อง จึงไม่แก้ด้วยการสลับ OUT1 เป็น USB ใน runtime

### วิธีลอง

1. เปิดแอปรุ่นที่ build จากโค้ดล่าสุด แล้วเปิดโปรเจกต์ที่ต้องการ
2. ระบุ **พัดลม USB** ในข้อความ ระบบสร้างไฟล์ Simulator คู่กับเฟิร์มแวร์อัตโนมัติ
3. ส่งคำขอ: `ให้ SW1 สลับเปิดปิดพัดลม USB มี debounce พร้อม log Fan ON/OFF`
4. ตรวจและกด Keep สำหรับ firmware และ companion ที่มี Diff โดยระบบไม่กด Keep/Undo แทนผู้ใช้
5. เปิด Run Sim ต่อพัดลม USB ผ่านเมนูอุปกรณ์ใน Unity แล้ว Run กด SW1 ครั้งแรกเปิด ครั้งที่สองปิด การกดค้างต้องสลับเพียงครั้งเดียว
6. หากต้องการบอร์ดจริง ให้ตรวจรุ่น/ขา/ระดับ active ตามวงจรก่อน แล้วใช้ Build & Flash ตามเดิม การเล่นซิมไม่เรียก Flash

หากยังไม่ทราบช่อง ระบบขอให้ระบุ ไม่เลือก OUT1 ให้เอง การลากใน Unity ยังไม่ส่ง inventory หรือภาพให้โมเดลอัตโนมัติ ถ้าใช้ OUT1/OUT2 จริงให้เลือกช่องให้ตรงกับโหลดและวงจรของบอร์ด

### สิ่งที่แก้

- `src/AiChat.tsx`: ส่งช่องที่เลือกพร้อม raw user request และ simulation opt-in โหลดแผนของแต่ละโปรเจกต์ แสดง tool error เป็นล้มเหลวและแสดงผลตรวจช่องแยกจากผล Build
- `src-tauri/src/fan_recipe.rs`: recipe classic KidBright สร้าง firmware และ simulation IR จาก typed port เดียว มี stable debounce, SW1 active LOW ฝั่งบอร์ดและ logical pressed=1 ฝั่งซิม
- `hardware_contract.rs`: แผน `.vibekidbright/hardware.json` แบบ atomic ภายใน workspace ตรวจ GPIO/API ทั้งก่อนสร้าง companion ตอนจบงานที่แก้โค้ด และตอนโหลดเข้า Run Sim
- `ai_chat.rs`, `ai/providers.rs`, `ai/tools.rs`, `ai/workflow.rs`, `lib.rs`: ผูกแผนและ tool `generate_fan_control` สำหรับ OpenAI-compatible และ Google พร้อม guard workspace และใช้ Diff pipeline เดิม
- `simulation_project.rs`: ตรวจช่องเพิ่มจาก hash pairing เดิม จึงไม่ยอมรับแค่ metadata ที่จับคู่ถูกแต่ GPIO/API ใช้ช่องผิด
- `toolResult.ts`, unit tests, `simulator-tests/fan-recipes.json`, `scripts/test-simulator.mjs`, `scripts/probe-fan-ui.mjs`, `examples/generate-fan.rs`: regression จาก compiler/interpreter และ Unity จริง
- อัปเดต `simulator_programs.md` ใน KB และ bundle ให้ระบุข้อจำกัดและช่องพัดลมอย่างชัดเจน ไม่มี dependency เพิ่มในรอบนี้

### โปรเจกต์ที่แก้และหลักฐาน

- แก้เฉพาะ `D:\kb_fan_toggle_sw1\main\main.c`, `simulation/main.sim.js` และแผน hardware ใหม่ โดยตรวจ hash ก่อนเขียนและสำรองของเดิมไว้ที่ `.vibekidbright\backups\fan-usb-20261026-173546`
- ESP-IDF v5.5.4 `idf.py reconfigure build` ผ่าน exit 0 ได้ `build/kb_fan_toggle_sw1.bin` ขนาด 168640 bytes ไม่มี Flash และตรวจว่า source/config/companion เดิมที่เข้าร่วม build ไม่ถูกแก้ระหว่าง build
- Browser probe ใช้ compiled UI, Unity และไฟล์ companion ของโปรเจกต์จริง กด SW1 ด้วย mouse input สองครั้งได้ USB `[0,1,0]`, OUT1/OUT2=0 และตรวจภาพพัดลมหมุนจริง รายงาน `src-tauri/target/fan-ui-verification.json` รูป `fan-usb-on.png`, `fan-usb-off.png` native IPC เป็น fixture และไม่ได้เรียกโมเดล

### ข้อจำกัด

recipe นี้ครอบคลุม SW1 toggle fan ตาม classic reference เท่านั้น ไม่ใช่ C-to-JS translator ทั่วไป ไม่รับรองว่าโค้ดจากโมเดลทุกชนิดหรือ custom driver ทุกแบบถูกต้อง ตัวตรวจ GPIO เป็น structural check ของ direct calls/aliases และ API ไม่ใช่ proof ของ semantic equivalence หรือการ Build ผ่าน ต้องรายงานกรณีตรวจไม่ได้ตามจริง

ยังไม่ได้เรียก Gemini/OpenAI จริง ไม่ได้ Flash/ทดสอบพัดลมบนบอร์ดจริง และยังต้องยืนยัน USB active polarity กับ revision ของบอร์ดผู้ใช้ โดยเฉพาะรุ่นอื่นที่ไม่ใช่ classic reference ไม่เชื่อม external Simulator API


ผลตรวจรอบสุดท้ายของการแก้พัดลม:

- `cargo test --locked`: 118 passed, 0 failed, 0 ignored
- `cargo check --locked`: ผ่าน
- `npx tsc --noEmit`: ผ่าน
- `npm test -- --reporter=dot`: 136 passed ใน 18 ไฟล์
- `npm run test:simulator`: 25 passed รวม recipe USB/OUT1/OUT2, glitch/bounce, กดค้าง, ปล่อยและกดซ้ำ
- `npm run build:release`: ผ่าน รวม hotfix/public/bundle checks
- `git diff --check`: ผ่าน

Browser probe รันซ้ำได้ด้วย `scripts/probe-fan-ui.mjs`: ตั้ง `VIBE_PROBE_TOOLS` เป็นโฟลเดอร์ isolated ที่มี playwright-core และติดตั้ง Chromium ในเครื่องไว้แล้ว ตัว probe ใช้ checked fixture เป็นค่าเริ่มต้น หากต้องการไฟล์โปรเจกต์จริง ให้ตั้ง `VIBE_PROBE_FAN_PROJECT` เป็น project root ก่อนรัน ไม่มีการติดตั้ง/ดาวน์โหลด dependency หรือเรียก AI/Flash ภายใน probe


## ขยายการสร้างโค้ดคู่ให้ครอบคลุมอุปกรณ์ (26 ตุลาคม 2026)

เพิ่ม generate_device_program / read_device_program สำหรับงานใหม่ KidBright32/ESP32 classic reference ที่ใช้ LED, temperature/light, SW1/SW2, digital IN1–4, OUT1/OUT2/USB, buzzer และ control flow จาก behavior เดียวกัน รักษา custom firmware และตรวจ Hello-only/คู่ไม่ตรงเพิ่มจาก hash เดิม แก้ NaN เปิดพอร์ตและ unary ลบของจำนวนลบ parse ไม่ผ่าน

ผล: Rust124, frontend137, interpreter26, C/JS19กรณีรวม222operator checks ผ่าน; ESP-IDF build6ชุดผ่าน; Unity/WebAudio local probes ผ่าน ไม่มีการเรียกโมเดลจริงหรือ Flash บอร์ด ดู [รายละเอียด ขอบเขต รายการไฟล์ และผลทดสอบ](SIMULATOR-DEVICE-COVERAGE-TH.md)


อัปเดตหน้าแชท: นำช่องเลือกพัดลมและคำแนะนำใต้ช่องออกแล้ว ระบุ USB/OUT1/OUT2 ในข้อความ หรือใช้แผนของโปรเจกต์เดิม หากข้อมูลไม่ชัดเจนระบบจะขอให้ระบุทางข้อความ การตรวจช่องของ firmware/ซิมยังทำงานตามเดิม


## 27 ตุลาคม 2026: สร้างไฟล์ซิมอัตโนมัติ

นำ checkbox สร้างไฟล์ Simulator ออกแล้ว ทุกคำขอสร้าง/แก้พฤติกรรมที่ซิมรองรับจะใช้ companion generation อัตโนมัติทั้ง frontend และ backend รวม client เก่าที่ส่ง flag false การถามข้อมูลอย่างเดียวไม่สร้างไฟล์ งานที่ซิมไม่รองรับต้องรายงานข้อจำกัดและรักษา firmware แทนการสร้างซิมหลอก การตรวจ Diff และการกด Run Sim ยังแยกจากการสร้างไฟล์ตามเดิม


## กรณีเฟิร์มแวร์ใช้สิ่งที่ซิมยังไม่รองรับ (27 ต.ค. 2026)

แก้บั๊กที่การสร้างโปรเจกต์เซอร์โวเขียน main/main.c ได้ แต่ตัวตรวจหลังจบงานเปิด simulation/main.sim.js ที่ไม่มีจนขึ้น Windows error 3:

- AI ต้องเขียนเฟิร์มแวร์จริงให้ครบก่อน แล้วใช้ report_simulation_unsupported ระบุ capability ที่ซิมยังไม่มี เช่น servo หรือ pwm สถานะที่ได้คือมีไฟล์เฟิร์มแวร์ให้ตรวจ แต่ยังรันซิมงานนี้ไม่ได้ ไม่ใช่ยืนยันว่า Build หรือบอร์ดจริงผ่าน
- รายงานนี้อ่านอย่างเดียว ผูกกับ hash ของเฟิร์มแวร์ปัจจุบัน/ที่รอ Keep เฉพาะ turn นี้ ตรวจซ้ำก่อนจบ หากมีการเขียนโค้ดต่อจะใช้รายงานเดิมไม่ได้ ไม่มีการ Keep อัตโนมัติ แก้ขา GPIO หรือลบไฟล์ซิมเก่า
- งานที่ซิมรองรับยังต้องผ่านการตรวจ C/JS เดิม หากไม่มี companion จะได้ข้อความระบุชื่อไฟล์และวิธีแก้ ห้ามใช้รายการ unsupported ที่เป็น LED/ปุ่ม/USB เพื่อข้ามข้อกำหนด และห้ามรายงานเฟิร์มแวร์ Hello หรือ owned shared program เป็นข้อยกเว้น
- UI แสดงข้อจำกัดแยกจากสถานะคู่ไฟล์สำเร็จ และเก็บคำอธิบายที่ AI ส่งมาก่อน validation error ไว้ให้อ่าน
- ตัวจำลองเซอร์โว/PWM ยังไม่ได้เพิ่ม การมีโมเดลเซอร์โวในภาพ Unity ไม่ยืนยันว่ามี API ควบคุมมุม การทดสอบบอร์ดต้องตรวจรุ่นและ wiring จริง โดยเฉพาะ GPIO15 ของแต่ละ revision


### ผลตรวจหลังแก้กรณีเซอร์โว (27 ต.ค. 2026)

ไฟล์โค้ดที่เปลี่ยนรอบนี้: src-tauri/src/simulation_project.rs, src-tauri/src/ai/providers.rs, src-tauri/src/ai/tools.rs, src-tauri/src/ai/workflow.rs, src/AiChat.tsx, src/test/aiHardware.test.tsx และ scripts/probe-simulator-ui.mjs; เอกสารนี้กับ knowledge_base/simulator_programs.md และ resources/knowledge_base/simulator_programs.md อัปเดตขั้นตอนใหม่ตรงกัน ไม่มีการเพิ่ม dependency หรือแก้ Cargo.toml/Cargo.lock ในรอบนี้

| ตรวจสอบ | ผล |
|---|---|
| cargo test --locked | ผ่าน 129 ข้อ ไม่มี failed/ignored |
| cargo check --locked | ผ่าน |
| npx tsc --noEmit | ผ่าน |
| frontend Vitest | ผ่าน 139 ข้อ ใน 18 ไฟล์ |
| runtime node --test scripts/test-simulator.mjs | ผ่าน 26 ข้อ |
| npm run build:release | ผ่าน รวม hotfix/public/bundle checks |
| node scripts/probe-simulator-ui.mjs | ผ่าน compiled IDE + Unity local จริง: อุณหภูมิ, A–Z, Stop/Keep/Save/Build isolation, unsupported receipt, คำอธิบาย และส่งข้อความต่อ ไม่มี page error/คำขอภายนอก; native/AI/บอร์ดเป็น fixture |
| เฟิร์มแวร์จริง D:/kb_servo_sw | สำเนาแยกคอมไพล์ด้วย ESP-IDF 5.5.4 ผ่าน สร้าง kb_servo_sw.bin ได้; hash ของ main.c และ CMake/defaults ต้นฉบับทั้ง 4 ไฟล์ไม่เปลี่ยน ไม่ได้แฟลช |

ระหว่างตรวจ Build พบ pyvenv.cfg ของ SDK ที่ติดตั้งในเครื่องเป็นไฟล์ว่าง จึงสำรองและซ่อมแบบ atomic ให้ home ชี้ bundled Python 3.11.2 ที่มีอยู่ แล้วตรวจ import idf_component_manager และ Build ซ้ำจนผ่าน แก้เฉพาะไฟล์ D:/.vibekidbright-toolchain/cc9024e1ea0b1b4d/toolchain/tools/python_env/idf5.5_py3.11_env/pyvenv.cfg ไม่ได้ดาวน์โหลดหรือติดตั้งแพ็กเกจเพิ่ม สาเหตุเดิมที่ไฟล์นี้ว่างยังไม่ยืนยัน

ยังไม่ได้เรียก Gemini/AI จริงเพื่อทดสอบการเลือก tool หรือ rate limit, ไม่ได้แฟลช/ต่อเซอร์โวจริง และยังไม่ได้ทดสอบบน macOS/Linux โค้ดเซอร์โวของผู้ใช้ระบุ V1.5/GPIO15 ซึ่งยังต้องตรวจบอร์ดจริง การ Build ผ่านไม่ยืนยันมุม/สัญญาณ/การต่อไฟถูกต้อง ตัวซิมยังไม่มี API เซอร์โวหรือ PWM ทั่วไป และรายงาน unsupported เป็นการเปิดเผยข้อจำกัด ไม่ใช่หลักฐานว่า AI เขียน C ถูกทุกพฤติกรรม ไม่มีการสร้าง .sim.js ปลอม แก้ขา หรือตัดสิน Keep/Undo ให้ผู้ใช้


## 27 ตุลาคม 2026: เซ็นเซอร์ Digital/Analog

เพิ่ม Analog API เดิมและ shared IR/ADC profile ที่ตรวจ board assembly พร้อมตัวอย่างเลือกในหน้าซิม ทดสอบเมาส์จริงครบ Digital 6 รายการในภาพและ Analog moisture/force/rain ในเมนู Plugins โดยไม่ฉีดค่า sensor แทน ทุกตัวแสดงผล LED ตามค่าที่ Unity ส่ง โค้ด C/JS เก่า 19 แบบยังเหมือนเดิมทุก byte; native 132/frontend139/runtime29/C-vs-JS23 ผ่าน และ SDK Build Analog 2 แบบผ่านโดยไม่แฟลช ดู [วิธีลอง รายการไฟล์ หลักฐาน และข้อจำกัด](LOCAL-SIMULATOR-SENSORS-TH.md)

## 27 ตุลาคม 2026: ชื่อโปรเจกต์ซ้ำและการลอง AI tool ใหม่

ตรวจสองภาพแล้วพบคนละสาเหตุ:

1. `moisture_sim` มีโฟลเดอร์อยู่แล้ว การปฏิเสธสร้างซ้ำถูกต้องและยังคงไว้ บั๊กคือ provider ตรวจเฟิร์มแวร์ของ workspace เดิมต่อหลังสร้างไม่สำเร็จ จนแจ้ง `Hello boilerplate` ซ้ำ แก้ให้เก็บผลสร้าง workspace แยกจากการเขียนโปรแกรม ตรวจเงื่อนไขก่อนอ่านไฟล์เดิม และรายงาน error สร้างจริง เมื่อสร้างได้เพียงโครงจะส่งสถานะ `scaffold_only` ไม่อ้างว่าสร้างโค้ดสำเร็จ
2. `generate_device_program` ครั้งแรกในภาพขาด field `op` แต่ครั้งถัดไปสำเร็จ การปฏิเสธ JSON ผิดรูปแบบถูกต้อง ปัญหาที่ปรับคือ argument `program_json` เป็น string ที่ schema ของ provider ตรวจเนื้อหาข้างในไม่ได้ และเดิมไม่มีคำอธิบาย field นี้สำหรับ paired tool เพิ่มคำอธิบาย/ตัวอย่างที่คอมไพล์ได้ตรงกันทั้ง Google/OpenAI-compatible และทั้ง generate_device_program/generate_simulation ระบุว่าทุก statement ต้องมี `op` ส่วน expression ใช้ `kind`

Compiler ยังปฏิเสธข้อมูลผิดทั้งหมด ไม่เดาหรือเติม operation ให้ AI ใหม่จะเพิ่มตำแหน่ง เช่น `$.steps[0].body[0]` สำหรับ missing op และบอกให้แก้ส่งโปรแกรมครบชุดอีกครั้ง การวิเคราะห์ตำแหน่งจำกัดความลึกและอยู่ภายใต้ input limit เดิม ไม่เพิ่ม dependency หรือเปลี่ยน schema/ผลลัพธ์ของโปรแกรมที่ถูกต้อง

หน้าแชทแสดง error ใต้ชื่อ tool เต็มความกว้างและตัด JSON wrapper ออก เมื่อ generation tool เดิมลองใหม่สำเร็จในข้อความและ workspace เดียวกัน จะระบุ “ลองใหม่สำเร็จแล้ว” พร้อมเก็บ error ก่อนหน้าไว้; error ของ tool อื่น/โปรเจกต์อื่น/ข้อความก่อนหน้ายังอยู่ กรณีแก้ไม่สำเร็จยังรายงาน failure และส่งแชทต่อได้ การใช้ tool สำเร็จยังแยกจาก receipt ตรวจ C/JS และผล Build/บอร์ดจริงตามเดิม

Local completion error ถูกแยกจาก API/provider error เพื่อไม่เปลี่ยนโมเดลเพียงเพราะชื่อโฟลเดอร์มีคำว่า quota, 429 หรือ not found ระบบไม่สร้างทับ/เปลี่ยนชื่อ/ลบ/ใช้ workspace เดิมอัตโนมัติเมื่อคำขอสร้างใหม่ล้มเหลว

ไฟล์แก้ในรอบชื่อซ้ำและ tool retry:

- `src-tauri/src/ai/workflow.rs`, `src-tauri/src/ai/providers.rs`, `src-tauri/src/ai_chat.rs`: ผลสร้าง workspace, completion guard, scaffold receipt และแยก local errors
- `src-tauri/src/simulation_compiler.rs`: คำอธิบาย argument และ missing-op diagnostic
- `src/AiChat.tsx`, `src/toolResult.ts`: error อ่านง่ายและสถานะ retry
- `src/test/aiHardware.test.tsx`, `src/test/toolResult.test.ts`, `scripts/probe-simulator-ui.mjs`: regression tests และ compiled UI probe
- `knowledge_base/simulator_compiler.md`, `resources/knowledge_base/simulator_compiler.md`: ตัวอย่าง Analog scale ที่มี op ครบและวิธี retry ตรงกันทั้งสองสำเนา
- เอกสารนี้ ไม่เปลี่ยน Cargo.toml/Cargo.lock/package dependencies ในรอบนี้

| ตรวจสอบหลังแก้ทั้งหมด | ผล |
|---|---|
| `cargo test --locked` | ผ่าน 142 ข้อ ไม่มี failed/ignored |
| `cargo check --locked` | ผ่าน |
| `npx tsc --noEmit` | ผ่าน |
| frontend Vitest | ผ่าน 145 ข้อ ใน 18 ไฟล์ |
| `npm run build:release` | ผ่าน รวม hotfix/public/bundle checks |
| C vs JS offline execution | ผ่าน 23 โปรแกรม ใช้ C host fixture เทียบ JS-Interpreter เดิม; Golden 19 โปรแกรมเดิมยังผ่าน native suite |
| `node scripts/probe-simulator-ui.mjs` | ผ่าน compiled IDE และ Unity local จริง: อุณหภูมิ/A–Z/Stop/Keep/Save/Build isolation, ชื่อซ้ำไม่แสดง Hello ซ้ำ, scaffold, tool ลองใหม่สำเร็จพร้อมเก็บ error และส่งแชทต่อ ไม่มี page error หรือ external request; native/AI/บอร์ดเป็น fixture |
| โปรเจกต์จริงในภาพ `D:/analog_moisture_sim` | อ่าน description จาก C แล้วคอมไพล์ซ้ำแบบ offline เทียบทั้ง C/JS ตรงกับไฟล์เดิม มี Analog IN1 และ LED Matrix; hash ไม่เปลี่ยน |
| โปรเจกต์ `D:/moisture_sim` เดิม | hash ทั้งหมดตรง snapshot ก่อนแก้ ยังเป็นโครง ไม่เขียนทับหรือสร้างงานต่อให้เอง |

ระหว่างตรวจภาพหลังปรับสีใน dark theme พบสคริปต์ probe ส่งข้อความที่สองก่อนคำตอบแรกจบ (รอแค่ invoke แล้ว) จน timeout เพิ่มการรอ completion และปุ่ม Stop หายก่อนส่งข้อความถัดไป รวม fixture ตอบช้าเพื่อครอบคลุม race นี้ โดยไม่เพิ่ม timeout หรือเปลี่ยนพฤติกรรมแอป จากนั้น probe ทั้งชุดผ่านอีกครั้ง

หลักฐาน UI อยู่ใน `src-tauri/target/simulator-ui-verification.json`, `project-create-error-readable.png`, `generation-retry-resolved.png` (ไฟล์ตรวจในเครื่อง ไม่รวมเป็น asset แอป) การตรวจ C/JS ใน D: เป็น read-only; การทดสอบ native ที่สร้างโปรเจกต์ใช้ temp directory แยก ไม่ตัดสิน Keep/Undo ให้ผู้ใช้

ยังไม่ได้ทดสอบเรียก Gemini/AI จริง, dialog เลือกโฟลเดอร์ผ่านบทสนทนาจริง, Build/Flash ของ moisture project, บอร์ดจริง หรือ macOS/Linux การส่ง JSON ผิดจากโมเดลยังเกิดได้ คำอธิบาย/ตัวอย่างช่วยลดความผิดพลาดและ diagnostic ช่วยให้แก้ retry ได้ ไม่รับรองว่าทุกโมเดลจะเลือก arguments ถูกทุกครั้ง การมี receipt ว่าสร้างคู่ผ่านไม่ได้รับรองว่าตรงความตั้งใจทุกข้อหรือว่าบอร์ดจริงทำงานแล้ว

ไฟล์ `D:/analog_moisture_sim` ระบุ hardware profile `kidbright32_v15_analog_bypass` แต่การตรวจนี้ไม่ได้ยืนยัน board assembly จริง ต้องตรวจบอร์ดและทาง bypass ก่อนใช้ Analog firmware จริง ค่า 0–4095 → 0–100 เป็นสเกลจำลอง ไม่ใช่ความชื้นที่สอบเทียบแล้ว และไม่มีการเพิ่มสิทธิ์/ข้ามเงื่อนไข profile จากการแก้ op

ลองต่อ: หากจะสร้างใหม่ให้ใช้ชื่อไม่ซ้ำ เช่น `moisture_sim_2`; หากต้องการโครงเดิมให้สั่ง “ทำต่อในโปรเจกต์ moisture_sim ที่เปิดอยู่” อย่างชัดเจน สำหรับ `analog_moisture_sim` ที่สร้างคู่สำเร็จแล้ว ให้ตรวจ Keep/Undo และโหลดโปรแกรมจากโปรเจกต์ใน Run Sim โดยเลือกอุปกรณ์ Analog ให้ตรงช่อง IN1
