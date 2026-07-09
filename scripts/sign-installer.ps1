# ============================================================
# scripts/sign-installer.ps1
# เซ็น Code Signing บน installer ที่ build จาก Tauri
#
# ใช้งานบนเครื่อง local:
#   $env:CERT_PASSWORD = "your-password"
#   $env:CERT_PFX_PATH = ".\certs\codesign.pfx"   # optional
#   .\scripts\sign-installer.ps1
#
# ใช้งานบน CI (GitHub Actions): ถูกเรียกอัตโนมัติหลัง tauri build
# ============================================================

# ── ค้นหา signtool.exe ──────────────────────────────────────────
function Find-SignTool {
    # 1. ตรวจใน PATH ก่อนเสมอ — GitHub Actions มี signtool ใน PATH อยู่แล้ว
    $inPath = Get-Command "signtool.exe" -ErrorAction SilentlyContinue
    if ($inPath) {
        Write-Host "[INFO] signtool found in PATH: $($inPath.Source)"
        return $inPath.Source
    }

    # 2. ค้นหาใน Windows Kits — เรียงจากใหม่ไปเก่า เอาตัวใหม่สุด
    $kitsBin = "C:\Program Files (x86)\Windows Kits\10\bin"
    if (Test-Path $kitsBin) {
        $found = Get-ChildItem $kitsBin -Recurse -Filter "signtool.exe" -ErrorAction SilentlyContinue |
                 Where-Object { $_.FullName -match "x64" } |
                 Sort-Object FullName -Descending |
                 Select-Object -First 1
        if ($found) {
            Write-Host "[INFO] signtool found in Windows Kits: $($found.FullName)"
            return $found.FullName
        }
    }

    throw "ไม่พบ signtool.exe — กรุณาติดตั้ง Windows SDK หรือ Visual Studio Build Tools"
}

# ── กำหนด Path ───────────────────────────────────────────────────
$SignTool     = Find-SignTool
$BundleDir    = Join-Path $PSScriptRoot "..\src-tauri\target\release\bundle"
$TimestampUrl = "http://timestamp.digicert.com"
$DigestAlgo   = "SHA256"

# ── รับ PFX Path ─────────────────────────────────────────────────
if ($env:CERT_PFX_PATH) {
    $PfxPath = $env:CERT_PFX_PATH
} else {
    $PfxPath = Join-Path $PSScriptRoot "..\certs\codesign.pfx"
}

if (-not (Test-Path $PfxPath)) {
    Write-Error "ERROR: ไม่พบไฟล์ PFX ที่ '$PfxPath'"
    exit 1
}

if (-not $env:CERT_PASSWORD) {
    Write-Error "ERROR: กรุณาตั้งค่า CERT_PASSWORD environment variable"
    exit 1
}

# ── ค้นหาไฟล์ installer (.exe และ .msi) ──────────────────────────
$Installers = @()
$Installers += Get-ChildItem -Path $BundleDir -Recurse -Include "*.exe" -ErrorAction SilentlyContinue
$Installers += Get-ChildItem -Path $BundleDir -Recurse -Include "*.msi" -ErrorAction SilentlyContinue

# ตัด .rss/.zip และไฟล์ที่ไม่ใช่ installer จริงออก
$Installers = $Installers | Where-Object {
    $_.Name -notmatch "\.(rss|zip|sig)$" -and
    ($_.Name -match "setup" -or $_.Name -match "installer" -or $_.Extension -eq ".msi")
}

if ($Installers.Count -eq 0) {
    Write-Error "ERROR: ไม่พบไฟล์ installer ใน $BundleDir"
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

    # ใช้ & โดยตรง — ไม่ capture output ($result = ...) และไม่ใช้ 2>&1
    # เพราะ signtool.exe ไม่รองรับ StandardOutputEncoding บน CI
    & $SignTool sign `
        /fd $DigestAlgo `
        /f  "$PfxPath" `
        /p  "$($env:CERT_PASSWORD)" `
        /tr $TimestampUrl `
        /td $DigestAlgo `
        "$($Installer.FullName)"

    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] เซ็นสำเร็จ: $($Installer.Name)"
        $SuccessCount++
    } else {
        Write-Warning "[FAIL] เซ็นไม่สำเร็จ: $($Installer.Name) (ExitCode=$LASTEXITCODE)"
        $FailCount++
    }
}

Write-Host ""
Write-Host "============================================"
Write-Host "  ผลลัพธ์: สำเร็จ $SuccessCount | ล้มเหลว $FailCount"
Write-Host "============================================"

if ($FailCount -gt 0) { exit 1 }
