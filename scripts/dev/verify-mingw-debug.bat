@echo off
setlocal

if not defined SAVT_QT_ROOT (
    echo SAVT_QT_ROOT is not set.
    echo Set it to your Qt MinGW kit root, for example: C:\Qt\6.9.3\mingw_64
    exit /b 1
)

if not defined SAVT_MINGW_ROOT (
    echo SAVT_MINGW_ROOT is not set.
    echo Set it to your MinGW root, for example: C:\Qt\Tools\mingw1310_64
    exit /b 1
)

for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"

pushd "%SAVT_ROOT%" >nul

echo [SAVT] Configuring windows-mingw-qt-debug...
cmake --preset windows-mingw-qt-debug
if errorlevel 1 goto :fail

echo [SAVT] Building savt_backend_tests, savt_snapshot_tests, and savt_ai_tests...
cmake --build --preset windows-mingw-qt-debug --target savt_backend_tests savt_snapshot_tests savt_ai_tests --parallel
if errorlevel 1 goto :fail

echo [SAVT] Running ctest baseline checks...
ctest --preset windows-mingw-qt-debug
if errorlevel 1 goto :fail

popd >nul
echo [SAVT] Windows MinGW debug baseline verification passed.
exit /b 0

:fail
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
echo [SAVT] Windows MinGW debug baseline verification failed with exit code %EXIT_CODE%.
exit /b %EXIT_CODE%
