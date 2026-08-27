# Investigate a zero-overhead ("AZDO") OpenGL path as an optional build

**Status:** blocked
**Priority:** 7
**Difficulty:** 5
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open questions below (what "zero-overhead" means + success metric).
**Recheck:** the Open questions below are answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Figure out if zero overhead opengl is possible, as an optional build,
besides opengl33-core profile."*

Investigate whether a higher-performance "Approaching Zero Driver Overhead" (AZDO) OpenGL path is
feasible as an *optional* third renderer alongside the current GL-3.3-core path.

## Context (investigation 2026-08-27)

- The renderer is **already an optional-backend abstraction** — parallel free-function sets `gl_*`
  (`src/gl_render.h:26-73`) and `vulkan_*` (`src/vulkan_render.*`), one compiled in per CMake option
  (`CMakeLists.txt:19-34`). A third renderer option would plug into the same seam.
- Shaders are `#version 330 core`, but the **runtime context requested is GL 4.6 core** (forward-compat,
  `main.c:2380-2383`) with a 3.3-core fallback (`main.c:2387-2390`). So AZDO techniques (bindless
  textures, persistent-mapped buffers, multi-draw-indirect) that need GL 4.3+/4.6 are already available
  in the context.
- Buffer path to study: `gl_gen_buffer`/`gl_gen_faces` (`gl_render.h:36,38`), per-chunk `gl_render_chunk`
  (`gl_render.h:53`).

## Plan (draft — this is an investigation/reference-doc task, not a commit to implement)

- [ ] Define "zero-overhead" for this project (Q1) and the success metric (Q2).
- [ ] Survey which AZDO techniques fit Craft's chunk renderer (persistent-mapped/streamed VBOs, MDI,
      bindless atlas) and what each demands.
- [ ] Recommend: worth a third CMake renderer option, or fold into the existing GL path?
- [ ] Write findings to `tasks/reference/` (a "will this still be true later" study).

## Open questions

1. **Definition** — does "zero-overhead" mean AZDO/bindless *within* GL, or a separate near-driver
   backend? *(Recommend: AZDO-within-GL, since the 4.6 context already supports it.)*
2. **Success metric** — what result would make this worthwhile (e.g. a measured frame-time delta vs the
   current 3.3/4.6 path at a given draw distance)?
3. **"Optional build"** — a third CMake renderer option paralleling GL/Vulkan (`CMakeLists.txt:19-34`),
   or a compile flag on the existing GL renderer?
