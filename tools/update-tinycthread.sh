#!/usr/bin/env bash
# Re-vendor tinycthread (just source/tinycthread.c + .h) into deps/tinycthread at a given ref.
#   Usage: tools/update-tinycthread.sh <SHA|branch|tag>   (default: master)
# Upstream github.com/tinycthread/tinycthread (zlib). The version macro is static (1.1), so
# pin by commit. Commit as 'tinycthread <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:-master}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
clone_ref https://github.com/tinycthread/tinycthread.git "$REF" "$tmp"
copy_files tinycthread "$tmp" source/tinycthread.c source/tinycthread.h
echo "tinycthread vendored at '$REF': $(show_version tinycthread/tinycthread.h 'define[[:space:]]+TINYCTHREAD_VERSION_MAJOR')"
echo "Commit as: 'tinycthread $REF'"
