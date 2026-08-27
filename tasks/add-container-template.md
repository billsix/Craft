# Add a Fedora-44 + Podman dev container (Dockerfile + Makefile + entrypoint/) per my project template

**Status:** proposed — needs go-ahead. Created 2026-08-16 (William Emerison Six <billsix@gmail.com>).
**Priority:** 5
**Difficulty:** 5

## Goal

Bring Craft onto my standard **container-per-project template** — the Fedora-44 + Podman
ephemeral-container dev environment (`Dockerfile`, a `Makefile` of `podman run --rm` targets, and
`entrypoint/` scripts) that gltron (`github.com/billsix/gltron-mirror`) and spimulator
(`github.com/billsix/spimulator`) use — so Craft builds and runs in the same reproducible sandbox as my
other C/C++ graphics projects.

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
