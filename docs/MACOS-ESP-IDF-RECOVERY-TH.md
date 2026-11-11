# แก้ Build บน macOS: No ESP-IDF Python venv found

วันที่ 11 พฤศจิกายน 2569 — แพตช์ Mac รวมใน source รุ่น 3.7.2 ให้ตรวจผล Build และไฟล์ติดตั้งจาก Actions ของ tag `v3.7.2`

## สาเหตุที่ยืนยันจากภาพและโค้ด

ข้อความ `No ESP-IDF Python venv found in .../Library/Application Support/th.nectec.vibekidbright/toolchain/tools/python_env` เกิดก่อนคอมไพล์ firmware: ตัวค้นหามองไม่พบ `idf*_py*_env/bin/python` หรือ `bin/python3` ที่มีอยู่จริงสำหรับ Mac ชุด Windows ที่มีเพียง `Scripts/python.exe` หรือ symlink ที่ชี้ไปเครื่องอื่นใช้แทนไม่ได้ ภาพเพียงอย่างเดียวยังไม่พิสูจน์ว่าเครื่องผู้ใช้มีกรณีใด

พบข้อบกพร่องในระบบเดิม: ดาวน์โหลดแพ็กเกจเดียวทุก OS, แสดง Ready จาก sentinel เป็นหลัก, ซ่อม pyvenv.cfg โดยย้าย home ไปที่ tools/venv, และ frontend เรียก cmd.exe/export.bat เมื่อมี custom SDK ส่วน SDK ใต้ Application Support ยังมีช่องว่างซึ่งไม่อยู่ในข้อกำหนด path ของ ESP-IDF

## สิ่งที่แก้ใน source

- macOS ใช้ `idf_tools.py` ทางการ ตรึง ESP-IDF v5.5.4 และเลือกเครื่องมือตาม Python/สถาปัตยกรรมจริง ไม่โหลด Windows tools.zip
- สร้าง Python venv บน Mac ในตำแหน่งใช้งานสุดท้าย ไม่ย้าย venv หลังสร้างและไม่แก้ home ด้วย Windows patcher
- ตำแหน่งเริ่มต้นใหม่คือ `$HOME/.vibekidbright/toolchain/macos/<arm64|x86_64>/` เก็บ framework และ tools แยกสถาปัตยกรรม SDK เดิมใต้ Application Support ไม่ถูกย้ายหรือลบทิ้งจากการติดตั้งนี้
- ตรวจ Python/dependencies, idf.py, CMake, Ninja และ compiler ว่ารันได้จริงก่อน Ready; custom SDK ใช้การตรวจเดียวกัน แสดงคำแนะนำเมื่อขาด prerequisite
- Build ส่ง argument array ไป backend โดยตรง Backend/AI runner ใช้ environment จาก official export รวม CMake.app/Contents/bin, virtualenv และ ROM paths
- Stop/Cancel installer ยุติ process group ทั้งต้นไม้บน Unix มี regression test สำหรับ process ลูกที่รันต่อ
- กรอง debug-console และ Bluetooth-Incoming-Port ออกจากตัวเลือกพอร์ต Mac ให้ผู้ใช้เลือก USB board port จริง
- เพิ่ม CI บน `macos-15` (Apple Silicon) และ `macos-15-intel` เรียก installer ชุดเดียวกับแอป แล้ว Build firmware คู่ C/JS สองโปรเจกต์ (LED/ปุ่ม/USB/buzzer และ Analog ADC) ตรวจ .bin โดยไม่ Flash; การสร้าง DMG ต้องผ่านงานนี้ก่อน

Prerequisites: Xcode Command Line Tools และ Python 3.10+ ที่ตรงสถาปัตยกรรม (แนะนำ 3.12) หากใช้ Homebrew ให้ติดตั้ง `python@3.12` ตัวแอปไม่ติดตั้ง Homebrew หรือขอสิทธิ์ admin แทนผู้ใช้ ต้องมีพื้นที่ว่างตาม preflight และ SDK/โปรเจกต์ต้องไม่มีช่องว่างใน full path

## ผู้ใช้ที่มีแอปรุ่นเดิมตามภาพ

ใช้ `scripts/repair-macos-idf.command` หรือ ZIP `VibeKidbright-Mac-Repair-2026-11-11.zip` ที่จัดเตรียมให้ เป็น launcher ซ่อมเฉพาะเครื่อง Mac เดิมที่ installer เคยขึ้น Ready แล้ว

