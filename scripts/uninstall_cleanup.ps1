# ============================================================
# VibeKidbright — Complete Uninstall Script
# ลบข้อมูลทั้งหมดที่แอปสร้างไว้: config, KB cache, ESP-IDF runtime, toolchain
# ============================================================
# วิธีใช้: คลิกขวา → Run with PowerShell (as Administrator ถ้าจำเป็น)
# ============================================================

param(
    [switch]$KeepProjects,   # ถ้าใส่ -KeepProjects จะไม่ลบไฟล์โปรเจกต์ผู้ใช้
    [switch]$Silent          # ไม่ถามยืนยัน (ใช้ใน uninstaller อัตโนมัติ)
)

$ErrorActionPreference = "Continue"

# ─── สีสำหรับ output ───────────────────────────────────────
function Write-Step  { param($msg) Write-Host "  ► $msg" -ForegroundColor Cyan }
function Write-Ok    { param($msg) Write-Host "  ✓ $msg" -ForegroundColor Green }
function Write-Skip  { param($msg) Write-Host "  - $msg" -ForegroundColor DarkGray }
function Write-Warn  { param($msg) Write-Host "  ⚠ $msg" -ForegroundColor Yellow }
function Write-Fail  { param($msg) Write-Host "  ✗ $msg" -ForegroundColor Red }

# ─── ฟังก์ชันลบโฟลเดอร์แบบเร็ว (ใช้ cmd /c rd แทน Remove-Item) ───
function Remove-Fast {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path $Path)) {
        Write-Skip "$Label (ไม่มีอยู่)"
        return
    }
    $size = (Get-ChildItem -Recurse -Force -ErrorAction SilentlyContinue $Path |
             Measure-Object -Property Length -Sum -ErrorAction SilentlyContinue).Sum
    $sizeMB = [math]::Round($size / 1MB, 1)
    Write-Step "กำลังลบ $Label (~${sizeMB} MB) ..."
    try {
        # cmd /c rd /s /q เร็วกว่า Remove-Item -Recurse มาก สำหรับโฟลเดอร์ใหญ่
        $null = & cmd /c rd /s /q """$Path""" 2>&1
        if (Test-Path $Path) {
            # Fallback: ลองใช้ robocopy trick (เร็วมากสำหรับ empty mirror)
            $emptyDir = [System.IO.Path]::GetTempPath() + "empty_$(Get-Random)"
            $null = New-Item -ItemType Directory -Path $emptyDir -Force
            $null = & robocopy $emptyDir $Path /MIR /NFL /NDL /NJH /NJS /nc /ns /np 2>&1
            Remove-Item -Path $emptyDir -Force -ErrorAction SilentlyContinue
            $null = & cmd /c rd /s /q """$Path""" 2>&1
        }
        Write-Ok "ลบ $Label สำเร็จ"
    } catch {
        Write-Fail "ลบ $Label ล้มเหลว: $_"
    }
}

function Remove-File {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path $Path)) { Write-Skip "$Label (ไม่มีอยู่)"; return }
    try {
        Remove-Item -Path $Path -Force -ErrorAction Stop
        Write-Ok "ลบ $Label สำเร็จ"
    } catch {
        Write-Fail "ลบ $Label ล้มเหลว: $_"
    }
}

# ─── รวบรวม paths ทั้งหมดที่แอปสร้าง ─────────────────────
$userProfile = $env:USERPROFILE
$appData     = $env:APPDATA
$publicDir   = $env:PUBLIC
if (-not $publicDir) { $publicDir = "C:\Users\Public" }

