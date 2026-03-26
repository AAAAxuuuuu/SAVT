@echo off
for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"
set "VS_VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
set "LLVM_ROOT=%SAVT_ROOT%\tools\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc"
set "QT_ROOT=F:\QT\6.9.3\msvc2022_64"

if not exist "%VS_VCVARS%" (
    echo vcvars64.bat not found: %VS_VCVARS%
    exit /b 1
)

if not exist "%LLVM_ROOT%\bin\clang.exe" (
    echo LLVM not found: %LLVM_ROOT%
    exit /b 1
)

if not exist "%QT_ROOT%\bin\qmake.exe" (
    echo Qt MSVC kit not found: %QT_ROOT%
    exit /b 1
)

call "%VS_VCVARS%" >nul
if errorlevel 1 (
    echo Failed to initialize MSVC environment.
    exit /b 1
)

set "PATH=%LLVM_ROOT%\bin;%PATH%"
set "LLVM_DIR=%LLVM_ROOT%\lib\cmake\llvm"
set "Clang_DIR=%LLVM_ROOT%\lib\cmake\clang"
set "Qt6_DIR=%QT_ROOT%\lib\cmake\Qt6"
set "SAVT_LLVM_ROOT=%LLVM_ROOT%"
set "SAVT_QT_MSVC_ROOT=%QT_ROOT%"

if /I "%~1"=="--no-shell" (
    exit /b 0
)

echo MSVC, Qt, and LLVM semantic environment is ready.
cmd /k
