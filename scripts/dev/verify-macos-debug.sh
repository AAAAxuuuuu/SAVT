#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${SAVT_QT_ROOT:-}" ]]; then
  echo "SAVT_QT_ROOT is not set."
  echo "Set it to your Qt macOS kit root, for example: /Users/you/Qt/6.9.2/macos"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

echo "[SAVT] Configuring macos-qt-debug..."
cmake --preset macos-qt-debug

echo "[SAVT] Building savt_backend_tests, savt_snapshot_tests, and savt_ai_tests..."
cmake --build --preset macos-qt-debug --target savt_backend_tests savt_snapshot_tests savt_ai_tests --parallel

echo "[SAVT] Running ctest baseline checks..."
ctest --preset macos-qt-debug
