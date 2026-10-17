#!/usr/bin/env bash
#
# Format Craft's first-party C/C++ with clang-format. Portable: runnable in-container
# (source bind-mounted at /craft) and on the host from anywhere. deps/ is vendored and
# NEVER formatted.
#
# cd to the repo root so the relative path resolves in either environment.
if [ -d /craft/src ]; then
    cd /craft
else
    root=$(git rev-parse --show-toplevel 2>/dev/null) && [ -n "$root" ] && cd "$root"
fi

# Exit nonzero if clang-format failed on ANY file (a plain loop would mask earlier
# failures behind the last iteration's status).
status=0
for x in $(find src \( -iname '*.c' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' \)); do
    clang-format -i "$x" || status=1
done
exit $status
