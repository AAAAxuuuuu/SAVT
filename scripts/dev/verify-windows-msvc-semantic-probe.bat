@echo off
setlocal

call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

pushd "%SAVT_ROOT%" >nul

echo [SAVT] Configuring windows-msvc semantic probe build...
cmake --preset windows-msvc-qt-debug ^
  -DSAVT_ENABLE_CLANG_TOOLING=ON ^
  -DSAVT_LLVM_ROOT=%SAVT_LLVM_ROOT%
if errorlevel 1 goto :fail

echo [SAVT] Building savt_backend_tests...
cmake --build --preset windows-msvc-qt-debug --target savt_backend_tests --parallel
if errorlevel 1 goto :fail

echo [SAVT] Running semantic probe backend checks...
ctest --preset windows-msvc-qt-debug -R savt_backend_tests
if errorlevel 1 goto :fail

popd >nul
echo [SAVT] Windows MSVC semantic probe verification passed.
exit /b 0

:fail
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
echo [SAVT] Windows MSVC semantic probe verification failed with exit code %EXIT_CODE%.
exit /b %EXIT_CODE%
