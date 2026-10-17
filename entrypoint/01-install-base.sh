#!/usr/bin/env bash
#
# 01-install-base.sh -- install the always-needed Fedora packages for Craft.
#
# One package group per script, no options: WHICH optional groups also get installed is
# decided by the Dockerfile's ARG `if` blocks (or by a human choosing which scripts to
# run). Same packages during `podman build`, on a bare Fedora host, or in a guest with
# no container runtime. Run base first, then any 0N-install-*.sh group you want.
#
# Multiple dnf calls, so accumulate a non-zero exit if any fails.
set -uo pipefail

if ! command -v dnf >/dev/null 2>&1; then
    echo "01-install-base.sh: needs 'dnf' (this installs Fedora packages), not found." >&2
    echo "Run on a Fedora host/guest, or inside the project's Fedora-based image." >&2
    exit 1
fi

status=0

dnf upgrade -y || status=1

dnf install -y --skip-unavailable \
              clang \
              clang-tools-extra \
              cppcheck \
              emacs \
              file \
              g++ \
              gcc \
              gdb \
              git \
              glfw-devel \
              libcurl-devel \
              lldb \
              man \
              man-db \
              man-pages \
              meson \
              mesa-libGL-devel \
              mesa-libGLU-devel \
              nano \
              ninja-build \
              pkgconf-pkg-config \
              sqlite-devel \
              tmux \
              which || status=1

exit $status
