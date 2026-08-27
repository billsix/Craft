# Port Craft's own source to C23

**Status:** blocked
**Priority:** 6
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open questions below (scope + intent + compiler support).
**Recheck:** the Open questions below are answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"port to C23"*.

Move Craft from C11 to C23.

## Context (investigation 2026-08-27)

- Current standard is **C11**, set at `CMakeLists.txt:114` (`set_property(TARGET craft PROPERTY
  C_STANDARD 11)`) and `:260` (smoke harness). C++ is C++17 (`CMakeLists.txt:5`). No C23 anywhere.
- Vendored deps (`deps/` — sqlite, lodepng, noise) are compiled into the target; per `CLAUDE.md`,
  `deps/` is treated as upstream — so a standard bump should be scoped to `src/`, not the deps.
- The sanitizer gate pins clang (`tasks/archive/2026/08/03/add-sanitizer-gate.md`); the Windows
  `buildRelease.bat`/MSVC path has weaker C23 support — a compiler-support question.

## Plan (draft)

- [ ] Decide scope (Q1) and whether to adopt C23 features or just compile clean under `-std=c23` (Q2).
- [ ] Flip `C_STANDARD` to 23 for the craft target (and smoke harness), leaving `deps/` untouched.
- [ ] Build clean under clang; check the MSVC/`buildRelease.bat` path (Q3).
- [ ] If adopting features: identify where C23 (`nullptr`, `constexpr`, `typeof`, `#embed`, attributes,
      `bool`/`static_assert` in `<stddef.h>`) improves `src/` clarity.

## Open questions

1. **Scope** — C23 for `src/` only (leaving vendored `deps/` at their standard), or the whole target?
   *(Recommend: `src/` only.)*
2. **Intent** — adopt C23 *features*, or merely compile clean under `-std=c23`?
3. **Compiler support** — which compilers must stay green? The sanitizer gate pins clang; the MSVC
   `buildRelease.bat` path has weaker C23 support — is dropping/relaxing MSVC acceptable?
