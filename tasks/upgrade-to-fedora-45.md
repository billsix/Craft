# Upgrade the container base from Fedora 44 to Fedora 45

**Status:** in progress
**Priority:** 3
**Difficulty:** 4

## BLUF
Bump `Dockerfile` to `fedora:45`; `make image` green nested. Craft is C (a
Minecraft-like, OpenGL) with several vendored source deps; the C compile under
GCC 16 is the risk. "Done" = image builds on 45.

## Context
Fleet-wide Fedora 44 → 45 bump (William Emerison Six <billsix@gmail.com>,
2026-11-05). Fedora 45: GCC 16.2 / clang LLVM 23, glibc 2.44. Watch for new
GCC-16 warnings-as-errors in Craft's C and its vendored deps (sqlite/glfw/etc.).

## Plan
1. `fedora:44` → `fedora:45`. 2. `make image` nested. 3. Fix toolchain breakage
minimally (discretion; log here). 4. Stage by path; archive.

## Notes / findings
(appended as the build runs)
