#!/usr/bin/env bash
# Records one or more trailer shots: tools/trailer/record.sh [shot-name ...]  (default: all)
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=build/trailer/clips; mkdir -p "$OUT"
shots=("$@"); [ ${#shots[@]} -eq 0 ] && shots=($(ls tools/trailer/shots | sed 's/\.txt$//'))
for s in "${shots[@]}"; do
  ud=$(mktemp -d)
  extra=(--play --fresh)
  [[ "$s" == *menus* ]] && extra=()
  echo "== recording $s"
  ./build/shadow_veil "${extra[@]}" --user-dir "$ud" --render-size 1920x1080 --no-music \
      --autoplay "tools/trailer/shots/$s.txt" --record "$OUT/$s.mp4" 2>&1 | grep -E "ERROR|warn" || true
  rm -rf "$ud"
done
