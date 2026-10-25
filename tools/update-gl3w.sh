#!/usr/bin/env bash
# Re-vendor gl3w into deps/gl3w at a given ref, REGENERATING its output. gl3w is a
# generator: gl3w_gen.py downloads glcorearb.h + khrplatform.h from the Khronos GL registry
# and writes include/GL/gl3w.h + src/gl3w.c. So this clones the ref, runs the generator
# (needs python3 + network to the Khronos registry), then vendors the whole tree.
#   Usage: tools/update-gl3w.sh <SHA|branch|tag>    e.g. d5ba9340cdeb9154323817f5c87e5a5c377fdef7
# Upstream github.com/skaslev/gl3w (public domain). Pin by commit. Commit as 'gl3w <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-gl3w.sh <commit|branch|tag>}"
command -v python3 >/dev/null || { echo "update-gl3w.sh needs python3 (runs gl3w_gen.py)"; exit 1; }
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
clone_ref https://github.com/skaslev/gl3w.git "$REF" "$tmp"
( cd "$tmp" && python3 gl3w_gen.py --root "$tmp" )
replace_tree gl3w "$tmp"
echo "gl3w vendored + regenerated at '$REF' (no version macro; pinned by commit)."
echo "Commit as: 'gl3w $REF'"
