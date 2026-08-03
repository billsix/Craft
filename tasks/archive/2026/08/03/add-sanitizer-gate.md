# Add an ASan + UBSan(trap) build gate to Craft

**Status:** complete
**Priority:** 5
**Difficulty:** 7
**Created:** 2026-06-16
**Implemented:** 2026-08-03
**Completed:** 2026-08-03
**Decision (step 5, Bill 2026-08-03):** leave `make sanitize` **on-demand** — NOT
wired into CI/automated build. Craft has no CI or container template yet, so the
human/CI-invokable `make sanitize` target is the intended interface for now.

## Work log (2026-08-03)

Built the gate as planned: scope option 1 (per-target flags, deps
un-instrumented). Verified natively in-sandbox with clang 22 (no nested-podman
image build — the harness is a light standalone target, not the game).

**Files added / changed:**

- `tests/smoke.c` (new) — the non-GL smoke harness. Drives every GL-free unit
  with adversarial inputs: `create_world` over a 3x3 grid of chunks into a Map
  (world-gen + noise + heavy `map_grow` rehashing); a standalone Map stress
  (small mask, ~1600 sets forcing grows, hit/miss `map_get`, `map_copy`); a
  Ring fill/drain past capacity across all 5 entry types; a SignList past
  capacity with duplicate-coord and over-length text; `db_init(":memory:")`
  driving both the synchronous paths (signs, auth, save/load state) and the
  async ring/worker-thread paths (blocks, lights, keys, commit) then loading
  back into a Map/SignList and `db_close`; the full `cube.c` mesh set
  (`make_cube` over all `items[]`, `make_plant`, `make_player`,
  `make_cube_wireframe`, `make_character` over the printable range,
  `make_character_3d` over all 8 faces, `make_sphere` detail 0-3); the whole
  `matrix.c` API; the `item.c` predicates over the negative ids world-gen
  produces; and the pure `util.c` helpers (`rand_*`, `char/string_width`,
  `wrap`, `tokenize`, `flip_image_vertical`, `malloc_faces`). Buffers are sized
  to each generator's exact maximum output so ASan overflow checks are
  meaningful. LSan is disabled via a weak `__asan_default_options` (guarded).
- `CMakeLists.txt` — `option(ENABLE_SANITIZER_GATE ... OFF)` +
  `SANITIZER_GATE_KIND` cache string, and a Linux-only block building
  `craft_smoke` from the harness + the 9 GL-free `src/*.c` units with the
  sanitizer flags scoped via `target_compile_options`/`target_link_options`.
  `deps/noise` + `deps/tinycthread` go into a separate un-instrumented
  `craft_gate_deps` OBJECT library; sqlite is the system library. The full
  `craft` game target is untouched and the block is inert when the option is
  OFF (verified: default configure succeeds, `craft_smoke` target absent).
- `Makefile` — `make sanitize` target: configures + builds + runs the harness
  for `address` then `undefined`, **accumulating a `status` var and
  `exit $status`** so a failure in either config (or in configure/build) fails
  the target — it does not rely on the last command's exit code.
- `.gitignore` — ignore `craft_smoke` and the `sanitizeBuild-*/` scratch dirs.
- `src/map.c` — **UB fix** (see below).

**UB/memory site fixed (step 4):**

- `src/map.c:32` `hash_int` — Thomas Wang's integer hash did its intended
  modulo-2^32 wraparound in signed `int`: `key << 15` on a negative `key`
  (left-shift of negative value) and `key * 2057` (signed overflow) are both
  UB, and diagnostic UBSan flagged exactly those two lines (reached via every
  `map_set`/`map_get`, i.e. world-gen and the map stress). Fixed by computing
  in `unsigned int` and casting back — the standard, well-defined form of this
  hash. Only the low bits are ever used (`& map->mask`), the map is transient
  and never persisted by hash layout, so the bucket-distribution change is
  behaviour-safe. This was the **only** UB site in `src/`; everything else was
  clean on the first diagnostic pass.

**Verification (all native in-sandbox, clang 22.1.8):**

- `make sanitize` → exit 0. ASan config: clean. UBSan-trap config: clean.
- Diagnostic (non-trap) UBSan over the whole harness after the fix: **0**
  runtime errors (was 2, both `map.c` `hash_int`).
