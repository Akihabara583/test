@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "WEBAPP_DIR=%PROJECT_ROOT%webapp"
set "USERMODE_DIR=%PROJECT_ROOT%usermode\release"

if not exist "%WEBAPP_DIR%\package.json" (
    echo [ERROR] Web application was not found:
    echo %WEBAPP_DIR%
    pause
    exit /b 1
)

if not exist "%USERMODE_DIR%\usermode.exe" (
    echo [ERROR] usermode.exe was not found:
    echo %USERMODE_DIR%\usermode.exe
    pause
    exit /b 1
)

echo Starting CS2 Usermode and Web Radar in this window...
echo Press Ctrl+C to stop the web server.
echo.

start "" /b /D "%USERMODE_DIR%" usermode.exe
cd /d "%WEBAPP_DIR%"
npm.cmd run dev

endlocal
