#!/usr/bin/env bash
# Re-vendor LodePNG (the whole upstream tree; only lodepng.c/.h are compiled) into
# deps/lodepng at a given ref.
#   Usage: tools/update-lodepng.sh <SHA|branch|tag>    e.g. 865adac2b8149774401626101ec618e3de45e1d8
# Upstream github.com/lvandeve/lodepng (zlib). Pin by commit. Commit as 'lodepng <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-lodepng.sh <commit|branch|tag>}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
clone_ref https://github.com/lvandeve/lodepng.git "$REF" "$tmp"
replace_tree lodepng "$tmp"
# Upstream ships the implementation as lodepng.cpp (it compiles as both C and C++); Craft
# vendors it renamed to lodepng.c (and keeps the other util/example .cpp files as-is).
mv "$DEPS/lodepng/lodepng.cpp" "$DEPS/lodepng/lodepng.c"
# Craft's vendored lodepng omits upstream's build Makefile (it's built via Craft's own
# CMake/Meson, not lodepng's Makefile).
rm -f "$DEPS/lodepng/Makefile"
echo "lodepng vendored at '$REF': $(show_version lodepng/lodepng.c 'LODEPNG_VERSION_STRING')"
echo "Commit as: 'lodepng $REF'"
