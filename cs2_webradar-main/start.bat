@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "WEBAPP_DIR=%PROJECT_ROOT%webapp"
set "VITE_BIN=%WEBAPP_DIR%\node_modules\vite\bin\vite.js"
set "WS_ENTRY=%WEBAPP_DIR%\ws\app.js"

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

if not exist "%WEBAPP_DIR%\package.json" (
    echo [ERROR] Webapp folder was not found:
    echo %WEBAPP_DIR%
    pause
    exit /b 1
)

if not exist "%WS_ENTRY%" (
    echo [ERROR] Missing file: %WS_ENTRY%
    echo The project copy is incomplete. Re-download or re-clone full repository.
    pause
    exit /b 1
)

if not exist "%VITE_BIN%" (
    echo [INFO] Installing webapp dependencies...
    pushd "%WEBAPP_DIR%"
    call npm.cmd install
    set "NPM_EXIT=%ERRORLEVEL%"
    popd
    if not "%NPM_EXIT%"=="0" (
        echo [ERROR] npm install failed with exit code %NPM_EXIT%.
        pause
        exit /b %NPM_EXIT%
    )
)

echo [INFO] Starting CS2 Web Radar...
node.exe "%PROJECT_ROOT%launcher.mjs"
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo [INFO] Launcher exited with code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%
