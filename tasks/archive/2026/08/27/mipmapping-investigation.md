# Investigate whether mipmapping is possible for Craft's block atlas

**Status:** DONE 2026-08-27 (William Emerison Six <billsix@gmail.com>) — investigation complete.

## Outcome

Wrote **`tasks/reference/mipmapping-feasibility.md`**. Verdict: mipmapping is *mechanically* ~2 lines but
the naive version is broken on this atlas by **two** independent blockers — (1) mip levels average across
the packed 16×16 tile boundaries (atlas bleed; the existing 1/8-texel inset in `cube.c:57` doesn't help at
mip levels), and (2) the fragment shader keys transparency/clouds on **exact** colors
(`block_fragment.glsl:26-33`), which mip blending destroys. The clean fix is a **texture array + real
alpha (+ anisotropy)**; atlas padding is a marginal stopgap given 16px tiles.

**Recommendation → follow-on task created:** `tasks/implement-mipmapping-texture-array.md` (blocked on the
crisp-vs-smooth aesthetic decision — if crisp is mandatory, that task is dropped and `GL_NEAREST` stays).

Facts (`texture.png` 256×256 / 16×16 tiles, `GL_NEAREST`, level-0-only upload, no anisotropy) and the full
option analysis are in the reference doc. Original goal + plan are in git history; lean archived record.
