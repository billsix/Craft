# Port Craft to Meson + a Fedora-44/Podman container (Dockerfile + Makefile + entrypoint/) with Emacs C support

**Status:** in progress — Created 2026-08-16; expanded 2026-10-17 (Meson port + Emacs/clangd).
**Meson build core DONE 2026-10-17** (coexisting with CMake; not yet torn down): `meson.build` +
`meson_options.txt` + `src/config.h.meson.in` build the Linux client, the `world` shared lib, and the
`craft_smoke` sanitizer gate; verified in-sandbox (clang) — `craft`/`libworld.so`/`craft_smoke` all
compile+link and `craft_smoke` runs clean under ASan. **Remaining:** the container contract
(Dockerfile + Makefile on the fleet template + `entrypoint/`) and Emacs/clangd support; then retire
CMakeLists.txt + the `.bat` files + `src/config.h.in` (Windows/macOS deps still described there).
**Priority:** 5
**Difficulty:** 7 (was 5; the CMake→Meson port raises it)

## Goal

Bring Craft onto my standard **container-per-project template** — the Fedora-44 + Podman
ephemeral-container dev environment (`Dockerfile`, a `Makefile` of `podman run --rm` targets, and
`entrypoint/` scripts) that gltron (`github.com/billsix/gltron-mirror`) and spimulator
(`github.com/billsix/spimulator`) use — so Craft builds and runs in the same reproducible sandbox as my
other C/C++ graphics projects. **Two additions decided 2026-10-17:** (1) **port the build from CMake
to Meson + Ninja** (the fleet C build system — apue and texExpToPng use it), and (2) wire **Emacs C
support (clangd + eglot) with a `make update-emacs-packages` target**. The direct exemplar for the
whole contract is **`/foo/opt/billsEmacsConfigs/c`** (its `meson.build` + `Dockerfile` + `Makefile` +
`entrypoint/dotfiles/.emacs.d/`). This is the **client**; the server is its own tasks
(`server-dockerfile.md` + `revive-python-server.md`), and the C-source modernization is
`modernize-c-like-gltron.md`.

**This reverses a current stance.** Craft's `CLAUDE.md` presently says: *"This project is not on Bill's
Fedora-44 + Podman container template — it's a plain CMake project … a legitimate divergence (it's an
upstream fork), not drift."* Adding the container makes that statement stale — **update that CLAUDE.md
note** as part of this task (Bill decided 2026-08-16 to bring it onto the template).

## What Craft is (constraints that shape the container)

- **C11 + C++17, CMake ≥ 4.0**; a real-time GLFW + **modern OpenGL** app with an in-progress **Vulkan**
  backend and **Dear ImGui** integration. Vendored deps live in `deps/`.
- **GPU / interactive:** opens a GLFW window, needs a GL (or Vulkan) context, input-driven event loop.
  **No headless mode, no automated test suite** — so the container's value is a reproducible *build +
  run* environment, not a CI gate.

## Follow the template (graphics/C-C++ family — mirror gltron/spimulator)

- **`Dockerfile`:** `FROM registry.fedoraproject.org/fedora:44`, the dnf-cache idiom
  (`--mount=type=cache` on `/var/cache/libdnf5` + `/var/lib/dnf`, `keepcache=True`), `dnf upgrade`, then
  install the toolchain via a host-runnable **`entrypoint/01-install-base.sh`** (`dnf install` group
  script the Dockerfile sources, per "Host-agnostic setup belongs in a script"). **Derive the dnf
  package list from `CMakeLists.txt` + `deps/`** — cmake, clang, GLFW, Mesa/OpenGL, GLEW, the Vulkan
  SDK/loader + validation layers, and Dear ImGui's needs — rather than guessing. `ARG` feature flags
  default `0` (Dockerfile) / `1` (Makefile): candidates `USE_VULKAN`, `USE_IMGUI`, `USE_EMACS_CONFIG`.
