# Investigate whether mipmapping is possible for Craft's block atlas

**Status:** blocked
**Priority:** 6
**Difficulty:** 3
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (is the pixel-art look mandatory?).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Figure out if mipmapping is a possibility."*

## Context (investigation 2026-08-27)

- The block texture atlas uses `GL_NEAREST` for **both** min and mag filters (`gl_render.c:287-288`),
  `glTexImage2D(... GL_RGBA ...)` at `gl_render.c:161`, and there is **no `glGenerateMipmap`** call
  anywhere. This is the deliberate blocky/pixel-art look.
- Mipmapping itself is a small change (generate mipmaps after load + switch the min filter to
  `GL_NEAREST_MIPMAP_LINEAR` or add anisotropy). **The real obstacle is atlas bleed:** mip levels blend
  neighbouring atlas tiles, so mipmapping a texture *atlas* needs tile padding, or a texture *array*
  (one layer per block) instead of an atlas. That is the feasibility crux — hence the question below.

## Plan (draft — investigation)

- [ ] Confirm the aesthetic constraint (Q1).
- [ ] If mipmapping is wanted: evaluate atlas padding vs converting the atlas to a `GL_TEXTURE_2D_ARRAY`;
      estimate the change to `gl_render.c` texture setup + the shader sampler.
- [ ] Report feasibility + recommended approach (possibly to `tasks/reference/`). Cross-links the
      shader-enhancements task.

## Open questions

1. **Is the pixel-art / blocky aesthetic mandatory?** Naive mipmapping is infeasible on the current
   atlas (mip bleed across tiles). Is atlas **padding** or a **texture array** acceptable to enable it,
   or must the current `GL_NEAREST` crispness be preserved (in which case mipmapping is a no)?
