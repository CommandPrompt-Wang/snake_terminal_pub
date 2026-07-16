@echo off
:: ============================================================
::  pack-windows.bat — Package snake into a single .exe via Enigma VB
:: ============================================================

setlocal enabledelayedexpansion

:: ---- 可配置项 ----
set "ENIGMA=C:\Program Files (x86)\Enigma Virtual Box\enigmavbconsole.exe"
set "EVB=%~dp0pack-windows.evb"
set "DIST=dist-windows"
:: --------------------

if not exist "%ENIGMA%" (
    echo "[ERROR] Enigma Virtual Box not found: %ENIGMA%"
    exit /b 1
)

if not exist "%EVB%" (
    echo "[ERROR] %EVB% not found. Create it via Enigma VB GUI first."
    exit /b 1
)

if not exist "%DIST%\snake.exe" (
    echo "[ERROR] %DIST%\snake.exe not found. Run build.bat first."
    exit /b 1
)

echo "==> Checking..."
echo "   OK: %ENIGMA%"
echo "   OK: %DIST%\snake.exe"

echo "==> Running Enigma Virtual Box..."
"%ENIGMA%" "%EVB%"

echo "==> Done."
