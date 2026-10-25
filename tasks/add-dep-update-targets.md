# Add `make update-<dep>` targets to re-vendor each third-party source dep from upstream

**Status:** in progress — all 8 `update-<dep>` targets + scripts implemented; sqlite + lodepng verified
byte-idempotent; imgui/glfw/curl re-vendor via `git archive` (content-correct); gl3w regenerates;
tinycthread/noise copy-files. See Implementation below.
**Priority:** 6 **Difficulty:** 5
**Created:** 2026-10-24 (William Emerison Six <billsix@gmail.com>). Requested: automate getting/updating
the vendored third-party deps (imgui, sqlite, and the rest of `deps/`), using the env-var-driven
update-target pattern from hanoi's C++ subproject (`make update-imgui IMGUI_REF=…`).

## BLUF

Craft (github.com/billsix/Craft) vendors **9 third-party deps as committed source** under `deps/`
(curl, gl3w, glfw, imgui, lodepng, noise, sqlite, tinycthread) with **no provenance record** — the
pinned ref lives only in git history (the bump commit's subject). Add, per dep, a **`make update-<dep>
<DEP>_REF=<branch|tag|SHA>`** target + a repo-relative `tools/update-<dep>.sh` that re-fetches from the
canonical upstream at the given ref, **overwrites** the vendored files, and prints the resulting
version. This is the Craft instance of the shared convention now in the Makefile contract (the personal
overlay's "`update-<dep>` targets for third-party deps vendored BY SOURCE"), modelled on hanoi's
`update-imgui`. "Done" = each dep has a working `update-<dep>` target (re-vendoring the current pin
leaves an empty `git diff` — idempotent), and `make help` lists them.

## Context (read first)

- **The pattern to copy:** hanoi's C++ subproject (`github.com/billsix/towersofhanoi` `cpp/`):
  `make update-imgui IMGUI_REF=<ref>` → `tools/update-imgui.sh <ref>` (full clone so a SHA checks out,
  copy the vendored file set, print `IMGUI_VERSION`, usage-guard on empty ref). Run on the HOST (git +
  cp, no container). Mirror that per dep here.
- **No provenance files in `deps/`** — the pin is the bump commit's subject (`git log --oneline --
  deps/`). Keep that: each `update-<dep>` run should be committed with subject `<dep> <ref>` (the
  existing convention). Current pins: sqlite `3.47.2`, lodepng `865adac2…`, gl3w `d5ba934…`, curl
  `curl-8_18_0`, imgui `1.92.5` (tag `v1.92.5`), glfw `3.4`; tinycthread/noise are static (pin by commit).
- **What Craft actually compiles** (so the re-vendor copies the right files, even when it replaces a
  whole tree): always `lodepng/lodepng.c`, `noise/noise.c`, `tinycthread/tinycthread.c`, `imgui/{imgui,
  imgui_demo,imgui_draw,imgui_widgets,imgui_tables}.cpp` + `imgui/backends/imgui_impl_glfw.cpp`; OpenGL
  backend adds `gl3w/src/gl3w.c` + `imgui/backends/imgui_impl_opengl3.cpp`; Vulkan adds
  `imgui/backends/imgui_impl_vulkan.cpp`; `sqlite/sqlite3.c`, `glfw/`, `curl/` are compiled/`add_subdirectory`'d
  **only on WIN32/APPLE** (Linux uses the system libs). Include dirs: `gl3w/include`, `lodepng`, `noise`,
  `tinycthread`, `imgui`, `imgui/backends` (+ `glfw/include`, `curl/include`, `sqlite` on Win/Mac).

## Per-dep re-vendor recipes (from research, 2026-10-24)

Two vendoring shapes: **whole tree** (clone ref, copy minus `.git`) vs **curated files** (copy a few).

| dep | upstream | pin by | fetch + copy | version readout |
|---|---|---|---|---|
| **imgui** | github.com/ocornut/imgui | tag `vX.Y.Z` | clone ref → copy whole tree (min: 5 core `.cpp` + root `.h` + `backends/imgui_impl_{glfw,opengl3,vulkan}.*`) | `imgui/imgui.h`: `IMGUI_VERSION` / `IMGUI_VERSION_NUM` |
| **sqlite** | sqlite.org (amalgamation, NOT git) | version `X.Y.Z` | download `https://sqlite.org/<year>/sqlite-amalgamation-<3+MM+PP+00>.zip` (e.g. 3.47.2 → `3470200`; **year from download.html**, not current year) → unzip → copy `sqlite3.c sqlite3.h sqlite3ext.h shell.c` | `sqlite/sqlite3.h`: `SQLITE_VERSION` / `SQLITE_VERSION_NUMBER` |
| **glfw** | github.com/glfw/glfw | tag `X.Y` | clone ref → copy whole tree minus `.git` (keep CMake files — `add_subdirectory` needs them) | `glfw/include/GLFW/glfw3.h`: `GLFW_VERSION_MAJOR/MINOR/REVISION` |
| **curl** | github.com/curl/curl | git tag `curl-X_Y_Z` | **git clone** (the vendored copy is a git checkout w/ git-only files, `-DEV` version) → checkout tag → copy whole tree minus `.git` | `curl/include/curl/curlver.h`: `LIBCURL_VERSION` + `_MAJOR/_MINOR/_PATCH` (note `-DEV`; prefer the macros + tag) |
| **lodepng** | github.com/lvandeve/lodepng | commit | clone → checkout commit → copy whole tree (min: `lodepng.c` + `lodepng.h`) | `lodepng/lodepng.c`: `LODEPNG_VERSION_STRING` (date `YYYYMMDD`) |
| **gl3w** | github.com/skaslev/gl3w | commit | clone ref → run `python3 gl3w_gen.py --root <checkout>` (regenerates `include/`+`src/` from the Khronos GL registry — needs network) → copy whole tree minus `.git` | no macro — pin by commit; GL coverage = `gl3w/include/GL/glcorearb.h` Khronos year |
| **tinycthread** | github.com/tinycthread/tinycthread | commit/tag | clone ref → copy `source/tinycthread.c` + `source/tinycthread.h` | `tinycthread/tinycthread.h`: `TINYCTHREAD_VERSION_MAJOR/MINOR` (static 1.1 — also pin by commit) |
| **noise** | caseman/noise (MIT) or fogleman/Craft `deps/noise/` | commit | clone source → copy `noise.c` + `noise.h` | no macro — pin by source commit |

## Plan

1. A `tools/update-<dep>.sh` per dep (repo-relative, host-runnable), each implementing its recipe above:
   usage-guard on an empty ref, fetch, copy the right file set, print the before/after version. Share a
   small common helper if it reduces duplication (optional).
2. A `make update-<dep> <DEP>_REF=<ref>` target per dep in the Makefile, `## `-documented, calling the
   script; each guards its own `*_REF` variable. (Keep the existing `update-emacs-packages` as the
   sibling precedent.)
3. **Verify idempotent:** for each dep, run `update-<dep>` with the CURRENT pin → `git diff deps/<dep>`
   empty (proves the fetch + copy are correct and overwrite-clean). gl3w's registry regen + sqlite's
   amalgamation are the ones to watch.
4. Commit each bump as `<dep> <ref>` (matching the git-history provenance convention).

## Decisions (my discretion, 2026-10-25, maintainer said "use your discretion")

1. **One `tools/update-<dep>.sh` per dep + a shared `tools/update_dep_common.sh` helper** (each recipe
   genuinely differs). The helper has `clone_ref`, `vendor_archive` (git-archive whole-tree),
   `replace_tree` (tar whole-tree), `copy_files`, `show_version`. **(2026-10-25) The helper was
   generalized into a SHARED, VENDORED tool** — canonical at `runClaudeInContainer
   tasks/reference/update_dep_common.sh` (mirror in runCrush), re-vendored into this `tools/` and kept
   byte-identical like `check_local_annotations.py`; Craft uses the default `VENDOR_ROOT=deps`. Pattern:
   `runClaudeInContainer tasks/reference/vendoring-third-party-source-deps.md`. sqlite re-verified
   byte-idempotent after the swap.
2. **gl3w regenerates** via `gl3w_gen.py` against the Khronos registry (so a bump refreshes GL coverage).
   Consequence: re-vendoring the *current* pin is NOT byte-idempotent (the registry moved on) — expected,
   not a bug.
3. **Whole-tree vendoring kept** (faithful to current practice), via `git archive` for imgui/glfw/curl
   (the export-ignore "release" view — drops CI/.github, keeps .gitignore — which matches how the trees
   were vendored). Trimming to only-what-Craft-compiles is a separate decision, not taken.

## Implementation (2026-10-25)

Added `tools/update_dep_common.sh` + `tools/update-{imgui,glfw,curl,lodepng,gl3w,sqlite,tinycthread,noise}.sh`
and a `make update-<dep> <DEP>_REF=<ref>` target for each (host-run; git/curl/unzip/python3 + cp).

- **sqlite** — `make update-sqlite SQLITE_REF=3.47.2 [SQLITE_YEAR=2024]`. **Verified byte-idempotent.**
  The amalgamation URL needs the release *year*; `download.html` only lists the current release, so
  **bumping to the latest auto-detects the year, but pinning an older version needs `SQLITE_YEAR`.**
- **lodepng** — whole-tree, **then rename `lodepng.cpp`→`lodepng.c`** (upstream ships `.cpp`; Craft
  vendors it as `.c`) **and remove upstream's `Makefile`** (Craft doesn't vendor it). **Verified
  byte-idempotent** at the current pin.