- **`Makefile`:** `.DEFAULT_GOAL := shell` (or help), `CONTAINER_NAME = craft`, `FILES_TO_MOUNT` with
  `-v $(shell pwd):/craft/:Z` + the entrypoint-script mounts + conditional host-config mounts; targets
  `all`→`image`→`shell`, `format`, `image-export`/`image-import`, and a `help` target with
  `##`-documented targets. X11 + Wayland passthrough blocks (this is a GUI app).
- **`entrypoint/`:** `shell.sh` (cd + `exec bash`), `format.sh` (clang-format over `*.{c,cpp,h,hpp}`),
  `01-install-base.sh`; a C/C++ `exit()`-trap in `~/.bashrc` running `format.sh` on shell exit.
  Model the exact scripts on `/foo/opt/billsEmacsConfigs/c/entrypoint/` (`shell.sh`/`shell-exec`
  pair, `format.sh`, `lint.sh`, `buildDebug.sh`, `entrypoint.sh`).

## Meson build port (CMake → Meson + Ninja) — decided 2026-10-17

Replace `CMakeLists.txt` with `meson.build`, keeping behavior. Surveyed structure (2026-10-17):

- **Targets to reproduce:** `craft` (executable from `src/*.c` + the C++ `gui.cpp` + the selected
  renderer's `gui-gl.cpp`/`gui-vulkan.cpp`, with the vendored `gl3w`/`imgui`/`lodepng`/`noise`/
  `tinycthread` sources compiled in); **`world`** (a `shared_library` from `deps/noise/noise.c` +
  `src/world.c`, installed to `server/` — the Python server loads it); and **`craft_smoke`** (the
  Linux/clang ASan+UBSan gate over `tests/smoke.c` + the GL-free units — preserve it, it is the only
  automated check).
- **Options** (Meson `meson_options.txt`): `opengl` (default true) vs `vulkan` (default false) —
  mutually-exclusive renderer source sets + the `IMGUI_IMPL_OPENGL_LOADER_GL3W` define; plus the
  `only_render_one_chunk` / `no_threads` toggles.
- **Dependencies (Linux path):** `dependency('glfw3')`, `dependency('libcurl')`,
  `dependency('sqlite3')`, GL via `dependency('gl')`/`dependency('dl')`, `m`, `threads`. Vendored
  `deps/` compiled from in-tree source (no fetch). On Linux, `sqlite`/`curl`/`glfw` come from the
  system (the vendored copies are Win/Mac-only — defer those platforms; note them).
- **Generated `config.h`:** `src/config.h.in` uses CMake `#cmakedefine` + `@VAR@`. Meson's
  `configure_file` uses **`#mesondefine`** — rewrite the `.in` (or generate via `configuration_data`).
  Watch `RESOURCE_PATH` (Debug = source dir, Release = install `share/craft/`).
- **Tricky bits (from the survey):** the mutually-exclusive renderer sets; the `#cmakedefine`→
  `#mesondefine` rewrite; `deps/glfw` + `deps/curl` are full CMake subprojects (Win/Mac only — rely
  on system libs on Linux, defer subproject wraps); `gl3w` ships a generator; heavy `WIN32`/`APPLE`/
  `else` branching → Meson `host_machine` logic; the `.bat`/MSVC multi-config story is replaced by
  Ninja. Scope **Linux client first**; Windows/macOS deferred and noted.

## Emacs C support (clangd + eglot) + update-emacs-packages — decided 2026-10-17

Mirror `/foo/opt/billsEmacsConfigs/c`:
- `Dockerfile` `ARG USE_EMACS=0` (Makefile passes `1`): install `emacs` + run
  `emacs --batch --load .emacs.d/install-melpa-packages.el`; ensure `clang-tools-extra` (clangd) is
  in the dnf list. The build must emit/symlink `compile_commands.json` (Meson does this natively in
  the build dir) for clangd.
