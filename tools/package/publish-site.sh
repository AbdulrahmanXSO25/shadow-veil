#!/usr/bin/env bash
# Publishes site/ to the gh-pages branch (served at https://abdulrahmanxso25.github.io/shadow-veil/).
# Only the site files go to that branch; nothing on main is touched.
set -euo pipefail
cd "$(dirname "$0")/../.."
REMOTE=$(git remote get-url origin)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp -r site/. "$TMP/"
touch "$TMP/.nojekyll"
cd "$TMP"
git init -q -b gh-pages
git add -A
git commit -qm "Website $(date -u +%Y-%m-%d)"
git push -qf "$REMOTE" gh-pages
echo "published site/ to gh-pages"
