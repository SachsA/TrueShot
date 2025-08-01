@echo off
echo Building TrueShot with vcpkg...

:: Try to find vcpkg toolchain file
set VCPKG_ROOT=
for %%d in ("%ProgramFiles%\vcpkg", "%LOCALAPPDATA%\vcpkg", "%USERPROFILE%\vcpkg", "C:\vcpkg", "D:\vcpkg") do (
    if exist "%%~d\scripts\buildsystems\vcpkg.cmake" (
        set VCPKG_ROOT=%%~d
        goto :found_vcpkg
    )
)

:found_vcpkg
if "%VCPKG_ROOT%"=="" (
    echo Warning: Could not automatically find vcpkg installation
    set VCPKG_TOOLCHAIN=
) else (
    set VCPKG_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
    echo Found vcpkg at: %VCPKG_ROOT%
)

if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake .. %VCPKG_TOOLCHAIN%

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building Debug...
cmake --build . --config Debug

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo Running game...
cd Debug
TrueShot.exe

pause
