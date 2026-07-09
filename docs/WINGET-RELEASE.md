# คู่มือการเผยแพร่แอปบน Windows Package Manager (Winget)

คู่มือนี้แนะนำขั้นตอนการนำแอปพลิเคชัน **vibeKidbright IDE** (`Naruebodee8.VibeKidbrightIDE`) เข้าสู่ระบบ Winget เพื่อให้ผู้ใช้งานติดตั้งแอปผ่าน PowerShell/CMD ได้ง่ายๆ ด้วยคำสั่ง:
```cmd
winget install Naruebodee8.VibeKidbrightIDE
```

---

## 1. วิธีสร้าง GitHub Personal Access Token (PAT)
การอัปเดตแอปบน Winget แบบอัตโนมัติผ่าน GitHub Actions (CI/CD) จำเป็นต้องใช้สิทธิ์ของบัญชีคุณในการ Fork และสร้าง Pull Request ไปยังคลังเก็บหลักของ Microsoft (`microsoft/winget-pkgs`).

### ขั้นตอนการสร้าง Token:
1. ไปที่ GitHub -> **Settings** -> **Developer Settings** -> **Personal Access Tokens** -> **Tokens (classic)**
2. คลิก **Generate new token (classic)**
3. ตั้งชื่อ Note เช่น `winget-release-token`
4. เลือก Scope ดังนี้:
   - **`public_repo`** (จำเป็นเพื่อเข้าถึงและ Fork คลังสาธารณะ)
5. กด **Generate token** และคัดลอกรหัส Token เก็บไว้
6. ไปที่ Repository ของคุณใน GitHub -> **Settings** -> **Secrets and variables** -> **Actions**
7. คลิก **New repository secret**
   - Name: `WINGET_GITHUB_TOKEN`
   - Value: วางรหัส Token ที่สร้างไว้ลงไป
   - กด **Add secret**

---

## 2. ขั้นตอนการส่งคำขอเผยแพร่ครั้งแรกแบบ Manual (First-time Submission)
เนื่องจากแอปยังไม่เคยอยู่ในฐานข้อมูลของ Winget มาก่อน คำสั่ง `wingetcreate update` ของ GitHub Actions จะยังไม่สามารถทำงานได้ (จะทำงานได้เมื่อมีเวอร์ชันแรกอยู่ในระบบแล้วเท่านั้น) ดังนั้นใน **เวอร์ชันแรก** ต้องส่งคำขอแบบแมนนวลก่อน:

### วิธีใช้ไฟล์ Manifest ที่เตรียมไว้:
เราได้เตรียมโครงสร้างไฟล์ Manifest สำหรับเวอร์ชัน `0.2.0` ไว้ในโฟลเดอร์ `winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0/` แล้ว

1. ติดตั้งเครื่องมือ `wingetcreate` บนเครื่องของคุณผ่าน PowerShell (Admin):
   ```powershell
   winget install Microsoft.WingetCreate
   ```
2. ทำการคอมไพล์และปล่อย Release ขึ้น GitHub ก่อน เพื่อให้ได้ลิงก์ดาวน์โหลดตัวติดตั้ง `.exe` จริง (ตัวอย่างลิงก์: `https://github.com/Naruebodee8/vibeKidbright-Idf/releases/download/v0.2.0/Kidbright_IDE_0.2.0_x64-setup.exe`)
3. ทำการตรวจสอบและคำนวณค่า SHA256 ของไฟล์ตัวติดตั้ง `.exe` ตัวจริงที่คุณดาวน์โหลดมา:
   ```powershell
   Get-FileHash -Path "เส้นทางไปยังไฟล์\Kidbright_IDE_0.2.0_x64-setup.exe" -Algorithm SHA256
   ```
