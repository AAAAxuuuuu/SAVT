@echo off
setlocal

call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

pushd "%SAVT_ROOT%" >nul

echo [SAVT] Configuring windows-msvc-qt-debug...
cmake --preset windows-msvc-qt-debug
if errorlevel 1 goto :fail

echo [SAVT] Building savt_backend_tests, savt_snapshot_tests, and savt_ai_tests...
cmake --build --preset windows-msvc-qt-debug --target savt_backend_tests savt_snapshot_tests savt_ai_tests --parallel
if errorlevel 1 goto :fail

echo [SAVT] Running ctest baseline checks...
ctest --preset windows-msvc-qt-debug
if errorlevel 1 goto :fail

popd >nul
echo [SAVT] Windows MSVC debug baseline verification passed.
exit /b 0

:fail
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
echo [SAVT] Windows MSVC debug baseline verification failed with exit code %EXIT_CODE%.
exit /b %EXIT_CODE%
