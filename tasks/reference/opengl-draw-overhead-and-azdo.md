# Craft — OpenGL draw overhead & the AZDO ("zero-overhead") ladder

**Reference document** — what the chunk render path costs today, and whether a lower-overhead ("zero
driver overhead"/AZDO) GL path is feasible as an optional build. Not a task; update in place. Created
2026-08-27 (William Emerison Six <billsix@gmail.com>) from a direct read of the GL renderer (commit
`f59081f3`); all facts verified. Companion to `tasks/reference/architecture-overview.md`.

## Current chunk draw path (verified)

- **Uniforms/textures are hoisted (good):** `gl_setup_render_chunks` (`gl_render.c:312-330`) binds the
  block program, sets all uniforms, and binds the block + sky textures **once** before the chunk loop.
- **Per chunk, per frame, `gl_render_chunk` (`gl_render.c:332-361`) does far too much:** it
  **`glGenVertexArrays` + `glBindVertexArray`**, binds the chunk VBO, enables 3 attrib arrays, sets 3
  `glVertexAttribPointer`s (stride 10 floats: pos3/normal3/uv4-packed), `glDrawArrays(GL_TRIANGLES, 0,
  chunk->faces*6)`, then disables the arrays, unbinds, and **`glDeleteVertexArrays`**. The function's own
  TODO (`:333-338`) says the VAO should be made once at init and only bind+draw should happen here.
- **One draw call per chunk.** Chunks are drawn in a loop (`render_chunks`, `main.c`). At
  `render_radius` up to 24, the visible set is ~(2·24+1)² ≈ **2,400 chunks → ~2,400 draw calls/frame**,
  each preceded by a VAO create + 6 state calls and followed by a VAO destroy. This is the driver
  overhead an AZDO path targets.
- **Buffers:** one `GL_STATIC_DRAW` VBO per chunk (`gl_gen_buffer`, `gl_render.c:79-86`), regenerated when
  a chunk re-meshes. Context is **GL 4.6 core** (requested at `main.c:2380`, 3.3 fallback), so 4.x AZDO
  features are available; shaders are `#version 330`.

## Is a zero-overhead path feasible? Yes — a three-rung ladder (increasing effort)

### Rung 1 — hoist the per-chunk VAO to init (free win, NO version bump)
The existing TODO. Create one VAO (and set the attrib format) **once** in `gl_initiliaze_global_state`;
per chunk just bind the VAO, bind the chunk VBO, and `glDrawArrays`. Removes `glGenVertexArrays` +
`glDeleteVertexArrays` + the enable/pointer/disable churn from **every chunk every frame** — pure
overhead reduction, no downside, works on GL 3.3. **This should be done regardless of any AZDO decision.**
(Even better on 3.3+: separate the vertex *format* from the buffer binding via `glVertexAttribFormat` +
`glBindVertexBuffer`, so the format is set once and only the buffer binding changes per chunk.)

### Rung 2 — persistent-mapped streaming for chunk uploads (GL 4.4)
Replace per-chunk `GL_STATIC_DRAW` re-uploads with a persistently-mapped buffer
(`GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT`) + fencing, so re-meshed chunks stream in without
allocating/orphaning a VBO each time. Reduces upload stalls when the world scrolls. Moderate.

### Rung 3 — multi-draw-indirect: ~2,400 draws → 1 (GL 4.3) — the real AZDO payoff
Put all visible chunks' vertex data in a **shared vertex arena** (one big buffer, or a few arenas) and
issue a single `glMultiDrawArraysIndirect` over a per-chunk indirect-command buffer (first-vertex +
count per chunk), with per-chunk transforms fed via an SSBO indexed by `gl_DrawID`. Collapses the
per-chunk draw-call + state overhead into one call. **This is the "zero-overhead" the bullet asks about
— and the biggest refactor:** it replaces the one-VBO-per-chunk model with arena allocation and touches
`gl_gen_buffer`/`gl_render_chunk`/chunk lifecycle. Bindless textures are **not** needed (single atlas).

## "Optional build besides opengl33-core" — how it fits

AZDO (rungs 2-3) needs GL 4.3+. The clean shape is a **third renderer option** paralleling the existing
`gl_*`/`vulkan_*` abstraction (`CMakeLists.txt:19-34`) — e.g. `ENABLE_OPENGL_AZDO_RENDERER` gated on a
4.3+ context — reusing the same shaders (MDI needs no shader change, just the buffer/draw restructure).
Rung 1 is not a separate build at all; it just improves the existing 3.3 path.

## Recommendation

- **Do rung 1 now** (`tasks/hoist-per-chunk-vao.md`) — it's a free, unambiguous overhead cut and the
  existing TODO; no version bump, no new build.
- **Treat rungs 2-3 as the optional AZDO renderer** (`tasks/azdo-renderer-multidraw-indirect.md`) — worth
  it only if profiling shows draw-call/CPU overhead is the bottleneck at large render radii (measure
  first; on llvmpipe/software GL the win won't show — needs real GPU). MDI over a shared arena is the
  high-value rung; persistent-mapped streaming pairs with it.
- **Success metric to set before rung 3:** frame-time (or CPU submit time) at `render_radius` 16-24, GL
  path vs AZDO path, on real hardware. Without a measured win, rung 1 alone is likely enough.

## Cross-links

- `tasks/reference/architecture-overview.md` — the renderer seam + `gl_render_chunk` VAO-churn note.
- `tasks/hoist-per-chunk-vao.md` (rung 1), `tasks/azdo-renderer-multidraw-indirect.md` (rungs 2-3).
