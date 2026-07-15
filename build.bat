@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

cd /d "%~dp0"

:: ============================================================
:: 可配置路径 —— 按需修改
:: ============================================================
set QT_ROOT=F:\Qt
set MINGW_PATH=%QT_ROOT%\Tools\mingw1310_64
set CMAKE_PATH=%QT_ROOT%\Tools\CMake_64
set BUILD_DIR=build-windows
set DIST_DIR=dist-windows
set GENERATOR=MinGW Makefiles

:: ============================================================
:: 编译选项
:: ============================================================
set BUILD_TYPE=Release
set CLEAN=false

:parse_args
if "%~1"=="" goto :done_parsing
if /i "%~1"=="--debug"  set BUILD_TYPE=Debug
if /i "%~1"=="--clean"  set CLEAN=true
shift
goto :parse_args
:done_parsing

:: 设置 PATH（MinGW + CMake）
set "PATH=%MINGW_PATH%\bin;%CMAKE_PATH%\bin;%PATH%"

:: 检查必要工具
where g++ >nul 2>&1 || (echo [ERROR] g++ not found: %MINGW_PATH%\bin & exit /b 1)
where cmake >nul 2>&1 || (echo [ERROR] cmake not found: %CMAKE_PATH%\bin & exit /b 1)

:: ============================================================
:: Clean
:: ============================================================
if "%CLEAN%"=="true" (
    echo Cleaning %BUILD_DIR% and %DIST_DIR%...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
    echo Done.
    exit /b 0
)

:: ============================================================
:: CMake configure
:: ============================================================
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

set RECONFIGURE=0
if not exist CMakeCache.txt set RECONFIGURE=1

if "%RECONFIGURE%"=="1" (
    echo "Configuring CMake (%BUILD_TYPE%, MinGW)..."
    cmake .. -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configure failed.
        cd ..
        exit /b 1
    )
)

:: ============================================================
:: CMake build
:: ============================================================
echo "Building (%BUILD_TYPE%)..."
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
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

if exist "%BUILD_DIR%\snake.exe" (
    copy "%BUILD_DIR%\snake.exe" "%DIST_DIR%\snake.exe" >nul
) else (
    echo [WARNING] snake.exe not found -- skipping binary copy.
)

xcopy resources "%DIST_DIR%\resources\" /E /I /Q >nul

echo.
echo ============================================================
<nul set /p ="  Build complete! Output in %DIST_DIR%\"
echo.
echo   Build dir: %BUILD_DIR%\
echo ============================================================
exit /b 0
