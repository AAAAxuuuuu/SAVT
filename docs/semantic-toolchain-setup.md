# Semantic Toolchain Setup

## Installed components

- Visual Studio 2022 Preview MSVC toolchain
- Qt 6.9.3 `msvc2022_64`
- LLVM/Clang 21.1.7 development archive
- Ninja from `F:/Qt/Tools/Ninja`

## Paths

- MSVC vcvars: `C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat`
- Qt MSVC root: `F:\QT\6.9.3\msvc2022_64`
- LLVM root: `g:\SAVT\tools\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc`
- LLVM CMake package: `g:\SAVT\tools\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc\lib\cmake\llvm\LLVMConfig.cmake`
- libclang import library: `g:\SAVT\tools\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc\lib\libclang.lib`

## Recommended entry points

1. Double-click [enter-semantic-env.bat](/g:/SAVT/scripts/dev/enter-semantic-env.bat) to open a shell with MSVC and LLVM on `PATH`.
2. Run [configure-msvc-qt-debug.bat](/g:/SAVT/scripts/dev/configure-msvc-qt-debug.bat) to configure the MSVC Qt build directory.
3. Run [build-msvc-qt-debug.bat](/g:/SAVT/scripts/dev/build-msvc-qt-debug.bat) to build that directory.\n4. If you want a terminal-only sanity check for the MSVC toolchain without Ninja, run [configure-msvc-vs-debug.bat](/g:/SAVT/scripts/dev/configure-msvc-vs-debug.bat).

## Notes

- Industrial-grade C++ semantic analysis should target the MSVC toolchain path, not the MinGW path.
- The LLVM package is installed locally inside the repository so the project can reference headers, CMake packages, and libraries without relying on system `PATH`.
- `compile_commands.json` should be generated from the MSVC/Ninja build once the semantic backend is fully integrated.
