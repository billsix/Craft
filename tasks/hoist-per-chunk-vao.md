# Hoist the per-chunk VAO to init time (remove per-frame VAO churn)

**Status:** proposed — needs go-ahead. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>).
**Priority:** 4
**Difficulty:** 2

## Goal

Rung 1 of `tasks/reference/opengl-draw-overhead-and-azdo.md` — the free overhead win, and the existing
in-code TODO. `gl_render_chunk` (`gl_render.c:332-361`) currently **`glGenVertexArrays` +
`glDeleteVertexArrays` every chunk every frame** (~2,400×/frame at render_radius 24), plus re-enabling and
re-pointing the 3 attribs each time. Create the VAO + attrib format **once** at init and keep only
bind-VBO + draw in the hot path.

## Context (verified)

- The hot function `gl_render_chunk:332-361` even documents this in its own TODO (`:333-338`): "make and
  initialize the VAO once at initialization time … only bind vao, bind vbo, set attrib pointers, draw."
- Uniforms/textures are already hoisted into `gl_setup_render_chunks:312-330`, so this is the remaining
  per-chunk churn. Vertex format: stride 10 floats (pos3 @0, normal3 @3, uv4-packed @6).
- No GL version bump needed — works on the 3.3 path.

## Plan

- [ ] Create one VAO in `gl_initiliaze_global_state`; set the attrib format there.
      Prefer separating format from buffer binding via `glVertexAttribFormat` + `glBindVertexBuffer`
      (GL 3.3+/4.3) so the format is set once and only the buffer binding changes per chunk; otherwise the
      classic single-VAO + per-chunk `glBindBuffer` + cached attrib setup.
- [ ] In `gl_render_chunk`: bind the VAO, bind `chunk->buffer`, `glDrawArrays(GL_TRIANGLES, 0,
      chunk->faces*6)` — drop the gen/delete and the enable/disable churn. Replace the magic `6` with a
      named constant (per the TODO).
- [ ] Rebuild (`make debug`) + `make sanitize` green; visual smoke check (the headless harness in
      `tasks/mipmapping-experiment-and-decide.md` can screenshot to confirm no regression).

## Notes

Pure overhead reduction, no behavior change — no open questions. The larger AZDO rungs (persistent-mapped
streaming, multi-draw-indirect) are `tasks/azdo-renderer-multidraw-indirect.md`.
