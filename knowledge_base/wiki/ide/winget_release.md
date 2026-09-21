# Winget Release — vibeKidbright IDE

> **Raw source**: `raw/ide_winget_release.md`
> **Related**: [[code_signing]] · [[vibkidbright_overview]]

---

## เป้าหมาย

ให้ผู้ใช้ติดตั้ง vibeKidbright IDE ผ่าน Windows Package Manager ด้วยคำสั่งเดียว:
```cmd
winget install Naruebodee8.VibeKidbrightIDE
```

**Package ID**: `Naruebodee8.VibeKidbrightIDE`

---

## ขั้นตอนที่ 1: สร้าง GitHub PAT

จำเป็นสำหรับ CI ที่จะ Fork และเปิด PR ไปยัง `microsoft/winget-pkgs` อัตโนมัติ

1. GitHub → Settings → Developer Settings → Personal Access Tokens → Tokens (classic)
2. สร้าง token ใหม่: ชื่อ `winget-release-token`
3. Scope: ✅ `public_repo` เท่านั้น
4. คัดลอก token → ไปที่ Repository → Settings → Secrets → Actions
   - Name: `WINGET_GITHUB_TOKEN`
   - Value: token ที่สร้าง

---

## ขั้นตอนที่ 2: First-time Manual Submission

> Winget ต้องการเวอร์ชันแรกส่งแบบ Manual ก่อน — CI `wingetcreate update` ทำงานได้เฉพาะเมื่อมีเวอร์ชันในระบบแล้ว

### ติดตั้ง wingetcreate
```powershell
winget install Microsoft.WingetCreate
```

### ขั้นตอน
1. Build และปล่อย Release บน GitHub ก่อน เพื่อให้มี URL ดาวน์โหลด `.exe` จริง
2. คำนวณ SHA256:
   ```powershell
   Get-FileHash -Path "Kidbright_IDE_0.2.0_x64-setup.exe" -Algorithm SHA256
   ```
3. แก้ไฟล์ `winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0/Naruebodee8.VibeKidbrightIDE.installer.yaml`:
   - `InstallerUrl` → ลิงก์ GitHub Release จริง
   - `InstallerSha256` → ค่า SHA256 ที่คำนวณ
4. Validate:
   ```powershell
   wingetcreate validate "winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0"
   ```
5. Submit:
   ```powershell
   wingetcreate submit "winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0" --token <YOUR-PAT>
   ```

---

## ขั้นตอนที่ 3: ตรวจสอบ PR Status

1. ไปที่ [microsoft/winget-pkgs/pulls](https://github.com/microsoft/winget-pkgs/pulls)
2. ค้นหา `Naruebodee8.VibeKidbrightIDE`
3. รอ bot ตรวจสอบ ~10-30 นาที
4. เมื่อสถานะเป็นสีเขียวและ Admin Merge → แอปพร้อมใช้ผ่าน `winget install`

---

## ⚠️ SmartScreen & Self-Signed Certificate

เนื่องจาก vibeKidbright ใช้ Self-Signed Certificate:

- **Microsoft bot อาจ flag**: ป้ายเตือน Unknown Publisher
- **แก้ไข**: แจ้ง Moderator ว่าแอปใช้ Self-Signed เพื่อการศึกษา/พัฒนาภายใน
- **คำแนะนำผู้ใช้**: ให้ import `.cer` เข้า Trusted Root ก่อนติดตั้ง (ดู [[code_signing]])

---

## Manifest Location

```
winget/manifests/n/Naruebodee8/VibeKidbrightIDE/
└── 0.2.0/
    ├── Naruebodee8.VibeKidbrightIDE.yaml
    ├── Naruebodee8.VibeKidbrightIDE.installer.yaml
    └── Naruebodee8.VibeKidbrightIDE.locale.en-US.yaml
```

---

## See Also

- [[code_signing]] — Code Signing ด้วย Self-Signed Certificate
- [[vibkidbright_overview]] — ภาพรวม vibeKidbright IDE
