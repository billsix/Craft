# Study the source; produce an orientation/architecture reference + improvement list

**Status:** DONE 2026-08-27 (William Emerison Six <billsix@gmail.com>) — deliverable produced.

## Outcome

Read the whole `src/` tree (fan-out, one reader per subsystem; commit `f59081f3`) and wrote the
orientation doc: **`tasks/reference/architecture-overview.md`** — the `file:line`-anchored companion to
`CLAUDE.md`. It maps every subsystem (core/main loop, chunk pipeline + threading, rendering, world
data & geometry, persistence & networking, Python stack, build/config), and — the highest-value part —
carries a **verified live-vs-dead table** and a **known-bugs/footguns** list (every verdict re-checked
against source, not taken from a single read).

The "add consts" and "rename" sub-parts were split to their own tasks (`const-correctness-sweep.md`,
`rename-generic-variables.md`), which the new reference doc now grounds. The improvement list the goal
asked for lives in the reference doc's "Known bugs / footguns" + "Live vs dead" sections.

**Notable findings surfaced (see the reference doc for `file:line`):** Vulkan game renderer is empty
stubs (greenfield, not a partial port); online auth points at the dead upstream URL; `server.py` won't
import under Py3; and several small verified bugs (`client_sendall` length, `gen_sky_buffer`/
`gen_player_buffer` fall off the end on a non-GL build, `db_worker_start` header/def mismatch,
`vulkan_viewport` param-order mismatch, `map_set(w==0)` silent no-op).

Original goal + plan are preserved in git history; this is the lean archived record per convention.