- Vendor `entrypoint/dotfiles/.emacs.d/` (`init.el`, `preferences.el`, `helm.el`,
  `install-melpa-packages.el`) with clangd/eglot for C/C++.
- Add the `ELPA_MOUNT` + **`make update-emacs-packages`** target (copy the exemplar's: force
  `USE_EMACS=1`, rebuild image, wipe+reinstall `elpa/`, strip `*.elc`/`*.eln`, `git add -f`). This is
  the target the billsEmacsConfigs top-level refresh script invokes.

## Convention adherence (self-contained images + live source)

Per the container template convention: **all third-party deps installed at image-build time** (dnf, and
Craft's vendored `deps/` are built from in-tree source — no external fetch), so an exported image is
self-contained offline; **only Craft's own source is (re)built from the bind-mounted tree at runtime**
(a `make -f … build`-style target that re-runs cmake/ninja against the mount, like gltron's `build`
target) so my host edits propagate. No runtime network fetch.

## Running Craft's OpenGL in-container is a SOLVED pattern — copy modelviewprojection

Craft needs a GL/Vulkan context, and Craft's `CLAUDE.md` cites "it's a GPU app with no headless mode"
as a reason to stay off the template. **That is not a blocker** (Bill, 2026-08-16):
**modelviewprojection (`github.com/billsix/modelviewprojection`) already runs and displays its OpenGL
demos in-container** — so a GLFW + modern-OpenGL app like Craft runs there too. Copy mvp's approach; do
not treat this as a design risk.

- **Interactive:** X11 socket + Wayland passthrough (the mvp/gltron/spimulator GUI blocks) — displays on
  the host.
- **Headless (optional, for smoke-runs/screenshots):** Xvfb + Mesa software GL, which **works for GLFW
  apps** (glfw reports `Mesa 4.6` through Xvfb), unlike freeglut/GLUT. Vulkan headless would need
  lavapipe (Mesa's software Vulkan) — verify if wanted; not required for the OpenGL path.

So the container Makefile just needs mvp-style X11/Wayland passthrough on its run targets. The GUI
concern in Craft's `CLAUDE.md` is stale, not a real obstacle — fold that correction into the CLAUDE.md
update this task already calls for.

## Related task ideas folded in here (2026-08-27, William Emerison Six <billsix@gmail.com>)

A batch triage mapped the maintainer bullet *"Make dockerfile for server, and one for client"* here.
The **client** half is this task (the dev container already builds + runs the C client). The
**server** half is a separate new task, `server-dockerfile.md`, which is blocked on
`revive-python-server.md` (the Python server must run under Py3 before it can be containerized). See
Q4 below for the one scope question this raises for the client side.

## Open questions

1. **Build at image-time or shell-time?** gltron compiles at image-build (ready-to-run image) *and*
   offers a `build` target that recompiles from the mount; spimulator builds at image-build with a
   test gate. Craft has no test suite, so a gate adds little. Recommend: **build at image-build for a
   ready image, plus a `build` target that recompiles from the mounted source** (matches gltron; gives
   both a working image and host-edit propagation). Bill's call.
2. **Which backends on by default?** OpenGL is the working path; Vulkan is in-progress and ImGui is
   integrated. Recommend flags `USE_VULKAN` / `USE_IMGUI` defaulting on in the Makefile, off in the
   Dockerfile (fleet convention), so a bare build is lean. Bill's call.
3. **Headless smoke-run wanted?** Since there's no test suite, a `make run`/screenshot target via
   Xvfb+Mesa could give a minimal "does it open a window and draw" check. Optional — include it or not?
4. **(added 2026-08-27) "Client Dockerfile" scope** — does the maintainer's "client" Dockerfile bullet
   mean *this* dev container (build + run the C client — recommended, avoids duplication), or a separate
   client *runtime* image distinct from the dev container? If the latter, that's a new task, not this.