1. ปิด VibeKidbright ก่อน และแตก ZIP บน Mac
2. หากยังไม่มี Command Line Tools ให้รัน `xcode-select --install` และติดตั้งให้เสร็จ ติดตั้ง Python 3.12 จาก Python.org หรือ Homebrew ตามวิธีที่เครื่องนั้นใช้
3. เปิด Terminal แล้วใช้คำสั่ง (ปรับตำแหน่งไฟล์และ .app ให้ตรงเครื่อง):

```sh
bash "$HOME/Downloads/VibeKidbright-Mac-Repair/repair-macos-idf.command" "/Applications/VibeKidbright IDE.app"
```

หรือพิมพ์ `bash ` แล้วลากสคริปต์ลง Terminal ตามด้วยเว้นวรรคและลากไฟล์ .app ตามลงไป

4. สคริปต์ติดตั้ง native SDK ที่ `$HOME/.vibekidbright/mac-repair/` ตรวจเครื่องมือ แล้วเปิดแอปโดยส่ง environment ให้ backend ใช้ SDK นี้ หลังจากติดตั้งครั้งแรก ครั้งต่อไปจะตรวจแล้วเปิดโดยไม่ติดตั้งซ้ำหาก SDK ยังใช้ได้
5. โปรเจกต์ต้องอยู่ใน path ที่ไม่มีช่องว่าง เลือกพอร์ต USB ของบอร์ด เช่น `/dev/cu.usbserial-*` หรือ `/dev/cu.usbmodem*` ที่ตรงกับฮาร์ดแวร์ แล้วลอง Build/Flash หรือ Build ที่มีในแอป

แอปรุ่นเดิมมีบั๊ก custom-path wrapper จึงเก็บสำเนาเฉพาะค่า SDK สองช่องใน `mac-repair/previous-sdk-paths.json` และเอาสองค่านั้นออกจาก config เพื่อใช้ environment override แทน ค่าอื่นทั้งหมดคงอยู่ ไม่สำรองหรือแสดง API keys ในไฟล์สำเนานี้ สคริปต์ไม่แก้ firmware ไม่ Flash ไม่ลบ SDK เดิม และไม่สร้าง readiness marker ปลอม หากรุ่นเก่าไม่เคยมี marker จะแจ้งให้ใช้แอปที่ rebuild แล้วแทน

จนกว่าจะติดตั้ง DMG ที่แก้แล้ว ให้เปิดรุ่นเดิมผ่าน launcher นี้ทุกครั้งและเก็บหน้าต่าง Terminal ไว้ขณะใช้ การเปิดไอคอนปกติไม่ได้รับ environment จาก launcher หลีกเลี่ยงการตั้ง SDK ผ่านหน้า Settings ของรุ่นเดิมระหว่างใช้วิธีนี้

ชุดซ่อมผ่านการตรวจ syntax/ส่วนแก้ config แบบแยกแล้ว แต่ยังไม่ได้ทดลองติดตั้ง/เปิดแอปจริงบน Mac จากสภาพแวดล้อม Windows นี้ หากมีปัญหาให้ส่งข้อความ Terminal โดยเฉพาะขั้นแรกที่ fail พร้อม `uname -m`, macOS version และ app version; ไม่ส่ง config.json ที่อาจมีข้อมูลลับ

## ผู้ดูแลซอฟต์แวร์: สร้าง DMG รุ่นแก้ไข

นำแพตช์และไฟล์ source ที่เกี่ยวข้องทั้งหมดขึ้น branch ที่จะ build แล้วเรียก workflow `Build macOS Installer` งานนี้ต้องรอ reusable workflow `macOS ESP-IDF firmware integration` ผ่านทั้งสอง CPU ก่อนจึงจะสร้าง Universal DMG ตาม workflow เดิม

รัน smoke โดยตรงบน Mac ได้ด้วย:

```sh
export VIBEKIDBRIGHT_BOOTSTRAP_PYTHON="$(command -v python3)"
cargo run --locked --manifest-path src-tauri/Cargo.toml --example macos-idf-smoke -- "$HOME/.vibekidbright/toolchain" "$TMPDIR/vibe-idf-smoke-new"
```

