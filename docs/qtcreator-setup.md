# Qt Creator Setup

## Goal

Use the shared CMake presets inside Qt Creator without reintroducing machine-specific paths into the repository.

## Recommended workflow

1. Install a Qt 6.9.x kit for your platform.
2. Open the repository root in Qt Creator as a CMake project.
3. Provide your local Qt path through either:
   - the `SAVT_QT_ROOT` environment variable
   - a local-only `CMakeUserPresets.json` copied from `CMakeUserPresets.template.json`
4. Select one of the shared presets:
   - macOS: `macos-qt-debug`
   - Windows MSVC: `windows-msvc-qt-debug`
   - Windows MinGW compatibility path: `windows-mingw-qt-debug`

## Notes by platform

### macOS

- Point `SAVT_QT_ROOT` at a path like `/Users/you/Qt/6.9.2/macos`.
- Use the shared `macos-qt-debug` preset for normal development.

### Windows

- Prefer the MSVC kit and the `windows-msvc-qt-debug` preset.
- Use the MinGW preset only when you explicitly need the compatibility path.
- If you want semantic analysis on Windows, initialize the environment described in `docs/semantic-toolchain-setup.md` before configuring.

## Local files that should stay untracked

- `CMakeLists.txt.user`
- `CMakeUserPresets.json`
- `config/*.local.json`

Qt Creator may regenerate `CMakeLists.txt.user` at any time. That file is local IDE state and must not be committed.
