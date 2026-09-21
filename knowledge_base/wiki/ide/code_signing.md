# Code Signing — vibeKidbright IDE

> **Raw source**: `raw/ide_code_signing.md`
> **Related**: [[winget_release]] · [[vibkidbright_overview]]

---

## ภาพรวมระบบ

```
Developer Machine                    GitHub Actions CI
──────────────────                   ─────────────────────────────────
1. รัน create-cert.ps1               4. Decode PFX จาก Secret
   → สร้าง codesign.pfx (private)    5. Tauri Build + Auto-Sign
   → สร้าง codesign.cer (public)     6. sign-installer.ps1 (fallback)
2. อัปโหลด Secret ไป GitHub          7. Upload Release
3. แจก .cer ให้ผู้ใช้ import
```

ใช้ **Self-Signed Certificate** (ไม่ใช่ CA เชิงพาณิชย์) สำหรับเซ็น Windows Installer (.exe / .msi)

---

## สร้าง Certificate (ทำครั้งเดียว)

> ⚠️ อย่า commit ไฟล์ `.pfx` เข้า Git เด็ดขาด!

### Step 1: รัน create-cert.ps1
```powershell
$env:CERT_PASSWORD = "your-very-secure-password"
.\scripts\create-cert.ps1
```

**ผลลัพธ์:**
- `certs/codesign.pfx` — private key (เก็บเป็นความลับ)
- `certs/codesign.cer` — public cert (แจกผู้ใช้ได้)
- **Thumbprint** — ค่าสำหรับ GitHub Secret

### Step 2: แปลง PFX เป็น Base64
```powershell
$b64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes(".\certs\codesign.pfx"))
$b64 | Set-Content "certs\codesign.pfx.b64"
```

> ⚠️ ลบไฟล์ `certs/` ออกจาก repo หลังเสร็จ (อยู่ใน `.gitignore` แล้ว)

---

## GitHub Secrets ที่ต้องตั้ง

| Secret Name | ค่า | วิธีหา |
|---|---|---|
| `CERT_PFX_BASE64` | เนื้อหา `certs/codesign.pfx.b64` | คัดลอกจากไฟล์ |
| `CERT_PASSWORD` | password ที่ใช้ตอนสร้าง | ตั้งเองตอนรัน |
| `CERT_THUMBPRINT` | Thumbprint จาก output | คัดลอก output |

---

## วิธี Import .cer ให้ผู้ใช้

### วิธีที่ 1 — ดับเบิลคลิก (ง่ายสุด)
1. ดับเบิลคลิก `codesign.cer` → **Install Certificate**
2. **Local Machine** → **Place all certificates in the following store**
3. Browse → **Trusted Root Certification Authorities** → Finish

### วิธีที่ 2 — PowerShell
```powershell
# รันในฐานะ Administrator
Import-Certificate -FilePath ".\codesign.cer" `
    -CertStoreLocation "Cert:\LocalMachine\Root"
```

### ตรวจสอบ
```powershell
Get-ChildItem "Cert:\LocalMachine\Root" | Where-Object { $_.Subject -like "*Naruebodee8*" }
```

---

## SmartScreen Behavior

| สถานการณ์ | ผลลัพธ์ |
|---|---|
| ผู้ใช้ **ไม่ได้ import** .cer | ขึ้น SmartScreen — "Windows protected your PC" |
| ผู้ใช้ **import .cer แล้ว** | ไม่ขึ้น warning |
| ไฟล์ **ไม่มี signature** เลย | Warning รุนแรงกว่า (ไม่มีชื่อ Publisher) |

### ผ่าน SmartScreen ชั่วคราว
1. คลิก **More info** → **Run anyway**

---

## โครงสร้างไฟล์ที่เกี่ยวข้อง

```
vibeKidbright/
├── scripts/
│   ├── create-cert.ps1       ← สร้าง certificate (local only)
│   └── sign-installer.ps1    ← เซ็น installer (CI/local)
├── docs/
│   └── CODE-SIGNING.md
├── src-tauri/tauri.conf.json ← ตั้งค่า auto-sign
├── .github/workflows/build-windows.yml
└── certs/                    ← ⚠️ อยู่ใน .gitignore!
    ├── codesign.pfx
    └── codesign.cer
```

> Certificate มีอายุ **5 ปี** — เมื่อหมดให้รัน `create-cert.ps1` ใหม่และอัปเดต Secrets ทั้ง 3 ตัว

---

## See Also

- [[winget_release]] — เผยแพร่ผ่าน Windows Package Manager
- [[vibkidbright_overview]] — ภาพรวม vibeKidbright IDE
