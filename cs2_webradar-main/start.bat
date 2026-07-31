@echo off
setlocal

set "PROJECT_ROOT=%~dp0"

where node.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Node.js was not found. Install Node.js and try again.
    pause
    exit /b 1
)

if not exist "%PROJECT_ROOT%launcher.mjs" (
    echo [ERROR] launcher.mjs was not found:
    echo %PROJECT_ROOT%launcher.mjs
    pause
    exit /b 1
)

node.exe "%PROJECT_ROOT%launcher.mjs"
set "EXIT_CODE=%ERRORLEVEL%"

endlocal & exit /b %EXIT_CODE%