4. เปิดไฟล์ `winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0/Naruebodee8.VibeKidbrightIDE.installer.yaml`
   - เปลี่ยนลิงก์ `InstallerUrl` ให้เป็นลิงก์ดาวน์โหลดตัวจริงที่ต้องการเผยแพร่
   - นำค่า SHA256 ที่คำนวณได้ในข้อ 3 ไปเขียนแทนที่ค่า `000000...` ในบรรทัด `InstallerSha256`
5. ทดสอบความถูกต้องของ Manifest ในเครื่องก่อนส่ง (Validate):
   ```powershell
   wingetcreate validate "winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0"
   ```
6. ส่งไฟล์คำขอขึ้นไปยัง Winget:
   ```powershell
   wingetcreate submit "winget/manifests/n/Naruebodee8/VibeKidbrightIDE/0.2.0" --token <GitHub-PAT-ของคุณ>
   ```
   *(เครื่องมือจะทำส่ง Fork และเปิด Pull Request บน GitHub ของ Microsoft ให้โดยอัตโนมัติ)*

---

## 3. วิธีเช็คสถานะการตรวจสอบ (Pull Request)
หลังจากทำการส่งคำขอแล้ว:
1. ให้เข้าไปดูที่หน้า Pull Requests ของคลัง [microsoft/winget-pkgs/pulls](https://github.com/microsoft/winget-pkgs/pulls)
2. ค้นหาชื่อแอปของคุณ เช่น `Naruebodee8.VibeKidbrightIDE`
3. ระบบทดสอบอัตโนมัติของ Microsoft จะรันตรวจสอบความปลอดภัยและความเข้ากันได้ (ประมาณ 10-30 นาที)
4. เมื่อสถานะเป็นสีเขียว (Passed) และมี Admin กด Merge คำขอ แอปพลิเคชันของคุณก็จะสามารถติดตั้งผ่าน `winget install` ได้ทันที!

---

## ⚠️ คำเตือนเรื่องความปลอดภัย (SmartScreen / Self-Signed Cert)
เนื่องจากแอปของเราใช้วิธีลงนามดิจิทัลแบบ **Self-Signed Certificate** (ไม่ได้ซื้อจากผู้ให้บริการใบรับรองเชิงพาณิชย์ที่เป็นที่ยอมรับโดยปริยาย เช่น DigiCert หรือ Sectigo)
* **การตรวจสอบอัตโนมัติ:** เมื่อ Microsoft รันระบบทดสอบเพื่อวิเคราะห์ตัวติดตั้ง อาจเกิดป้ายเตือนเรื่อง **Windows SmartScreen (Unknown Publisher)** หรือไม่ผ่านเกณฑ์การทดสอบแบบเงียบ (Silent test) เนื่องจากไม่เชื่อถือใบรับรองนั้น
* **การแก้ไข:** ใน PR ดังกล่าว อาจมีเจ้าหน้าที่ผู้ดูแลระบบ (Admin/Moderator) ของ Microsoft เข้ามาถามข้อมูลเพิ่มเติม หรือตรวจสอบด้วยตัวบุคคล (Manual Review) ให้คุณแจ้งแก่ทีมงานว่าแอปพลิเคชันนี้ใช้เพื่อการพัฒนาภายในหรือเพื่อการศึกษาและใช้วิธีลงนามแบบ Self-signed เพื่อความปลอดภัยของรหัสภายในแอปพลิเคชัน เมื่อเจ้าหน้าที่อนุมัติก็จะผ่านเข้าสู่ระบบได้ปกติ
* **คำแนะนำสำหรับผู้ใช้:** แนะนำให้สร้างเนื้อหาคู่มืออธิบายแก่ผู้ใช้งานว่าจำเป็นต้องยอมรับป้ายเตือน SmartScreen ระหว่างติดตั้ง หรือทำการนำเข้าไฟล์ `.cer` (Public key) ไปยัง `Trusted Root Certification Authorities` ในเครื่องผู้ใช้งานก่อนการรันติดตั้งแบบเงียบ
