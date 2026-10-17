#!/usr/bin/env bash
#
# lint.sh -- ADVISORY static analysis over Craft's first-party src/ (clang-tidy
# bug-finding checks + cppcheck). Always exits 0; it is not a gate. Promoting it to a
# fatal gate + a warning gate is tracked in tasks/modernize-c-like-gltron.md. Uses the
# Meson build's compile_commands.json; run `make build` (or buildDebug.sh) first.
#
# Portable: in-container (/craft) and on the host from anywhere.
set -uo pipefail

if [ -d /craft/src ]; then cd /craft; else
  root=$(git rev-parse --show-toplevel 2>/dev/null) && [ -n "$root" ] && cd "$root"
fi

build="${BUILD_DIR:-builddir}"
files=$(find src \( -iname '*.c' -o -iname '*.cpp' \))

echo "== clang-tidy (bug-finding checks; advisory) =="
if command -v clang-tidy >/dev/null 2>&1 && [ -f "$build/compile_commands.json" ]; then
  for f in $files; do
    clang-tidy -quiet -p "$build" \
      -checks='-*,bugprone-*,clang-analyzer-*,cert-*,-bugprone-easily-swappable-parameters' "$f" 2>/dev/null || true
  done
else
  echo "lint: clang-tidy or $build/compile_commands.json missing; run 'make build' first" >&2
fi

echo "== cppcheck (advisory) =="
if command -v cppcheck >/dev/null 2>&1; then
  cppcheck --enable=warning,style,performance,portability --quiet \
    --suppress=missingIncludeSystem -I src $files 2>&1 || true
else
  echo "lint: cppcheck not installed; skipping" >&2
fi

echo "== lint.sh is advisory; exit 0 regardless =="
exit 0
