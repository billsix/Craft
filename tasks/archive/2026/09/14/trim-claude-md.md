# Trim Craft's CLAUDE.md (13,329 B ≈ 3.3K tok, loaded every session)

**Status:** Done — trimmed 2026-09-13 (archived 2026-09-14)
**Priority:** 4
**Difficulty:** 3

## Result (2026-09-13)
`CLAUDE.md` **13,329 B → 11,100 B** (−2,229 B). Moved to
`tasks/reference/architecture-overview.md`: the `main.c` walkthrough and the Python
multiplayer-stack detail (unique facts — `MAX_CHUNKS`=8192, hit-testing/collision, the
`<prefix>/server` install layout — folded in verbatim under a "Folded from CLAUDE.md" note;
the rest was already covered there). The render-backend section was trimmed to a ~2-line
pointer. The Tasks "Completed" sanitizer-gate detail was dropped (it lives in the archived
task + the Status section) and collapsed to a one-line pointer, which also removed a
container-absolute cross-repo path. The `src/` file tables, reference-doc pointer block,
build/run, CMake options, deps, conventions, and the open-issues list stayed inline per the
MODERATE target. arch-overview.md grew 19,500 B → 20,525 B.

## BLUF
Craft's `CLAUDE.md` is 13,329 B (~3.3K tok) and re-splices into the AI context every
turn. Roughly a third of it is deep mechanics — a `main.c` code tour, the render-backend
internals, and the Python-stack walkthrough — that already have a home in
`tasks/reference/architecture-overview.md`. Move that detail into the reference set and
keep `CLAUDE.md` as the lean high-level map + operational how-to it says it wants to be
(line 24: "This file stays the lean high-level map; detail lives there.").

## Context
- **Why:** `CLAUDE.md` loads on every session/turn, so its byte cost is paid
  continuously. Method + measured numbers: runCrushInContainer
  `tasks/reference/crush-context-assembly.md` (2026-09-13: geometricalgebra's 74,732 B
  CLAUDE.md measured at ~18,683 tok/turn, 47% of Crush's system prompt).
- **Current size:** 13,329 B ≈ 3.3K tok. **@-imports:** none.
- **Convention:** `CLAUDE.md` stays lean and operational (what-it-is + brief layout,
  build/run + gate, invariant conventions, pointers to reference docs); rationale,
  deep mechanics, and walkthroughs move to `tasks/reference/<slug>.md`.
- **Existing `tasks/reference/` docs (destinations already exist):**
  - `architecture-overview.md` (19,500 B) — the deep `file:line`-anchored companion,
    "every subsystem mapped, plus a verified live-vs-dead table and known-bugs list."
    This is the natural home for the moved walkthroughs.
  - `coordinate-and-chunk-vocabulary.md`, `threading.md`, `scriptability-options.md`,
    `mipmapping-feasibility.md`, `opengl-draw-overhead-and-azdo.md`.

## Stay vs move (section-by-section)

| section (heading) | ~bytes | Verdict | Destination |
|---|---|---|---|
| Intro / "what this is" (fork, no-headless, not-on-template) (L1–18) | ~1,100 | STAY | CLAUDE.md |
| Reference docs (pointer list) (L20–40) | ~2,000 | STAY | CLAUDE.md (it IS the pointer block) |
| Layout tree (L42–57) | ~700 | STAY | CLAUDE.md |
| `src/` two-halves file tables (L59–86) | ~1,900 | STAY (trim if long) | CLAUDE.md — good orientation; keep the two tables |
| `config.h` generated note (L87–91) | ~400 | STAY | CLAUDE.md (operational: edit the `.in`) |
| Render-backend abstraction — deep detail (L93–102) | ~900 | TRIM → 2 lines | detail to `architecture-overview.md` |
| `main.c` at a glance — code walkthrough (L104–122) | ~1,500 | MOVE | `architecture-overview.md` (leave a 1-line pointer) |
| Python multiplayer stack — detail (L124–135) | ~800 | TRIM → pointer | detail to `architecture-overview.md` |
| Build / run (L137–153) | ~1,000 | STAY | CLAUDE.md (operational) |
| CMake options table (L154–165) | ~700 | STAY | CLAUDE.md (operational) |
| Dependencies ("treat deps/ as upstream") (L166–173) | ~700 | STAY | CLAUDE.md (invariant convention) |
| Conventions (C11/C++17, clang-format, license) (L175–185) | ~700 | STAY | CLAUDE.md |
| Status / known issues (L187–208) | ~1,400 | STAY (tighten) | CLAUDE.md (live open-issues list) |
| Tasks (L210–225) | ~1,100 | TRIM | CLAUDE.md — drop the completed-sanitizer detail (already in the archived task); keep a one-line pointer |

## Projected result
- Moving the `main.c` walkthrough + render-backend internals + Python-stack detail, and
  trimming the completed-tasks prose, should cut roughly **3.5–4.5 KB** — landing the file
  near **~9 KB (~2.2K tok)**, dominated by the still-useful file tables, build/run, and the
  open-issues list.
- No information lost: `architecture-overview.md` already exists and is the stated deep
  companion; the moves consolidate rather than duplicate.

## Open questions
1. Fold the moved walkthroughs into the existing `architecture-overview.md`, or split
   the `main.c` tour into its own `tasks/reference/main-c-walkthrough.md`?
   **Recommendation:** fold into `architecture-overview.md` (it already claims to map
   every subsystem — avoids a new file).
2. The `src/` file tables (L59–86) are borderline — keep in `CLAUDE.md` as orientation,
   or move to the reference doc too? **Recommendation:** keep them in `CLAUDE.md`; a
   file→responsibility map is exactly the lean orientation the file should carry.

## Related
- runCrushInContainer `tasks/reference/crush-context-assembly.md` — measurement + method.
- `tasks/reference/architecture-overview.md` — the move destination.
