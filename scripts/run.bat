@echo off
:: Build ^& run TrueShot on Windows.
::
:: Run from anywhere - the script resolves the repo root from its own path.
::
::   scripts\run.bat                          Build + run offline.
::   scripts\run.bat --server 192.168.1.42    Build + run as a client.
::
:: Everything you pass is forwarded to the TrueShot binary, so any client
:: flag works here (see `TrueShot --help`).
::
:: Environment:
::   BUILD_TYPE   Release (default) ^| Debug ^| RelWithDebInfo
::   BUILD_DIR    build (default) - where CMake writes
::   VCPKG_ROOT   path to your vcpkg checkout (auto-detected if unset)
::
:: Flags:
::   --no-run     Configure + build, don't launch.
::   --help, -h   Show this help.

setlocal EnableDelayedExpansion

set "NO_RUN=0"
set "GAME_ARGS="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--no-run" ( set "NO_RUN=1" & shift & goto parse_args )
if /I "%~1"=="--help"   goto usage
if /I "%~1"=="-h"       goto usage
set "GAME_ARGS=!GAME_ARGS! %1"
shift
goto parse_args

:usage
echo Usage: scripts\run.bat [--no-run] [game args...]
echo.
echo   --no-run     Configure + build, don't launch.
echo   --help, -h   Show this help.
echo.
echo Any other argument is forwarded to TrueShot.exe, e.g.
echo   scripts\run.bat --server 192.168.1.42
echo.
echo Environment: BUILD_TYPE, BUILD_DIR, VCPKG_ROOT
exit /b 0

:args_done

:: Resolve the repo root from the script's own location (scripts\ -> ..)
:: so `cmake -S .` is correct no matter where this was invoked from.
cd /d "%~dp0.."
if not exist "CMakeLists.txt" goto not_repo
if not exist "vcpkg.json"     goto not_repo
goto repo_ok

:not_repo
echo [error] can't find the TrueShot repo root
echo         ^(expected CMakeLists.txt and vcpkg.json in "%CD%"^)
exit /b 1

:repo_ok

if not defined BUILD_TYPE set "BUILD_TYPE=Release"
if not defined BUILD_DIR  set "BUILD_DIR=build"

set "VCPKG_TOOLCHAIN="
set "CANDIDATES=%VCPKG_ROOT%;%ProgramFiles%\vcpkg;%LOCALAPPDATA%\vcpkg;%USERPROFILE%\vcpkg;C:\vcpkg;D:\vcpkg"

for %%d in ("%CANDIDATES:;=" "%") do (
    if exist "%%~d\scripts\buildsystems\vcpkg.cmake" (
        set "VCPKG_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=%%~d\scripts\buildsystems\vcpkg.cmake"
        echo Found vcpkg at: %%~d
        goto :have_vcpkg
    )
)
echo [warning] vcpkg toolchain not found. CMake may fail to find dependencies.

:have_vcpkg
echo [1/3] Configuring TrueShot (%BUILD_TYPE%)...
cmake -S . -B "%BUILD_DIR%" %VCPKG_TOOLCHAIN% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo [2/3] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

if "%NO_RUN%"=="1" (
    echo [3/3] --no-run: build complete, not launching.
    exit /b 0
)

echo [3/3] Running...
:: Multi-config generators nest the binary under the config name; single-
:: config ones don't. Try the nested path first, fall back to the flat one.
if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\TrueShot.exe" (
    "%BUILD_DIR%\bin\%BUILD_TYPE%\TrueShot.exe"!GAME_ARGS!
) else (
    "%BUILD_DIR%\bin\TrueShot.exe"!GAME_ARGS!
)
pause
