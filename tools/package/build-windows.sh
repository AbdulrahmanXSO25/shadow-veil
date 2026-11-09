#!/usr/bin/env bash
# Builds the Windows x64 release from Linux with MinGW-w64 and zips it:
#   dist/ShadowVeil-<version>-windows-x64.zip  (ShadowVeil.exe + data/ + assets/, no installer needed)
# Requires: mingw-w64-gcc, cmake, ninja, git (SDL3 / FreeType / zlib are fetched at pinned versions).
# Optional: wine, to run the test suite against the Windows build (set SV_WINE_TESTS=1).
set -euo pipefail
cd "$(dirname "$0")/../.."
VERSION=$(sed -n 's/^project(ShadowVeil VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
B=build-win
cmake -S . -B "$B" -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release -DSV_RELEASE=ON
cmake --build "$B" --parallel
if [[ "${SV_WINE_TESTS:-0}" == 1 ]] && command -v wine >/dev/null; then
  WINEDEBUG=-all SHADOW_VEIL_DATA="$PWD" wine "$B/sv_tests.exe"
fi
STAGE=dist/stage-windows/ShadowVeil
rm -rf dist/stage-windows && mkdir -p "$STAGE"
cmake --install "$B" --prefix "$STAGE" --strip
cp tools/package/README-player.txt "$STAGE/README.txt"
mkdir -p dist
OUT="dist/ShadowVeil-$VERSION-windows-x64.zip"
rm -f "$OUT"
python3 - "$OUT" <<'PY'
import os, sys, zipfile
out = sys.argv[1]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for root, _, files in os.walk("dist/stage-windows/ShadowVeil"):
        for f in sorted(files):
            p = os.path.join(root, f)
            z.write(p, os.path.relpath(p, "dist/stage-windows"))
PY
echo "Windows build: $OUT ($(du -h "$OUT" | cut -f1))"
