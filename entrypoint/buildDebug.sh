#!/usr/bin/env bash
#
# buildDebug.sh -- configure + compile + install Craft with Meson/Ninja, so the image
# ships a ready-to-run binary (client in /usr/local/bin, world lib + server scripts in
# /usr/local/server, resources in /usr/local/share/craft). Run at image-build time; also
# runnable by hand in a shell. The renderer defaults to OpenGL (-Dopengl=true).
set -euo pipefail

cd /craft
if [ -d builddir ]; then
    meson setup --reconfigure builddir --prefix=/usr/local
else
    meson setup builddir --prefix=/usr/local
fi
meson compile -C builddir
meson install -C builddir
