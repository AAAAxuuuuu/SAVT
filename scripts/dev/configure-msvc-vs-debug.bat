@echo off
setlocal

call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

cmake -S "%SAVT_ROOT%" ^
  -B "%SAVT_ROOT%\build\windows-vs-msvc-debug" ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_PREFIX_PATH=%SAVT_QT_ROOT% ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
  -DSAVT_ENABLE_CLANG_TOOLING=ON ^
  -DSAVT_LLVM_ROOT=%SAVT_LLVM_ROOT%
