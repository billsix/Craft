#!/usr/bin/env bash
# Re-vendor the SQLite amalgamation (sqlite3.c/.h, sqlite3ext.h, shell.c) into deps/sqlite.
# SQLite is NOT distributed via git -- it's the amalgamation zip from sqlite.org, whose URL
# carries the RELEASE YEAR (not the current year). This takes the version, computes the
# 7-digit id (3.47.2 -> 3470200 = 3, minor%02, patch%02, "00"), then reads the download
# page to find the year-prefixed relative URL, downloads, unzips, and copies the 4 files.
#   Usage: tools/update-sqlite.sh <version> [release-year]    e.g. 3.47.2 2024
# The amalgamation URL is https://sqlite.org/<release-year>/sqlite-amalgamation-<num>.zip.
# download.html lists only the CURRENT release, so bumping to the latest auto-detects the
# year from it; pinning an OLDER version needs the release year passed (arg 2, or
# SQLITE_YEAR). Public domain. Commit as 'sqlite version-<version>' (existing convention).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/update_dep_common.sh"
REF="${1:?usage: update-sqlite.sh <version, e.g. 3.47.2> [release-year, e.g. 2024]}"
YEAR="${2:-${SQLITE_YEAR:-}}"
command -v unzip >/dev/null || { echo "update-sqlite.sh needs 'unzip'"; exit 1; }
IFS=. read -r maj min pat <<<"$REF"
num="$(printf '%d%02d%02d00' "$maj" "$min" "$pat")"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
if [ -n "$YEAR" ]; then
    rel="$YEAR/sqlite-amalgamation-$num.zip"
else
    # No year given: find it on the download page (works only for the CURRENT release).
    rel="$(curl -fsSL https://sqlite.org/download.html \
           | grep -oE '[0-9]{4}/sqlite-amalgamation-'"$num"'\.zip' | head -1)"
    [ -n "$rel" ] || { echo "sqlite $REF (amalgamation-$num) not on download.html -- it only lists the current release; pass the release year: update-sqlite.sh $REF <year>"; exit 1; }
fi
curl -fsSL "https://sqlite.org/$rel" -o "$tmp/amalg.zip" \
    || { echo "download failed: https://sqlite.org/$rel (check the version + year)"; exit 1; }
( cd "$tmp" && unzip -q amalg.zip )
copy_files sqlite "$tmp/sqlite-amalgamation-$num" sqlite3.c sqlite3.h sqlite3ext.h shell.c
echo "sqlite vendored at '$REF' (from /$rel): $(show_version sqlite/sqlite3.h 'define[[:space:]]+SQLITE_VERSION[[:space:]]')"
echo "Commit as: 'sqlite version-$REF'"
