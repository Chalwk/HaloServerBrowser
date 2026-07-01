REM Copyright (c) 2026 Jericho Crosby (Chalwk).
REM Licensed under the GPL License.

@echo off
setlocal enabledelayedexpansion

if "%1"=="" (
    echo Usage: package.bat ^<build-dir^>
    exit /b 1
)

set BUILD_DIR=%~1
set INSTALL_DIR=%BUILD_DIR%\package

if exist "%INSTALL_DIR%" rmdir /s /q "%INSTALL_DIR%"
mkdir "%INSTALL_DIR%"

set EXE_SRC=
for /r "%BUILD_DIR%" %%f in (HaloServerBrowser.exe) do (
    if exist "%%f" set EXE_SRC=%%f
)
if not defined EXE_SRC (
    echo ERROR: Could not find HaloServerBrowser.exe in build directory.
    exit /b 1
)

echo Found executable: %EXE_SRC%
copy "%EXE_SRC%" "%INSTALL_DIR%\"
if errorlevel 1 exit /b 1

set EXE=%INSTALL_DIR%\HaloServerBrowser.exe

where windeployqt >nul 2>nul
if not errorlevel 1 (
    echo Running windeployqt in release mode with compiler runtime...
    windeployqt --release --compiler-runtime "%EXE%"
    if errorlevel 1 (
        echo windeployqt failed. Please ensure Qt is correctly installed.
        exit /b 1
    )
) else (
    echo windeployqt not found on PATH. You must manually copy Qt DLLs and the VC++ redistributable.
)

if exist "gslist.exe" (
    copy "gslist.exe" "%INSTALL_DIR%\"
) else (
    echo WARNING: gslist.exe not found in the project root. Please place it there before packaging.
)

if exist "LICENSE" (
    copy "LICENSE" "%INSTALL_DIR%\"
)

echo.
echo Package staged in %INSTALL_DIR%
echo.