: << 'END_WIN'
@echo off
call "%~dp0build-windows.bat" %*
exit /b %ERRORLEVEL%
END_WIN
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$SCRIPT_DIR/build-linux.sh" "$@"
