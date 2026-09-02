# Fix the verified bugs found during the architecture review

**Status:** A1–A3 done (2026-09-02, Claude Opus 4.8, granted discretion); B4–B7 still open (need
maintainer decisions — Q1–Q5). Created 2026-08-27 (William Emerison Six <billsix@gmail.com>).
**Priority:** 5
**Difficulty:** 3

## Goal

The source-study that produced `tasks/reference/architecture-overview.md` and
`tasks/reference/threading.md` surfaced a set of **verified** defects (each re-checked against source,
`file:line` below). Collect them into one cleanup pass. Most are small mechanical fixes; a few need a
maintainer judgment call (see Open questions) before touching.

Scope note: all fixes are in **`src/` first-party code** — never `deps/` (vendored, upstream).

## A. Clear mechanical fixes — DONE (2026-09-02)

All three applied and verified: `make debug` compiled clean and `make sanitize` (ASan + UBSan) stayed
green (exit 0). A2 additionally cleared the C23 `-Wdeprecated-non-prototype` warning. A3 was
compile-verified by a standalone syntax-check of `vulkan_render.c` under `-DENABLE_VULKAN_RENDERER=YES`
(the full Vulkan `craft` target is separately broken by `main.c`'s unconditional GL coupling — that is
item B4 / the known incomplete-Vulkan issue, not A3).

1. **DONE — `client_sendall` sent the wrong length** — `src/client.c:61`. Root cause was subtler than
   "stale length": the loop both advanced `count` and decremented `length` by `n`, so with condition
   `count < length` the two met at the halfway point and a real partial send **stopped early with data
   unsent**. **Fixed:** pass `length - count` to `send` and stop mutating `length`.

2. **DONE — `db_worker_start` signature mismatch** — `src/db.h:53` vs `src/db.c:499`. The sole caller
   (`db.c:166`) already passed no argument, and `db_worker_run` ignores its thread `arg` (state is the
   file-scope ring/mtx/cnd). **Fixed:** dropped the unused parameter — declaration and definition are
   now `(void)`, `thrd_create` passes `NULL`.

3. **DONE — `vulkan_viewport` parameter-order mismatch** — `src/vulkan_render.c:51` was
   `(x_min, x_max, y_min, y_max)`. **Fixed:** aligned the definition to the header
   `(x_min, y_min, x_width, y_width)`, which already matches the parallel `gl_viewport` signature.
   (Kept here rather than folded into the graphics-backends task — it was a one-line consistency fix.)

## B. Fixes that need a judgment call (see Open questions)

4. **`gen_sky_buffer` / `gen_player_buffer` fall off the end on a non-GL build** — `src/main.c:112-126`.
   Both return `GLuint` but the only `return` is inside `#ifdef ENABLE_OPENGL_CORE_PROFILE_RENDERER`; with
   that macro off (a Vulkan build) the functions run off the end of a non-void function → UB. They're also
   inherently GL-typed (`GLuint`), which quietly breaks the render-backend abstraction the fork
   advertises. **Only reachable on a Vulkan build, which is currently all stubs** — so low urgency, but a
   real latent UB. See Q1.

5. **`map_set(..., w==0)` on a missing key is a silent no-op** — `src/map.c` (`map_set`, ~`:98-130`).
   "Deleting" a block that isn't present does nothing (no real removal path). May be **intentional**
   (deletion handled by overwriting with a sentinel elsewhere) — needs confirmation before calling it a
   bug. See Q2.

6. **DB-handle concurrency** — `src/db.c`. The background commit thread does `sqlite3_step` **outside**
   any lock (`db_worker_run`), while main-thread reads hold only `load_mtx` and synchronous sign writes
   hold no lock — all on the **same `sqlite3 *` connection**. `mtx` guards the write *queue*, not the
   handle; `load_mtx` guards *reads*, not against the writer thread. Safe only if sqlite is compiled
   **SERIALIZED**. **Investigate** the effective threading mode (default is SERIALIZED, but confirm the
   amalgamation/build flags) and either document it as safe or add real serialization. See Q3.

7. **`ENABLE_NO_THREADS` does not work as written** — `src/main.c:2434`. The macro guards only the worker
   spawn; `ensure_chunks` still marks chunks clean-but-unmeshed and signals nonexistent workers, so only
   the 3×3 around the player ever builds (details in `tasks/reference/threading.md`). This is a
   maintainer-added learning-time flag, not upstream. **Either** add a synchronous compute path under the
   macro so it actually works, **or** remove the flag so it isn't a misleading broken option. See Q5.

## Lower-confidence smells (flagged during the read, not independently deep-verified — confirm first)

- `initialize_craft`'s final `return 0` sits inside a block scope (`src/main.c` ~`:2446`) — compiles only
  because that block always reaches it; structurally fragile. Worth a look, not asserted as a bug.
- Shadowed `light` variable near `src/main.c:708`.

## Plan

- [x] Apply A1–A3 (mechanical); rebuild (`make debug`) + `make sanitize` green. **Done 2026-09-02.**
- [ ] Resolve Q1–Q5, then apply B4–B7 as decided. **Still open — untouched this pass.**
- [x] Re-verify: build clean, sanitizer gate green (A1–A3 only).

## Open questions

1. **B4 (fall-through):** fix now with a fallback return, or defer until the Vulkan backend is real
   (it's only reachable there, and that's greenfield)? *(Recommend: add the fallback return now — it's
   one line and removes a latent UB.)*
2. **B5 (`map_set` no-op):** is the no-delete-on-missing behavior intentional? If so, no change (maybe a
   comment); if not, it's a real correctness gap.
3. **B6 (DB concurrency):** OK for me to investigate sqlite's threading mode and report before changing
   anything? (No code change without your nod on the approach.)
4. **A3/B4 Vulkan items:** fix the `vulkan_viewport` mismatch here, or hand it to
   `tasks/graphics-backends-vulkan-metal-d3d.md` (since the whole Vulkan renderer is greenfield)?
5. **B7 (`ENABLE_NO_THREADS`):** make it actually work (synchronous compute path), or remove the flag?
