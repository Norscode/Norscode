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

Write-Host "Norscode installasjon for Windows" -ForegroundColor Cyan
Write-Host "Installasjonsskatalog: $BinDir"

# ─── Lag installasjonskatalog ────────────────────────────────────────────────
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null

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
    # ─── Last ned Windows binary ─────────────────────────────────────────────
    $ExePath = Join-Path $BinDir "norscode.exe"
    Write-Host "Laster ned $($Asset.name)..."
    Invoke-WebRequest -Uri $Asset.browser_download_url -OutFile $ExePath -UseBasicParsing

    # Lag nc.exe-alias
    Copy-Item $ExePath (Join-Path $BinDir "nc.exe")
    Write-Host "Native binary installert: $ExePath" -ForegroundColor Green
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
Write-Host "  norscode --help"
Write-Host "  norscode run app.no"
Write-Host ""
Write-Host "Start en ny terminal for at PATH-endringen skal tre i kraft."
