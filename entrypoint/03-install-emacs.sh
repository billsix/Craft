#!/usr/bin/env bash
#
# 03-install-emacs.sh -- Emacs (the maintainer's editor) + its GTK/pgtk builds. Binary
# only: the packages are NOT installed here. They are vendored in
# entrypoint/dotfiles/.emacs.d/elpa, bind-mounted by `make shell`, and byte-compiled by
# init.el on Emacs's first start. clangd (the C language server) is in base
# (clang-tools-extra). Corresponds to the Dockerfile's USE_EMACS flag; the Dockerfile
# runs this only when USE_EMACS=1. No options -- see 01-install-base.sh. Host-runnable:
# on a bare Fedora host, run this to add Emacs.
#
# Single dnf call, so its own exit status is this script's exit status.
set -uo pipefail

dnf install -y \
    emacs \
    emacs-gtk+x11 \
    emacs-pgtk
