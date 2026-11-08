# Stop installing/compiling Emacs packages in the Dockerfile; match geometricalgebra/modelviewprojection

**Status:** proposed — needs go-ahead. Created 2026-11-08 (William Emerison Six <billsix@gmail.com>).
**Priority:** 4
**Difficulty:** 2

## BLUF

Craft's `Dockerfile` downloads and byte-compiles the whole MELPA package set at image build
(`emacs --batch --load install-melpa-packages.el`), then `make shell` bind-mounts the vendored
`.emacs.d/` over it, so that layer is network-dependent build time that is never used. Make Craft
follow the Emacs contract the way github.com/billsix/geometricalgebra and
github.com/billsix/modelviewprojection do: Emacs installed binary-only by an `0N-install-emacs.sh` group
script, the vendored `elpa/` used from the bind mount, and the packages byte-compiled at run time by
`init.el` when Emacs first starts in `make shell`. Done = no MELPA step in the `Dockerfile`, `make image`
succeeds with `USE_EMACS=1` and `USE_EMACS=0`, and Emacs in `make shell` opens a `.c` file with clangd
attached.

## Context

Read first: the `emacs-container-contract.md` reference doc in github.com/billsix/runClaudeInContainer
(`tasks/reference/emacs-container-contract.md`, section "Container side") and the emacs bullet of the
Dockerfile contract in the personal overlay (github.com/billsix/dotfiles
`.ai-coding-conventions.personal.md`).

**How the two model repos do it (verified 2026-11-08):**

- `entrypoint/0N-install-emacs.sh` (geometricalgebra `05-install-emacs.sh`, modelviewprojection
  `04-install-emacs.sh`) is a plain `dnf install -y emacs emacs-gtk+x11 emacs-pgtk` (+ the language
  server where it isn't already in base). The `Dockerfile` runs it under `if [ "$USE_EMACS" = "1" ]`.
  No `emacs --batch` at build.
- `COPY entrypoint/dotfiles/ /root/` puts `init.el`, `helm.el`, `preferences.el`,
  `install-melpa-packages.el` into the image; `.dockerignore` excludes
  `entrypoint/dotfiles/.emacs.d/elpa` so the vendored packages are not baked.
- Makefile `ELPA_MOUNT = -v $(CURDIR)/entrypoint/dotfiles/.emacs.d/elpa:/root/.emacs.d/elpa:U,z` (the
  `elpa/` subdir only), in `SHELL_RUN_FLAGS`.
- `init.el` line 2, right after `(package-initialize)`:
  `(byte-recompile-directory package-user-dir 0)`. Emacs compiles any `.el` in `elpa/` that has no
  `.elc` yet. The `.elc` land in the bind-mounted host `elpa/` (gitignored; `update-emacs-packages`
  strips them), so only the first Emacs start after a refresh is slow.
- `update-emacs-packages` mounts `elpa/` plus `install-melpa-packages.el` (`:ro`), wipes `elpa/`,
  reinstalls, strips `*.elc`/`*.eln`, `git add -f`.

**Craft today (verified 2026-11-08):**

- `Dockerfile` lines 23–32: `COPY …/install-melpa-packages.el`, then a `RUN` that, under `USE_EMACS=1`,
  runs `dnf install -y emacs` and `emacs --batch --load /root/.emacs.d/install-melpa-packages.el`
  (network at build).
- `entrypoint/01-install-base.sh` also lists `emacs`, so `USE_EMACS=0` still installs it.
- Makefile `ELPA_MOUNT` mounts the **whole** `.emacs.d/` (`…/.emacs.d/:/root/.emacs.d/:U,z`), so the
  build-time packages are always hidden. `update-emacs-packages` mounts the whole `.emacs.d/` too.
- There is no `.dockerignore`, and the `Dockerfile` does not `COPY` the dotfiles.
- `init.el` has no `byte-recompile-directory` line, so the vendored packages load as uncompiled source.
- `.gitignore` already ignores `*.elc`, `*.eln`, `eln-cache/`.

## Plan

1. Add `entrypoint/03-install-emacs.sh` (header comment in the style of geometricalgebra's
   `05-install-emacs.sh`): `dnf install -y emacs emacs-gtk+x11 emacs-pgtk`. clangd stays in base
   (`clang-tools-extra`). Commit it `+x` (`git add --chmod=+x`).
2. Remove `emacs` from `entrypoint/01-install-base.sh`.
3. `Dockerfile`: delete the `install-melpa-packages.el` `COPY` and the emacs `RUN`. Copy
   `03-install-emacs.sh` with the other group scripts and add
   `if [ "$USE_EMACS" = "1" ]; then /usr/local/bin/03-install-emacs.sh; fi` to the dnf `RUN`. Add
   `COPY entrypoint/dotfiles/ /root/` before the source `COPY`s. Update the header comment (USE_EMACS
   now means "Emacs binary"; the packages come from the mount).
4. Add `.dockerignore` excluding `entrypoint/dotfiles/.emacs.d/elpa`, plus `.git`, `builddir/`,
   `debugBuild/`, `debugBuildInstall/`, `*.tar`. Check that the `Dockerfile`'s `COPY`s (and
   `buildDebug.sh` at build) need none of those.
5. Makefile: `ELPA_MOUNT` → the `elpa/` subdir only; `update-emacs-packages` mounts `elpa/` plus
   `install-melpa-packages.el:ro,z` like geometricalgebra (and drops the `mkdir -p`/whole-dir mount).
   Update the comments above both. Consequence: an `init.el`/`helm.el`/`preferences.el` edit now needs
   `make image` (the contract's documented trade-off).
6. `init.el`: add `(byte-recompile-directory package-user-dir 0)` after `(package-initialize)`.
7. `CLAUDE.md` lines ~119–126: say `USE_EMACS=1` installs the Emacs binary, and that packages are the
   vendored `elpa/`, mounted at `make shell` and compiled by `init.el` on first start.

## Verify

- `make image` (default flags) and `make image USE_EMACS=0` both succeed. In the `USE_EMACS=0` image,
  `command -v emacs` fails. The build log shows no MELPA fetch.
- `make shell-exec CMD='emacs --batch -l ~/.emacs.d/init.el --eval "(message \"ok\")"'` exits 0, and
  afterwards `entrypoint/dotfiles/.emacs.d/elpa` contains `.elc` files that `git status` does not show.
- In `make shell`, open `src/main.c` in Emacs: lsp-mode attaches clangd. Only the maintainer can do this
  on a real display.
- `runClaudeInContainer/tools/check_helm_file_at_point.sh entrypoint/dotfiles/.emacs.d` passes (run from
  github.com/billsix/runClaudeInContainer).

## Open questions

None.
