# Const-correctness sweep of Craft's own source

**Status:** blocked
**Priority:** 6
**Difficulty:** 3
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (clang-tidy-driven sweep acceptable? scope).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Merges two maintainer bullets, verbatim: *"Study source ... add consts ..."* and *"add consts wherever
possible"* (the second is a strict subset of the first). Do a systematic `const`-correctness pass over
Craft's own source.

## Context (investigation 2026-08-27)

- `const` usage is already partial — e.g. `main.c:818,955` use `*const`. A systematic pass would touch
  `src/*.c` (`main.c`, `cube.c`, `matrix.c`, `world.c`, `item.c`, …).
- **Scope to `src/` only** — vendored `deps/` (sqlite, lodepng, noise) are upstream per `CLAUDE.md` and
  must not be swept.
- `.clang-format` is google-style with `SortIncludes: Never`, so a const pass won't fight formatting.
- Cross-links `source-study-and-orientation.md` (the parent "study" task).

## Plan (draft)

- [ ] Decide mechanism (Q1): a `clang-tidy` `misc-const-correctness`-driven sweep vs manual.
- [ ] Apply to `src/` only, reviewing the judgment calls (const-pointer vs const-pointee).
- [ ] Build clean (and keep the sanitizer gate green).

## Open questions

1. **Mechanism + scope** — is a `clang-tidy` (`misc-const-correctness`) driven sweep, scoped to `src/`
   only (never `deps/`), acceptable? It's largely mechanical but has real judgment calls (const the
   pointer vs the pointee). *(Recommend: clang-tidy-assisted, `src/`-only, human-reviewed.)*
