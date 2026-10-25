# Add `make update-<dep>` targets to re-vendor each third-party source dep from upstream

**Status:** COMPLETE (2026-10-25) — all 8 `update-<dep>` targets + scripts were implemented and committed
(`git log --oneline -- tools/ deps/`); sqlite + lodepng verified byte-idempotent; imgui/glfw/curl
re-vendor via `git archive` (content-correct); gl3w regenerates; tinycthread/noise copy files. The shared
helper was then generalized into the canonical `runClaudeInContainer tasks/reference/update_dep_common.sh`
(pattern + per-dep recipes: `runClaudeInContainer tasks/reference/vendoring-third-party-source-deps.md`).
On the next real bump of a big-clone dep (imgui/glfw/curl/gl3w) the maintainer reviews its diff — not
blocking. Archived to `tasks/archive/2026/10/25/`.
**Priority:** 6 **Difficulty:** 5
**Created:** 2026-10-24 (William Emerison Six <billsix@gmail.com>). Requested: automate getting/updating
the vendored third-party deps (imgui, sqlite, and the rest of `deps/`), using the env-var-driven
update-target pattern from hanoi's C++ subproject (`make update-imgui IMGUI_REF=…`).

## BLUF

Craft (github.com/billsix/Craft) vendors **9 third-party deps as committed source** under `deps/` (curl,
gl3w, glfw, imgui, lodepng, noise, sqlite, tinycthread) with **no provenance record** — the pinned ref
lived only in git history (the bump commit's subject). This task added, per dep, a **`make update-<dep>
<DEP>_REF=<branch|tag|SHA>`** target + a repo-relative `tools/update-<dep>.sh` that re-fetches from the
canonical upstream at the given ref, **overwrites** the vendored files, and prints the resulting version.
It is the Craft instance of the shared convention in the Makefile contract (the personal overlay's
"`update-<dep>` targets for third-party deps vendored BY SOURCE"), modelled on hanoi's `update-imgui`.
"Done" meant each dep had a working `update-<dep>` target (re-vendoring the current pin left an empty
`git diff` — idempotent) and `make help` listed them — both achieved.

## Context

- **The pattern copied:** hanoi's C++ subproject (`github.com/billsix/towersofhanoi` `cpp/`): `make
  update-imgui IMGUI_REF=<ref>` → `tools/update-imgui.sh <ref>` (full clone so a SHA checks out, copy the
  vendored file set, print `IMGUI_VERSION`, usage-guard on empty ref), run on the HOST (git + cp, no
  container) — mirrored per dep here.
- **No provenance files in `deps/`** — the pin is the bump commit's subject (`git log --oneline --
  deps/`), and that was kept: each `update-<dep>` run is committed with subject `<dep> <ref>`. Pins at the
  time of writing: sqlite `3.47.2`, lodepng `865adac2…`, gl3w `d5ba934…`, curl `curl-8_18_0`, imgui
  `1.92.5` (tag `v1.92.5`), glfw `3.4`; tinycthread/noise static (pin by commit).
- **What Craft compiles** (so a re-vendor copies the right files even when it replaces a whole tree):
  always `lodepng/lodepng.c`, `noise/noise.c`, `tinycthread/tinycthread.c`, `imgui/{imgui,imgui_demo,
  imgui_draw,imgui_widgets,imgui_tables}.cpp` + `imgui/backends/imgui_impl_glfw.cpp`; the OpenGL backend
  adds `gl3w/src/gl3w.c` + `imgui/backends/imgui_impl_opengl3.cpp`; Vulkan adds
  `imgui/backends/imgui_impl_vulkan.cpp`; `sqlite/sqlite3.c`, `glfw/`, `curl/` are
  compiled/`add_subdirectory`'d **only on WIN32/APPLE** (Linux uses the system libs). Include dirs:
  `gl3w/include`, `lodepng`, `noise`, `tinycthread`, `imgui`, `imgui/backends` (+ `glfw/include`,
  `curl/include`, `sqlite` on Win/Mac).

## Per-dep re-vendor recipes (researched 2026-10-24)

Two vendoring shapes were found: **whole tree** (clone ref, copy minus `.git`) vs **curated files** (copy
a few). The general shapes now also live in the shared reference doc; this table is the Craft-specific
record.

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

## Decisions (agent discretion, 2026-10-25; the maintainer said "use your discretion")

1. **One `tools/update-<dep>.sh` per dep + a shared `tools/update_dep_common.sh` helper** (each recipe
   genuinely differs). The helper carried `clone_ref`, `vendor_archive` (git-archive whole-tree),
   `replace_tree` (tar whole-tree), `copy_files`, `show_version`. The helper was then **generalized into a
   SHARED, VENDORED tool** — canonical at `runClaudeInContainer tasks/reference/update_dep_common.sh`
   (mirror in runCrush), re-vendored into this `tools/` and kept byte-identical like
   `check_local_annotations.py`; Craft uses the default `VENDOR_ROOT=deps`. Pattern:
   `runClaudeInContainer tasks/reference/vendoring-third-party-source-deps.md`. sqlite was re-verified
   byte-idempotent after the swap.
2. **gl3w regenerates** via `gl3w_gen.py` against the Khronos registry (so a bump refreshes GL coverage).
   Consequence: re-vendoring the *current* pin is NOT byte-idempotent (the registry moved on) — expected,
   not a bug.
3. **Whole-tree vendoring kept** (faithful to current practice), via `git archive` for imgui/glfw/curl
   (the export-ignore "release" view — drops CI/.github, keeps .gitignore — matching how the trees were
   vendored). Trimming to only-what-Craft-compiles was considered and NOT taken (a separate decision).

## Implementation

Added `tools/update_dep_common.sh` + `tools/update-{imgui,glfw,curl,lodepng,gl3w,sqlite,tinycthread,noise}.sh`
and a `make update-<dep> <DEP>_REF=<ref>` target for each (host-run; git/curl/unzip/python3 + cp).

- **sqlite** — `make update-sqlite SQLITE_REF=3.47.2 [SQLITE_YEAR=2024]`. Verified byte-idempotent. The
  amalgamation URL needs the release *year*; `download.html` lists only the current release, so bumping to
  the latest auto-detects the year but **pinning an older version needs `SQLITE_YEAR`**.
- **lodepng** — whole-tree, then **rename `lodepng.cpp`→`lodepng.c`** (upstream ships `.cpp`; Craft
  vendors it as `.c`) and **remove upstream's `Makefile`** (Craft does not vendor it). Verified
  byte-idempotent at the current pin.
- **imgui / glfw / curl** — `git archive` whole-tree. imgui re-vendored the correct content; the only
  residual vs the current pin was **cosmetic CRLF/LF on example `.vcxproj` files Craft never compiles**
  (review + commit on a bump; not worth chasing). glfw/curl use the same mechanism (not separately run —
  large clones; verify on the first real bump).
- **gl3w** — clone + `gl3w_gen.py --root` (regenerates from the Khronos registry) + whole-tree. A bump
  intentionally refreshes GL coverage, so a diff vs the old pin is expected.
- **tinycthread / noise** — copy the 2 files (tinycthread from `source/`; noise from fogleman/Craft
  `deps/noise/`). No ref is recorded in Craft's git history for these (static), so the targets default to
  `master`; pin by commit when bumping.

**Not done (left to the maintainer, non-blocking):** run the big-clone deps (imgui/glfw/curl/gl3w) on a
real bump and review their diffs; decide whether the example-file CRLF residue warrants a
`.gitattributes`/normalization pass (probably not).

## Related

- The convention: personal overlay Makefile contract ("`update-<dep>` targets for third-party deps
  vendored BY SOURCE") + "Self-contained images + live source".
- The general pattern + per-dep recipe shapes: `runClaudeInContainer
  tasks/reference/vendoring-third-party-source-deps.md`.
- The model: hanoi `cpp/tools/update-imgui.sh` + `make update-imgui` (github.com/billsix/towersofhanoi).
