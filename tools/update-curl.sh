#!/usr/bin/env bash
# Re-vendor curl (the whole upstream "release" tree via git archive) into deps/curl at a
# given ref. The vendored copy historically tracks the git tag (the curlver.h shows a
# `-DEV` version), so pin by the git tag.
#   Usage: tools/update-curl.sh <git tag|SHA>    e.g. curl-8_18_0
# Upstream github.com/curl/curl (curl license). Commit as 'curl <ref>'.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-curl.sh <git tag|SHA, e.g. curl-8_18_0>}"
vendor_archive curl https://github.com/curl/curl.git "$REF"
echo "curl vendored at '$REF': $(show_version curl/include/curl/curlver.h 'define[[:space:]]+LIBCURL_VERSION[[:space:]]')"
echo "Commit as: 'curl $REF'"
