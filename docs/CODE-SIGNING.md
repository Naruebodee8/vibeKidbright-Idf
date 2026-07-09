# ============================================================
# docs/CODE-SIGNING.md
# คู่มือ Code Signing สำหรับ vibeKidbright IDE
# ============================================================

# Code Signing — vibeKidbright IDE

> [!NOTE]
> คู่มือนี้อธิบายระบบ Code Signing ด้วย **Self-Signed Certificate**
> ที่ใช้ในโปรเจกต์ vibeKidbright IDE สำหรับ Windows Installer (.exe / .msi)

---

## 📋 สารบัญ

1. [ภาพรวมระบบ](#ภาพรวมระบบ)
2. [สร้าง Certificate (ทำครั้งแรกครั้งเดียว)](#สร้าง-certificate)
3. [ตั้งค่า GitHub Secrets](#ตั้งค่า-github-secrets)
4. [วิธี Import .cer ให้ผู้ใช้](#วิธี-import-cer-ให้ผู้ใช้)
5. [คำเตือน SmartScreen](#คำเตือน-smartscreen)
6. [อัปเดตเวอร์ชันใหม่](#อัปเดตเวอร์ชันใหม่)

---

## ภาพรวมระบบ

```
Developer Machine                    GitHub Actions CI
──────────────────                   ─────────────────────────────────
1. รัน create-cert.ps1               4. Decode PFX จาก Secret
   → สร้าง codesign.pfx (private)      5. Tauri Build + Auto-Sign
   → สร้าง codesign.cer (public)        6. sign-installer.ps1 (fallback)
2. อัปโหลด Secret ไป GitHub            7. Upload Release
3. แจก .cer ให้ผู้ใช้ import
```

---

## สร้าง Certificate

> [!IMPORTANT]
> ทำขั้นตอนนี้บนเครื่อง Windows ของคุณ ทำเพียงครั้งเดียว
> อย่า commit ไฟล์ `.pfx` เข้า Git เด็ดขาด!

### ขั้นตอนที่ 1 — รันสคริปต์สร้าง Certificate

เปิด **PowerShell** ในฐานะ Administrator แล้วรัน:

```powershell
# 1. ตั้ง password ที่ต้องการ (จะถูกใช้ protect ไฟล์ .pfx)
$env:CERT_PASSWORD = "your-very-secure-password"

# 2. สร้าง certificate
.\scripts\create-cert.ps1
```

ผลลัพธ์ที่ได้:
- `certs/codesign.pfx` — ไฟล์ certificate พร้อม private key (**เก็บเป็นความลับ**)
- `certs/codesign.cer` — ไฟล์ public certificate (แจกผู้ใช้ได้)
- **Thumbprint** — ค่าสำหรับใส่ใน GitHub Secret

### ขั้นตอนที่ 2 — แปลง .pfx เป็น Base64

```powershell
$b64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes(".\certs\codesign.pfx"))
$b64 | Set-Content "certs\codesign.pfx.b64"
```

> [!CAUTION]
> ลบไฟล์ `certs/` ออกจาก repo ทันทีหลังเสร็จ
> โฟลเดอร์ `certs/` ถูก `.gitignore` ไว้แล้ว แต่ควรตรวจสอบอีกครั้ง

---

## ตั้งค่า GitHub Secrets

ไปที่ GitHub Repository → **Settings** → **Secrets and variables** → **Actions** → **New repository secret**

| Secret Name | ค่า | วิธีหา |
|---|---|---|
| `CERT_PFX_BASE64` | เนื้อหาในไฟล์ `certs/codesign.pfx.b64` | คัดลอกจากไฟล์ที่สร้างในขั้นตอน 2 |
| `CERT_PASSWORD` | password ที่ใช้ตอน `$env:CERT_PASSWORD` | ตั้งเองตอนรัน create-cert.ps1 |
| `CERT_THUMBPRINT` | ค่า Thumbprint ที่แสดงหลังรันสคริปต์ | คัดลอกจาก output ของ create-cert.ps1 |

> [!WARNING]
> ห้าม commit ค่า Secret ใดๆ ลง repo โดยตรง
> GitHub Secrets จะถูก mask ในทุก log อัตโนมัติ

---

## วิธี Import .cer ให้ผู้ใช้

> ทำขั้นตอนนี้เพื่อให้ผู้ใช้ที่ได้รับไฟล์ `.cer` ไม่เห็น warning ขณะติดตั้ง

### วิธีที่ 1 — ดับเบิลคลิก (ง่ายที่สุด)

1. ดาวน์โหลดไฟล์ `codesign.cer`
2. ดับเบิลคลิกที่ไฟล์ → คลิก **Install Certificate**
3. เลือก **Local Machine** → คลิก **Next**
4. เลือก **Place all certificates in the following store**
5. คลิก **Browse** → เลือก **Trusted Root Certification Authorities**
6. คลิก **Next** → **Finish**

### วิธีที่ 2 — PowerShell (สำหรับ Admin)

```powershell
# รันในฐานะ Administrator
Import-Certificate -FilePath ".\codesign.cer" `
    -CertStoreLocation "Cert:\LocalMachine\Root"
Write-Host "Import สำเร็จ!"
```

### ตรวจสอบว่า Import แล้ว

```powershell
Get-ChildItem "Cert:\LocalMachine\Root" | Where-Object { $_.Subject -like "*Naruebodee8*" }
```

---

## คำเตือน SmartScreen

> [!WARNING]
> **Self-Signed Certificate ยังคงขึ้น Windows SmartScreen warning** สำหรับผู้ใช้ทั่วไป
> ที่ไม่ได้ import .cer เข้าเครื่องของตัวเอง

### สรุปพฤติกรรม SmartScreen

| สถานการณ์ | ผลลัพธ์ |
|---|---|
| ผู้ใช้ **ไม่ได้ import** .cer | ขึ้น SmartScreen — "Windows protected your PC" |
| ผู้ใช้ **import .cer แล้ว** | ไม่ขึ้น warning, เห็น Certificate ปรากฏใน Properties |
| ไฟล์ **ไม่มี signature** เลย | ขึ้น warning รุนแรงกว่า (ไม่มีข้อมูล Publisher) |

### วิธีผ่าน SmartScreen ชั่วคราว (สำหรับผู้ใช้ทั่วไป)

1. เมื่อขึ้น SmartScreen → คลิก **More info**
2. คลิก **Run anyway**

> [!NOTE]
> การมี Digital Signature (แม้เป็น self-signed) ทำให้ผู้ใช้เห็นชื่อ Publisher ในหน้าต่าง Properties
> และใน Digital Signatures tab ซึ่งช่วยสร้างความน่าเชื่อถือมากกว่าไม่มี signature เลย

---

## อัปเดตเวอร์ชันใหม่

เมื่อ push โค้ดไปที่ branch `main` หรือกด **Run workflow** บน GitHub Actions:

1. CI จะดึง PFX จาก Secret โดยอัตโนมัติ
2. Tauri จะ build และเซ็น installer ด้วย certificate เดิม
3. `sign-installer.ps1` จะเซ็นซ้ำเพื่อให้ครอบคลุมทุกไฟล์
4. Release ถูกสร้างเป็น Draft ให้ตรวจสอบก่อน Publish

> Certificate มีอายุ **5 ปี** นับจากวันที่สร้าง
> เมื่อหมดอายุให้รัน `create-cert.ps1` ใหม่แล้วอัปเดต Secret ทั้ง 3 ตัว

---

## โครงสร้างไฟล์ที่เกี่ยวข้อง

```
vibeKidbright/
├── scripts/
│   ├── create-cert.ps1          ← สร้าง certificate (รันบน local)
│   └── sign-installer.ps1       ← เซ็น installer (รันบน CI หรือ local)
├── docs/
│   └── CODE-SIGNING.md          ← คู่มือนี้
├── src-tauri/
│   └── tauri.conf.json          ← ตั้งค่า auto-sign ผ่าน env var
├── .github/workflows/
│   └── build-windows.yml        ← CI pipeline + signing steps
└── certs/                       ← ⚠️ อยู่ใน .gitignore, ห้าม commit!
    ├── codesign.pfx             ← private key (เก็บเป็นความลับ)
    └── codesign.cer             ← public cert (แจกผู้ใช้ได้)
```
