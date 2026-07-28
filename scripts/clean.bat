@echo off
:: Clean TrueShot build artefacts on Windows.
::
:: Run from anywhere - the script resolves the repo root from its own path.
::
:: Three levels, from cheapest to most destructive:
::
::   scripts\clean.bat              Build artefacts only (default).
::   scripts\clean.bat --deps       ...plus vcpkg dependencies (needs re-download).
::   scripts\clean.bat --all        ...plus the global vcpkg cache (very slow rebuild).
::
:: Flags:
::   --dry-run   Print what would be deleted, delete nothing.
::   --yes, -y   Skip the confirmation prompt (for CI / scripting).
::   --help, -h  Show this help.
::
:: Nothing tracked by git is ever touched - the script only removes paths
:: that are listed in .gitignore. Run `git status` after a clean to
:: confirm the working tree is unchanged.

setlocal EnableDelayedExpansion

set "LEVEL=build"
set "DRY_RUN=0"
set "ASSUME_YES=0"

:: ------------------------------------------------------------------ args
:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--deps"    ( set "LEVEL=deps"    & shift & goto parse_args )
if /I "%~1"=="--all"     ( set "LEVEL=all"     & shift & goto parse_args )
if /I "%~1"=="--nuke"    ( set "LEVEL=all"     & shift & goto parse_args )
if /I "%~1"=="--dry-run" ( set "DRY_RUN=1"     & shift & goto parse_args )
if /I "%~1"=="-n"        ( set "DRY_RUN=1"     & shift & goto parse_args )
if /I "%~1"=="--yes"     ( set "ASSUME_YES=1"  & shift & goto parse_args )
if /I "%~1"=="-y"        ( set "ASSUME_YES=1"  & shift & goto parse_args )
if /I "%~1"=="--help"    goto usage
if /I "%~1"=="-h"        goto usage
echo [error] unknown argument: %~1
echo         run 'scripts\clean.bat --help' for usage
exit /b 1

:usage
echo Usage: scripts\clean.bat [--deps ^| --all] [--dry-run] [--yes]
echo.
echo   (no flag)   Remove build artefacts only.
echo   --deps      ...plus vcpkg_installed\ and the in-tree vcpkg\ clone.
echo   --all       ...plus the global vcpkg cache ^(slow rebuild after^).
echo   --dry-run   Show what would be removed, remove nothing.
echo   --yes       Skip the confirmation prompt.
exit /b 0

:args_done

:: ------------------------------------------------------- repo root guard
:: Resolve the repo root from the script's own location (scripts\ -> ..)
:: and refuse to run if it doesn't look right. Deleting build\ and
:: vcpkg_installed\ from the wrong directory would be a bad day.
cd /d "%~dp0.."
if not exist "CMakeLists.txt" goto not_repo
if not exist "vcpkg.json"     goto not_repo
goto repo_ok

:not_repo
echo [error] can't find the TrueShot repo root
echo         ^(expected CMakeLists.txt and vcpkg.json in "%CD%"^)
exit /b 1

:repo_ok
set "REPO_ROOT=%CD%"

:: ------------------------------------------------------------- targets
:: Space-separated is unreliable with paths, so we append to a numbered
:: pseudo-array instead.
set "N=0"

call :add "build"
call :add "netcode\build"
call :add "out"
call :add "dist"
call :add "CMakeCache.txt"
call :add "CMakeFiles"
call :add "CMakeUserPresets.json"
call :add "compile_commands.json"

:: CLion / JetBrains convention.
for /d %%d in (cmake-build-*) do call :add "%%d"

if /I "%LEVEL%"=="deps" goto add_deps
if /I "%LEVEL%"=="all"  goto add_deps
goto targets_done

:add_deps
call :add "vcpkg_installed"
call :add "vcpkg"
if /I not "%LEVEL%"=="all" goto targets_done

:: vcpkg's per-user download + binary cache. Wiping this means every
:: dependency is re-downloaded and rebuilt from source next time.
call :add "%LOCALAPPDATA%\vcpkg"
:: If VCPKG_ROOT points at an out-of-tree vcpkg, clean its build scratch
:: dirs but never the clone itself (that's the user's install).
if not defined VCPKG_ROOT goto targets_done
if /I "%VCPKG_ROOT%"=="%REPO_ROOT%\vcpkg" goto targets_done
call :add "%VCPKG_ROOT%\buildtrees"
call :add "%VCPKG_ROOT%\packages"
call :add "%VCPKG_ROOT%\downloads"

:targets_done

:: ---------------------------------------------------------- report/act
echo TrueShot clean -- level: %LEVEL%
echo.

set "FOUND=0"
for /L %%i in (1,1,%N%) do (
    if exist "!T%%i!" set /a FOUND+=1
)

if %FOUND%==0 (
    echo Nothing to clean. Already pristine.
    exit /b 0
)

echo Will remove:
for /L %%i in (1,1,%N%) do (
    if exist "!T%%i!" echo    !T%%i!
)
echo.

if %DRY_RUN%==1 (
    echo ^(--dry-run: nothing was deleted^)
    exit /b 0
)

if /I "%LEVEL%"=="all" if %ASSUME_YES%==0 (
    echo Level 'all' wipes the global vcpkg cache -- the next build will
    echo re-download and recompile every dependency ^(can take 10+ min^).
    set /p "REPLY=Continue? [y/N] "
    if /I not "!REPLY!"=="y" (
        echo Aborted.
        exit /b 0
    )
)

for /L %%i in (1,1,%N%) do (
    if exist "!T%%i!" (
        if exist "!T%%i!\" (
            rd /s /q "!T%%i!" 2>nul
        ) else (
            del /f /q "!T%%i!" 2>nul
        )
    )
)

echo Done. Working tree should be unchanged -- verify with 'git status'.
echo.
if /I "%LEVEL%"=="build" echo Next: scripts\run.bat ^(dependencies were kept, so this is fast^).
if /I "%LEVEL%"=="deps"  echo Next: scripts\run.bat ^(vcpkg will re-install the manifest deps^).
if /I "%LEVEL%"=="all"   echo Next: scripts\run.bat ^(full dependency rebuild -- grab a coffee^).
exit /b 0

:: --------------------------------------------------------------- helper
:add
set /a N+=1
set "T%N%=%~1"
goto :eof
