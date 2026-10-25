#!/usr/bin/env bash
# Re-vendor Dear ImGui (the whole upstream "release" tree via git archive -- honours
# export-ignore, so CI/.github are dropped and .gitignore kept, matching the vendored
# snapshot) into deps/imgui at a given ref.
#   Usage: tools/update-imgui.sh <tag|branch|SHA>    e.g. v1.92.5
# Upstream github.com/ocornut/imgui (MIT). Pin by tag. Commit the result as 'imgui <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-imgui.sh <tag|branch|SHA, e.g. v1.92.5>}"
vendor_archive imgui https://github.com/ocornut/imgui.git "$REF"
echo "imgui vendored at '$REF': $(show_version imgui/imgui.h 'define[[:space:]]+IMGUI_VERSION[[:space:]]')"
echo "Commit as: 'imgui $REF'"
