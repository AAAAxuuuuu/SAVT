# MinGW Debug Baseline Validation

Date: March 24, 2026

This document records the phase-0 baseline that later refactor batches should preserve.

## Scope

- Preset: `qt-mingw-debug`
- Configure flag: `-DSAVT_BUILD_TESTS=ON`
- Unified verification entry: `scripts/dev/verify-mingw-debug.bat`
- Goal: confirm the smallest repeatable configure/build/test path without changing product behavior

## Command sequence

```powershell
.\scripts\dev\verify-mingw-debug.bat
```

Manual equivalent:

```powershell
& 'F:\Qt\Tools\CMake_64\bin\cmake.exe' --preset qt-mingw-debug -DSAVT_BUILD_TESTS=ON
& 'F:\Qt\Tools\CMake_64\bin\cmake.exe' --build --preset qt-mingw-debug --target savt_backend_tests savt_ai_tests
& 'F:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir .\build\qtcreator-qt6.9.3-mingw-debug --output-on-failure
```

## Recorded result

On March 24, 2026 this path completed successfully and the current baseline passed:

- `savt_backend_tests`
- `savt_ai_tests`
- `savt_utf8_check`

Expected summary: `100% tests passed, 0 tests failed out of 3`.

## Explicit non-goals for this batch

- No UI behavior changes
- No `AnalysisController` refactor
- No semantic backend feature work
- No L3 page implementation
- No changes to the hand-maintained `src/ui/src/moc_AnalysisController.cpp`
