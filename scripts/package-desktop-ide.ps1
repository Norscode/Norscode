$ErrorActionPreference = 'Stop'

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$AppDir = Join-Path $Root 'desktop_ide'
$DistDir = Join-Path $Root 'dist_desktop_ide'
$WebAppDir = Resolve-Path (Join-Path $Root '..' '..' 'norscode-website')
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
  throw "Python not found. Install Python 3 first."
}

$pyinstaller = Get-Command pyinstaller -ErrorAction SilentlyContinue
if (-not $pyinstaller) {
  Write-Host "PyInstaller not found. Installing..."
  python -m pip install pyinstaller
}

$Name = 'Norscode IDE'
Set-Location $Root
$AddData = "$WebAppDir;norscode-website"
python -m PyInstaller `
  --name "$Name" `
  --windowed `
  --noconfirm `
  --distpath "$DistDir" `
  --workpath (Join-Path $DistDir '.build') `
  --add-data "$AddData" `
  (Join-Path $AppDir 'main.py')

Write-Host "Build finished."
Write-Host "Executable: $DistDir\$Name\$Name.exe"
Write-Host "Noen eldre antivirus-løsere flagger pyinstaller-bygde apper ved første gangs kjøring."
