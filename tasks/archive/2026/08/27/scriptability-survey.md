# Survey ways to make Craft scriptable (Python / Lua / protobuf)

**Status:** DONE 2026-08-27 (William Emerison Six <billsix@gmail.com>) — deliverable produced.

## Outcome

Wrote **`tasks/reference/scriptability-options.md`** — a survey of the four options (extend the existing
`/command` + wire protocol; embedded Lua; embedded Python; protobuf/gRPC RPC), each with trade-offs, plus
the API surface to expose, safety/sandboxing notes, a recommendation, and fun ideas.

**Key finding:** an out-of-process scripting path **already exists** — `builder.py`'s `Client` speaks the
wire protocol directly to place blocks, on top of the C primitive `builder_block` (`main.c:1292`); the
in-game `/command` parser (`main.c:1700`) is a fixed `sscanf` chain with no logic/loops; no embedded
interpreter exists (grep-confirmed).

**Recommendation:** embed **Lua** for *in-game* scripting (smallest/fastest/sandboxable; adds the
logic/loops/events nothing currently provides), keep **`builder.py`** as the out-of-process path (after
its Py3 fix, `revive-python-server.md`), and defer **protobuf** unless a stable multi-language public API
becomes an explicit goal.

**Open question resolved:** "pick a direction, or is this the exploration?" — this task *was* the
exploration; it recommends Lua. **The implementation is a follow-on task** (`embed-lua-scripting.md`),
NOT yet created — it awaits the maintainer confirming the Lua direction before scaffolding.

Original goal + plan are in git history; this is the lean archived record per convention.