Python ที่เลือกต้องเป็น 3.10+ และตรง architecture ให้ใช้โฟลเดอร์ smoke ใหม่ที่ยังไม่มีอยู่ ผลผ่านต้องมี `verification.json` และ .bin สองไฟล์ จากนั้นทดสอบ DMG ที่ติดตั้งจริงด้วยโปรเจกต์ใหม่/เก่า การบันทึกไฟล์ Build ล้มเหลว/สำเร็จ และเลือก USB port สำหรับ Flash บอร์ดจริง การผ่าน CLI smoke ไม่เท่ากับผ่าน GUI หรือ Flash

ผลตรวจในเครื่องด้านล่างเป็นผลก่อนส่งขึ้น GitHub งานเตรียมรีลีสเปลี่ยนเวอร์ชันเป็น 3.7.2 และใช้ tag `v3.7.2` เพื่อเริ่ม CI; ตรวจสถานะจริงใน Actions และ Draft Release ไฟล์ดาวน์โหลดรุ่นเดิมไม่ได้รับแพตช์อัตโนมัติ

## ไฟล์ที่เปลี่ยน

- `src-tauri/src/macos_idf.rs` ใหม่: native installer, official environment export, preflight, process-tree cancellation
- `src-tauri/src/toolchain.rs`, `toolchain_storage.rs`, `esp_idf.rs`, `lib.rs`: การติดตั้ง/ตำแหน่ง/ตรวจ Ready/Build API ของ Mac
- `src-tauri/src/ai_chat.rs`, `ai/tools.rs`: หา CMake.app และใช้ SDK environment สำหรับ AI commands
- `src/App.tsx`, `src/ToolchainSetup.tsx`: argument-array Build, แสดงข้อความ repair ใน mini UI และยกเลิกการซ่อมได้
- `src/test/simulatorIntegration.test.tsx`, `toolchainActivation.test.tsx` และ Rust tests: custom SDK, false Ready, native layout/architecture, ports และข้อจำกัดการ patch venv
- `src-tauri/examples/macos-idf-smoke.rs`, `.github/workflows/macos-idf-smoke.yml`, `.github/workflows/build-macos.yml`: ติดตั้ง/Build firmware บน Mac จริงก่อน DMG
- `scripts/repair-macos-idf.command`, เอกสารนี้ และ `docs/macos-installer-setup.md`

ไม่มีการเพิ่ม dependency หรือเปลี่ยน Cargo.toml/Cargo.lock/package.json/package-lock.json ในงาน Mac รอบนี้ และไม่แก้ไฟล์ firmware โปรเจกต์ผู้ใช้

## ผลตรวจในเครื่อง Windows

| ตรวจสอบ | ผล |
|---|---|
| `cargo test --locked --lib` | ผ่าน 148 ข้อ ไม่มี failed/ignored |
| `cargo check --locked` | ผ่าน |
| `cargo check --locked --example macos-idf-smoke` | ผ่านการ compile-check บน Windows; ไม่ได้ execute Mac body |
| frontend Vitest | ผ่าน 148 ข้อ ใน 18 ไฟล์ |
| `npx tsc --noEmit` | ผ่าน |
| `npm run build:release` | ผ่าน frontend bundle และ hotfix/public/bundle checks; ไม่ใช่การสร้าง DMG |
| repair launcher | Bash syntax, --help และ config-preservation checks ผ่าน |
| compiled UI + Unity ในเครื่อง | ผ่าน: เปิด/หยุดซิม, Build หลังปิดซิม, save failure blocks Build; native IPC ใช้ fixture และไม่ Flash |
| macOS install / firmware smoke / GUI / Flash | ยังไม่ได้รันจริง; CI ถูกเพิ่มไว้แต่ยังไม่ dispatch |

การรัน cargo test แบบเต็มครั้งแรกติด disk full ตอนสร้าง static archive จึงล้างเฉพาะ Cargo incremental cache ที่ตรวจว่าอยู่ใต้ target ของโครงการ แล้วใช้ --lib/--check ตามตาราง ไม่ลบซอร์ส SDK หรือข้อมูลโปรเจกต์ ผู้ใช้ยังต้องยืนยัน Mac-specific tests ที่ถูกเลือกด้วย cfg บน runner จริง รวม process group/pyvenv preservation และ Build สอง CPU ไม่มีการนำผล Windows มาอ้างเป็นผลผ่าน Mac

## อ้างอิงเทคนิค

- [Espressif ESP-IDF v5.5.4: Linux/macOS setup, installation, export และข้อจำกัด path](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/get-started/linux-macos-setup.html)
- [GitHub-hosted runner architectures/labels](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
