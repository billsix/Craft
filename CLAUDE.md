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
headless mode and no automated test suite** — keep that in mind for anything that
wants to "just run it" (CI, sanitizers, screenshots).

This project is **not** on Bill's Fedora-44 + Podman container template — it's a
plain CMake project (no `Dockerfile`, no `entrypoint/`). That's a legitimate
divergence (it's an upstream fork), not drift.

## Layout

```
Craft/
├── CMakeLists.txt        # the real build (CMake ≥ 4.0, C11 + C++17)
├── Makefile              # thin convenience wrapper: `make debug` / `make release`
├── buildDebug.bat        # Windows (VS) equivalents
├── buildRelease.bat
├── src/                  # FIRST-PARTY code
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

`main.c` is written against a backend-agnostic API. `gl_render.h` and
`vulkan_render.h` declare **parallel** function sets (`gl_render_chunk` /
`vulkan_render_chunk`, `gl_gen_buffer` / `vulkan_gen_buffer`, …). CMake compiles
exactly one backend's `.c` into the binary based on the options below, and the
ImGui glue (`gui-gl.cpp` vs `gui-vulkan.cpp`) is swapped to match. The OpenGL
path is complete and the default; the Vulkan path is a scaffold (`TODO.md` tracks
"open a black window → render sky → render blocks → the rest"). A Metal backend
is a future TODO.

### `main.c` at a glance

Global game state is a single `Model *g` (defined in `main.h`): the GLFW window,
the `WORKERS`(=4) chunk worker threads, up to `MAX_CHUNKS`(=8192) `Chunk`s, up to
`MAX_PLAYERS` `Player`s, the local player's position/orientation, render radii,
typing/chat buffers, and offline/online mode. Responsibilities, roughly in file
order: chunk lifecycle (`create/init/delete/ensure_chunks`, `compute_chunk`,
`gen_chunk_buffer`) run partly on the worker threads; block get/set/record;
hit-testing (ray-cast along sight vector) and collision; the build primitives
(`cube`, `sphere`, `cylinder`, `tree`) also exposed in `builder.py`; the GLFW
input callbacks (`on_key`/`on_char`/`on_mouse_button`/`on_scroll`) and
movement/orientation; the chat `/command` parser; the render orchestration
(`render_chunks/signs/sky/players/text/...`, each delegating to the `gl_*`
backend); and `main()` → `initialize_craft()` → event loop.

Threading: chunk generation + meshing happens on the worker threads (tinycthread);
sqlite writes are queued through `ring.c` and committed every `COMMIT_INTERVAL`
(5 s) on a background thread. `-DENABLE_NO_THREADS=YES` makes it single-threaded
(higher FPS on old hardware, worse latency).

### Python multiplayer stack

- `world.py` — a `World` class that `ctypes`-loads `./libworld.so` and calls into
  the **same** `world.c`/`noise.c` the client uses, so the server generates
  identical terrain.
- `server.py` — a threaded TCP server speaking the line-based ASCII protocol
  (`A`uth, `B`lock, `C`hunk, `K`ey, `P`osition, …; default port **4080**).
- `builder.py` — scripting helpers (sphere/cylinder/cuboid/pyramid/…) for
  programmatically placing blocks.

CMake builds the `world` target as `libworld.so` and installs the Python files to
`<prefix>/server`.

## Build / run

The real build is CMake; the `Makefile` is a thin wrapper (and the default goal
is `help`):

- `make debug` → configures `debugBuild/`, builds, installs to
  `debugBuildInstall/`; run `./debugBuildInstall/bin/craft`.
- `make release` → same via `releaseBuild/` → `releaseBuildInstall/`.
  Both force the **OpenGL** backend (`-DENABLE_VULKAN_RENDERER=NO`).
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
  include order is intentional.
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
- **No test suite / no headless mode.** Nothing exercises the code without a
  display+GPU. This is the central obstacle for the proposed sanitizer gate
  (`tasks/add-sanitizer-gate.md`): the GL-free core (`world/map/ring/db/cube/
  item/matrix/sign`) is the realistically testable+sanitizable surface and would
  need a smoke/unit harness first.
- **`Makefile` `all` target is broken** — `all: clean image html` depends on
  `image` and `html`, which don't exist. Use `debug`/`release` directly.
- **README is partly stale** — it predates the Vulkan/ImGui work, its Linux
  instructions clone `fogleman/Craft` (not this fork) and reference a `make`
  flow that differs from the current `Makefile`, and the hosted multiplayer
  server it mentions is gone.
- **Python server is legacy** — `server.py` mixes `socketserver` (Py3) with
  `SocketServer` (Py2) names and targets the old auth URL; it likely needs a
  Python-3 port + self-host config before multiplayer works.

## Tasks

Per the global convention, in-flight work lives in `tasks/<slug>.md`; completed
work moves to `tasks/archive/<YYYY>/<MM>/<DD>/`. Current:

- `tasks/add-sanitizer-gate.md` — proposed ASan + UBSan(trap) build gate, scoped
  to the GL-free `src/` core (needs a smoke harness first). Mirrors the gate in
  the sibling **spimulator** project; background primer on ASan/UBSan lives at
  `/billopt/spimulator/tasks/archive/2026/06/16/ubsan-sweep.md`.
