@echo off
setlocal

call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

pushd "%SAVT_ROOT%" >nul
cmake --build --preset windows-msvc-qt-debug --parallel
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
exit /b %EXIT_CODE%
