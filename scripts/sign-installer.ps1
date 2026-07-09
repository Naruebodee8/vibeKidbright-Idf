# ============================================================
# scripts/sign-installer.ps1
# เซ็น Code Signing บน installer ที่ build จาก Tauri
#
# ใช้งานบนเครื่อง local:
#   $env:CERT_PASSWORD = "your-password"
#   $env:CERT_PFX_PATH = ".\certs\codesign.pfx"   # optional, ถ้าไม่ตั้งใช้ค่า default
#   .\scripts\sign-installer.ps1
#
# ใช้งานบน CI (GitHub Actions) จะถูกเรียกอัตโนมัติ หลัง tauri build
# ============================================================

# ── ค้นหา signtool.exe ──────────────────────────────────────────
function Find-SignTool {
    $candidates = @(
        # Windows 10/11 SDK 10.0.x
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\signtool.exe"
    )
    # ค้นหาจาก wildcard ถ้ายังหาไม่เจอ
    foreach ($path in $candidates) {
        if (Test-Path $path) { return $path }
    }
    $found = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Recurse -Filter "signtool.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { return $found.FullName }
    throw "ไม่พบ signtool.exe กรุณาติดตั้ง Windows SDK"
}

# ── กำหนด Path ───────────────────────────────────────────────────
$SignTool     = Find-SignTool
$BundleDir    = Join-Path $PSScriptRoot "..\src-tauri\target\release\bundle"
$TimestampUrl = "http://timestamp.digicert.com"
$DigestAlgo   = "SHA256"

# ── รับ PFX และ Password ─────────────────────────────────────────
# ทำงานได้ทั้ง 2 โหมด:
#   1. Local: ระบุ path ของ .pfx ผ่าน env CERT_PFX_PATH
#   2. CI:    รับ path จากไฟล์ชั่วคราวที่ CI decode มาให้

if ($env:CERT_PFX_PATH) {
    $PfxPath = $env:CERT_PFX_PATH
} else {
    $PfxPath = Join-Path $PSScriptRoot "..\certs\codesign.pfx"
}

if (-not (Test-Path $PfxPath)) {
    Write-Error "ERROR: ไม่พบไฟล์ PFX ที่ '$PfxPath'"
    Write-Host "  กรุณาสร้าง cert ก่อนด้วย: .\scripts\create-cert.ps1"
    exit 1
}

if (-not $env:CERT_PASSWORD) {
    Write-Error "ERROR: กรุณาตั้งค่า environment variable CERT_PASSWORD"
    exit 1
}

# ── ค้นหาไฟล์ installer ทั้งหมด (.exe และ .msi) ──────────────────
$Installers = @()
$Installers += Get-ChildItem -Path $BundleDir -Recurse -Include "*.exe" -ErrorAction SilentlyContinue
$Installers += Get-ChildItem -Path $BundleDir -Recurse -Include "*.msi" -ErrorAction SilentlyContinue

if ($Installers.Count -eq 0) {
    Write-Error "ERROR: ไม่พบไฟล์ installer ใน $BundleDir"
    Write-Host "  กรุณา build โปรเจกต์ก่อนด้วย: npm run tauri build"
    exit 1
}

Write-Host "[OK] พบ $($Installers.Count) ไฟล์สำหรับเซ็น:"
$Installers | ForEach-Object { Write-Host "  - $($_.FullName)" }
Write-Host ""

# ── เซ็นแต่ละไฟล์ ─────────────────────────────────────────────────
$SuccessCount = 0
$FailCount    = 0

foreach ($Installer in $Installers) {
    Write-Host "[...] กำลังเซ็น: $($Installer.Name)"

    # ไม่ใช้ $result = ... 2>&1 เพราะ signtool.exe ไม่รองรับบน CI
    # ใช้ Start-Process แทน เพื่อหลีกเลี่ยง StandardOutputEncoding error
    $proc = Start-Process -FilePath $SignTool `
        -ArgumentList @(
            "sign",
            "/fd", $DigestAlgo,
            "/f",  $PfxPath,
            "/p",  $env:CERT_PASSWORD,
            "/tr", $TimestampUrl,
            "/td", $DigestAlgo,
            $Installer.FullName
        ) `
        -Wait `
        -PassThru `
        -NoNewWindow

    if ($proc.ExitCode -eq 0) {
        Write-Host "[OK] เซ็นสำเร็จ: $($Installer.Name)"
        $SuccessCount++
    } else {
        Write-Warning "[FAIL] เซ็นไม่สำเร็จ: $($Installer.Name) (ExitCode=$($proc.ExitCode))"
        $FailCount++
    }
}

Write-Host ""
Write-Host "============================================"
Write-Host "  ผลลัพธ์: สำเร็จ $SuccessCount | ล้มเหลว $FailCount"
Write-Host "============================================"

if ($FailCount -gt 0) { exit 1 }
