@echo off
setlocal

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

if not defined SAVT_LLVM_ROOT set "SAVT_LLVM_ROOT=%SAVT_ROOT%\tools\llvm"

set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined SAVT_VCVARS_PATH (
    set "VS_VCVARS=%SAVT_VCVARS_PATH%"
) else (
    set "VS_VCVARS="
)

if not defined VS_VCVARS if exist "%VSWHERE_EXE%" (
    for /f "usebackq tokens=*" %%I in (`"%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_INSTALL_DIR=%%~I"
    )
    if defined VS_INSTALL_DIR set "VS_VCVARS=%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
)

if not defined VS_VCVARS (
    for %%E in (Preview Community Professional Enterprise BuildTools) do (
        if not defined VS_VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
            set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)

if not defined SAVT_QT_ROOT (
    echo SAVT_QT_ROOT is not set.
    echo Set it to your Qt MSVC kit root, for example: C:\Qt\6.9.3\msvc2022_64
    exit /b 1
)

if not defined VS_VCVARS (
    echo Could not find vcvars64.bat automatically.
    echo Set SAVT_VCVARS_PATH to the full path of vcvars64.bat and try again.
    exit /b 1
)

if not exist "%VS_VCVARS%" (
    echo vcvars64.bat not found: %VS_VCVARS%
    exit /b 1
)

if not exist "%SAVT_QT_ROOT%\bin\qmake.exe" (
    echo Qt MSVC kit not found: %SAVT_QT_ROOT%
    exit /b 1
)

if not exist "%SAVT_LLVM_ROOT%\bin\clang.exe" (
    echo LLVM not found: %SAVT_LLVM_ROOT%
    echo Set SAVT_LLVM_ROOT if your LLVM package is installed elsewhere.
    exit /b 1
)

call "%VS_VCVARS%" >nul
if errorlevel 1 (
    echo Failed to initialize the MSVC environment.
    exit /b 1
)

if defined SAVT_NINJA_ROOT if exist "%SAVT_NINJA_ROOT%" (
    set "PATH=%SAVT_NINJA_ROOT%;%PATH%"
)

set "PATH=%SAVT_LLVM_ROOT%\bin;%SAVT_QT_ROOT%\bin;%PATH%"
set "LLVM_DIR=%SAVT_LLVM_ROOT%\lib\cmake\llvm"
set "Clang_DIR=%SAVT_LLVM_ROOT%\lib\cmake\clang"
set "Qt6_DIR=%SAVT_QT_ROOT%\lib\cmake\Qt6"

if /I "%~1"=="--no-shell" (
    exit /b 0
)

echo MSVC + Qt + LLVM environment is ready.
cmd /k
