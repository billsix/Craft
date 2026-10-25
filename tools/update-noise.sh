#!/usr/bin/env bash
# Re-vendor the simplex-noise pair (noise.c + noise.h) into deps/noise. These two files are
# the adapted copy carried by fogleman's upstream Craft (deps/noise/), itself derived from
# caseman/noise (MIT, Casey Duncan). This fork mirrors fogleman's pair, so re-vendor from
# fogleman/Craft.
#   Usage: tools/update-noise.sh <SHA|branch|tag>   (default: master)
# No version macro -- pin by the source commit. Commit as 'noise <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:-master}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
clone_ref https://github.com/fogleman/Craft.git "$REF" "$tmp"
copy_files noise "$tmp/deps/noise" noise.c noise.h
echo "noise vendored from fogleman/Craft @ '$REF' (deps/noise/noise.{c,h}; no version macro)."
echo "Commit as: 'noise $REF'"
