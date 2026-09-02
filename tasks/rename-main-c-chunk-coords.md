# Rename main.c's `p`/`q` chunk coordinates to meaningful names

**Status:** proposed — needs go-ahead (best done with the game runnable to verify)
**Priority:** 8
**Difficulty:** 5
**Created:** 2026-09-02 (William Emerison Six <billsix@gmail.com>) — spun out of the completed
`tasks/archive/2026/09/02/rename-generic-variables.md` (the deferred item).

## BLUF

Bulk-rename `main.c`'s pervasive chunk-column variables `p`/`q` → `chunk_x`/`chunk_z` (the
`Chunk` and `WorkerItem` struct fields plus the ~hundreds of locals that use them), so the
block/chunk math reads without the reader having to already know the upstream convention.
Deferred from the main rename pass because `main.c` has **no headless test** (compiler-only) and
the change touches hundreds of sites — high blast radius, low verifiability. "Done" = the rename
applied, `make debug` clean, `make sanitize` green, AND the running game visually confirmed
unchanged.

## Context

- **Read first:** `tasks/reference/coordinate-and-chunk-vocabulary.md` — `p` = chunk column index
  along world X, `q` = along world Z (a chunk is one `CHUNK_SIZE`=32-wide column). The meanings are
  already fully decoded and documented in-file (struct-field comments on `Chunk.p/.q` and
  `WorkerItem.p/.q`, and a vocabulary block above `chunked()`), so this rename is lower-risk than a
  cold one — it's now purely mechanical.
- **Why it was deferred (from the parent task):** `main.c` (~2882 lines of GL + threading) is not
  exercised by `tests/smoke.c`; only the compiler checks it. A behaviour-preserving rename *should*
  be safe, but "should" isn't "verified" for code with no test — and `p/q` are struct fields touched
  at hundreds of call sites. The parent task's rule was "leave what you can't verify"; this is that
  leftover.
- **Scope:** `src/main.c` (and the `Chunk`/`WorkerItem` definitions in `main.h` if the fields live
  there) ONLY. Never `deps/`. `p/q` also appear in `db.c` bound to sqlite **column** names `p`/`q` —
  do NOT rename those (a `chunk_x`-bound-to-column-`p` mismatch reads worse; see the reference doc).
- **The judgment call for the maintainer:** whether this is worth doing at all. It's a large diff of
  a pervasive upstream (Fogleman) convention; some contributors prefer keeping `p/q` *because* it
  matches upstream. Hence `proposed — needs go-ahead`.

## Approach (when greenlit)

1. Rename the `Chunk.p/.q` and `WorkerItem.p/.q` fields → `chunk_x/chunk_z`; let the compiler find
   every use, and fix each (they're field accesses + locals named to match).
2. Do it in reviewable chunks if possible (e.g. the worker/chunk-management block, then the render
   block), `make debug` + `make sanitize` green after each.
3. **Verify by running the game** (the whole reason it was deferred) — walk around, confirm chunks
   load/render/save correctly; a rename bug here would misplace or corrupt chunks. The headless gate
   cannot catch that.
4. Leave `db.c`'s `p/q` (sqlite column mirrors) alone.

## Relationships

- Parent (completed): `tasks/archive/2026/09/02/rename-generic-variables.md`.
- Vocabulary reference: `tasks/reference/coordinate-and-chunk-vocabulary.md`.
