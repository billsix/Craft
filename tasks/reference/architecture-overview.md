# Craft — architecture overview

**Reference document** — the deeper, `file:line`-anchored companion to the high-level map in the
repo `CLAUDE.md`. Read this to get oriented before touching a subsystem. Not a task; update in place.
Created 2026-08-27 (William Emerison Six <billsix@gmail.com>) from a fan-out read of the whole tree
(commit `f59081f3`); every "live/dead" verdict below was re-verified against the source, not taken
from a single read. Companion to the per-subsystem craft task docs in `tasks/`.

## What Craft is

A voxel ("Minecraft-like") engine in **C11 + C++17**, forked from Michael Fogleman's Craft (2013) by
William Emerison Six (2020). A real-time GLFW + modern-OpenGL app: opens a window, needs a GL (or
Vulkan) context, runs an input-driven event loop. **No headless mode**; the only automated coverage is
the sanitizer smoke test over the GL-free core. ~8,400 lines of first-party C/C++ in `src/` plus a
Python multiplayer server. Build is CMake (≥4.0); `Makefile` is a thin wrapper.

**The fork's central idea:** a **compile-time-selected render backend**. `main.c` is written against a
backend-agnostic API; `gl_render.{c,h}` and `vulkan_render.{c,h}` declare **parallel** function sets
(same 30-function contract, `gl_`/`vulkan_` prefix), and CMake compiles exactly one in.

## The two halves (from `CLAUDE.md`, confirmed)

- **GL-free core** (`world map ring db cube item matrix sign util`) — no window/GPU; the part
  `tests/smoke.c` + `make sanitize` exercise under ASan/UBSan.
- **Window / GPU / network** (`main.c`, `gl_render`, `vulkan_render`, `gui*`, `client`, `auth`) — needs
  a context or the outside world.

## Build & configuration

- **`CMakeLists.txt`** — targets `craft` (game exe, `:96-111`), `world` (shared lib for `world.py`,
  `add_library(world SHARED deps/noise/noise.c src/world.c)` `:194`), `craft_smoke` (sanitizer harness,
  `:248-259`). Standards: C++17 (`:5`), C11 (`set_property(... C_STANDARD 11)` `:114,260`).
- **Options + defaults:** `ENABLE_OPENGL_CORE_PROFILE_RENDERER` **ON** (`:10`), `ENABLE_VULKAN_RENDERER`
  **OFF** (`:11`), `ENABLE_PYTHON` **OFF** (`:12`), `ENABLE_ONLY_RENDER_ONE_CHUNK` **OFF** (`:13`),
  `ENABLE_NO_THREADS` **OFF** (`:14`), `ENABLE_SANITIZER_GATE` **OFF** (`:15`) + `SANITIZER_GATE_KIND`
  default `"address"` (`:16`). The renderer option swaps the `IMGUI_RENDERERS` source set
  (GL `:19-26` / Vulkan `:28-34`); pick exactly one.
- **`config.h` is generated** from `src/config.h.in` by `configure_file` (`CMakeLists.txt:78`) — holds
  `CHUNK_SIZE 32`, chunk radii (`CREATE/RENDER/DELETE_CHUNK_RADIUS`), key bindings (`CRAFT_KEY_*`),
  `DB_PATH`, `DAY_LENGTH`, `COMMIT_INTERVAL`, window/vsync, and the `ENABLE_*` cmakedefines. **Edit the
  `.in`, not `config.h`.**
