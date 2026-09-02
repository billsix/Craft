# Rename Craft's generic/terse variables to meaningful names

**Status:** DONE 2026-09-02 (William Emerison Six <billsix@gmail.com>). "Done" = renamed the
safely-verifiable identifiers (gate-checked), documented the pervasive upstream `p/q/x/y/z/w`
convention rather than renaming it, and left `main.c`'s bulk `p/q` rename for a separate
game-runnable pass — spun out to `tasks/rename-main-c-chunk-coords.md`. Durable vocabulary
harvested to `tasks/reference/coordinate-and-chunk-vocabulary.md`.
**Priority:** 7
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Progress:** 2026-09-02 (Claude Opus 4.8, on the maintainer's granted discretion)

## Goal

Maintainer's idea, verbatim: *"Rename variables. Lots of them have generic names, I didn't write the
original source code, and I don't know what a lot of them mean."* Rename terse/generic identifiers in
Craft's own source to meaningful names.

## Context (investigation 2026-08-27; orientation completed 2026-09-02)

- Dense terse names cluster in `cube.c` (mesh generation), `matrix.c`, and `main.c` block/chunk math.
- `.clang-format` has `SortIncludes: Never`, so renames won't fight formatting.
- **Scope to `src/` only** — never rename inside vendored `deps/`.
- **Note:** the standalone `source-study-and-orientation.md` prerequisite was never actually created
  as a file; the orientation now lives in `tasks/reference/architecture-overview.md`, which is
  sufficient to drive the rename. The 2026-09-02 pass read all of `src/` (GL-free core + fix sites)
  before touching anything.

## Decision (resolves the Open question — William Emerison Six <billsix@gmail.com>, 2026-09-02)

Granted discretion. The governing rule for this pass:

- Rename only **terse/generic** identifiers in `src/`; never `deps/`.
- **Preserve** any identifier that matches a recognizable **upstream (Fogleman Craft)** or **textbook**
  convention.
- Go **per-subsystem**, small reviewable commits, build + sanitizer green after each.
- Where a name's meaning is genuinely unclear, **leave it and note it** — do not guess.

## What that meant in practice

The pervasive Craft coordinate vocabulary is a **preserved upstream convention** and was NOT renamed
(it is used across every file, so renaming would be enormous blast radius and would fight the
convention):

- `p, q` = chunk column coordinates
- `x, y, z` = block/world coordinates
- `w` = block type ("what")

## Progress log — 2026-09-02 (done, verified)

Each committed separately; after each, `make debug` compiled clean and `make sanitize` (ASan + UBSan)
stayed green. Renames are behaviour-preserving; the GL-free core files below are all exercised by
`tests/smoke.c`, and the geometry paths' anti-optimization sink was unchanged, confirming
byte-identical output.

- **`matrix.c` — `mat_frustum` temporaries.** `temp/temp2/temp3/temp4` → `two_near` (2·znear),
  `width` (right−left), `height` (top−bottom), `depth` (zfar−znear). Function-local.
- **`world.c` — `create_world` terrain locals.** `flag`→`boundary_sign`, `f`→`base_noise`,
  `g`→`amplitude_noise`, `mh`→`height_scale`, `h`→`terrain_height`, `t`→`sea_level`, inner
  `w`→`flower`, `ok`→`place_tree`, `d`→`dist_sq`. Local to one function; expressions unchanged.
- **`cube.c` / `cube.h` — mesh-gen scale/glyph params.** `n`→`size` (cube/plant/wireframe/char-3d
  half-extent scale), `n,m`→`half_width,half_height` (`make_character`), `w = c - 32`→`glyph` (font
  atlas glyph index; the old `w` shadowed the block-type convention). Parameters are positional, so no
  call-site changes. Recognizable mesh-gen conventions were left as-is: `du/dv`, `uvs`, `normals`,
  `positions`, `indices`, the half-texel insets `a`/`b`, the tile size `s`.

## Deliberately left as-is (with reasons)

- **`ring.c`, `sign.c`, `item.c` / `item.h`, `util.c`** — already cleanly named (this fork has done
  prior cleanup); nothing generic worth renaming.
- **`map.c` / `map.h` `dx/dy/dz`** (the map's origin offsets). The maintainer's own `map.h` TODO flags
  these as not-definitively-understood. Derived meaning (from `map_set`/`map_grow`/world usage): they
  are the map's world-space origin subtracted to convert world→local storage coords (for chunk (p,q),
  `dx = p·CHUNK_SIZE`, etc.). Confident, but **left per the "leave what the maintainer flagged
  uncertain" rule** — a struct-field rename the maintainer wants to bless first, touching map.c/h and
  callers. Candidate future rename: `origin_x/origin_y/origin_z`.
- **`matrix.c` math** — `mat_*` names, `left/right/bottom/top`, `fov`, `aspect`, `znear/zfar`,
  `dx/dy/dz`, loop `i/j/c/r`, `a/b` working matrices: standard graphics-math/textbook conventions,
  preserved.
- **`cube.c` `_make_sphere` `r`** (radius): conventional, left.
- **`db.c`** — well-named sqlite boilerplate (`*_stmt` handles, `rc`, `e`); no rename (its A2 bug fix
  is tracked in `fix-verified-bugs.md`, not here).
- **`main.c` — DEFERRED.** ~2882 lines of GL + threading code with **no headless verification** (only
  the compiler checks it; there is no behavioural test), the largest blast radius, and the highest
  regression risk without being able to run the game. Per "leave what you can't verify / when in
  doubt leave it," this pass did not touch it. The maintainer flagged block/chunk math here as terse;
  it should be a separate, carefully-reviewed pass (ideally with the ability to run the game to
  confirm behaviour). Much of what looks terse there is the preserved `p/q/x/y/z/w` convention anyway.

## Chunk / coordinate vocabulary — DECODED (2026-09-02, pass 2)

> **The decoded vocabulary is canonical in
> [`tasks/reference/coordinate-and-chunk-vocabulary.md`](../../../../reference/coordinate-and-chunk-vocabulary.md).**
> Read that for the living version. The table below is this (now-archived) pass's historical work
> record — kept as-is; the reference doc is the one to update if the vocabulary changes.

The maintainer's follow-up request (verbatim): *"in Craft there are a lot of variables with
single-letter names that I have no idea what they are, I think they had to do with chunks."* Pass 1
had deliberately left these as a preserved upstream convention; pass 2 decodes them from the source
and math, documents them, and renames where confident and safe.

**The decode table (evidence in parentheses):**

| Name | Meaning | Evidence |
|------|---------|----------|
| `p` | Chunk column index along world **X**. A "chunk" is one `CHUNK_SIZE`(=32)-wide column of the world. | `p = chunked(x) = floor(round(x)/CHUNK_SIZE)` (`main.c` `chunked`); `create_world`/`compute_chunk` compute world X as `p*CHUNK_SIZE + …`. |
| `q` | Chunk column index along world **Z**. | Symmetric to `p`: world Z `= q*CHUNK_SIZE + …`. |
| `x, y, z` | Block/**world** coordinates; **y is up**. | Used as absolute world coords everywhere (`map_get`, `builder_block`, `hit_test`, world gen). |
| `w` | **Block type** ("what": grass/sand/stone/…), signed so world gen can mark a neighbour chunk's edge blocks negative; consumers take `ABS(w)`. | `builder_block(x,y,z,w)`; `blocks[ABS(w)][…]` in `item.c`; `MapEntry.e.w` union field. **Context exception:** in `light_fill` `w` is the **light level** being flooded (it is decremented `w--` as it spreads), not a block type. |
| `dx, dy, dz` **(Map struct)** | The map's **world-space origin** — subtracted from a world coord to get the local 0..255 byte stored in an entry (`map_set`/`map_get`), added back to recover the world coord (`map_grow`). For chunk (p,q): `origin_x = p*CHUNK_SIZE-1`, `origin_y = 0`, `origin_z = q*CHUNK_SIZE-1` (the `-1` is a one-block pad). | `main.c init_chunk` sets them; `map.c` subtracts/adds them. **Renamed → `origin_x/origin_y/origin_z`.** |
| `dx, dz` **(`create_world` locals)** | **Different meaning:** the block's **offset within the chunk** (loop `-pad..CHUNK_SIZE+pad`). | `x = p*CHUNK_SIZE + dx` in `world.c`. **Renamed → `local_x/local_z`.** |
| `dp, dq` **(`main.c`)** | Neighbour **offset in chunk space** (`-1..1`); `a = p+dp`, `b = q+dq` name a neighbouring chunk's column. | `ensure_chunks_worker`, the `block_maps[dp+1][dq+1]` fill. |
| `ox, oy, oz` **(`compute_chunk`)** | A **scratch array's world-space origin** (world coord minus origin → index into the 3×3-chunk opaque/light scratch arrays). Analogous to `Map.origin_*` but for a temporary array, so left as-is. | `ox = item->p*CHUNK_SIZE - CHUNK_SIZE - 1`, then `x = ex - ox`. |
| `ex, ey, ez / ew` | A stored entry's **recovered world coordinate** (`entry.local + map->origin`) and its type. | `ex = entry->e.x + map->origin_x`. |
| `face` **(db/sign)** | Which of a block's **6 faces** a sign is attached to. | `db_insert_sign(…, face, text)`; `sign` table. |
| `key` **(db)** | A per-chunk **change counter** used to sync with the server. | `db_get_key/db_set_key`, `key` table. |
| `n, e, f, s, u, v` **(`main.c`)** | Not a chunk vocabulary — these are ordinary conventional locals, decoded per site: `n` = a count/length (`strlen`, loop bound), `e` = a `Sign*`/`RingEntry` entry pointer, `u` = the Unicode codepoint in `on_char`, `du/dv`/`uvs` = texture-coord deltas in `cube.c`. Left as-is. |

**Correction found while decoding:** the `Block` struct comment in `main.h` claimed `x/y/z/w` were
*homogeneous coordinates* ("divide x/y/z by w"). That is wrong — `w` is the block type, and the
builder primitives never divide by it. Comment corrected.

## Renamed vs left (pass 2)

**Renamed (confident + gate-verified):**

- **`Map.dx/dy/dz` → `origin_x/origin_y/origin_z`** (`map.h`, `map.c`, + the field-access sites and
  `init_chunk` origin locals in `main.c`). map.c is exercised by `tests/smoke.c`; the main.c edits are
  mechanical, compiler-checked. Replaces the two `map.h` "TODO figure out what this is" comments with
  real documentation. *(This resolves pass-1's open item that wanted a maintainer bless first — the
  maintainer's follow-up request is that bless.)*
- **`create_world` `p→chunk_x`, `q→chunk_z`, `dx→local_x`, `dz→local_z`** (`world.c`, `world.h`).
  Fully exercised by `exercise_world`.

**Left as-is (documented, not renamed):**

- **`x, y, z`** — universal Cartesian convention; the maintainer already reads them as coordinates,
  and they are pervasive. Documented (y is up) rather than renamed.
- **`w`** — kept (block type). Pervasive across `map`/`db`/`world`/`item`/`Block`; renaming everywhere
  would be enormous and would diverge from the `w`-in-SQL-schema columns. Documented, including the
  `light_fill` light-level exception.
- **`db.c` params `p/q/x/y/z/w/face/key`** — kept, because they mirror the sqlite **column** names the
  values bind to; a `chunk_x`-bound-to-column-`p` mismatch would be *less* clear. Documented via a
  header comment instead.
- **`main.c` `p/q` (Chunk/WorkerItem fields and the ~hundreds of locals)** — the bulk rename is still
  deferred. It is the pervasive upstream convention, `main.c` has **no headless test** (compiler-only),
  and `p/q` are struct fields touched at hundreds of sites — high blast radius, low verifiability.
  Documented instead: struct-field comments on `Chunk.p/.q` and `WorkerItem.p/.q`, and a vocabulary
  block above `chunked()`. If the maintainer wants the full rename, do it with the game runnable to
  verify, as its own reviewed pass.

## Follow-on (spun out on completion)

The one deferred item — bulk-renaming `main.c`'s `p/q` chunk coordinates — is now its own task:
**`tasks/rename-main-c-chunk-coords.md`** (proposed; best done with the game runnable to verify).
It is *optional* and does not reopen this task; the vocabulary it needs is documented in
`tasks/reference/coordinate-and-chunk-vocabulary.md`.
