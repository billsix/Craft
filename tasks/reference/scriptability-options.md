# Craft — scriptability options survey

**Reference document** — a survey of ways to make Craft scriptable (embedded Lua / embedded Python /
protobuf-RPC / extending what's already there), with the trade-offs and a recommendation. Not a task;
update in place. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>) from a read of the
existing seams (commit `f59081f3`). Companion to `tasks/reference/architecture-overview.md`. The
implementation itself is a **follow-on task** to be spawned once a direction is picked.

## What already exists (verified) — you are not starting from zero

1. **The C build primitive — `builder_block(x,y,z,w)` (`main.c:1292`).** Destroys the destructable block
   at (x,y,z) then places `w`. **Every** shape command sits on it (`cube`/`sphere`/`cylinder`/`tree`/
   `array`, `main.c:1525-1680`), which sit on `set_block`/`get_block`. This is the natural API surface any
   scripting layer would call.
2. **The in-game `/command` console — `parse_command` (`main.c:1700-1781`).** A hardcoded `if/else
   sscanf` chain: `/cube /fcube /sphere /fsphere /circle[xyz] /cylinder /tree /array /copy /paste /view
   /online /offline /login /identity`. Unmatched input (with `forward`) is sent to the server as chat
   (`client_talk`). It's a fixed command set — **no loops, variables, or logic** — and a rigid chain (no
   dispatch table).
3. **An out-of-process scripting path ALREADY works — `builder.py`.** A client-side Python script whose
   `Client` class (`builder.py:164`) opens a TCP socket and speaks the wire protocol directly
   (`sendall('B,%d,%d,%d,%d\n')` = set block, `builder.py:183`; `A,…` auth). It ships geometry generators
   (sphere/circle/cylinder/cuboid/pyramid/bitmap) that compute coordinate sets and stream `B` commands.
   **So "external Python that places shapes over the network" is a solved pattern** — it just needs the
   Py3 fix (`tasks/revive-python-server.md`; `sendall(str)` → bytes) and a live server.
4. **The wire protocol itself** (single-letter line commands `V/A/P/C/B/L/S/T` in `client.c` /
   `server.py`) is a **language-agnostic** scripting surface — anything that speaks it can drive the
   world. `builder.py` proves it.
5. **No embedded interpreter exists** — grep-confirmed: no `lua_*`, no `Py_Initialize`, no `sol::`. Any
   *in-process* scripting is net-new.

**The key framing question this raises:** do you want **in-game scripting** (logic/loops/events running
*inside* the game — which nothing today provides), or **richer out-of-process control** (which `builder.py`
+ the protocol already start)? The four options below split along exactly that line.

## The options

### A. Extend the existing `/command` + wire protocol (lowest effort)
Add commands / protocol messages; keep driving from `builder.py`-style external clients.
- **Pros:** mostly already there; no new dependency; the protocol is simple and debuggable.
- **Cons:** not a real scripting language — no in-game loops/logic/events; the `sscanf` chain doesn't
  scale (and has no exhaustive default — see `tasks/rename-generic-variables.md`/dispatch-table note); the
  protocol is an untyped, unversioned block-placement API.
- **Good if:** you only want "place shapes / bulk edits from a script," and you're happy that the logic
  lives in the external client (Python/whatever) rather than in-game.

### B. Embedded Lua (in-process) — the classic game-scripting choice
Vendor `deps/lua` (or `sol2` for the C++ side) + a thin binding exposing `builder_block`/`set_block`/
`get_block`, player position/orientation, block-type constants (`item.h`), and optionally event hooks.
Run scripts from a new `/lua` console command and/or `.lua` files.
- **Pros:** tiny footprint (~250 KB), fast, trivially embeddable in C, **easy to sandbox**, purpose-built
  for exactly this. Adds real **in-game** logic/loops/events the current paths lack. Idiomatic (Minecraft
  mods, ComputerCraft, Garry's Mod, WoW all use Lua).
- **Cons:** a new dependency + a C↔Lua binding layer to write and maintain; another language in the tree.
- **Good if:** you want *in-game* scripting — build bots, procedural structures, event reactions — with
  minimal runtime weight. **Best fit for a small C game.**

### C. Embedded Python (in-process)
Embed CPython (`Py_Initialize` + a C extension module exposing the block API), reusing `builder.py`'s
shape library in-process.
- **Pros:** the maintainer already knows Python and the repo already has Python tooling; you could import
  the existing `builder.py` generators directly.
- **Cons:** **heavy** runtime (tens of MB, a full interpreter to ship and initialize), the GIL, harder to
  sandbox than Lua, and a real deployment/packaging burden for a small C game. In-process crashes/segfaults
  in native extensions take down the game.
- **Good if:** Python ergonomics matter more than footprint, and you accept the runtime weight. For most
  of what Craft needs, Lua gives the same "in-game scripting" win far more cheaply — and out-of-process
  Python is already covered by `builder.py`.

### D. Protobuf / gRPC RPC boundary (out-of-process, language-agnostic)
Replace/augment the ad-hoc single-letter protocol with a typed **protobuf** schema and a small service
layer; any language with protobuf bindings can drive the game.
- **Pros:** a clean, **typed, versioned, multi-language** API — the real answer to "language-agnostic";
  isolates scripts in their own process; good if you want bots/tools in many languages against a stable
  contract.
- **Cons:** the **heaviest** infra (proto compiler in the build, a generated service layer, a schema to
  design and version); overkill for solo/creative scripting; still out-of-process (no in-game logic).
  Duplicates what the current line protocol already does for the single-user case.
- **Good if:** the goal is a **stable public API** for third-party tools/bots across languages — a
  product-shaped goal, not a "let me script my world" goal.

## Cross-cutting axes

- **In-process (B, C) vs out-of-process (A, D):** in-process = direct game-state access, low latency,
  in-game logic/events — but a script bug can crash the game, so **sandboxing matters** (Lua is easy to
  sandbox; Python isn't). Out-of-process = isolation + multi-client + language freedom — but limited to
  what the protocol exposes, with network overhead, and no *in-game* logic.
- **API surface to expose (any option):** `builder_block`/`set_block`/`get_block`; player
  position/orientation; block-type constants (`item.h`); time-of-day; and — the high-value add — **event
  hooks** (`on_block_place`, `on_tick`, `on_player_move`) that no current path offers.
- **Safety:** only in-process embedding runs arbitrary code in the game's address space; if scripts might
  be shared, prefer Lua's sandbox (or run untrusted scripts out-of-process via A/D).

## Recommendation

**Primary: embed Lua (option B)** for *in-game* scripting, and **keep `builder.py` (option A)** as the
out-of-process path (fix it to Py3 first — `tasks/revive-python-server.md`). Rationale: Lua is the
smallest, fastest, most sandboxable way to add the thing nothing today provides — logic/loops/events
inside the game — and it fits a small C codebase without the weight of an embedded CPython or the infra
of protobuf. External-Python shape-scripting is *already* a solved pattern via `builder.py`, so C is
redundant for that, and D (protobuf) is only worth it if the goal becomes a stable multi-language public
API — defer it until that's an explicit goal.

**Suggested first implementation slice (for the follow-on task):** vendor Lua, add a `/lua <expr>`
console command + a `run_script(path)` that binds `set_block`/`get_block`/`builder_block`/player-pos/
block-types, and port two `builder.py` generators (sphere, cylinder) to Lua as proof. Add one event hook
(`on_block_place`) to demonstrate the in-game-logic win.

## Fun ideas (the maintainer asked for these)

- **Turtle/build-bot** scripting à la ComputerCraft — a scripted agent that walks and places blocks
  (`forward()/turn()/place()`), driving `builder_block`.
- **Procedural structures:** fractal/L-system trees, recursive mazes, Sierpinski/Menger sponges, arches
  and staircases from a few lines of Lua.
- **Cellular-automata terrain / Game-of-Life** running on a block layer via an `on_tick` hook.
- **Scripted art:** plot `f(x,z)` height-fields or parametric surfaces as blocks (a nice tie-in to the
  math theme in the maintainer's other projects).
- **A live REPL console** in-game (the `/lua` command) for immediate experimentation.
- **Event reactions:** "when I place a torch, spawn light"; "when I break glass, play a sound" — the
  hook-driven gameplay that embedding unlocks.

## Follow-on

Once a direction is confirmed, spawn an implementation task (e.g. `embed-lua-scripting.md`) with the
first-slice plan above. Until then this survey stands as the decision record.
