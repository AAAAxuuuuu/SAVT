@echo off
setlocal

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"
set "CMAKE_EXE=F:\Qt\Tools\CMake_64\bin\cmake.exe"
set "CTEST_EXE=F:\Qt\Tools\CMake_64\bin\ctest.exe"

pushd "%SAVT_ROOT%" >nul

echo [SAVT] Configuring qt-mingw-debug with tests enabled...
"%CMAKE_EXE%" --preset qt-mingw-debug -DSAVT_BUILD_TESTS=ON
if errorlevel 1 goto :fail

echo [SAVT] Building savt_backend_tests, savt_snapshot_tests, and savt_ai_tests...
"%CMAKE_EXE%" --build --preset qt-mingw-debug --target savt_backend_tests savt_snapshot_tests savt_ai_tests
if errorlevel 1 goto :fail

echo [SAVT] Running ctest baseline checks...
"%CTEST_EXE%" --test-dir "%SAVT_ROOT%\build\qtcreator-qt6.9.3-mingw-debug" --output-on-failure
if errorlevel 1 goto :fail

popd >nul
echo [SAVT] MinGW debug baseline verification passed.
exit /b 0

:fail
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
echo [SAVT] MinGW debug baseline verification failed with exit code %EXIT_CODE%.
exit /b %EXIT_CODE%
