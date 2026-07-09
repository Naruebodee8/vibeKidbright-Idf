# ============================================================
# scripts/create-cert.ps1
# สร้าง Self-Signed Code Signing Certificate สำหรับ vibeKidbright
#
# ใช้งาน:
#   $env:CERT_PASSWORD = "your-password"
#   .\scripts\create-cert.ps1
# ============================================================

# ── ตรวจสอบว่า environment variable ถูกตั้งค่าแล้ว ─────────────
if (-not $env:CERT_PASSWORD) {
    Write-Error "ERROR: กรุณาตั้งค่า environment variable CERT_PASSWORD ก่อนรันสคริปต์นี้"
    Write-Host "  ตัวอย่าง: `$env:CERT_PASSWORD = 'your-secure-password'"
    exit 1
}

$Subject    = "CN=Naruebodee8"
$OutputDir  = Join-Path $PSScriptRoot "..\certs"
$PfxPath    = Join-Path $OutputDir "codesign.pfx"
$CerPath    = Join-Path $OutputDir "codesign.cer"

# สร้างโฟลเดอร์ certs/ ถ้ายังไม่มี
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "[OK] สร้างโฟลเดอร์ certs/ แล้ว"
}

# ── สร้าง Certificate ใน Certificate Store ─────────────────────
Write-Host "[...] กำลังสร้าง Self-Signed Certificate..."
$cert = New-SelfSignedCertificate `
    -Subject $Subject `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -KeyUsage DigitalSignature `
    -Type CodeSigningCert `
    -KeyAlgorithm RSA `
    -KeyLength 2048 `
    -HashAlgorithm SHA256 `
    -NotAfter (Get-Date).AddYears(5)

Write-Host "[OK] สร้าง Certificate สำเร็จ: Thumbprint = $($cert.Thumbprint)"

# ── Export .pfx (มี private key) ─────────────────────────────────
$SecurePassword = ConvertTo-SecureString -String $env:CERT_PASSWORD -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath $PfxPath -Password $SecurePassword | Out-Null
Write-Host "[OK] Export .pfx ไปที่: $PfxPath"

# ── Export .cer (public cert สำหรับแจก) ─────────────────────────
Export-Certificate -Cert $cert -FilePath $CerPath -Type CERT | Out-Null
Write-Host "[OK] Export .cer ไปที่: $CerPath"

# ── แสดง Thumbprint สำหรับใส่ใน tauri.conf.json ─────────────────
Write-Host ""
Write-Host "============================================"
Write-Host "  Certificate Thumbprint (คัดลอกไปใส่ใน CI):"
Write-Host "  $($cert.Thumbprint)"
Write-Host "============================================"
Write-Host ""
Write-Host "ขั้นตอนถัดไป:"
Write-Host "  1. นำค่า Thumbprint ด้านบนไปตั้งเป็น GitHub Secret ชื่อ CERT_THUMBPRINT"
Write-Host "  2. Export PFX เป็น Base64 แล้วตั้งเป็น GitHub Secret ชื่อ CERT_PFX_BASE64:"
Write-Host "     `$b64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes('$PfxPath'))"
Write-Host "     `$b64 | Set-Content certs\codesign.pfx.b64"
Write-Host "  3. นำ .cer ไปแจกให้ผู้ใช้ import เข้า Trusted Root (ดู docs/CODE-SIGNING.md)"
Write-Host "  4. ลบไฟล์ certs/ ออกจาก repo (อย่า commit .pfx เด็ดขาด!)"
