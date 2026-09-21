# dev.ps1 - รัน tauri dev พร้อม MinGW PATH อัตโนมัติ
# ใช้แทน: npm run tauri dev

# Kill processes เดิมที่อาจค้างอยู่
Write-Host "Clearing previous processes..." -ForegroundColor Cyan
Get-Process -Name "tauri-app","node" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

# Refresh PATH จาก registry (ไม่ต้องเปิด terminal ใหม่)
$machinePath = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine')
$userPath = [System.Environment]::GetEnvironmentVariable('PATH', 'User')
$env:PATH = "$machinePath;$userPath"

Write-Host "Starting tauri dev..." -ForegroundColor Green
npm run tauri dev
