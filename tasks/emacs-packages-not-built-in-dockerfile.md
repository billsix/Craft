# Emacs packages are not installed or compiled in the Dockerfile

Done 2026-11-08 (William Emerison Six <billsix@gmail.com> asked for it). Work commit `f6f7d2ba`, merged in
`40b9c359`.

## What was wrong

The `Dockerfile` ran `emacs --batch --load install-melpa-packages.el` under `USE_EMACS=1`: a network
download plus byte-compile of the whole MELPA set at image build, re-run whenever an earlier layer
changed. `make shell` then bind-mounted the whole vendored `.emacs.d/` over `/root/.emacs.d`, so the
build-time packages were never used. `emacs` was also listed in `01-install-base.sh`, so `USE_EMACS=0`
still installed it, and there was no `.dockerignore`.

## What changed

Craft now follows the Emacs contract (the `emacs-container-contract.md` reference doc in
github.com/billsix/runClaudeInContainer `tasks/reference/`), matching github.com/billsix/geometricalgebra
and github.com/billsix/modelviewprojection:

- `entrypoint/03-install-emacs.sh` installs the Emacs binary (`emacs emacs-gtk+x11 emacs-pgtk`); the
  `Dockerfile` runs it under `USE_EMACS`. `emacs` is no longer in `01-install-base.sh`. clangd stays in
  base (`clang-tools-extra`).
- The `Dockerfile` copies `entrypoint/dotfiles/` to `/root/`. The new `.dockerignore` excludes
  `entrypoint/dotfiles/.emacs.d/elpa`, `.git`, the host build trees, and `*.tar`.
- `ELPA_MOUNT` and `update-emacs-packages` mount only `elpa/` (the refresh also mounts
  `install-melpa-packages.el` read-only). An `init.el`/`helm.el`/`preferences.el` edit now needs
  `make image`.
- `init.el` runs `(byte-recompile-directory package-user-dir 0)` after `(package-initialize)`: the first
  Emacs start in `make shell` compiles the vendored packages into the mounted host `elpa/` (gitignored).
- `CLAUDE.md` "Build / run" describes this.

## Verification (2026-11-08, nested podman in the runClaudeInContainer sandbox)

- `make image` (default flags) succeeded; the build log has no MELPA fetch. The image has `/usr/bin/emacs`,
  `/root/.emacs.d/{init,helm,preferences,install-melpa-packages}.el`, and no `/root/.emacs.d/elpa`.
- `make shell-exec CMD='emacs --batch -l /root/.emacs.d/init.el -f kill-emacs …'` exited 0 and compiled
  317 `.elc` files into the host `elpa/`; `git status` stayed clean.
- `make image USE_EMACS=0` succeeded and the image has no `emacs` on `PATH`.
- `tools/check_helm_file_at_point.sh entrypoint/dotfiles/.emacs.d` (github.com/billsix/runClaudeInContainer):
  7/7 ERT cases passed.
- Not checked here: lsp-mode attaching clangd to a `.c` buffer in an interactive `make shell` on a real
  display.
