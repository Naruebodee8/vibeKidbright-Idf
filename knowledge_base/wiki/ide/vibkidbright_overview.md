# vibeKidbright IDE — ภาพรวม

> **Raw source**: `raw/vibkidbright_readme.md`
> **Related**: [[code_signing]] · [[winget_release]]

---

## คืออะไร

**vibeKidbright** คือ AI-powered IDE สำหรับพัฒนา ESP-IDF firmware บน ESP32 โดยเฉพาะ
สร้างด้วย **Tauri + React + TypeScript** ให้ประสบการณ์ native desktop app

---

## ✨ Features หลัก

| Feature | รายละเอียด |
|---|---|
| **Project Management** | สร้าง/เปิด ESP-IDF project |
| **Vibe Coder (AI Assistant)** | Chat panel ที่อ่านโค้ด, แนะนำ, inject โค้ดได้เลย |
| **Interactive Terminal** | Terminal ในตัวสำหรับ build logs และ ESP-IDF commands |
| **One-Click Build & Flash** | Compile + Flash ด้วยปุ่มเดียว |
| **Modern Dark UI** | Tailwind CSS, premium dark theme |

---

## Board Support

- KidBright32 iA (INEX)
- Formula Kid Controller V1.1
- KidBright32 V1.5 Rev.3.1
- KidBright32 V1.5 Rev.3.1G

---

## Tech Stack

| Layer | Technology |
|---|---|
| Frontend | React · TypeScript · Vite |
| Styling | Tailwind CSS |
| Desktop/Backend | Tauri · Rust |
| Embedded | ESP-IDF |

---

## Prerequisites

- **Node.js** v18+
- **Rust** + Tauri CLI (`cargo install tauri-cli`)
- **ESP-IDF** (configured ใน environment variables)

---

## Development

```bash
# Clone
git clone https://github.com/your-repo/vibeKidbright.git
cd vibeKidbright
npm install

# Dev mode
npm run tauri dev
```

## Production Build

```bash
npm run build
npm run tauri build
# Output: src-tauri/target/release/bundle/
```

---

## Distribution

- **Code Signing**: ดู [[code_signing]] — เซ็น installer ด้วย Self-Signed Cert
- **Winget**: ดู [[winget_release]] — เผยแพร่ผ่าน Windows Package Manager

---

## License

MIT License

---

## See Also

- [[code_signing]] — Code signing workflow สำหรับ Windows installer
- [[winget_release]] — เผยแพร่แอปผ่าน `winget install`
