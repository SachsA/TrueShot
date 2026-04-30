@echo off
:: Build & run TrueShot on Windows.
:: Looks for a vcpkg toolchain in common locations or %VCPKG_ROOT%.
setlocal EnableDelayedExpansion

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

echo [3/3] Running...
"%BUILD_DIR%\bin\%BUILD_TYPE%\TrueShot.exe"
if errorlevel 1 "%BUILD_DIR%\bin\TrueShot.exe"
pause
