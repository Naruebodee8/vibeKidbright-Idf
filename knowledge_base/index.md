# 📚 KidBright Knowledge Base — LLM Wiki Index

> **ระบบ**: LLM Wiki สำหรับ vibeKidbright IDE
> **โครงสร้าง**: ไฟล์ต้นฉบับอยู่ใน `/raw` (ห้ามแก้) · หน้าสรุปอยู่ใน `/wiki`
> **อัพเดตล่าสุด**: 2026-08-24

---

## 🔧 Hardware

หน้าเกี่ยวกับบอร์ด KidBright32 และ GPIO

| หน้าวิกิ | เนื้อหา | Raw Source |
|---------|--------|-----------|
| [[kidbright32_reference]] | ประวัติรุ่น ทุกรุ่น V2016–μAI, GPIO quick ref | `kidbright32iA_updated.md` (3600+ lines) |
| [[kidbright32_pinout]] | Pinout ละเอียดทุก GPIO | `kidbright32iA_updated.md` |
| [[sensor_guide]] | Sensor map ทุกรุ่น, ADC API v5.x, I2C init order | `kidbright_sensor_guide.md` |
| [[adc_api_guide]] | ADC Oneshot API รายละเอียด (ESP-IDF v5.x) | `kidbright32iA_updated.md` |
| [[gpio_conflict]] | GPIO conflict table ทุกรุ่น | `kidbright32iA_updated.md` |

---

## 🔌 Peripherals

หน้าเกี่ยวกับ peripheral ที่ต่อกับ KidBright

| หน้าวิกิ | เนื้อหา | Raw Source |
|---------|--------|-----------|
| [[led_matrix]] | LED Matrix 16×8 (HT16K33), interleaved mapping, anti-patterns | `led_16x8_matrix_mapping.md` |
| [[oled_sh1106]] | OLED SH1106 128×64, framebuffer, column offset +2 | `oled_sh1106_template.md` |

---

## 🚗 Projects

หน้าเกี่ยวกับโปรเจกต์ KidBright แต่ละรูปแบบ

| หน้าวิกิ | เนื้อหา | Raw Source |
|---------|--------|-----------|
| [[formula_kid]] | Formula Kid Controller สนามแข่ง | `formula_kid_controller.md` |
| [[skate]] | KidBright Skate V1.3 — 2-wheel self-balancing robot | `skate_rev1_3.md` |
| [[minibike]] | Minibike Workshop — Self-balancing + ESP-NOW + Line Follower | `minibike_workshop.md` |
| [[micromouse]] | Micromouse ภาพรวม + Lab Guide ทีละ STEP | `micromouse_overview.md` + `step_guide.md` + `calibration.md` + `library.md` |
| [[micromouse_architecture]] | Flowchart + Sequence diagram 4-layer architecture | `micromouse_architecture.md` |
| [[micromouse_ble]] | BLE wireless testing, mm_ui.py, คำสั่งต่อ Lab | `micromouse_ble_testing.md` |

---

## 💻 IDE & DevOps

หน้าเกี่ยวกับ vibeKidbright IDE และการ deploy

| หน้าวิกิ | เนื้อหา | Raw Source |
|---------|--------|-----------|
| [[vibkidbright_overview]] | ภาพรวม vibeKidbright IDE, Tech Stack, Build | `vibkidbright_readme.md` |
| [[code_signing]] | Code Signing ด้วย Self-Signed Certificate, GitHub Secrets | `ide_code_signing.md` |
| [[winget_release]] | เผยแพร่ผ่าน Windows Package Manager | `ide_winget_release.md` |

---

## 📁 โครงสร้างโฟลเดอร์

```
knowledge_base/
├── index.md              ← 📍 คุณอยู่ที่นี่
├── raw/                  ← ⚠️ ไฟล์ต้นฉบับ — ห้ามแก้ไข
│   ├── kidbright32iA_updated.md
│   ├── kidbright_sensor_guide.md
│   ├── led_16x8_matrix_mapping.md
│   ├── oled_sh1106_template.md
│   ├── formula_kid_controller.md
│   ├── skate_rev1_3.md
│   ├── minibike_workshop.md
│   ├── micromouse_overview.md
│   ├── micromouse_architecture.md
│   ├── micromouse_step_guide.md
│   ├── micromouse_ble_testing.md
│   ├── micromouse_calibration_worksheet.md
│   ├── micromouse_library_readme.md
│   ├── ide_code_signing.md
│   ├── ide_winget_release.md
│   └── vibkidbright_readme.md
└── wiki/                 ← ✅ หน้าสรุปที่ AI สร้าง
    ├── hardware/
    │   ├── kidbright32_reference.md
    │   ├── kidbright32_pinout.md
    │   ├── sensor_guide.md
    │   ├── adc_api_guide.md
    │   └── gpio_conflict.md
    ├── peripherals/
    │   ├── led_matrix.md
    │   └── oled_sh1106.md
    ├── projects/
    │   ├── formula_kid.md
    │   ├── skate.md
    │   ├── minibike.md
    │   ├── micromouse.md
    │   ├── micromouse_architecture.md
    │   └── micromouse_ble.md
    └── ide/
        ├── vibkidbright_overview.md
        ├── code_signing.md
        └── winget_release.md
```

---

## 🔗 Backlink Map (ความสัมพันธ์ระหว่างหน้า)

```
kidbright32_reference ←→ kidbright32_pinout
                      ←→ sensor_guide
                      ←→ adc_api_guide
                      ←→ gpio_conflict

sensor_guide ←→ adc_api_guide
             ←→ gpio_conflict
             ←→ kidbright32_pinout

led_matrix ←→ oled_sh1106 (ใช้ I2C_NUM_0 เดียวกัน)
           ←→ sensor_guide

skate ←→ sensor_guide (I2C init)
      ←→ gpio_conflict

minibike ←→ sensor_guide
         ←→ formula_kid

micromouse ←→ micromouse_architecture
           ←→ micromouse_ble

vibkidbright_overview ←→ code_signing
                      ←→ winget_release
```

---

## ⚠️ กฎทองสำหรับ AI

| กฎ | รายละเอียด |
|---|---|
| ADC API | ใช้ `esp_adc/adc_oneshot.h` เสมอ — ห้ามใช้ `driver/adc.h` |
| Struct name | `adc_oneshot_unit_init_cfg_t` (ไม่ใช่ `...config_t`) |
| XTAL frequency | KidBright ใช้ **26 MHz** (ไม่ใช่ 40 MHz) |
| I2C init | เรียก `i2c_driver_install()` ได้ครั้งเดียวต่อ port |
| SW2 | GPIO14 ทุกรุ่น V1.5+ |
| Pre-coding rule | กิจกรรม Minibike: ถามค่า config ก่อนเสมอ (ยกเว้น 2.1 และ 3.1) |
