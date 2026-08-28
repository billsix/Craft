# Optional AZDO renderer: multi-draw-indirect over a shared chunk arena

**Status:** proposed — needs go-ahead. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>).
**Priority:** 6
**Difficulty:** 7

## Goal

Rungs 2-3 of `tasks/reference/opengl-draw-overhead-and-azdo.md` — the actual "zero-overhead OpenGL as an
optional build" the bullet asked about. Collapse the ~2,400 per-chunk draw calls/frame into ~one via
**multi-draw-indirect (MDI)** over a **shared vertex arena**, with **persistent-mapped** chunk streaming.

## Context (verified)

- Today: one `GL_STATIC_DRAW` VBO per chunk + one `glDrawArrays` per chunk (`gl_render_chunk:332-361`,
  `gl_gen_buffer:79-86`). Context is GL 4.6, so 4.3 (MDI) + 4.4 (persistent map) are available; shaders
  are `#version 330` and MDI needs no shader change (just the buffer/draw restructure + a `gl_DrawID`-
  indexed transform SSBO).
- Bindless textures are **not** needed (single block atlas).

## Plan (draft)

- [ ] **Prerequisite:** `tasks/hoist-per-chunk-vao.md` (rung 1) first — no reason to build MDI on the
      per-frame-VAO-churn path.
- [ ] **Profile first (gate):** measure frame/CPU-submit time at `render_radius` 16-24 on **real GPU
      hardware** (not llvmpipe — software GL won't show the win). Only proceed if draw-call/CPU overhead
      is actually the bottleneck.
- [ ] Rung 2: persistent-mapped (`GL_MAP_PERSISTENT_BIT|COHERENT`) streaming buffer + fencing for chunk
      mesh uploads (replaces per-chunk VBO orphaning).
- [ ] Rung 3: a shared vertex arena (one/few big buffers) with per-chunk allocation; build a per-frame
      indirect-command buffer (first-vertex + count per visible chunk) + a per-chunk transform SSBO
      indexed by `gl_DrawID`; issue `glMultiDrawArraysIndirect`.
- [ ] Wire as an **optional build** — a third renderer option (`ENABLE_OPENGL_AZDO_RENDERER`) paralleling
      the existing `gl_*`/`vulkan_*` abstraction (`CMakeLists.txt:19-34`), gated on a GL 4.3+ context;
      the 3.3 path stays the default.
- [ ] Verify parity (same visuals) + the measured win; `make sanitize` green.

## Open questions

1. **Is this worth pursuing at all?** It's a large refactor of chunk buffer management whose payoff only
   shows on real hardware at large render radii. *(Recommend: do rung 1 first, then profile; only build
   this if the profile justifies it — otherwise close as not-worth-it.)*
2. **Arena allocation strategy** — fixed-size chunk slots (simple, some waste) vs a free-list allocator
   (compact, complex)? Decide during implementation.
