# Craft — mipmapping feasibility for the block atlas

**Reference document** — is mipmapping the block texture possible, and at what cost? The findings and
options, so the design decision (which is the maintainer's aesthetic call) is well-informed. Not a task;
update in place. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>) from a direct read of the
texture path (commit `f59081f3`); all facts verified. Companion to `tasks/reference/architecture-overview.md`.

## Current state (verified)

- **Block atlas:** `textures/texture.png` = **256×256 RGBA**, a **16×16 grid of 16×16-pixel tiles**
  (`s = 0.0625 = 1/16` in UV; `cube.c:57,64`). Tile index → `(du,dv) = (tile%16, tile/16) * s`.
- **Filtering:** `GL_NEAREST` for **both** min and mag (`gl_render.c:287-288`) — crisp, deliberate
  pixel-art look. `sign.png` is the same; `font.png`/`sky.png` use `GL_LINEAR`.
- **Only mip level 0 is uploaded** — `gl_load_png_texture` calls `glTexImage2D(GL_TEXTURE_2D, 0, …)` once
  (`gl_render.c:161`) and there is **no `glGenerateMipmap`** anywhere. So even switching the min-filter to
  a mipmap mode would sample an **incomplete texture** (renders black) until a chain is generated.
- **An anti-bleed UV inset already exists:** `cube.c:57` uses `a = 1/2048`, `b = s − 1/2048` — a **1/8 of
  a texel** inset (256 × 1/2048 = 0.125 px) pulling sampled UVs inward from each tile edge. It reduces
  bleed **at the base level under NEAREST**; it does nothing at coarser mip levels (see below).
- **No anisotropic filtering** anywhere (grep-confirmed) — the usual companion to mipmaps for ground seen
  at grazing angles.

## Why naive mipmapping is NOT a drop-in — two independent blockers

Enabling mipmaps mechanically is ~2 lines (`glGenerateMipmap(GL_TEXTURE_2D)` after upload + set
`GL_TEXTURE_MIN_FILTER` to a `*_MIPMAP_*` mode). But on **this** atlas that produces artifacts, for two
distinct reasons:

1. **Atlas bleed across tile boundaries.** Each mip level averages 2×2 texel blocks. The 16px tiles are
   packed edge-to-edge, so from mip level 1 onward the averaging **straddles tile boundaries** — a grass
   tile's edge blends into the stone tile next to it, and the further away a block is, the more it bleeds.
   The existing 1/8-texel inset does **not** help: it only offsets the *sampled UV* at level 0; the
   hardware still builds each mip by averaging neighboring texels regardless of the inset. (At mip 4 the
   whole 256×256 atlas is 16×16 px — one texel per tile — fully cross-contaminated.)
2. **The color-key transparency breaks (the subtler, deeper blocker).** `block_fragment.glsl:26-33`
   decides transparency by **exact color equality**: a texel exactly `(1,0,1)` magenta → `discard`
   (transparent), exactly `(1,1,1)` white → treated as **cloud**. Mip generation **blends** colors, so at
   any level > 0 the magenta/white edge texels become intermediate colors that **no longer match** — so
   the edges of glass/leaves/plants stop discarding, and cloud detection degrades. Color-keying and
   mip-filtering are fundamentally incompatible; this must be reworked (real alpha channel) for *any*
   smooth filtering, independent of the atlas-bleed issue.

So "can we mipmap?" → **yes mechanically, but the naive version is broken by both bleed and the
color-key transparency.** The real question is which fix, and whether the crisp look should change at all.

## Options (increasing effort)

- **(a) Do nothing — keep `GL_NEAREST`, no mipmaps.** The deliberate pixel-art look; no bleed, no
  transparency breakage. The artifact mipmaps *fix* — distant texture shimmer/aliasing — is relatively
  mild for low-frequency blocky textures. **This is the aesthetic branch and it's the maintainer's call.**
- **(b) Atlas padding / gutters.** Add a border of duplicated edge texels around each tile so mip
  averaging stays within a tile's own colors. Standard fix — **but** with only 16×16-px tiles, gutters
  wide enough for more than ~1 mip level consume a large fraction of each tile; only 1–2 usable levels
  before tiles are too small. A partial stopgap, and it still needs the transparency rework (2).
- **(c) Texture array (`GL_TEXTURE_2D_ARRAY`) — the correct fix.** One layer per tile instead of an atlas.
  Mip levels are generated **per layer**, so there is **zero cross-tile bleed** (layers never blend). This
  is the standard modern voxel approach. Cost: convert the atlas → array, sampler → `sampler2DArray`, emit
  a **layer index per vertex** (instead of / alongside UV), rework `item.c`'s tile-index table into layer
  indices, and change the UV emission in `cube.c`. Moderate, clean refactor; also unlocks per-layer
  anisotropy.
- **(d) Replace color-key transparency with a real alpha channel** (required by (b) and (c) for smooth
  filtering). The PNG is already RGBA (`lodepng_decode32`), so alpha exists on the wire — the art/shader
  just need to use alpha instead of the magenta/white sentinels. Do this alongside whichever of (b)/(c).
- **(e) Anisotropic filtering** (`GL_TEXTURE_MAX_ANISOTROPY`). The higher-value sharpness win for ground
  at grazing angles; cheap to add once a mip chain exists; pairs with (c).

## Feasibility verdict & recommendation (the aesthetic decision stays the maintainer's)

- **If the crisp pixel-art look is mandatory:** **don't mipmap** (option a). Current `GL_NEAREST` is
  correct; the shimmer mipmaps would remove is minor for these textures.
- **If reducing distant shimmer/aliasing is the goal:** the only *clean* path is **(c) texture array +
  (d) real alpha**, optionally + **(e) anisotropy**. Atlas padding (b) is a stopgap that the tiny tiles
  make marginal.
- **Cheapest experiment to see the trade-off before committing:** temporarily add
  `glGenerateMipmap(GL_TEXTURE_2D)` + set min-filter to **`GL_NEAREST_MIPMAP_NEAREST`** (nearest mip,
  nearest texel — no in-level blur, so it preserves crispness while adding distance LOD) and look at the
  result. It still exhibits the cross-tile bleed and the color-key breakage at coarse levels, but it shows
  quickly whether distance-LOD is even wanted before investing in the array conversion.

**Bottom line:** mipmapping is *possible* but not a two-line change here — the atlas layout and the
color-keyed transparency both fight it. The worthwhile version is a texture-array + alpha rework; the
naive version is not worth shipping. Whether to pursue any of it depends on the crisp-vs-smooth aesthetic
call, which is deliberately left to the maintainer.

## Cross-links

- `tasks/reference/architecture-overview.md` — texture setup (`gl_render.c:281-310`), the "no mipmaps"
  note, the `cube.c` mesh/UV emission.
- `tasks/shader-graphical-enhancements.md` — anisotropy / filtering ideas live in the same GL texture path.
