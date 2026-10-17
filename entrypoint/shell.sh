#!/usr/bin/env bash
# What `make shell` / `make shell-exec` run: cd to the repo and exec bash
# (interactive with no args; runs the shell-exec payload otherwise).
set -e
cd /craft
exec bash "$@"
