# Mipmapping: try variants headless, compare screenshots, decide interactively

**Status:** proposed — needs go-ahead (interactive — needs the maintainer present for visual feedback).
**Priority:** 4
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)

## Goal

The crisp-vs-smooth aesthetic decision that blocks `tasks/implement-mipmapping-texture-array.md` is far
easier to make by **looking** than in the abstract. So: **actually try mipmapping variants on Craft,
render before/after screenshots headless, show them to the maintainer, and iterate on feedback** until we
converge on a decision (keep crisp / a cheap mip mode is enough / pursue the clean texture-array path).

This is an experiment/spike whose *output is a decision* (plus possibly a small landed change). Background,
the two blockers, and the option ladder are in `tasks/reference/mipmapping-feasibility.md`.

## Feasibility — verified in-sandbox 2026-08-27 (the harness is real)

- Craft's Linux build deps are all present as **system** libs (pkg-config): `glfw3` 3.4.0, `sqlite3`
  3.51.2, `libcurl` 8.18.0 — i.e. `cmake -S. -Bbuild && cmake --build build` works here directly (no
  container; Craft isn't on the template).
- Headless render works: **Xvfb + Mesa `llvmpipe` report OpenGL 4.6**, so Craft's requested 4.6-core (or
  3.3 fallback) context should create under `DISPLAY=:99`. ImageMagick `import -window root` captures
  frames. (Verify the core-profile context actually creates under llvmpipe on first run — very likely
  fine; fall back to the 3.3 path if not.)

## The harness (build once, screenshot per variant)

- Build Craft in-sandbox (Debug so `RESOURCE_PATH` = source tree, shaders/textures found in place).
- Run headless: `Xvfb :99 -screen 0 1280x800x24 &` then `DISPLAY=:99 ./build/craft` (offline mode).
- **Deterministic viewpoint** (so shots are comparable across variants): mipmapping only shows at
  distance/grazing angles, so pre-seed a fixed world + player pose. Two clean options:
  1. Pre-populate the offline `craft.db` `positionAndOrientation` row with a known pose looking across
     terrain at distance, launch, capture the root window after a few frames.
  2. **Temporary dev-only hook** (preferred, cleaner): add a "render N frames at a fixed camera, `import`
     the framebuffer, exit" path under a throwaway `#ifdef SHOT` — *covered by the standing authorization
     for temporary dev-only build additions; revert at task end.* Save the harness under
     `tasks/adhoc/mipmapping-experiment/` if it's substantive.
- Capture **near** (crispness/detail) and **far** (shimmer/bleed) shots each variant; label them.

## Experiment ladder (build → screenshot → SHOW → ask → iterate)

Run these in order; after each, present the shots next to the baseline and ask the maintainer which
direction to go. Stop as soon as a decision is reached — don't grind through all of them mechanically.

1. **Baseline** — current `GL_NEAREST`, no mipmaps (`gl_render.c:287-288`). The reference shots.
2. **`GL_NEAREST_MIPMAP_NEAREST` + `glGenerateMipmap`** — nearest mip, nearest texel: adds distance LOD
   while keeping per-texel crispness (no in-level blur). Cheapest possible change. *(Expect: still some
   cross-tile bleed + color-key transparency artifacts at coarse mips — note them in the shot.)*
3. **`GL_LINEAR_MIPMAP_LINEAR`** (trilinear) — the smooth end: shows how much bleed/blur full filtering
   introduces on the packed atlas.
4. **Only if a smooth variant is wanted:** the clean fix — **texture array + real alpha (+ anisotropy)** —
   which removes bleed and fixes transparency. This is the bigger refactor and lives in
   `tasks/implement-mipmapping-texture-array.md`; this spike would hand off to it rather than doing it
   inline.

## Interactive loop (how to run it)

For each variant: make the one-line filter change (+ `glGenerateMipmap`), rebuild, capture near+far shots,
then **stop and show the maintainer the comparison and ask**: "crisper or smoother? does the bleed /
transparency artifact bother you here? pursue this direction, try the next rung, or stop?" Use the answer
to pick the next rung or conclude. (This is a `/loop`-friendly, feedback-driven task — the point is the
maintainer's eye, not a fixed script.)

## Outcomes (any of these ends the task)

- **Keep crisp** → `GL_NEAREST` stays; **drop** `implement-mipmapping-texture-array.md` (record why).
- **A cheap mip mode is good enough** (e.g. variant 2 acceptable) → land that small change directly
  (revert the SHOT hook first), update `mipmapping-feasibility.md`, done.
- **Pursue smooth properly** → the texture-array path: proceed with
  `implement-mipmapping-texture-array.md` (this spike resolves its aesthetic blocker).

## Cleanup

Revert any temporary `SHOT` hook / dev-only build changes (standing authorization: temporary dev
additions are removed by task end). Promote the harness to `tasks/adhoc/mipmapping-experiment/` if worth
keeping; otherwise remove. Leave only the chosen real change (if any) staged.

## Cross-links

- `tasks/reference/mipmapping-feasibility.md` — the analysis + option ladder.
- `tasks/implement-mipmapping-texture-array.md` — the clean-path implementation this spike may trigger (or
  drop); this task is how its "crisp vs smooth" blocker gets resolved.