$paths = @{
    # Config + KB cache (เล็ก — ลบเร็ว)
    "config_user"   = "$userProfile\.vibekidbright"
    "config_appdata"= "$appData\.vibekidbright"

    # Portable toolchain — C:\Users\Public\.vibekidbright
    "toolchain"     = "$publicDir\.vibekidbright"

    # Tauri app data (WebView cache, settings ของ Tauri)
    "tauri_appdata" = "$appData\com.cake.tauri-app"
    "tauri_local"   = "$env:LOCALAPPDATA\com.cake.tauri-app"

    # WebView2 user data (บางครั้งถูกสร้างในโฟลเดอร์ Tauri)
    "webview2"      = "$appData\Kidbright_IDE"
    "webview2_local"= "$env:LOCALAPPDATA\Kidbright_IDE"
}

# ─── แสดงสิ่งที่จะลบ ──────────────────────────────────────
Write-Host ""
Write-Host "══════════════════════════════════════════════════" -ForegroundColor Magenta
Write-Host "   VibeKidbright — Complete Uninstall Cleaner" -ForegroundColor Magenta
Write-Host "══════════════════════════════════════════════════" -ForegroundColor Magenta
Write-Host ""
Write-Host "📂 จะลบโฟลเดอร์ต่อไปนี้:" -ForegroundColor White
foreach ($key in $paths.Keys) {
    $p = $paths[$key]
    if (Test-Path $p) {
        Write-Host "   • $p" -ForegroundColor Yellow
    }
}
Write-Host ""

if (-not $Silent) {
    $confirm = Read-Host "ต้องการดำเนินการต่อหรือไม่? (y/N)"
    if ($confirm -notmatch '^[yY]$') {
        Write-Host "ยกเลิกการลบ" -ForegroundColor Gray
        exit 0
    }
}

Write-Host ""

# ─── ลบ Tauri app data ขนาดเล็กก่อน (เร็ว) ────────────────
Remove-Fast $paths["tauri_appdata"] "Tauri App Data (APPDATA)"
Remove-Fast $paths["tauri_local"]   "Tauri App Data (LOCALAPPDATA)"
Remove-Fast $paths["webview2"]      "WebView2 Cache (APPDATA)"
Remove-Fast $paths["webview2_local"] "WebView2 Cache (LOCALAPPDATA)"

# ─── ลบ config + KB ────────────────────────────────────────
Remove-Fast $paths["config_appdata"] "ESP-IDF Runtime + Config (APPDATA)"
Remove-Fast $paths["config_user"]    "VibeKidbright Config (USERPROFILE)"

# ─── ลบ portable toolchain (ใหญ่สุด) ──────────────────────
Write-Host ""
Write-Host "🔧 Portable Toolchain (ส่วนใหญ่ใหญ่ที่สุด ~3-5 GB)" -ForegroundColor White
Remove-Fast $paths["toolchain"] "Portable Toolchain (Public)"

# ─── ตรวจสอบ registry (Tauri installer) ───────────────────
Write-Host ""
Write-Step "ตรวจสอบ Registry entries..."
$regPaths = @(
    "HKCU:\Software\com.cake.tauri-app",
    "HKCU:\Software\Kidbright_IDE",
    "HKLM:\Software\Kidbright_IDE"
)
foreach ($reg in $regPaths) {
    if (Test-Path $reg) {
        try {
            Remove-Item -Path $reg -Recurse -Force -ErrorAction Stop
            Write-Ok "ลบ Registry $reg"
        } catch {
            Write-Fail "ลบ Registry ล้มเหลว: $_"
        }
    } else {
        Write-Skip "Registry $reg (ไม่มีอยู่)"
    }
}

Write-Host ""
Write-Host "══════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "   ✅ ล้างข้อมูลสำเร็จ!" -ForegroundColor Green
Write-Host "══════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""
Write-Host "หมายเหตุ: หากติดตั้ง VibeKidbright จาก .msi" -ForegroundColor Gray
Write-Host "         ให้ไปที่ Settings → Apps → ถอนการติดตั้งก่อน" -ForegroundColor Gray
Write-Host "         แล้วค่อยรัน script นี้" -ForegroundColor Gray
Write-Host ""
