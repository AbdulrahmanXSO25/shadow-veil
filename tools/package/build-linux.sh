#!/usr/bin/env bash
# Builds the portable Linux x64 release (Wayland and X11) and packs it:
#   dist/ShadowVeil-<version>-linux-x64.tar.gz   (run ./shadow-veil inside the folder)
#   dist/ShadowVeil-<version>-x86_64.AppImage    (if appimagetool is on PATH)
# SDL3 / FreeType / zlib are linked statically; libstdc++ too. The binary needs only glibc,
# libGL and whatever display/audio stack the system has (SDL loads Wayland, X11, PipeWire or
# PulseAudio at runtime). Build on the oldest distro you want to support (CI uses Ubuntu 22.04).
set -euo pipefail
cd "$(dirname "$0")/../.."
VERSION=$(sed -n 's/^project(ShadowVeil VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
B=build-linux-release
cmake -S . -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSV_RELEASE=ON
cmake --build "$B" --parallel
ctest --test-dir "$B" --output-on-failure
STAGE=dist/stage-linux/ShadowVeil
rm -rf dist/stage-linux && mkdir -p "$STAGE"
cmake --install "$B" --prefix "$STAGE" --strip
cp tools/package/README-player.txt "$STAGE/README.txt"
cp packaging/linux/dev.shadowveil.game.desktop "$STAGE/"
cp assets/icon/shadow-veil-256.png "$STAGE/shadow-veil.png"
cat > "$STAGE/shadow-veil" <<'SH'
#!/bin/sh
# Launcher: runs the game from wherever this folder was unpacked. Wayland is used when available.
HERE="$(dirname "$(readlink -f "$0")")"
exec "$HERE/shadow_veil" "$@"
SH
chmod +x "$STAGE/shadow-veil"
mkdir -p dist
OUT="dist/ShadowVeil-$VERSION-linux-x64.tar.gz"
tar -C dist/stage-linux -czf "$OUT" ShadowVeil
echo "Linux build: $OUT ($(du -h "$OUT" | cut -f1))"

if command -v appimagetool >/dev/null; then
  APPDIR=dist/stage-linux/ShadowVeil.AppDir
  rm -rf "$APPDIR" && mkdir -p "$APPDIR/usr/bin"
  cp -r "$STAGE"/{shadow_veil,data,assets} "$APPDIR/usr/bin/"
  cp packaging/linux/dev.shadowveil.game.desktop "$APPDIR/"
  cp assets/icon/shadow-veil-256.png "$APPDIR/dev.shadowveil.game.png"
  printf '#!/bin/sh\nHERE="$(dirname "$(readlink -f "$0")")"\nexec "$HERE/usr/bin/shadow_veil" "$@"\n' > "$APPDIR/AppRun"
  chmod +x "$APPDIR/AppRun"
  ARCH=x86_64 appimagetool -n "$APPDIR" "dist/ShadowVeil-$VERSION-x86_64.AppImage"
fi
