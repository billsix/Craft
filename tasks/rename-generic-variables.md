# Rename Craft's generic/terse variables to meaningful names

**Status:** blocked
**Priority:** 7
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** (1) `source-study-and-orientation.md` — renaming requires understanding the code first;
(2) maintainer answers the Open question below.
**Recheck:** the study task has produced enough understanding AND the Open question is answered
(maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Rename variables. Lots of them have generic names, I didn't write the
original source code, and I don't know what a lot of them mean."* Rename terse/generic identifiers in
Craft's own source to meaningful names. This is the "names" slice of the broader
`source-study-and-orientation.md` study.

## Context (investigation 2026-08-27)

- Dense terse names cluster in `cube.c` (mesh generation), `matrix.c`, and `main.c` block/chunk math.
- **Renaming depends on first understanding the code** — the maintainer explicitly doesn't know what
  many identifiers mean, so this is sequenced after `source-study-and-orientation.md`.
- `.clang-format` has `SortIncludes: Never`, so renames won't fight formatting.
- **Scope to `src/` only** — never rename inside vendored `deps/`.

## Plan (draft — after the study)

- [ ] Use the orientation understanding to map what each terse identifier means.
- [ ] Rename in `src/` only, per file/subsystem, keeping diffs reviewable and the build green.

## Open questions

1. **Approach** — rename incrementally per subsystem as understanding lands (recommended), scoped to
   `src/` only? Any names to deliberately preserve (e.g. ones matching the upstream/book conventions)?
