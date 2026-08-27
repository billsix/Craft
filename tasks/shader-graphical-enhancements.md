# Suggest fun shader / graphical enhancements

**Status:** blocked
**Priority:** 7
**Difficulty:** 2
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open questions below (research-only vs implement; GL-version target).
**Recheck:** the Open questions below are answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Suggest fun shader or graphical enhancements."*

Produce a menu of fun shader / visual enhancement ideas for Craft.

## Context (investigation 2026-08-27)

- 8 shaders, all `#version 330 core` (`shaders/`), e.g. `shaders/block_vertex.glsl:1`. Sky/fog/day-night
  already present (`block_fragment.glsl`, `sky_*`).
- The runtime GL context is **4.6 core** with a 3.3 fallback (`main.c:2380-2390`), so more advanced
  effects are available if we don't require the 3.3 fallback path.
- Cross-links: the zero-overhead-GL investigation (`zero-overhead-opengl-investigation.md`) and
  mipmapping (`mipmapping-investigation.md`) both touch the same GL render/texture path.

## Plan (draft — suggestion/brainstorm)

- [ ] Produce a ranked menu of ideas (e.g. ambient occlusion on block edges, smooth lighting, water
      reflection/refraction, better fog/atmospherics, shadow mapping, bloom, TAA, normal-mapped blocks),
      each with rough effort + whether it needs >GL 3.3.
- [ ] Note which stay within the 3.3 fallback vs assume 4.6.

## Open questions

1. **Research or commit?** Is this a *suggestion* task (produce a menu of ideas) or a commitment to
   implement some? *(Recommend: suggestion menu first; spin implementation into follow-on tasks.)*
2. **GL-version target** — should suggestions stay within GL 3.3 (keeps the fallback path working), or
   assume the 4.6 context?