- **Deps** (`deps/`, treat as upstream — don't reformat): compiled straight into `craft`
  (`CMakeLists.txt:99-110`) — lodepng, noise, tinycthread, all of imgui + backends; **sqlite** amalgamation
  on Win/macOS, **system** sqlite3/glfw3/libcurl via pkg-config on Linux (`:184-191`). gl3w built in the
  GL set (`:21`). curl is vendored/used **only** for the (dead) auth POST.
- **`Makefile`:** `make debug`/`make release` (configure→build→install into `*BuildInstall/`, GL forced,
  Vulkan off), `make sanitize` (builds+runs both ASan and UBSan-trap configs with clang, correct
  status-accumulate `:32-48`). `clean = git clean -fdx`. **`all` target is broken** (depends on
  nonexistent `image`/`html`). Windows: `buildDebug.bat`/`buildRelease.bat`.
- **`.githooks/pre-commit`** blocks non-ASCII filenames **and** auto-runs `clang-format -i` on staged
  `*.{c,cpp,cc,h,hpp}` outside `deps/`, re-staging — but is inactive unless
  `git config core.hooksPath .githooks`. `.clang-format` = `BasedOnStyle: google`, `SortIncludes: Never`.

## Core / `main.c` (2882) + `main.h` (261)

- **Global state:** a single `Model` (`main.h:159-195`), reached via `Model *g` (`main.c:77-78`).
  Key fields: `window`, `workers[WORKERS]` (`:161`), `chunks[MAX_CHUNKS]`+`chunk_count` (`:162-163`),
  chunk radii (`:164-167`), `players[MAX_PLAYERS]` (`:168-169`), typing/chat buffers (`:170-173`),
  `observe1/observe2` (main + picture-in-picture camera, `:176-177`), `item_index`, `ortho/fov`,
  `mode/mode_changed` (`:184-185`), `db_path/server_addr/server_port`. Render toggles are **loose
  extern bools** (`main.h:243-253`, defined `main.c:80-90`; a header TODO wants them in `Model`).
- **Two-loop structure:** `main()` (`:2450`) → `initialize_craft()` (`:2347`, window/GL/threads) then an
  **outer** `while(running)` loop (`:2460`, one iteration per world/session — owns DB+client init,
  `reset_model`, and shutdown `:2852-2875`) wrapping an **inner** per-frame `while(1)` (`:2514`). Inner
  phases in order: window/DPI (`:2517`), `glfwPollEvents` (`:2533`), imgui frame when paused (`:2538`),
  timing (`:2544`), input (`handle_orientation_input` `:2559`, `handle_movement` `:2562`), server recv +
  `parse_buffer` (`:2565`), DB commit every `COMMIT_INTERVAL` (`:2572`), position send (`:2578`), chunk
  management (`:2588`), 3-D render (sky→chunks→signs→players→wireframe, `:2635-2649`), HUD
  (`:2657-2794`), PiP (`:2797`), imgui render (`:2836`), swap+exit (`:2841`).
- **GL context:** requests **4.6 core forward-compat** (`:2380-2383`), **falls back to 3.3 core** if the
  4.6 window is NULL (`:2386-2393`). Init order in `main`: **DB → client (online only) → reset_model →
  self-player → `force_chunks` (synchronous first chunks)** (`:2462-2503`).
- **Input:** callbacks `on_key` (`:1844`), `on_char` (`:1939`), `on_scroll` (`:1971`), `on_mouse_button`
  (`:1987`); **mouse-look is polled** each frame (`handle_orientation_input` `:2023`), not a callback;
  **gamepad** move/look on `glfwGetJoystickAxes` (`:2042-2056`, `:2094-2107`). Block edit:
  `on_left_click` (`:1795`, remove), `on_right_click` (`:1811`, place), `on_middle_click` (`:1828`,
  pick), `on_light` (`:1783`).
- **In-game `/command` parser** `parse_command` (`:1700`): `/identity /logout /login /online /offline
  /view /copy /paste /tree /array /cube /fcube /sphere /fsphere /circle[xyz] /cylinder` (± solid-fill
  `f` variants); unmatched text with `forward` → `client_talk`. Builder helpers `cube/sphere/cylinder/
  tree/array` (`:1575-1680`) all sit on `builder_block` (`:1292`) — the same primitives `builder.py`
  drives over the wire.

## Chunk pipeline & threading

- **Worker pool:** `WORKERS`=4 (`main.h:35`), `Model.workers[]`. `Worker` (`main.h:89-96`) =
  index/state/`thrd`/`mtx`/`cnd` + an embedded `WorkerItem` (`main.h:77-87`: `p,q`, `load`,
  `block_maps[3][3]`/`light_maps[3][3]` = chunk+8 neighbors, `faces`, generated `float *data`).
- **Flow:** `worker_run` (`:1124`) waits on `cnd` for `WORKER_BUSY`, then `load_chunk`(if load) +
  `compute_chunk` + `WORKER_DONE`. `ensure_chunks` (`:1111`, per frame) → `check_workers` (`:953`,
  uploads GL buffers for DONE workers on the **main thread**) + `force_chunks` + hands each IDLE worker
  a chunk via `ensure_chunks_worker` (`:1016`, sharded by `(ABS(a)^ABS(b))%WORKERS`). The heavy mesher is
  `compute_chunk` (`:538`: opaque array, light flood-fill, face counting, AO, geometry). **No global
  chunk-list mutex** — the chunk array is touched only on the main thread; workers operate on copied
  neighbor maps (each `Worker.mtx` guards just its own state/item).
- **`ENABLE_NO_THREADS`** guards only the spawn (`:2434`); `ensure_chunks`/`check_workers` still signal
  workers that were never created, so async load never completes — only `force_chunks`' synchronous 3×3
  runs. Treat it as an **incomplete/experimental** mode, not a working config.

## Rendering (`gl_render.c` 691, the live backend)

- **The seam:** `gl_render.h` / `vulkan_render.h` are the **same 30-function contract** (viewport/clear,
  buffer/shader/program/texture setup, `*_render_chunk`, signs/players/sky/lines/text/…). Selection is
  two-level: the CMake option decides which TU compiles, and each file's whole body is wrapped in
  `#ifdef ENABLE_*_RENDERER` (`gl_render.c:25…691`, `vulkan_render.c:25…127`). `gui.cpp` (panel content)
  compiles unconditionally; only the backend gui (`gui-gl.cpp`/`gui-vulkan.cpp`) is swapped.
- **GL path:** four shader programs — **block/line/text/sky** — loaded in `gl_initiliaze_global_state`
  (`:225-279`), attrib/uniform handles captured into the `Block_/Line_/Text_/Sky_Attributes` globals
  (`gl_render.c:58-61`; **these structs are declared in `main.h:199-238` and consumed here** — not dead,
  just declared far from use). Buffers: `gl_gen_buffer` (`:79`) / `gl_gen_faces` (frees CPU data,
  `:90`). **Per-chunk render generates a VAO per call** (`gl_render_chunk:332-361`, flagged TODO — real
  per-frame churn). Draw paths: sky (`:477`), lines (`:531`), wireframe (`:547`, logic-op invert), text
  (`:564`), signs (`:394`, polygon-offset), players (`:446`), item/plant/cube (`:613`), crosshairs
  (`:676`).
- **Textures** (`gl_initiliaze_textures:281-310`): block atlas + sign use `GL_NEAREST` min/mag
  (`:287-288,307-308`); font + sky use `GL_LINEAR`; **no mipmaps anywhere** (no `glGenerateMipmap`) — see
  `tasks/mipmapping-investigation.md`. Upload via lodepng decode + `flip_image_vertical` +
  `glTexImage2D(GL_RGBA)` (`:150-164`).
- **Shaders** (all `#version 330 core`): block V/F (fog+AO+daylight+light, discards magenta as
  transparent), line V/F (constant color + CPU logic-op), sky V/F (scrolls by `timer`/time-of-day), text
  V/F (`is_sign` white-discard vs HUD alpha floor).
- **imgui:** `gui.cpp` builds the "Craft" panel incl. per-draw-path render toggles (`:80-100`);
  `gui-gl.cpp` is thin (`ImGui_ImplOpenGL3`); **`gui-vulkan.cpp` (550) hand-rolls a full ImGui Vulkan
  device/swapchain/command-buffer stack** — real Vulkan code, but it draws only the ImGui overlay.

## World data & geometry

- **Block store — `map.c/h`:** open-addressing hash map, linear probe, keyed (x,y,z)→`w`. `MapEntry`
  (`map.h:31-39`) is a **union** — a 32-bit `value` overlapping `{unsigned char x,y,z; char w}`, so
  emptiness is one compare vs `EMPTY_ENTRY`; coords are **chunk-local bytes 0-255**. `Map` = origin
  offsets + power-of-two `mask` + `data`. `map_set` (`:98`) auto-grows at `size*2>mask`; **setting `w==0`
  on a missing key is a no-op — deletion isn't a real removal.** (There is **no `MAP_FOR_EACH` macro** in
  this fork — iteration is a plain loop over `data[0..mask]` skipping empties; don't cite an upstream
  macro that isn't here.)
- **Mesh gen — `cube.c` (the terse file):** `make_cube_faces` (`:29-81`, static) is the core — 10
  floats/vertex (pos3/normal3/uv2/ao1/light1), 60/face. Cryptic params: `left..back` = **per-face
  visibility flags** (caller computes exposure from neighbor transparency); `wleft..wback` = **atlas tile
  indices** (16×16 atlas, `s=0.0625`); `ambient_occlusion[6][4]`/`light[6][4]` written straight into the
  stream; the **AO-driven quad-flip** (`:65-66`) rotates the triangle diagonal so AO interpolates without
  the anisotropic-AO artifact; `a/b` (`:57`) is a half-texel inset against atlas bleed. Wrappers:
  `make_cube` (`:83`), `make_plant` (`:93`, crossed quads), `make_player` (`:140`), `make_cube_wireframe`
  (`:159`), `make_character[_3d]` (`:174/:205`), and `make_sphere`/`_make_sphere` (`:286-376`, recursive
  subdivision) — **live: it builds the sky dome** (`main.c:114`, `gen_sky_buffer`).
- **World gen — `world.c`:** `create_world(p,q,func,arg)` (`:27`) emits blocks through a **callback**
  (`world_func`, `world.h:26`) — fully decoupled from `map`. Height from octave-summed `simplex2`
  (`deps/noise`), sand below `t=12` else grass; plants/trees/clouds gated by `SHOW_*` config flags. A
  1-block pad emits neighbor-edge blocks with **negative `w`** (boundary marker) — absorbed everywhere via
  `ABS(w)` (`item.c` predicates, `map`'s signed `char w`).
- **Matrix — `matrix.c`:** column-major float[16] — identity/translate/rotate(Rodrigues)/multiply/
  frustum/perspective/ortho, `frustum_planes` for culling (`zfar=radius*32+64`), `set_matrix_3d` (camera
  MVP), `set_matrix_item` (inventory preview), `mat_apply` (transform packed vertex data). `#undef
  near/far` (`:209-210`) dodges the Windows macros.
- **Ring buffer — `ring.c/h`:** the work queue feeding DB writes. `RingEntryType` = `BLOCK LIGHT KEY
  COMMIT EXIT` (`ring.h:26`); classic head/tail circular buffer, `ring_grow` doubles. Typed producers
  `ring_put_block/light/key/commit/exit`; consumed by the DB worker.
- **Items/signs — `item.c/sign.c`:** `blocks[256][6]` (`item.c:42-108`) maps block id → 6 face tiles
  (read by `make_cube`); `plants[256]` for plant tiles; predicates `is_plant/obstacle/transparent/
  destructable` all `ABS(w)` first (`is_transparent` drives face exposure). `sign.c` = growable text-sign
  list (64-char text, per (x,y,z,face)).
- **Util — `util.c`:** RNG (`rand_int` unbiased), `load_file` (shader slurp), `update_fps`, sign/HUD text
  layout (`tokenize/char_width/string_width/wrap` — proportional font), `malloc_faces` (mesh allocator),
  `flip_image_vertical`. **PNG decode is NOT here** — lodepng runs in `gl_render.c:150-164`; util only
  flips already-decoded rows. `BEGIN/END_C_DECL` C++ guards live in `util.h` (used by the `.cpp` gui glue).

## Persistence & networking

- **DB — `db.c` (sqlite):** gated on `db_enabled`. Six tables (`db.c:62-115`): `auth.identity_token`
  (in an **attached `auth.db`**, `:63`), `positionAndOrientation`, `block`, `light`, `key`, `sign`
  (`p,q`=chunk, `x,y,z`=block, `w`=type). 10 cached prepared statements (`:35-44`, prepared `:145-164`);
  opens a long-running `begin;` transaction. **Write model:** block/light/key inserts + commits are
  **queued to the ring buffer** and executed on a **background commit thread** (`db_worker_run:525-553`,
  dispatches on `RingEntryType`; commit every `COMMIT_INTERVAL`=5s); **signs write synchronously** on the
  caller; **reads are synchronous** under a separate `load_mtx`. The DB layer touches only local files —
  **single-player persistence is fully intact offline.**
- **Client — `client.c`:** a **line-based single-letter-command TCP protocol**, default port **4080**
  (`client.h:26`). Outbound: `V`ersion/`A`uth/`P`osition/`C`hunk/`B`lock/`L`ight/`S`ign/`T`alk
  (`:82-167`). `recv_worker` thread appends to a 1 MiB queue; `client_recv` (`:169`) frames complete
  lines (scans back to the last `\n`) — **the actual command dispatch (`parse_buffer`) lives in
  `main.c:2218`**, not here. `client_stop` deliberately leaks the recv thread/mutex (join commented out,
  `:269-273`).
- **Auth — `auth.c`:** one function `get_access_token` (`:43`) POSTs via libcurl to a **hardcoded remote
  URL** `https://craft.michaelfogleman.com/api/1/identity` (`auth.c:45`) — the upstream author's server,
  which this fork doesn't control. **No local fallback.**

## Python stack

- **`server.py` (696):** threaded TCP multiplayer server, sqlite-backed (`craft.db`), speaking the same
  protocol (tokens `:60-73`, verified against the client parser). Delegates terrain to `world.py`.
- **`world.py` (67):** ctypes-loads `./libworld.so` (the `world` CMake target = `src/world.c` +
  `deps/noise`) so the server generates **identical** terrain to the client. Hardcoded `.so` path is
  Linux-only (its own TODO).
- **`builder.py` (270):** a **client-side** script — connects to the server and speaks the wire protocol
  to place blocks; provides geometry generators (sphere/circle/cylinder/cuboid/pyramid/bitmap). Its
  `main()` is a gallery of commented-out examples.

## Live vs dead / vestigial (verified 2026-08-27)

**Live & working:** the GL renderer (default) and its whole draw set; the worker pool + `compute_chunk`;
all input incl. gamepad; `parse_command`; `map/world/matrix/ring/item/sign/util`; `make_sphere` (sky
dome); the `*_Attributes` structs (declared `main.h`, consumed `gl_render.c`); the **entire `db.c` layer
(offline persistence)**; the `world` lib + `world.py` on Linux; `craft_smoke` (`make sanitize`); the
clang-format pre-commit hook.

**Dead / broken / vestigial:**
- **Vulkan *game* renderer = empty scaffold, not a partial port.** Every function in `vulkan_render.c` is
  an empty body / `return 0` (verified — e.g. `vulkan_render_chunk(){}` `:91`, `vulkan_gen_buffer(){return
  0}` `:65`); zero real `vk*` calls. The signatures mirror the GL API (interface defined) but nothing is
  implemented — treat a Vulkan backend as **greenfield**. The only real Vulkan code is ImGui's own
  device/swapchain bring-up in `gui-vulkan.cpp` (reusable as reference). *(This refines the
  `graphics-backends-vulkan-metal-d3d.md` task's "already scaffolded" — scaffolded at the build/signature
  level only.)*
- **Online auth is dead** — the remote URL (`auth.c:45`) is upstream's, not this fork's; cached tokens in
  `auth.identity_token` are inert without it. Multiplayer as shipped therefore can't authenticate.
- **`server.py` won't even import under Py3** — Py2 `print` statements (`:83,660,674`) and `print >>`
  (`:677`) are SyntaxErrors; `SocketServer`/`Queue` (`:114,126…`) are NameErrors (imports were
  modernized, the body wasn't). See `tasks/revive-python-server.md`.
- **`ENABLE_PYTHON`** (OFF) gates `add_subdirectory(deps/python)` — but there is **no `deps/python`**, so
  it's unbuildable if enabled.
- **`ENABLE_NO_THREADS`** is an incomplete mode (spawn guarded, signalling not) — async chunk load never
  completes.

## Known bugs / footguns (verified)

- **`gen_sky_buffer`/`gen_player_buffer` (`main.c:112-126`)** return `GLuint` but only `return` inside the
  GL `#ifdef` — they **fall off the end (UB) on a non-GL build**, and are inherently GL-typed (a real
  wrinkle for the render abstraction the fork advertises).
- **`client_sendall` (`client.c:61`)** passes `length` (not `length - count`) to `send` each iteration —
  wrong for a true partial send; harmless only because messages are small single packets.
- **`db_worker_start`** — declared `()` (`db.h:53`) but defined `(char *path)` (`db.c:499`); the arg is
  unused. K&R-style empty-paren decl hides the mismatch.
- **`vulkan_viewport`** param order differs between header (`vulkan_render.h:26`) and impl
  (`vulkan_render.c:51`) — latent, harmless while the body is a stub.
- **`map_set(w==0)` on a missing key is a silent no-op** — "deleting" an absent block does nothing.
- *Lower-confidence smells flagged during the read (not all independently verified):* `initialize_craft`'s
  final `return` sits inside a block scope (structurally fragile); shadowed `light` var near
  `main.c:708`; several `TODO`s (`PositionAndOrientation.t`, `Block.w` semantics, VAO/magic-number
  cleanup).

## Cross-links

- Repo `CLAUDE.md` — the lean high-level map + honest known-issues list (this doc is its deep companion).
- Subsystem tasks this informs: `tasks/graphics-backends-vulkan-metal-d3d.md` (the Vulkan-is-greenfield
  finding), `tasks/mipmapping-investigation.md` (no mipmaps, `GL_NEAREST` atlas), `tasks/revive-python-
  server.md` (the exact Py2-isms), `tasks/const-correctness-sweep.md` / `tasks/rename-generic-variables.md`
  (`cube.c` is the terse hotspot; const is already partial), `tasks/zero-overhead-opengl-investigation.md`
  (per-call VAO churn in `gl_render_chunk`), `tasks/explain-threading-and-cpp-decision.md` (the worker
  pool + DB commit thread mapped above).