- Negative test (confirms the gate bites and stays scoped to `src/`): injecting
  a signed overflow into `matrix.c` traps under UBSan (SIGILL, rc 132);
  injecting a heap overflow into `sign.c` aborts under ASan
  (`heap-buffer-overflow`). Deps compiled un-instrumented, so their intentional
  UB cannot fail the gate.
- Default build path unaffected: configure with the option OFF succeeds and
  exposes no `craft_smoke` target.
- **Not run:** the full `craft` game binary under sanitizers — impossible by
  design (interactive OpenGL, needs a display/GPU, no headless mode); and no
  nested-podman image build (resource limit + Craft has no container template).
  The harness is the honest coverage of the sanitizable surface.

**Latent issues noticed, deliberately NOT fixed here (out of scope, low-risk,
flagged for Bill):**

- `src/db.c:499` `db_worker_start(char *path)` is defined taking an argument but
  declared `void db_worker_start()` in `db.h:53` and called with no argument at
  `db.c:166` — clang warns (`-Wdeprecated-non-prototype`), and the `path`
  parameter is read from a garbage register/slot and handed to `thrd_create`,
  though `db_worker_run` ignores its arg so it's harmless in practice. A
  one-line fix (drop the unused `char *path` param, or thread the db path
  through) would clear the warning; left alone to keep this change scoped to the
  sanitizer gate.
- `src/util.c:117` `wrap()` guards its `strncat` lengths with
  `max_length - strlen(output) - 1` (a `size_t`); the guard holds for the game's
  realistic callers (generous 1 KB buffers) so the harness drives it that way
  and it's clean, but the arithmetic is fragile if `output` ever fills. Not a
  gate finding; noted as a possible hardening follow-up.

## Goal

Add a build-time **sanitizer gate** to Craft that compiles its **own `src/`
code** under two sanitizers and exercises it, failing the build on any
undefined behaviour or memory error:

- **UBSan in trap mode** — `-fsanitize=undefined -fsanitize-trap=undefined`.
  Any UB (signed-int overflow, bad shift, null/misaligned deref, out-of-range
  enum, …) emits `ud2` and dies with SIGILL. This is the reliable pass/fail
  gate. (Diagnostic UBSan, `-fsanitize=undefined` without trap, *under-reports*
  — see the spim sweep below — so trap mode is authoritative.)
- **ASan** — `-fsanitize=address`. Catches buffer overflow/underflow,
  use-after-free, double-free, and (via bundled LSan) leaks.

This mirrors the gate just landed in the sibling project **spimulator**.
Primer on what ASan/UBSan are and *why* trap mode is the gate (with the
integer-UB background):
`/billopt/spimulator/tasks/archive/2026/06/16/ubsan-sweep.md`. The spim wiring
to copy the shape from: its `Dockerfile` (search `RUN_SANITIZERS`, a
`RUN if [ "$RUN_SANITIZERS" = "1" ]` block that does a second/third instrumented
build + test run) and `Makefile` (`RUN_SANITIZERS ?= 1`, threaded as a
`--build-arg`).

## The hard part: Craft is an interactive OpenGL game with no tests

Two caveats make this materially harder than spim, and shape the whole plan:

### (a) Display / GPU dependency — cannot run the full binary as a gate

Craft (`src/main.c`) is an interactive voxel game: it opens a GLFW window,
needs an OpenGL/Vulkan context and a GPU, and blocks in an event loop on user
input. There is **no headless mode** and **no test suite at all** (a scan of
the repo found none — no `tests/`, no `ctest`, no smoke harness). So "build
instrumented, then run it and watch for a trap" — the move that makes spim's
gate work (it has a 29-case `meson test` suite) — **has no equivalent here yet.**
A sanitizer build with nothing to run is worthless: sanitizers are *dynamic*,
they only catch bugs on code paths that actually execute.

The realistically sanitizable part of Craft is its **non-GL logic**, which is
substantial and self-contained:

