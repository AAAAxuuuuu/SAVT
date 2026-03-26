# Qt Creator Setup

## Recommended kits

- Industrial precision path: `Qt 6.9.3 MSVC2022 64-bit`
- Legacy syntax-only shell path: `Qt 6.9.3 MinGW 64-bit`

## Recommended build directories

- MSVC Debug: `g:/SAVT/build/qtcreator-qt6.9.3-msvc2022_64-debug`
- MSVC Release: `g:/SAVT/build/qtcreator-qt6.9.3-msvc2022_64-release`
- MinGW Debug: `g:/SAVT/build/qtcreator-qt6.9.3-mingw-debug`
- MinGW Release: `g:/SAVT/build/qtcreator-qt6.9.3-mingw-release`

Use the MSVC kit for any work related to industrial-grade semantic analysis. The MinGW kit is only for the existing lightweight shell and compatibility work.

## Installed semantic toolchain

- LLVM root: `g:/SAVT/tools/llvm/clang+llvm-21.1.7-x86_64-pc-windows-msvc`
- LLVM version: `21.1.7`
- MSVC environment script: [enter-semantic-env.bat](/g:/SAVT/scripts/dev/enter-semantic-env.bat)
- MSVC configure script: [configure-msvc-qt-debug.bat](/g:/SAVT/scripts/dev/configure-msvc-qt-debug.bat)\n- MSVC VS-generator validation script: [configure-msvc-vs-debug.bat](/g:/SAVT/scripts/dev/configure-msvc-vs-debug.bat)
- MSVC build script: [build-msvc-qt-debug.bat](/g:/SAVT/scripts/dev/build-msvc-qt-debug.bat)

## What to open

Open the root [CMakeLists.txt](/g:/SAVT/CMakeLists.txt).

Do not open files under `third_party/Sourcetrail` as if they were the main application entry point. They are reference code only.

## Practical rule

- Edit and debug code in `apps/` and `src/`.
- Read `third_party/` when you want ideas or implementation references.
- For industrial precision work, open Qt Creator with the MSVC kit or start from [enter-semantic-env.bat](/g:/SAVT/scripts/dev/enter-semantic-env.bat).
