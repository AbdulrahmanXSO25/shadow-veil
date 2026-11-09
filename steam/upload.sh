#!/usr/bin/env bash
# Uploads a build to Steam with steamcmd. Not run automatically: it needs a Steamworks partner
# account, an App ID and two depot IDs (Windows, Linux) filled in under steam/scripts/.
#   STEAM_USER=<builder account> steam/upload.sh path/to/ShadowVeil-*-windows-x64.zip path/to/ShadowVeil-*-linux-x64.tar.gz
set -euo pipefail
cd "$(dirname "$0")"
[[ $# -eq 2 ]] || { sed -n '2,5p' "$0"; exit 1; }
grep -q APPID_HERE scripts/app_build.vdf && { echo "Fill in AppID / depot IDs in steam/scripts/*.vdf first" >&2; exit 1; }
rm -rf content && mkdir -p content/windows content/linux output
unzip -q "$1" -d content/windows
tar -xzf "$2" -C content/linux
steamcmd +login "${STEAM_USER:?set STEAM_USER}" +run_app_build "$PWD/scripts/app_build.vdf" +quit
