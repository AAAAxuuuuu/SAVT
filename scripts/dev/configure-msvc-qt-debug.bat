@echo off
setlocal

call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

pushd "%SAVT_ROOT%" >nul

cmake --preset windows-msvc-qt-debug ^
  -DSAVT_ENABLE_CLANG_TOOLING=ON ^
  -DSAVT_LLVM_ROOT=%SAVT_LLVM_ROOT%
set "EXIT_CODE=%ERRORLEVEL%"

popd >nul
exit /b %EXIT_CODE%