| Unit            | What it does                          | GL-free? |
|-----------------|---------------------------------------|----------|
| `src/world.c`   | procedural world-gen (calls `noise`)  | yes      |
| `src/map.c`     | voxel hash-map (alloc/grow/get/set)   | yes      |
| `src/ring.c`    | ring buffer                           | yes      |
| `src/db.c`      | sqlite persistence (blocks/signs/auth)| yes      |
| `src/cube.c`    | cube-mesh geometry generation         | yes      |
| `src/item.c`    | block/item tables                     | yes      |
| `src/matrix.c`  | 4×4 matrix math                       | yes      |
| `src/sign.c`    | sign-text list management             | yes      |
| `deps/noise/noise.c` | simplex noise (used by world)    | yes (dep)|

(Verified: none of the above `#include` GLFW/gl3w/imgui/OpenGL/Vulkan.
By contrast `util.c`, `main.c`, `client.c`/`auth.c` and the `gui*`/`*_render*`
files pull in GLFW / curl / GL and are **not** unit-sanitizable in isolation.)

**Therefore the gate prerequisite is a small smoke/unit harness** (a
`tests/`-style program, not committed into the game binary) that drives this
non-GL surface with adversarial inputs:

- `create_world()` over a spread of chunk coords (drives `world.c` + `noise.c`);
- `Map` alloc → many `map_set`/`map_get` forcing `map_grow` rehashes, then
  `map_free`;
- a `Ring` fill/drain cycle past capacity;
- `db_init(":memory:")` → insert/load blocks, lights, signs, auth, state →
  `db_commit` → `db_close` (drives sqlite + tinycthread);
- `make_cube`/mesh helpers and `matrix.c` operations on representative inputs.

Run this harness under each sanitizer; **any trap/abort fails the build.** This
is honest coverage of the testable part — it does **not** cover the
render/input/networking code, which needs a display and is out of scope for an
automated gate.

### (b) Bundled third-party deps would get sanitized too

Craft bundles its deps in-tree (`deps/`: gl3w, glfw, imgui, lodepng, noise,
sqlite, tinycthread, curl) and compiles several straight into the `craft`
target via `CMakeLists.txt` (`lodepng.c`, `noise.c`, `tinycthread.c`,
`imgui*.cpp`, …). A blanket `-DCMAKE_C_FLAGS=-fsanitize=...` would instrument
**all of them**, and third-party C like **sqlite** and **lodepng** is very
likely to surface its own benign UB (sqlite is notorious for intentional
unsigned/shift tricks) and its own leaks — noise that would fail the gate for
reasons that aren't Craft's bugs.

**Scope the sanitizer flags to Craft's own `src/` translation units, not
`deps/`.** Options, preferred first:

1. **Per-target / per-source flags in CMake.** Don't set global
   `CMAKE_C_FLAGS`. Instead apply the sanitizer flags only to the
   Craft-authored sources — e.g. build the harness + the listed `src/*.c` units
   as their own instrumented target/object library with
   `target_compile_options(... -fsanitize=...)` +
   `target_link_options(... -fsanitize=...)`, linking the deps it needs
   (sqlite, noise, tinycthread) **un-instrumented**. This is the clean answer
   and keeps deps out of the blast radius entirely.
