@echo off
setlocal
call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1
for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"
"F:\Qt\Tools\CMake_64\bin\cmake.exe" --build "%SAVT_ROOT%\build\qtcreator-qt6.9.3-msvc2022_64-debug" --parallel 8
