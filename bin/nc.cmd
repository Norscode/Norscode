@echo off
:: bin/nc.cmd - Norscode CLI for Windows
::
:: Rekkefølge:
::   1. dist\norscode.exe  (native Windows binary, når tilgjengelig)
::   2. python main.py     (fallback via Python)
setlocal enabledelayedexpansion

set "ROOT_DIR=%~dp0.."

:: ─── Native Windows binary ───────────────────────────────────────────────────
set "NATIVE_EXE=%ROOT_DIR%\dist\norscode.exe"
if exist "%NATIVE_EXE%" (
    "%NATIVE_EXE%" %*
    exit /b %errorlevel%
)

:: ─── Python-fallback ─────────────────────────────────────────────────────────
set "MAIN_SCRIPT=%ROOT_DIR%\main.py"
if exist "%MAIN_SCRIPT%" (
    python "%MAIN_SCRIPT%" %*
    exit /b %errorlevel%
)

echo Norscode: fant ingen kompilator. >&2
echo Installer med: py -m pip install norscode >&2
exit /b 1