2. **Suppressions, if some dep must be instrumented** (e.g. you choose to
   sanitize `noise.c` since world-gen leans on it): keep a
   `UBSAN_OPTIONS=suppressions=...` / `ASAN_OPTIONS=...` file (or
   `-fsanitize-blacklist=`/`-fsanitize-ignorelist=`) excluding `deps/sqlite/*`,
   `deps/lodepng/*`, etc. Note **trap mode can't be suppressed at runtime** (it
   has no runtime — it's a `ud2`), so for any dep you cannot fix, you must
   either *not* trap-instrument it (use the ignorelist at compile time) or run
   it under diagnostic UBSan with suppressions. This is why option 1
   (don't instrument deps at all) is preferred.

### ASan + leaks

If the smoke harness (or any sanitizable path) is intentionally leaky or you
don't want LSan failing the gate on unfreed-at-exit allocations, default leak
detection **off** the way spim does — a weak, ASan-guarded
`__asan_default_options`:

```c
#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
const char *__asan_default_options(void) { return "detect_leaks=0"; }
#endif
```

Put it in the harness (or a small shared tu), guarded so non-ASan builds ignore
it. The gate is then for *corruption* (overflow/UAF/double-free), not leaks.
Prefer fixing real leaks in `src/` over blanket-disabling, but Craft's
map/db code may legitimately hold allocations for process lifetime.

## Where the flags go (CMake)

- The build is **CMake** (`CMakeLists.txt`) driven by a thin `Makefile`
  (`make debug` / `make release` → `cmake -S. -B… -DCMAKE_BUILD_TYPE=…`). There
  is no Dockerfile in this repo today (unlike spim), so the gate likely lives
  as a **new CMake option** (e.g. `option(ENABLE_SANITIZER_GATE ...)`) plus a
  `make sanitize` Makefile target, rather than a Dockerfile `RUN` block.
- **Do not** use global `add_compile_options` / `CMAKE_C_FLAGS` for the
  sanitizer flags — that hits `deps/`. Use `target_compile_options` /
  `target_link_options` on the instrumented harness target only (see scope
  option 1).
- Both compile *and* link need the flag (`-fsanitize=address` /
  `-fsanitize=undefined -fsanitize-trap=undefined` on each).
- Build with **clang** (matches spim; its trap UBSan + ASan are well-trodden),
  `CMAKE_BUILD_TYPE=Debug`.

## In-container only

Per the working arrangement, do everything in-container. Trap UBSan needs **no
extra package** (no runtime to link — it's just `ud2`). ASan needs the
compiler-rt ASan runtime, normally already present with clang; if a temporary
dev package is needed it's an allowed tracked/temporary build-file addition
(remove before done). Craft has no container template of its own yet — if the
gate is to run automatically it'd need a CI/Make entry point; for now a
`make sanitize` target that a human/CI invokes is the minimum.

## Plan

1. **Write the smoke/unit harness** (`tests/smoke.c` or similar) driving the
   non-GL units in the table above with adversarial inputs. This is the gate's
   substance and the bulk of the work — without it there is nothing to run.
2. **Add a CMake `ENABLE_SANITIZER_GATE` option** that builds the harness +
   `src/` units instrumented (scoped per-target, deps un-instrumented), one
   configuration for UBSan-trap and one for ASan.
3. **Add a `make sanitize` target** that configures both, builds, and runs the
   harness under each; non-zero exit (SIGILL trap / ASan abort) fails it.
4. **Fix** any UB/memory sites the harness surfaces in `src/` (smallest diff;
   compute-in-unsigned for intended wrap, as spim's sweep did). Record each
   fixed site (file:line, what + why) here before archiving.
5. **Decide** whether to wire the gate into a CI/automated build (a
   policy choice — like spim leaving the permanent lane to Bill).

## Acceptance

- A non-GL smoke harness exists and exercises world-gen, the voxel map, the
  ring buffer, sqlite persistence, noise, cube/matrix/sign logic.
- That harness runs clean under both `-fsanitize=undefined -fsanitize-trap=undefined`
  (trap) and `-fsanitize=address` (ASan), with flags **scoped to `src/`, not
  `deps/`** (verified: a deliberately-broken `src/` line traps; deps are not
  instrumented / are suppressed).
- Each UB/memory site fixed in `src/` is noted here (file:line, what + why).
- ASan-leak policy recorded (LSan off via `__asan_default_options`, or real
  leaks fixed).
- Decision recorded on whether the gate runs automatically (CI/Make) vs.
  on-demand.

## Feasibility verdict

**Partially feasible, and worthwhile — but it is *not* the same gate as spim's.**
The full Craft binary is **not** automatically gateable (interactive OpenGL,
needs a display/GPU, no headless mode). What *is* realistically sanitizable is
Craft's non-GL core — world-gen, the voxel hash-map, ring buffer, sqlite
persistence, noise, cube/matrix/sign logic — which is a real and bug-prone
chunk of the code. The blocking prerequisite is that **no test/smoke harness
exists**, so step 1 (write one for the non-GL units) is the actual work; the
sanitizer flags are easy once there's something to run. Scoping flags to `src/`
(not the bundled `deps/`, esp. sqlite/lodepng) is essential to avoid
false-gate-failures from third-party UB/leaks.
