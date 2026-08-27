# Graphics backends: Vulkan / Metal / D3D (investigate + implement)

**Status:** blocked
**Priority:** 7
**Difficulty:** 6
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (confirm the shape: one shared reference doc +
per-backend tasks, and whether to migrate the `TODO.md` roadmaps in).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Merges three maintainer bullets, verbatim: *"Add vulkan implementation, or at least investigate how it
would be done"*, *"Add metal implementation, or at least investigate..."*, *"Add d3d implementation, or
at least investigate..."*.

This is an **umbrella** task while the shape is decided; the recommendation is to split it (see below)
once confirmed, so it isn't built as one unmanageable combined effort.

## Context (investigation 2026-08-27)

- The renderer is a compile-time-selected abstraction: parallel free-function sets `gl_*`
  (`src/gl_render.h`) and `vulkan_*` (`src/vulkan_render.{c,h}`), chosen via CMake options
  (`CMakeLists.txt:19-34`). Any new backend plugs into this seam.
- **Vulkan is scaffolded at the build/signature level only — the game renderer is EMPTY STUBS**
  (verified 2026-08-27, see `tasks/reference/architecture-overview.md`): every function in
  `src/vulkan_render.c` is an empty body / `return 0` with zero real `vk*` calls — so a Vulkan backend is
  effectively **greenfield**, not a partial port. The signatures mirror the `gl_*` API (interface
  defined) and the build wiring exists (`CMakeLists.txt:28-34,162-171`, `GLFW_NO_API` at `main.c:2359`),
  and `src/gui-vulkan.cpp` has a **real** ImGui Vulkan device/swapchain bring-up reusable as reference
  (it draws only the ImGui overlay). There is a **Vulkan roadmap at `TODO.md:3-13`** and a **Metal
  roadmap at `TODO.md:15-25`**. **Metal and D3D have no code at all**; **D3D is not in `TODO.md`.**
- Platform constraints: Metal = macOS-only; D3D = Windows-only; Vulkan = cross-platform.

## Recommended shape (from triage — confirm via Q1)

- **One shared reference doc** — "adding a graphics backend": the `gl_*`/`vulkan_*` seam, what each API
  demands, platform constraints. Then **per-backend tasks** hanging off it:
  - **Vulkan** — implementation task (most ready); migrate `TODO.md:3-13` in.
  - **Metal** — investigation task; absorb `TODO.md:15-25`.
  - **D3D** — investigation task (greenfield).
- Do NOT collapse all three into one build effort — maturity + platform differences make that
  unmanageable.

## Open questions

1. **Confirm the shape** — one shared "add-a-graphics-backend" reference doc plus per-backend tasks
   (Vulkan implementation, Metal/D3D investigations), and should the existing `TODO.md` Vulkan/Metal
   roadmaps be **migrated into** those task docs (rather than left split across `TODO.md`)? Once
   confirmed, this umbrella splits into those docs.
