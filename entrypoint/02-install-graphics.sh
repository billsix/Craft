#!/usr/bin/env bash
#
# 02-install-graphics.sh -- X11 / Mesa / Vulkan libraries so the GLFW+OpenGL game can
# open a window (via `make shell` display passthrough, mvp/gltron pattern) and so the
# optional Vulkan renderer can build. Corresponds to the Dockerfile's USE_GRAPHICS flag;
# the Dockerfile runs this only when USE_GRAPHICS=1. No options -- see 01-install-base.sh.
#
# Single dnf call, so its own exit status is this script's exit status.
set -uo pipefail

dnf install -y \
    mesa-dri-drivers \
    xorg-x11-server-Xvfb \
    libXScrnSaver \
    libXtst \
    libXcomposite \
    libXcursor \
    libXdamage \
    libXfixes \
    libXft \
    libXi \
    libXinerama \
    libXmu \
    libXrandr \
    libXrender \
    libXres \
    libXv \
    libXxf86vm \
    libglvnd-gles \
    mesa-demos \
    vulkan-headers \
    vulkan-loader-devel \
    vulkan-tools
