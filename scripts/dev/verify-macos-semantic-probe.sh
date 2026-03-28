#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${SAVT_QT_ROOT:-}" ]]; then
  echo "SAVT_QT_ROOT is not set."
  echo "Set it to your Qt macOS kit root, for example: /Users/you/Qt/6.9.2/macos"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/macos-qt-semantic-probe"

mkdir -p "$BUILD_DIR"

LLVM_ARG=()
if [[ -n "${SAVT_LLVM_ROOT:-}" ]]; then
  LLVM_ARG+=("-DSAVT_LLVM_ROOT=${SAVT_LLVM_ROOT}")
fi

echo "[SAVT] Configuring macOS semantic probe build..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$SAVT_QT_ROOT" \
  -DSAVT_BUILD_TESTS=ON \
  -DSAVT_ENABLE_CLANG_TOOLING=ON \
  "${LLVM_ARG[@]}"

echo "[SAVT] Building savt_backend_tests..."
cmake --build "$BUILD_DIR" --target savt_backend_tests --parallel

echo "[SAVT] Running semantic probe backend checks..."
ctest --test-dir "$BUILD_DIR" --output-on-failure -R savt_backend_tests
