# Implement mipmapping via a texture array + real alpha (+ anisotropy)

**Status:** blocked
**Priority:** 7
**Difficulty:** 5
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** the maintainer's aesthetic decision — **is the crisp pixel-art look mandatory, or do you
want to reduce distant texture shimmer/aliasing?** If crisp is mandatory, **drop this task** (keep
`GL_NEAREST`, no mipmaps). This task only exists if you want the smoother look. **The way to resolve this
is `tasks/mipmapping-experiment-and-decide.md`** — an interactive spike that renders variants headless and
gets your visual feedback; it either drops this task or hands off to it.
**Recheck:** the aesthetic decision is answered — most likely via `mipmapping-experiment-and-decide.md`
(maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

The **recommended implementation** from `tasks/reference/mipmapping-feasibility.md`: enable mipmapping the
clean way — convert the block atlas to a **`GL_TEXTURE_2D_ARRAY`** (one layer per tile, so mip levels
never bleed across tiles) and **replace the color-key transparency with a real alpha channel** (mip
blending destroys the exact-magenta/exact-white keying), optionally adding **anisotropic filtering**.

This is the follow-on to the archived investigation (`tasks/archive/2026/08/27/mipmapping-investigation.md`);
the full analysis, the two blockers (atlas bleed + color-key transparency), and the rejected options
(naive 2-line mipmap; atlas padding) live in the reference doc.

## Plan (draft — after the aesthetic decision)

- [ ] Convert `textures/texture.png` (256×256, 16×16 tiles) into a texture-array upload
      (`GL_TEXTURE_2D_ARRAY`, one 16×16 layer per tile) in `gl_render.c` (`gl_load_png_texture` /
      `gl_initiliaze_textures:281-310`); `glGenerateMipmap` per array.
- [ ] Change the block shader sampler to `sampler2DArray` and emit a **layer index per vertex** (rework
      `cube.c:55-80` UV emission + `item.c`'s tile-index table → layer indices).
- [ ] Replace the `block_fragment.glsl:26-33` exact-color discard (magenta/white) with **alpha-based**
      transparency (PNG is already RGBA via `lodepng_decode32`).
- [ ] Set min-filter to `GL_*_MIPMAP_LINEAR`; optionally add `GL_TEXTURE_MAX_ANISOTROPY`.
- [ ] Verify: build + run, confirm no cross-tile bleed at distance and correct glass/leaf/cloud
      transparency; `make sanitize` still green.

## Open questions

1. **The aesthetic decision (blocks everything):** keep crisp `GL_NEAREST` pixel-art (→ drop this task),
   or accept a smoother look at distance to remove shimmer (→ do the texture-array path)?
2. If proceeding: is dropping the magenta/white **color-key convention** for real alpha acceptable (it
   changes the texture-authoring workflow), and should this also cover `sign.png` (same `GL_NEAREST`)?
