@echo off
setlocal
call "%~dp0enter-semantic-env.bat" --no-shell
if errorlevel 1 exit /b 1
for %%I in ("%~dp0..\..") do set "SAVT_ROOT=%%~fI"
"F:\Qt\Tools\CMake_64\bin\cmake.exe" -S "%SAVT_ROOT%" -B "%SAVT_ROOT%\build\qtcreator-qt6.9.3-msvc2022_64-debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=F:\QT\6.9.3\msvc2022_64 -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_MAKE_PROGRAM=F:\Qt\Tools\Ninja\ninja.exe -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSAVT_ENABLE_CLANG_TOOLING=ON -DSAVT_LLVM_ROOT=%SAVT_ROOT%\tools\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc
