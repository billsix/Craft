# Retire CMake in favor of Meson (decision: do we drop Windows/macOS?)

**Status:** archived decision record — **DECIDED 2026-10-17: keep both builds in parallel (option 1).**
The maintainer (William Emerison Six <billsix@gmail.com>) chose to keep CMake as the cross-platform
build for Windows/macOS alongside the Linux-only Meson dev build. **No teardown.** Archived as the
decision record + the ready analysis; **revive this doc** (move it back to `tasks/`) only if Craft goes
Linux-only or someone needs Win/Mac off CMake — the option 2/3 analysis and teardown checklist below
are ready to use then.
**Priority:** 9 (parked/decided)
**Difficulty:** 6 (the teardown is easy; a cross-platform Meson build is the hard part)
**Revisit when:** Craft decides to drop Windows/macOS support, OR a cross-platform Meson build is
wanted (option 2 below) — then this becomes actionable.

## BLUF

The Meson build added 2026-10-17 (`tasks/archive/2026/10/17/add-container-template.md`) is
**Linux-only** — it uses system `glfw3`/`libcurl`/`sqlite3` via pkg-config and compiles the
vendored `deps/` from in-tree source, with no Windows/macOS path. `CMakeLists.txt` +
`src/config.h.in` + `buildDebug.bat`/`buildRelease.bat` are what currently build Craft on
Windows and macOS. **So "retire CMake" cannot happen without either (a) dropping Win/Mac
support, or (b) first making the Meson build cross-platform.** This task exists to get that
decision from the maintainer; until then CMake stays and the two builds coexist. Done = the
maintainer has chosen a path (and, if teardown, CMake/`.bat`/`config.h.in` removed with
Win/Mac either ported or explicitly dropped).

## Context (cold-start)

- Both builds are live and produce the same OpenGL client:
  - **Meson** (`meson.build`, `meson_options.txt`, `src/config.h.meson.in`) — Linux dev path,
    built in the Fedora/Podman container (`make image`/`build`). `#mesondefine` config header;
    `gnu11`/`gnu++17`; system deps on Linux; vendored `deps/` compiled in.
  - **CMake** (`CMakeLists.txt`, `src/config.h.in` with `#cmakedefine`, the `.bat` files) —
    cross-platform. On Win/Mac the vendored `deps/glfw` + `deps/curl` are full CMake
    subprojects (that is why they are vendored); on Linux it uses system libs. `make
    debug`/`release`/`sanitize` wrap it (see the `Makefile`).
- The Meson port deliberately scoped to the Linux client first and **noted Win/Mac as
  deferred** (see the archived `add-container-template.md`, "Meson build port" section).

## The decision

1. **Keep both (status quo). ← CHOSEN 2026-10-17.** CMake = cross-platform, Meson = Linux dev.
   Cost: two build files to keep in sync (two config-header templates, two option lists).
   **This is the current state and the decided path.**
2. **Make Meson cross-platform, then retire CMake.** Add Meson subproject wraps (or system
   deps) for glfw/curl/sqlite on Win/Mac, port the `WIN32`/`APPLE` branching to Meson
   `host_machine` logic, replace the `.bat`/MSVC multi-config story with a documented Meson
   invocation. Only then remove `CMakeLists.txt` + `src/config.h.in` + the `.bat` files. This
   is the real work; difficulty is here, not in the teardown.
3. **Drop Win/Mac, retire CMake now.** If Craft is Linux-only going forward, delete
   `CMakeLists.txt` + `src/config.h.in` + `buildDebug.bat`/`buildRelease.bat` + the host CMake
   Makefile targets (`debug`/`release`/`sanitize`) and the CLAUDE.md/README Win/Mac text. Fast,
   but irreversible-ish (loses the only Win/Mac build).

**The question that decodes to a one-word-ish answer:** (1) keep both, (2) port Meson to
Win/Mac first, or (3) drop Win/Mac and delete CMake now?

## If option 2 or 3 is chosen — teardown checklist (do NOT run yet)

- Remove `CMakeLists.txt`, `src/config.h.in`, `buildDebug.bat`, `buildRelease.bat`.
- Remove the host CMake targets from the `Makefile` (`debug`/`release`/`sanitize`/`clean`),
  or repoint `sanitize` at the Meson `sanitizer_gate` option.
- Update `CLAUDE.md` (drop the "CMake stays" text + the CMake rows), `README`, and any
  reference docs that mention the CMake options table.
- Confirm the Meson `sanitizer_gate` option fully replaces `make sanitize` (it was ported).
- (Option 2 only) land the cross-platform Meson work + verify a Win/Mac build first.

## Related

- `tasks/archive/2026/10/17/add-container-template.md` — added the Meson build + container.
- `tasks/modernize-c-like-gltron.md` — the C-source modernization + warning/lint gate; keep the
  gate build-system-agnostic so this decision doesn't block it.
