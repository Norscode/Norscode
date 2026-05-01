#requires -Version 5.1
param(
  [string]$InstallRoot = "$env:LOCALAPPDATA\Programs",
  [switch]$NoShortcut
)

$ErrorActionPreference = 'Stop'

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$AppName = "Norscode IDE"
$SourceExe = Join-Path $Root "dist_desktop_ide\$AppName\$AppName.exe"
$TargetRoot = Join-Path $InstallRoot $AppName
$TargetExe = Join-Path $TargetRoot "$AppName.exe"

if (-not (Test-Path $SourceExe)) {
  throw "Fant ikke bygg-pakken: $SourceExe`nKjør først: .\scripts\package-desktop-ide.ps1"
}

New-Item -ItemType Directory -Path $TargetRoot -Force | Out-Null
Copy-Item -Path $SourceExe -Destination $TargetExe -Force

if (-not $NoShortcut) {
  $desktopDir = [Environment]::GetFolderPath('Desktop')
  $shell = New-Object -ComObject WScript.Shell
  $shortcut = $shell.CreateShortcut((Join-Path $desktopDir "$AppName.lnk"))
  $shortcut.TargetPath = $TargetExe
  $shortcut.WorkingDirectory = $TargetRoot
  $shortcut.Description = "Norscode AI IDE"
  $shortcut.WindowStyle = 1
  $shortcut.Save()
  Write-Host "Lagret snarvei på skrivebordet: $desktopDir\$AppName.lnk"
}

Write-Host "Installert: $TargetExe"
Write-Host "Kjør appen direkte med: $TargetExe"
