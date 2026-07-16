@echo off
:: ============================================================
::  pack-windows.bat — Package snake into a single .exe via Enigma VB
:: ============================================================

chcp 65001 2>&1 > nul
echo 打包后如无法运行请自行修改pack-windows.evb中的打包路径
timeout 1 2>&1 > nul

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

timeout 3 2>&1 > nul