- **imgui / glfw / curl** — `git archive` whole-tree. imgui re-vendors the correct content; the only
  residual vs the current pin is **cosmetic CRLF/LF on example `.vcxproj` files Craft never compiles**
  (review + commit on a bump; not worth chasing). glfw/curl use the same mechanism (not separately run —
  large clones; verify on first real bump).
- **gl3w** — clone + `gl3w_gen.py --root` (regenerates from the Khronos registry) + whole-tree. A bump
  intentionally refreshes GL coverage, so expect a diff vs the old pin.
- **tinycthread / noise** — copy the 2 files (tinycthread from `source/`; noise from fogleman/Craft
  `deps/noise/`). No ref is recorded in Craft's git history for these (static), so the targets default to
  `master`; pin by commit when bumping.

**Remaining:** run the big-clone deps (imgui/glfw/curl/gl3w) on a real bump + review their diffs; decide
whether the example-file CRLF residue is worth a `.gitattributes`/normalization pass (probably not).

## Related

- The convention: personal overlay Makefile contract ("`update-<dep>` targets for third-party deps
  vendored BY SOURCE") + "Self-contained images + live source".
- The worked example: hanoi `cpp/tools/update-imgui.sh` + `make update-imgui`
  (github.com/billsix/towersofhanoi).
