# Modernize Craft's C like the gltron sweep (cold-start)

**Status:** proposed — needs go-ahead
**Priority:** 6
**Difficulty:** 6
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Supersedes / consolidates:** `port-to-c23.md` and `const-correctness-sweep.md` (both the piecemeal
precursors of this — do those *as part of this* unified sweep, not separately). Complements
`fix-verified-bugs.md` (its remaining B4–B7 defects) and the existing archived sanitizer gate.
**Depends on:** `add-container-template.md` (the Meson port + container contract) landing first, so
there is a warning gate + in-container build to run against.

## BLUF

Run a single behavior-preserving C modernization sweep over Craft's first-party source (`src/`,
~7 KLOC: 14 `.c` + 3 `.cpp`), using the method gltron used on 2026-10-17
(`github.com/billsix/gltron-mirror`, `tasks/archive/2026/10/17/c-modernization-sweep.md`). The
headline for Craft: **it has no warning flags at all today** (not even `-Wall`) — so the biggest,
highest-value step is wiring the raised `-Werror` warning gate and driving it to zero. Then the real
safety payload (the ~28 unchecked allocations), then the safe idioms (`const`, double-`const`,
`size_t` at boundaries), then the C23 niceties. "Done" = the raised gate is green under clang **and**
gcc, the existing ASan+UBSan gate (`make sanitize`) still passes, and no gameplay behavior changed.
`deps/` is upstream — **scope to `src/` only**.

## Context (read first)

1. **Method template** — gltron's archived sweep (above) + the shared `c-coding-standard.md` and
   `c-modernization-sweep.md` reference docs (`github.com/billsix/runClaudeInContainer`,
   `tasks/reference/`). Class-by-class, behavior-preserving, **boundary casts not member retypes** for
   sign-conversion, fresh per-directory Edit-only subagents for the mechanical bulk, verify each.
2. **Craft C state (surveyed 2026-10-17):**
   - **C11, C++17. Zero warning flags** anywhere (CMake/Makefile) — the sanitizer gate adds
     `-fsanitize` but no `-W`. `.clang-format` = google, `SortIncludes: Never`.
   - **Already decent:** 0 bare `sprintf` (20 `snprintf`); heavy `const` already (e.g. `main.c` ~203
     `const`, some `char *const` idioms); good hygiene for 2013-era code.
   - **Biggest payload — ~28 malloc/calloc/realloc sites mostly unchecked**, e.g. `auth.c:35`,
     `client.c:180/193`, `util.c:60/70/119`, `gl_render.c:106/131`, `main.c:1118/1123`, and a
     `malloc_faces()` helper. NULL-check + state owner contracts (the gltron payload).
   - `NULL` (47 in `.c`) — leave as C `NULL` or move to `nullptr` under a C23 pin; the 3 `.cpp` files
     should use `nullptr`. int/size_t mixing is modest (only 3 `size_t` in `src/`); signed `int`
     lengths feed size-taking APIs — tighten at boundaries with casts, don't churn the domain ints.
   - Platform `#ifdef`s confined to `auth.c` + `client.c` (Winsock vs POSIX). `main.c` is 2900 lines
     (a possible later split — out of scope here).
3. **Craft is a reference fork, not a strict teaching artifact** like gltron — but still be surgical
   and behavior-preserving; there is **no headless game test**, only the GL-free `make sanitize` gate,
   so state per change what went unverified.

## Plan (phased; mirrors the gltron sweep)

### Phase 0 — nets
- [ ] Confirm the ASan+UBSan gate (`make sanitize` / `craft_smoke`) builds and passes as the runtime
      net before mutating memory code. (It exists: `tasks/archive/2026/08/03/add-sanitizer-gate.md`.)

### Phase 1 — wire the warning gate (the big new thing for Craft)
- [ ] In the Meson build (`add-container-template.md`), add the raised first-party warning set
      scoped to `src/` (not `deps/`): `-Wall -Wextra -Wpedantic -Wshadow -Wvla -Wwrite-strings
      -Wformat=2 -Wsign-conversion` (+ clang-only `-Wshorten-64-to-32 -Wimplicit-int-conversion`),
      `werror=true` once clean. Build under **clang and gcc**. Capture the worklist first; triage per
      class (fix / opt-out / drop a flag) like gltron did — expect a lot, since nothing was enforced.

### Phase 2 — safety payload
- [ ] NULL-check the ~28 allocations; `realloc` into a temp; state owner contracts; `[[nodiscard]]`
      on allocators. Keep any by-design exit-time leaks. (sprintf is already clean; audit `strcpy`.)

### Phase 3 — safe idioms, class-by-class
- [ ] `const` the rest; **double-`const` (`const T* const`)** for never-reassigned pointer
      locals/params over const data (the maintainer's cross-fleet idiom, now in the shared C standard).
- [ ] `size_t` / boundary casts for the signed-length→size-API sites (cast at the boundary; don't
      retype domain ints that do arithmetic).

### Phase 4 — C23 idioms (if the C standard is pinned to C23 — this absorbs `port-to-c23.md`)
- [ ] Pin `-std=c23` for `src/` (CMake/Meson); `nullptr` in the `.cpp` (and `.c` under the pin);
      `[[maybe_unused]]`/`[[fallthrough]]` where apt; `static_assert` for real invariants.

## Verification
Raised-warning build green under clang + gcc; `make sanitize` (ASan+UBSan) green; built in the
container (`add-container-template.md`). No general test suite — state unverified changes per the
sweep method.
