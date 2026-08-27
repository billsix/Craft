# Survey ways to make Craft scriptable (Python / Lua / protobuf)

**Status:** blocked
**Priority:** 7
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (pick a direction, or is this the exploration?).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Make scriptable via python or lua, or through protobuf for
language-agnostic. Come up with fun ideas."*

## Context (investigation 2026-08-27)

- A scripting seam already half-exists: `builder.py` (sphere/cylinder/cuboid helpers) and the in-game
  chat `/command` parser in `main.c`; the build primitives (`cube`/`sphere`/`cylinder`/`tree`) are
  exposed in both C and `builder.py` (`CLAUDE.md:90-92`).
- The line protocol (`client.c`, `server.py`) is another existing scripting surface — related to
  `revive-python-server.md`.
- Three named mechanisms are very different: embedded Lua/Python vs a protobuf RPC boundary.

## Plan (draft — survey/brainstorm, spawning an implementation task later)

- [ ] Survey options (embedded Lua via `lua`/`sol2`; embedded CPython; a protobuf/gRPC command boundary;
      extending the existing `/command` + line protocol), with effort + fun-factor.
- [ ] Recommend a direction; produce fun scripting ideas.
- [ ] Spin the chosen mechanism into a follow-on implementation task.

## Open questions

1. **Direction** — pick a mechanism up front (embedded Lua/Python vs protobuf RPC), or is this bullet
   explicitly the *exploration* of which to use? *(Recommend: treat this as a survey reference doc that
   recommends one and spawns an implementation task — the three options diverge too much to build blind.)*
