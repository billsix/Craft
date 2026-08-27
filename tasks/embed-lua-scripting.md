# Embed Lua for in-game scripting

**Status:** proposed — needs go-ahead. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>).
**Priority:** 5
**Difficulty:** 6

## Goal

The **recommended implementation** from `tasks/reference/scriptability-options.md`: embed **Lua** as
Craft's in-game scripting layer — the smallest/fastest/most-sandboxable way to add the logic/loops/events
that nothing today provides. (Out-of-process Python scripting is already covered by `builder.py` once it's
Py3-fixed — `tasks/revive-python-server.md`; protobuf/gRPC is deferred unless a multi-language public API
becomes a goal.)

This is the follow-on to the archived survey
(`tasks/archive/2026/08/27/scriptability-survey.md`); the option comparison and rationale live in
`tasks/reference/scriptability-options.md`.

## Plan (first slice, from the survey)

- [ ] Vendor Lua under `deps/lua` (treat as upstream, like the other deps) + wire into `CMakeLists.txt`.
- [ ] Add a thin C binding exposing the block API: `set_block`/`get_block`/`builder_block` (`main.c:1292`),
      player position/orientation, block-type constants (`item.h`), time-of-day.
- [ ] Add a `/lua <expr>` console command (extend `parse_command`, `main.c:1700`) + a `run_script(path)`
      for `.lua` files.
- [ ] Port two `builder.py` generators (sphere, cylinder) to Lua as proof-of-parity.
- [ ] Add one **event hook** (`on_block_place`) to demonstrate the in-game-logic win embedding unlocks.
- [ ] Sandbox the Lua environment (restrict `os`/`io`) so shared scripts are safe.
- [ ] Build + run verification; `make sanitize` stays green (Lua is a new `deps/` TU — un-instrumented).

## Fun follow-ups (from the survey)

ComputerCraft-style turtle/build-bot; procedural structures (L-system trees, mazes, Menger sponge);
cellular-automata terrain via `on_tick`; plotting `f(x,z)` height-fields as blocks; an in-game `/lua`
REPL.

## Open questions

1. **Binding style** — hand-written C bindings (fits the C codebase), or `sol2` (C++ header-only, cleaner
   but pulls Lua binding into the C++/gui side)? *(Recommend: hand-written C bindings — keeps it in the C
   core; `sol2` only if the binding surface grows large.)*
2. **Scope of the first slice** — is the block API + `/lua` command + one event hook the right MVP, or do
   you want the turtle/agent abstraction in v1?
