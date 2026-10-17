# Craft

A voxel ("Minecraft-like") game engine in **C with modern OpenGL**, forked by
**William Emerison Six** from **Michael Fogleman's Craft**
(<https://github.com/fogleman/Craft>). Upstream is "a few thousand lines of C
using modern OpenGL (shaders)"; this fork keeps that spirit and adds a
**pluggable render backend** (an in-progress Vulkan path alongside the working
OpenGL one), **Dear ImGui** integration, refactors that pull rendering out of
`main.c`, and routinely-refreshed vendored dependencies.

It is a real-time, interactive, GPU application: it opens a GLFW window, needs an
OpenGL (or Vulkan) context, and runs an input-driven event loop. There is **no
headless mode and no general test suite** — keep that in mind for anything that
wants to "just run it" (CI, screenshots). The GL-free core *is* covered by a
sanitizer smoke harness (`tests/smoke.c`).

This project **is** on the maintainer's Fedora-44 + Podman container template as of
2026-10-17 (`Dockerfile`, `Makefile`, `entrypoint/`), same shape as gltron and
`/foo/opt/billsEmacsConfigs/c`. The Linux dev build is **Meson** (`meson.build`),
built in-image and re-runnable from the mounted tree via `make build`. **CMake stays**
as the cross-platform build (Meson here is Linux-only): `CMakeLists.txt` +
`src/config.h.in` + the `.bat` files carry Windows/macOS. See
`tasks/archive/2026/10/17/add-container-template.md` (done) and `tasks/retire-cmake.md`
(the open decision about dropping CMake).

## Reference docs

- **`tasks/reference/architecture-overview.md`** — the deep, `file:line`-anchored companion to this
  file: every subsystem mapped, plus a **verified live-vs-dead table** and a known-bugs list. Read it
  before touching a subsystem. (This file stays the lean high-level map; detail lives there.)
- **`tasks/reference/coordinate-and-chunk-vocabulary.md`** — what Craft's pervasive single-letter
  names mean (`p`/`q` = chunk column indices X/Z, `x`/`y`/`z` = block/world coords with `y` up,
  `w` = block type, `Map.origin_*`, `dp`/`dq`, `ex`/`ey`/`ez`, …), decoded with evidence. Read it
  before reading the block/chunk math in `main.c`/`world.c`/`map.c`.
- **`tasks/reference/threading.md`** — the threading deep-dive: the two subsystems (chunk worker pool +
  DB commit thread), the invariants that keep them safe, the sqlite-handle concurrency caveat, the
  `ENABLE_NO_THREADS` finding (it does not work as written), and the recorded C++-vs-C11 decision.
- **`tasks/reference/scriptability-options.md`** — survey of ways to make Craft scriptable (extend the
  `/command`+protocol, embedded Lua, embedded Python, protobuf-RPC), with a recommendation (Lua for
  in-game scripting; `builder.py` already covers out-of-process). Read before designing a scripting layer.
- **`tasks/reference/mipmapping-feasibility.md`** — whether the block atlas can be mipmapped: verdict is
  "not a drop-in" (atlas bleed + color-key transparency both fight it); the clean fix is a texture array +
  real alpha. Read before touching texture filtering.
- **`tasks/reference/opengl-draw-overhead-and-azdo.md`** — the chunk draw path costs (~2,400 draws/frame,
  per-frame VAO churn) and the AZDO ladder (hoist the VAO → persistent-mapped streaming → multi-draw-
  indirect). Read before touching the render hot path or a "faster GL" build.

## Layout

```
Craft/
├── meson.build           # Linux dev build (Meson + Ninja, C11 + C++17); built in-container
├── meson_options.txt     # opengl/vulkan/only_render_one_chunk/no_threads/sanitizer_gate
├── CMakeLists.txt        # cross-platform build (CMake ≥ 4.0); carries Windows/macOS
├── Makefile              # container contract (make image/build/shell/run/format/lint/…)
│                         #   + host CMake targets (make debug/release/sanitize/clean)
├── Dockerfile            # Fedora 44; builds the Meson client in-image
├── entrypoint/           # install/shell/format/lint/build scripts + vendored .emacs.d
├── buildDebug.bat        # Windows (VS) equivalents
├── buildRelease.bat
├── src/                  # FIRST-PARTY code (config.h.in for CMake, config.h.meson.in for Meson)
├── deps/                 # VENDORED third-party libs (see Dependencies)
├── shaders/              # GLSL: block/line/sky/text × vertex/fragment
├── textures/             # PNG atlases: texture, font, sign, sky
├── server.py world.py builder.py   # Python multiplayer server + tooling
├── TODO.md               # Vulkan/Metal backend roadmap
└── tasks/                # task docs (the convention below)
```

### `src/` — the two halves

**GL-free core (self-contained, no window/GPU — the testable/sanitizable part):**

| File         | Responsibility                                                    |
|--------------|-------------------------------------------------------------------|
| `world.c`    | procedural terrain generation (drives `deps/noise`)               |
| `map.c`      | the voxel hash map — `(x,y,z) → w` block storage, alloc/grow      |
| `ring.c`     | ring/circular buffer — the queue feeding background DB writes     |
| `db.c`       | sqlite3 persistence (blocks, signs, auth, player state)           |
| `cube.c`     | cube/plant/text mesh geometry generation (vertex data)            |
| `item.c`     | block & item tables (which textures, which are plants/obstacles)  |
| `matrix.c`   | 4×4 matrix math (translate/rotate/perspective/ortho/frustum)      |
| `sign.c`     | sign-text list management                                         |
| `util.c`     | helpers: rng, file load, PNG flip, tokenizer, text wrapping, FPS  |

**Window / GPU / network (needs a context or the outside world):**

| File                       | Responsibility                                          |
|----------------------------|---------------------------------------------------------|
| `main.c` (~2900 lines)     | the whole game (see below)                              |
| `gl_render.{c,h}`          | OpenGL 3.3 core-profile backend (`gl_*` API)            |
| `vulkan_render.{c,h}`      | Vulkan backend (`vulkan_*` API) — **WIP, see TODO.md**  |
| `gui.cpp` / `gui.h`        | Dear ImGui lifecycle (C-callable via `BEGIN/END_C_DECL`)|
| `gui-gl.cpp` / `gui-vulkan.cpp` | ImGui platform/renderer backend glue              |
| `client.c`                 | multiplayer client: socket send/recv, line protocol     |
| `auth.c`                   | online auth: CURL HTTPS POST → identity token           |

`config.h` is **generated** from `src/config.h.in` by CMake (`configure_file`).
It holds compile-time settings: window size, vsync, key bindings, chunk radii
(`CREATE/RENDER/DELETE_CHUNK_RADIUS`), `CHUNK_SIZE 32`, `DB_PATH`, `DAY_LENGTH`,
`COMMIT_INTERVAL`, and the `RESOURCE_PATH` / `ENABLE_*` cmakedefines. Edit the
`.in`, not the generated `config.h`.

### The render-backend abstraction (this fork's core idea)

`main.c` is written against a backend-agnostic API; CMake compiles exactly one backend
(`gl_render.c` / `vulkan_render.c`, with matching `gui-gl.cpp` / `gui-vulkan.cpp`). OpenGL is
complete and the default; Vulkan is a scaffold (see Status). Deep mechanics — the 30-function
seam, draw paths, the `main.c` event-loop walkthrough, the chunk/threading pipeline, and the
Python multiplayer stack — live in **`tasks/reference/architecture-overview.md`**.

## Build / run

Two builds coexist. The default `make` goal is `help`; `make help` lists every target.

**Linux dev — container + Meson (primary):**

- `make image` → builds the Fedora image and compiles+installs the Meson client
  in-image (ready-to-run `craft`). `USE_EMACS=1` (the Makefile default) also pulls
  Emacs + the vendored MELPA packages; `USE_GRAPHICS=1` pulls the X11/Mesa/Vulkan libs.
- `make shell` → shell in an ephemeral container with X11 + Wayland passthrough (run
  `craft` there); `make run` → run the installed `craft` directly.
- `make build` → re-run the Meson build against the mounted tree (writes `./builddir`,
  which is also clangd's `compile_commands.json` dir).
- `make format` → clang-format `src/` in-container; `make lint` → advisory
  clang-tidy + cppcheck; `make update-emacs-packages` → refresh the vendored `elpa/`.
- Direct Meson for options: `meson setup builddir -Dvulkan=true && meson compile -C builddir`.

**Cross-platform — host CMake (Windows/macOS, or Linux without the container):**

- `make debug` → configures `debugBuild/`, builds, installs to `debugBuildInstall/`;
  run `./debugBuildInstall/bin/craft`.
- `make release` → same via `releaseBuild/` → `releaseBuildInstall/`.
  Both force the **OpenGL** backend (`-DENABLE_VULKAN_RENDERER=NO`).
- `make sanitize` → the src/-scoped ASan + UBSan(trap) smoke gate via CMake.
- Direct CMake for non-default backends/options, e.g.
  `cmake -S. -Bbuild -DENABLE_VULKAN_RENDERER=YES && cmake --build build`.

`RESOURCE_PATH` (shaders/textures) is the **source tree** in Debug builds and the
install `share/craft/` in Release — so a Release binary must be `install`ed (or
run with the resources beside it) to find its assets.

### CMake options

| Option                                  | Default | Effect                                             |
|-----------------------------------------|---------|----------------------------------------------------|
| `ENABLE_OPENGL_CORE_PROFILE_RENDERER`   | ON      | build the OpenGL 3.3+ backend                      |
| `ENABLE_VULKAN_RENDERER`                | OFF     | build the Vulkan backend (WIP; needs `find_package(Vulkan)`) |
| `ENABLE_PYTHON`                         | OFF     | add the bundled python subdir (Win/Mac)            |
| `ENABLE_ONLY_RENDER_ONE_CHUNK`          | OFF     | debug viz: render a single chunk                   |
| `ENABLE_NO_THREADS`                     | OFF     | single-threaded chunk gen (old-hardware FPS)       |

Pick **exactly one** renderer (the `IMGUI_RENDERERS` source set is chosen by
whichever is on).

### Dependencies

Linux uses **system** `glfw3`, `libcurl`, `sqlite3` via `pkg-config`. Everything
under `deps/` is **vendored** and refreshed periodically (see `git log` — sqlite,
imgui, lodepng, gl3w, curl bumps): `gl3w` (GL extension loader), `glfw`
(windowing, Win/Mac), `imgui` (Dear ImGui), `lodepng` (PNG), `noise` (simplex
noise), `sqlite`, `tinycthread` (threads), `curl` (Win). Treat `deps/` as
upstream — don't reformat or "fix" it; changes belong upstream.

## Conventions

- **C11** for `src/*.c`, **C++17** for the ImGui/gui glue.
- `clang-format` **google** style with `SortIncludes: Never` (`.clang-format`) —
  include order is intentional. Run it with `make format` (in-container, over `src/`
  only; `deps/` is never formatted).
- C/C++ interop: C++ files that expose C entry points wrap them in
  `BEGIN_C_DECL`/`END_C_DECL` (the `extern "C"` macros from `util.h`).
- `.githooks/pre-commit` exists but is the git **sample** hook and is not active
  unless enabled (`git config core.hooksPath .githooks`).
- License: **MIT** (Fogleman 2013 + Six 2020), per-file headers; ImGui retains
  Omar Cornut's MIT header.

## Status / known issues

Keep this list honest — remove items when fixed (don't annotate them "done").

- **Vulkan backend is incomplete** — scaffolded only; `TODO.md` is the roadmap
  (open a window → sky → blocks → parity with the GL event loop). Metal is not
  started.
- **The game binary has no headless mode.** Nothing exercises `main.c` / the
  render / input / networking code without a display+GPU. The GL-free core
  (`world/map/ring/db/cube/item/matrix/sign` + the pure `util` helpers) *is* now
  covered: `tests/smoke.c` drives it and `make sanitize` runs it under ASan and
  UBSan-trap (scoped to `src/`, deps un-instrumented). See
  `tasks/archive/2026/08/03/add-sanitizer-gate.md`. There is still no general unit-test suite.
- **README is partly stale** — it predates the Vulkan/ImGui work, its Linux
  instructions clone `fogleman/Craft` (not this fork) and reference a `make`
  flow that differs from the current `Makefile`, and the hosted multiplayer
  server it mentions is gone.
- **Python server is legacy** — `server.py` mixes `socketserver` (Py3) with
  `SocketServer` (Py2) names and targets the old auth URL; it likely needs a
  Python-3 port + self-host config before multiplayer works.

## Tasks

Per the global convention, in-flight work lives in `tasks/<slug>.md`; completed
work moves to `tasks/archive/<YYYY>/<MM>/<DD>/`. **Several in-flight tasks were added
2026-08-27** (C23 port, const sweep, rename, threading/scriptability/mipmapping/AZDO
follow-ons, graphics backends, server revive/dockerfile, `fix-verified-bugs`, …) — see
`tasks/` and the reference docs above. Most are `blocked`/`proposed — needs go-ahead`.

**Done 2026-10-17:** the **CMake→Meson port + Fedora/Podman container + Emacs/clangd** shipped
(`tasks/archive/2026/10/17/add-container-template.md`) — Meson is the Linux dev build (in-container),
CMake stays cross-platform. **Open decision:** whether to retire CMake at all turns on dropping
Windows/macOS (Meson here is Linux-only) — `tasks/retire-cmake.md` (needs a decision, not started).
`modernize-c-like-gltron.md` is a unified behavior-preserving C sweep (warning `-Werror` gate — Craft
has none today — + allocation safety + `const`/double-`const` + C23) that **supersedes**
`port-to-c23.md` and `const-correctness-sweep.md`. The server half stays `server-dockerfile.md` +
`revive-python-server.md`.

Completed work is in `tasks/archive/`; e.g. the ASan + UBSan(trap) sanitizer gate
(`tasks/archive/2026/08/03/add-sanitizer-gate.md` — also summarised under Status above).
