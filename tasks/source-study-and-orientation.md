# Study the source; produce an orientation/architecture reference + improvement list

**Status:** blocked
**Priority:** 6
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (deliverable: reference doc vs inline comments).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Study source, suggesting improvements, orientation, add consts, names."*

Read Craft's source, produce an orientation/architecture overview, and suggest improvements. This is
the umbrella "study" task; its two mechanical sub-parts are split out to avoid a grab-bag:
- **"add consts"** → `const-correctness-sweep.md` (merged with the separate "add consts wherever
  possible" bullet).
- **"names"** → `rename-generic-variables.md` (depends on the understanding produced here).

## Context (investigation 2026-08-27)

- `main.c` is ~2900 lines; terse identifiers cluster in `cube.c` (mesh gen), `matrix.c`, and `main.c`
  block/chunk math. `const` usage is already partial (e.g. `Worker *const worker`, `WorkerItem *const
  item` at `main.c:955,818`).
- No `tasks/reference/` exists yet — this project has no orientation doc.
- Sources are a fork; the maintainer did not write the original and doesn't know what many identifiers
  mean (see `rename-generic-variables.md`), which is exactly why the study comes first.

## Plan (draft)

- [ ] Read the tree; produce a `tasks/reference/architecture-overview.md` (subsystems: render abstraction
      `gl_*`/`vulkan_*`, chunk/world, worker pool + db thread, matrix/cube math, item/inventory), each
      claim `file:line`-anchored.
- [ ] Compile an improvement list (feeds the const sweep, rename, and other craft tasks).
- [ ] Cross-link the const sweep and rename tasks (they depend on this understanding).

## Open questions

1. **Deliverable** — is the orientation output a `tasks/reference/` architecture doc, or inline code
   comments (or both)? *(Recommend: a reference doc — reusable and keeps the source uncluttered.)*
