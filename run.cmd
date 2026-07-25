@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=demo"

if /I "%MODE%"=="help" goto :help
if /I "%MODE%"=="--help" goto :help
if /I "%MODE%"=="-h" goto :help

set "PYTHON_EXE=python"
set "PYTHON_ARGS="
where python >nul 2>&1
if errorlevel 1 (
  where py >nul 2>&1
  if not errorlevel 1 (
    set "PYTHON_EXE=py"
    set "PYTHON_ARGS=-3"
  )
)

set "SERVER_DIR=%ROOT%\analysis-backend"
set "CLIENT_DIR=%ROOT%\client-agent"
set "BACKEND_PY=%SERVER_DIR%\simple_server.py"
set "CLIENT_PY=%CLIENT_DIR%\simple_client.py"

if /I "%MODE%"=="check" goto :check
goto :after_check

:check
echo [CHECK] Root: "%ROOT%"
echo.
echo [CHECK] Python:
where "%PYTHON_EXE%" >nul 2>&1
if errorlevel 1 (
  echo   NOT FOUND
) else (
  %PYTHON_EXE% %PYTHON_ARGS% --version
)
echo.
echo [CHECK] Files:
if exist "%BACKEND_PY%" (echo   OK  "%BACKEND_PY%") else (echo   MISSING  "%BACKEND_PY%")
if exist "%CLIENT_PY%" (echo   OK  "%CLIENT_PY%") else (echo   MISSING  "%CLIENT_PY%")
exit /b 0

:after_check

if /I "%MODE%"=="demo" (
  call :launch_server
  if errorlevel 1 exit /b 1
  call :launch_client
  if errorlevel 1 exit /b 1
  goto :done
)

if /I "%MODE%"=="server" (
  call :launch_server
  if errorlevel 1 exit /b 1
  goto :done
)

if /I "%MODE%"=="client" (
  call :launch_client
  if errorlevel 1 exit /b 1
  goto :done
)

echo [ERROR] Unknown mode: %MODE%
echo.
goto :help

:launch_server
if not exist "%BACKEND_PY%" (
  echo [ERROR] Missing file: "%BACKEND_PY%"
  exit /b 1
)
echo [INFO] Starting backend (HTTP 127.0.0.1:3001, TCP 127.0.0.1:8080)...
start "memk - backend" cmd /k "cd /d ""%SERVER_DIR%"" && %PYTHON_EXE% %PYTHON_ARGS% ""%BACKEND_PY%"""
exit /b 0

:launch_client
if not exist "%CLIENT_PY%" (
  echo [ERROR] Missing file: "%CLIENT_PY%"
  exit /b 1
)
echo [INFO] Starting client (connects to 127.0.0.1:8080)...
start "memk - client" cmd /k "cd /d ""%CLIENT_DIR%"" && %PYTHON_EXE% %PYTHON_ARGS% -u ""%CLIENT_PY%"""
exit /b 0

:help
echo memk launcher
echo.
echo Usage:
echo   run.cmd
echo   run.cmd demo
echo   run.cmd server
echo   run.cmd client
echo   run.cmd check
echo.
echo Notes:
echo   - Close the opened terminal windows to stop.
exit /b 0

:done
echo [OK] Launched: %MODE%
echo      Close the opened windows to stop.
exit /b 0
