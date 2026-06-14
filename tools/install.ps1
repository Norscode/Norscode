# tools/install.ps1 - Installer Norscode på Windows
#
# Bruk:
#   irm https://raw.githubusercontent.com/Norscode/Norscode/main/tools/install.ps1 | iex
#   .\tools\install.ps1
#   .\tools\install.ps1 -Prefix C:\Tools
#
# Krever PowerShell 5.1+ eller PowerShell 7+

param(
    [string]$Prefix = "$env:LOCALAPPDATA\Norscode"
)

$ErrorActionPreference = "Stop"
$Repo = "Norscode/Norscode"
$BinDir = Join-Path $Prefix "bin"
$VersionsDir = Join-Path $Prefix "versions"
$CurrentVersionFile = Join-Path $Prefix "CURRENT_VERSION.txt"

Write-Host "Norscode installasjon for Windows" -ForegroundColor Cyan
Write-Host "Installasjonsrot: $Prefix"

# ─── Lag installasjonskatalog ────────────────────────────────────────────────
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
New-Item -ItemType Directory -Force -Path $VersionsDir | Out-Null

# ─── Hent siste release ──────────────────────────────────────────────────────
$ReleasesUrl = "https://api.github.com/repos/$Repo/releases/latest"
Write-Host "Henter siste release..."

try {
    $Release = Invoke-RestMethod -Uri $ReleasesUrl -UseBasicParsing
    $Asset = $Release.assets | Where-Object { $_.name -like "norscode-windows*" } | Select-Object -First 1
} catch {
    Write-Warning "Kunne ikke hente release fra GitHub: $_"
    $Asset = $null
}

if ($Asset) {
    # ─── Last ned Windows app-zip ────────────────────────────────────────────
    $ArchivePath = Join-Path $env:TEMP $Asset.name
    Write-Host "Laster ned $($Asset.name)..."
    Invoke-WebRequest -Uri $Asset.browser_download_url -OutFile $ArchivePath -UseBasicParsing

    $Version = if ($Release.tag_name) { $Release.tag_name.TrimStart("v") } else { "ukjent" }
    $VersionRoot = Join-Path $VersionsDir $Version
    $ExpandedRoot = Join-Path $VersionRoot "expanded"
    $LayoutRoot = Join-Path $VersionRoot "Norscode"

    if (Test-Path $ExpandedRoot) {
        Remove-Item -Recurse -Force $ExpandedRoot
    }

    New-Item -ItemType Directory -Force -Path $VersionRoot | Out-Null
    Expand-Archive -Path $ArchivePath -DestinationPath $ExpandedRoot -Force

    $ExtractedLayout = Join-Path $ExpandedRoot "Norscode"
    if (-not (Test-Path $ExtractedLayout)) {
        Write-Error "Ugyldig Windows release-layout: manglar Norscode/-katalog i ZIP."
        exit 1
    }

    if (Test-Path $LayoutRoot) {
        Remove-Item -Recurse -Force $LayoutRoot
    }

    Move-Item -Path $ExtractedLayout -Destination $LayoutRoot

    $VersionBin = Join-Path $LayoutRoot "bin"
    if (-not (Test-Path (Join-Path $VersionBin "norscode.exe"))) {
        Write-Error "Ugyldig Windows release-layout: manglar bin\\norscode.exe."
        exit 1
    }

    if (Test-Path $BinDir) {
        Get-ChildItem -Path $BinDir -Force | Remove-Item -Recurse -Force
    } else {
        New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
    }

    Copy-Item -Path (Join-Path $VersionBin "*") -Destination $BinDir -Recurse -Force
    Set-Content -Path $CurrentVersionFile -Value $Version

    Write-Host "Windows app installert: $LayoutRoot" -ForegroundColor Green
    Write-Host "Aktiv bin-mappe: $BinDir" -ForegroundColor Green
    Write-Host "Aktiv versjon: $Version" -ForegroundColor Green
} else {
    Write-Error "Ingen Windows native binary tilgjengelig i siste release."
    Write-Host "Bygg lokalt med tools\\build-bootstrap-binary.sh eller publiser en Windows release." -ForegroundColor Yellow
    exit 1
}

# ─── Legg til PATH ───────────────────────────────────────────────────────────
$CurrentPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($CurrentPath -notlike "*$BinDir*") {
    [Environment]::SetEnvironmentVariable("PATH", "$BinDir;$CurrentPath", "User")
    Write-Host "Lagt til PATH (gjelder nye terminaler): $BinDir" -ForegroundColor Green
} else {
    Write-Host "PATH allerede konfigurert."
}

Write-Host ""
Write-Host "Norscode installert!" -ForegroundColor Green
Write-Host "  nc.ps1 --help"
Write-Host "  nc.cmd --help"
Write-Host "  norscode.exe --help"
Write-Host ""
Write-Host "Start en ny terminal for at PATH-endringen skal tre i kraft."
