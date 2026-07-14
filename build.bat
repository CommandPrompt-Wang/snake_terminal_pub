@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

cd /d "%~dp0"

set BUILD_TYPE=Release
set BUILD_RAYLIB=false
set CLEAN=false

:parse_args
if "%~1"=="" goto :done_parsing
if /i "%~1"=="--debug"        set BUILD_TYPE=Debug
if /i "%~1"=="--build-raylib" set BUILD_RAYLIB=true
if /i "%~1"=="--clean"        set CLEAN=true
shift
goto :parse_args
:done_parsing

if "%CLEAN%"=="true" (
    echo Cleaning build and dist...
    if exist build rmdir /s /q build
    if exist dist  rmdir /s /q dist
    echo Done.
    exit /b 0
)

:: ============================================================
:: CMake configure
:: ============================================================
if not exist build mkdir build
cd build

set RECONFIGURE=0
if "%BUILD_RAYLIB%"=="true" set RECONFIGURE=1
if not exist CMakeCache.txt set RECONFIGURE=1

if "%RECONFIGURE%"=="1" (
    echo -- Configuring CMake ^(%BUILD_TYPE%^)...
    cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configure failed.
        echo If symlink creation failed, try running as Administrator
        echo or enable Developer Mode in Windows Settings.
        cd ..
        exit /b 1
    )
)

:: ============================================================
:: CMake build
:: ============================================================
echo -- Building (%BUILD_TYPE%)...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd ..
    exit /b 1
)
cd ..

:: ============================================================
:: Distribute
:: ============================================================
if exist dist rmdir /s /q dist
mkdir dist

:: Multi-config generators (Visual Studio) output to build/%BUILD_TYPE%/
:: Single-config generators (Ninja, MinGW) output directly to build/
if exist "build\%BUILD_TYPE%\snake.exe" (
    copy "build\%BUILD_TYPE%\snake.exe" dist\snake.exe >nul
) else if exist build\snake.exe (
    copy build\snake.exe dist\snake.exe >nul
) else (
    echo [WARNING] snake.exe not found — skipping binary copy.
)

xcopy resources dist\resources\ /E /I /Q >nul

echo.
echo ============================================================
echo   Build complete! Output in dist\
echo ============================================================
exit /b 0
