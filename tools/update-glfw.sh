#!/usr/bin/env bash
# Re-vendor GLFW (the whole upstream "release" tree via git archive; keeps its CMake files,
# which add_subdirectory needs on Win/Mac) into deps/glfw at a given ref.
#   Usage: tools/update-glfw.sh <tag|branch|SHA>    e.g. 3.4
# Upstream github.com/glfw/glfw (zlib/libpng). Pin by tag. Commit as 'glfw <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-glfw.sh <tag|branch|SHA, e.g. 3.4>}"
vendor_archive glfw https://github.com/glfw/glfw.git "$REF"
echo "glfw vendored at '$REF':"
show_version glfw/include/GLFW/glfw3.h 'define[[:space:]]+GLFW_VERSION_MAJOR'
show_version glfw/include/GLFW/glfw3.h 'define[[:space:]]+GLFW_VERSION_MINOR'
show_version glfw/include/GLFW/glfw3.h 'define[[:space:]]+GLFW_VERSION_REVISION'
echo "Commit as: 'glfw $REF'"
