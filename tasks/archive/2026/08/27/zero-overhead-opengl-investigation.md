# Investigate a zero-overhead ("AZDO") OpenGL path as an optional build

**Status:** DONE 2026-08-27 (William Emerison Six <billsix@gmail.com>) — investigation complete.

## Outcome

Wrote **`tasks/reference/opengl-draw-overhead-and-azdo.md`**. Key verified finding: `gl_render_chunk`
(`gl_render.c:332-361`) **creates and destroys a VAO every chunk every frame** (~2,400×/frame at
render_radius 24) with one `glDrawArrays` per chunk — the driver overhead an AZDO path targets; the
uniforms/textures are already hoisted. Feasibility = a three-rung ladder: (1) hoist the per-chunk VAO to
init (free win, no version bump — the existing in-code TODO), (2) persistent-mapped chunk streaming (GL
4.4), (3) multi-draw-indirect over a shared arena (GL 4.3) collapsing ~2,400 draws → ~1. AZDO fits as a
third renderer option paralleling the existing `gl_*`/`vulkan_*` abstraction; the context is already 4.6.

**Recommendation → follow-on tasks created:**
- `tasks/hoist-per-chunk-vao.md` (rung 1 — do now, free overhead cut, `proposed`).
- `tasks/azdo-renderer-multidraw-indirect.md` (rungs 2-3 — the optional AZDO build; profile on real GPU
  first, `proposed`).

Full analysis + the "optional build" shape are in the reference doc. Original goal + plan are in git
history; lean archived record.
