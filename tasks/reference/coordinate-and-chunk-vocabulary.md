# Craft's coordinate & chunk vocabulary

**Reference document.** What Craft's pervasive single-letter variable names mean —
`p`/`q`/`x`/`y`/`z`/`w`/`dx`/`dy`/`dz` and friends. Decoded from the source + math
2026-09-02 (Claude Opus 4.8, on the maintainer's discretion) in response to the
maintainer's note: *"there are a lot of variables with single-letter names that I
have no idea what they are, I think they had to do with chunks."* These names are a
**preserved upstream (Fogleman Craft) convention** used across nearly every file, so
they were mostly documented rather than renamed — this doc is that documentation.
The rename work-log (what was renamed vs left, and why) is in
`tasks/rename-generic-variables.md`; the subsystem overview is in
`tasks/reference/architecture-overview.md`.

## The world is a grid of chunks

A **chunk** is one `CHUNK_SIZE` (= 32)-wide square column of the world (full height).
The world is tiled into chunks indexed by `(p, q)`; within a chunk, individual blocks
have world coordinates `(x, y, z)`.

## The vocabulary

| Name | Meaning | Evidence |
|------|---------|----------|
| **`p`** | Chunk column index along world **X**. | `p = chunked(x) = floor(round(x)/CHUNK_SIZE)` (`main.c` `chunked`); world X `= p*CHUNK_SIZE + …`. |
| **`q`** | Chunk column index along world **Z**. | Symmetric to `p`: world Z `= q*CHUNK_SIZE + …`. |
| **`x, y, z`** | Block/**world** coordinates; **`y` is up**. | Absolute world coords everywhere (`map_get`, `builder_block`, `hit_test`, world gen). |
| **`w`** | **Block type** ("what": grass/sand/stone/…), signed so world gen can mark a neighbour chunk's edge blocks negative; consumers take `ABS(w)`. | `builder_block(x,y,z,w)`; `blocks[ABS(w)][…]` in `item.c`; `MapEntry.e.w`. **Context exception:** in `light_fill`, `w` is the **light level** being flooded (`w--` as it spreads), not a block type. |
| **`dx, dy, dz`** *(Map struct — now `origin_x/y/z`)* | The map's **world-space origin**: subtracted from a world coord to get the local 0..255 byte stored in an entry, added back to recover the world coord. For chunk `(p,q)`: `origin_x = p*CHUNK_SIZE-1`, `origin_y = 0`, `origin_z = q*CHUNK_SIZE-1` (the `-1` is a one-block pad). | `main.c init_chunk` sets them; `map.c` (`map_set`/`map_get`/`map_grow`) subtracts/adds them. **Renamed → `origin_x/origin_y/origin_z`** 2026-09-02. |
| **`dx, dz`** *(`create_world` locals — now `local_x/z`)* | **Different meaning:** the block's **offset within the chunk** (loop `-pad..CHUNK_SIZE+pad`). | `x = p*CHUNK_SIZE + dx` in `world.c`. **Renamed → `local_x/local_z`.** |
| **`dp, dq`** *(`main.c`)* | Neighbour **offset in chunk space** (`-1..1`); `a = p+dp`, `b = q+dq` name a neighbouring chunk's column. | `ensure_chunks_worker`, `block_maps[dp+1][dq+1]`. |
| **`ox, oy, oz`** *(`compute_chunk`)* | A **scratch array's world-space origin** (world coord minus origin → index into the 3×3-chunk opaque/light scratch arrays). Like `Map.origin_*` but for a temporary array, so left as-is. | `ox = item->p*CHUNK_SIZE - CHUNK_SIZE - 1`, then `x = ex - ox`. |
| **`ex, ey, ez / ew`** | A stored entry's **recovered world coordinate** (`entry.local + map->origin`) and its type. | `ex = entry->e.x + map->origin_x`. |
| **`face`** *(db/sign)* | Which of a block's **6 faces** a sign is attached to. | `db_insert_sign(…, face, text)`; the `sign` table. |
| **`key`** *(db)* | A per-chunk **change counter** used to sync with the server. | `db_get_key`/`db_set_key`, the `key` table. |
| **`n, e, f, s, u, v`** *(`main.c`)* | **Not chunk vocabulary** — ordinary conventional locals, per site: `n` = a count/length, `e` = a `Sign*`/`RingEntry` pointer, `u` = the Unicode codepoint in `on_char`, `du/dv`/`uvs` = texture-coord deltas in `cube.c`. |

## Why most of these were documented, not renamed

- **`x, y, z`** — universal Cartesian convention; pervasive. Documented (`y` up), not renamed.
- **`w`** — block type, pervasive across `map`/`db`/`world`/`item`/`Block`, and it mirrors the
  `w` **column** in the sqlite schema. Renaming everywhere would be huge and would diverge
  from the schema. Documented (incl. the `light_fill` light-level exception).
- **`db.c` params `p/q/x/y/z/w/face/key`** — kept, because they mirror the sqlite **column** names
  the values bind to; a `chunk_x`-bound-to-column-`p` mismatch would read as *less* clear.
- **`main.c` `p/q`** (Chunk/WorkerItem fields + ~hundreds of locals) — bulk rename **deferred**:
  pervasive upstream convention, `main.c` has no headless test (compiler-only), high blast radius.
  Documented in-file (struct-field comments + a vocabulary block above `chunked()`); a full rename,
  if wanted, is best done with the game runnable to verify (see `tasks/rename-generic-variables.md`
  "Next steps").

## Correction found while decoding

`main.h`'s `Block` struct comment claimed `x/y/z/w` were *homogeneous coordinates* ("divide x/y/z
by w"). That is **wrong** — `w` is the block type, and the builder primitives never divide by it.
The comment was corrected in the same pass.
