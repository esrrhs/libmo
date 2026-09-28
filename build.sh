#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="${1:-Debug}"

case "${BUILD_TYPE}" in
  release|Release)
    BUILD_TYPE="Release"
    ;;
  debug|Debug|"")
    BUILD_TYPE="Debug"
    ;;
  *)
    echo "usage: $0 [debug|release]" >&2
    exit 1
    ;;
esac

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DLIBMO_BUILD_TESTS=ON

cmake --build "${BUILD_DIR}" --parallel

ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "build ok (${BUILD_TYPE})"
