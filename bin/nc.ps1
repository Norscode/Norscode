# bin/nc.ps1 - Norscode CLI for Windows (PowerShell)
#
# Rekkefølge:
#   1. dist\norscode.exe  (native Windows binary, når tilgjengelig)
#   2. python main.py     (fallback via Python)

$RootDir = Split-Path -Parent $PSScriptRoot

# ─── Native Windows binary ───────────────────────────────────────────────────
$NativeExe = Join-Path $RootDir "dist\norscode.exe"
if (Test-Path $NativeExe) {
    & $NativeExe @args
    exit $LASTEXITCODE
}

# ─── Python-fallback ─────────────────────────────────────────────────────────
$MainScript = Join-Path $RootDir "main.py"
if (Test-Path $MainScript) {
    python $MainScript @args
    exit $LASTEXITCODE
}

Write-Error "Norscode: fant ingen kompilator."
Write-Error "Installer med: py -m pip install norscode"
exit 1
