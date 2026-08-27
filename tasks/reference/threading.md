# Craft — threading model

**Reference document** — how Craft uses threads, the synchronization protocol, the invariants that
keep it correct, and a recorded decision on whether to switch the threading to C++. Not a task; update
in place. Created 2026-08-27 (William Emerison Six <billsix@gmail.com>) from a direct read of the
threading code (commit `f59081f3`); every claim below is `file:line`-anchored and verified. Companion
to `tasks/reference/architecture-overview.md` and the task `explain-threading-and-cpp-decision`.

## Primitive layer — tinycthread (C11 `<threads.h>` API)

All threading uses **tinycthread** (`deps/tinycthread`), a portable shim for the C11 threads API — so
the code reads as standard `<threads.h>`: `thrd_create`/`thrd_join`, `mtx_init`/`mtx_lock`/`mtx_unlock`/
`mtx_destroy` (`mtx_plain`), `cnd_init`/`cnd_wait`/`cnd_signal`/`cnd_destroy`. No pthreads, no C++
`std::thread`. (C++17 *is* in the build, but only for the imgui glue — see the C++ decision below.)

There are **two independent threading subsystems**, each a self-contained producer/consumer with its
own mutex+condvar. They do not share locks.

## Subsystem 1 — the chunk worker pool (`main.c` / `main.h`)

**Purpose:** generate and mesh terrain chunks off the main thread so the render loop doesn't stall.

**Structures:**
- `Worker` (`main.h:89-96`): `index`, `int state`, `thrd_t thrd`, `mtx_t mtx`, `cnd_t cnd`, and an
  **embedded `WorkerItem item`**. Pool = `Model.workers[WORKERS]`, `WORKERS` = 4 (`main.h:35`).
- `WorkerItem` (`main.h:77-87`): the job — `p,q` (chunk coords), `load` flag, `block_maps[3][3]` /
  `light_maps[3][3]` (the chunk + its 8 neighbors, as **copies**), `faces`, and the generated mesh
  `float *data`.
- States (`main.h:51-53`): `WORKER_IDLE`, `WORKER_BUSY`, `WORKER_DONE`.

**Spawn** — `main.c:2437-2444` (inside the `#ifdef ENABLE_NO_THREADS … #else` — see below): for each
worker, `state=WORKER_IDLE`, `mtx_init`, `cnd_init`, `thrd_create(&worker->thrd, worker_run, worker)`.

**The handoff protocol (the state field is an ownership token):**

```
IDLE ──[main: fill item, state=BUSY, cnd_signal]──▶ BUSY ──[worker: compute]──▶ DONE ──[main: consume+GL upload, free, state=IDLE]──▶ IDLE
        (ensure_chunks_worker, under worker->mtx)          (worker_run)                 (check_workers, under worker->mtx)
```

- **`worker_run` (`main.c:1124-1143`)** — the thread body. Locks `mtx`, `cnd_wait`s while `state !=
  WORKER_BUSY` (`:1129-1131`), then **unlocks and computes outside the lock** (`load_chunk` if
  `item->load`, then `compute_chunk`, `:1133-1137`), then re-locks to set `state=WORKER_DONE`
  (`:1138-1140`). Computing unlocked is safe because the state contract guarantees the main thread will
  not touch `item` while `state==BUSY`.
- **`ensure_chunks_worker` (`main.c:1016-1109`)** — runs on the **main thread with `worker->mtx` already
  held** (called from `ensure_chunks`). Picks the best dirty/missing chunk in `create_radius` for *this*
  worker, fills `item` (p,q,load), **`malloc`+`map_copy`s the 3×3 neighbor maps into the item**
  (`:1081-1105`), sets `chunk->dirty=0` (`:1106`), then `state=WORKER_BUSY` + `cnd_signal` (`:1107-1108`).
- **`ensure_chunks` (`main.c:1111-1122`)** — per frame (called from `render_chunks`, `:1308`):
  `check_workers()` → `force_chunks()` → for each worker, `mtx_lock`, and if `IDLE` call
  `ensure_chunks_worker`, `mtx_unlock`.
- **`check_workers` (`main.c:953-990`)** — main thread. For each worker: lock `mtx`; if `DONE`,
  `find_chunk` and (on load) copy the freshly-loaded maps back into the live chunk + `request_chunk`
  (`:961-968`), then **`generate_chunk` uploads the mesh to GL** (`:970` — GL calls happen only here, on
  the main thread), free the malloc'd neighbor-map copies (`:972-985`), set `state=WORKER_IDLE`; unlock.
- **`force_chunks` (`main.c:992-1014`)** — **synchronous** generation of the 3×3 around the player, on
  the main thread (`gen_chunk_buffer`). Used at startup (`:2503`) and after teleport (`:2235`) so the
  player never falls through unbuilt terrain.

**Why per-worker locks are enough — the two invariants that make this safe:**
1. **Coordinate sharding.** Each chunk `(a,b)` is owned by exactly one worker via
   `index = (ABS(a) ^ ABS(b)) % WORKERS` (`main.c:1037`); a worker skips coords that aren't its index.
   So two workers never target the same chunk — no inter-worker contention, hence no global chunk mutex.
2. **The main thread solely owns `g->chunks[]`.** `find_chunk`, `create_chunk`, `init_chunk`, and
   `chunk_count++` all run on the main thread. **Workers never touch `g->chunks`** — they operate only on
   the `map_copy`'d neighbor maps inside their own `item`. The generated mesh flows back through the
   `DONE` state and is uploaded to GL by the main thread in `check_workers`. The only shared mutable
   state between a worker and main is `worker->item` + `worker->state`, both guarded by `worker->mtx`.

## Subsystem 2 — the DB commit thread (`db.c`)

**Purpose:** move sqlite writes off the main thread and batch them, committing every `COMMIT_INTERVAL`
(5 s).

- **One background thread** started in `db_worker_start` (`db.c:499-508`): `ring_alloc(&ring, 1024)`,
  `mtx_init(&mtx)`, `mtx_init(&load_mtx)`, `cnd_init(&cnd)`, `thrd_create(&thrd, db_worker_run, path)`.
- **A ring buffer is the SPSC queue.** Producers on the **main thread** — `db_insert_block` (`:327`),
  `db_insert_light` (`:348`), `db_set_key` (`:481`), `db_commit` (`:189`) — each `mtx_lock` → `ring_put_*`
  → `cnd_signal` → `mtx_unlock`. `RingEntryType` = `BLOCK LIGHT KEY COMMIT EXIT` (`ring.h:26`).
- **`db_worker_run` (`db.c:525-553`)** — the consumer: `mtx_lock`, `cnd_wait` while `ring_get` is empty,
  `mtx_unlock`, then `switch(e.type)` to the real `_db_*` sqlite calls **outside the lock**. `EXIT` stops
  the loop.
- **`db_worker_stop` (`db.c:510-523`)** enqueues `EXIT`, signals, `thrd_join`s, destroys the primitives.

**What is NOT queued (runs synchronously on the main thread):**
- **Signs** — `db_insert_sign`/`db_delete_*` call `sqlite3_step` directly.
- **Reads** — `db_load_blocks`/`db_load_lights` run synchronously under a **separate** `load_mtx`
  (`db.c:419,437`); `db_load_signs`/`db_get_key` take no lock.
- **Auth / player-state** helpers prepare+run ad-hoc statements on the caller.

**⚠ Concurrency caveat worth knowing (not a proven bug — depends on sqlite's build):** the two mutexes
guard *different* things — `mtx` guards the write **queue** (not the DB), `load_mtx` guards **reads**.
Neither serializes the shared `sqlite3 *db` **handle** against the other. So a background
`_db_insert_block` (writer thread, holding no lock during `sqlite3_step`) can run concurrently with a
main-thread `db_load_blocks` (holding only `load_mtx`) or a synchronous `db_insert_sign` (holding no
lock) — all on the *same* connection. Whether that's safe hinges on sqlite's compile-time threading mode
(SERIALIZED tolerates it; MULTITHREAD does not allow one connection on multiple threads at once). Flag
for anyone touching the DB threading: the locking here protects the queue, not the connection.

## `ENABLE_NO_THREADS` — an incomplete experimental mode; does NOT work as written

**Provenance (William Emerison Six, 2026-08-27):** this flag is **not upstream** — the maintainer added
it while learning the codebase and was unsure whether it works. **Verified answer: it does not work
correctly as written.**

The `#ifdef ENABLE_NO_THREADS` at `main.c:2434` guards **only the worker spawn** (`:2437-2444` sit in the
`#else`). But `ensure_chunks` is still called every frame (`:1308`) with no such guard, and
`ensure_chunks_worker` still sets `chunk->dirty=0`, `state=WORKER_BUSY`, and `cnd_signal`s — to a thread
that was never created. With no worker to move `BUSY→DONE`, `check_workers` never fires `generate_chunk`,
so those chunks are marked **clean but never meshed** (and never uploaded to GL). Only `force_chunks`'
synchronous 3×3 around the player ever builds. Net effect with `ENABLE_NO_THREADS=YES`: terrain beyond
the immediate 3×3 never appears. So `CLAUDE.md`'s "single-threaded, higher FPS on old hardware"
description is **aspirational, not what the code does today.**

**The fix (if the mode is wanted):** under `ENABLE_NO_THREADS`, make `ensure_chunks_worker` (or a
sibling) do the `load_chunk`/`compute_chunk`/`generate_chunk` **synchronously** on the main thread
instead of handing off to a `cnd_signal`'d worker — i.e. collapse the BUSY→DONE round-trip into one
inline call. Otherwise, remove the flag to avoid a misleading broken option. (This is a "should it work
or be removed?" call for the maintainer — noted in the bugs/cleanup task.)

## Known threading-adjacent defects (see `architecture-overview.md` / the bugs task)

- **`db_worker_start`** is declared `()` (`db.h:53`) but defined `(char *path)` (`db.c:499`); `path` is
  passed as the thread arg but `db_worker_run` ignores it. Harmless, but a real signature mismatch.
- The DB-handle concurrency caveat above.

## Decision — should the threading be rewritten in C++ (for understanding)?

**Recommendation: NO — keep C11 + tinycthread.** Reasoning, so the decision is recorded:

- **The hard part isn't the primitives; it's the two invariants** (coordinate sharding + main-thread-owns-
  `g->chunks`) and the state-machine handoff. A language switch doesn't simplify those — *documentation*
  does (this doc). Once the invariants are written down, the existing code reads clearly.
- **What C++ genuinely would improve** (recording the trade-off honestly): RAII locks
  (`std::lock_guard`/`std::unique_lock`) would remove the manual `mtx_lock`/`mtx_unlock` pairing that is
  the main footgun here (e.g. `worker_run`'s lock→unlock→relock dance, and the DB producers); a
  `std::condition_variable::wait(lock, pred)` collapses the `while(!cond) cnd_wait` loops; `std::jthread`
  auto-joins; `std::atomic<int>` could carry the `state` field. These are real readability/safety wins
  for the *mechanics*.
- **Why it still isn't worth it:** the project is deliberately **C11 for `src/`**, C++ only for the imgui
  glue. Moving threading to C++ would drag `main.c`/`db.c` (or a chunk of them) into C++ or an `extern
  "C"` boundary, mixing paradigms into the **GL-free core that is currently clean C and sanitizer-tested**
  (`make sanitize`). For a teaching-oriented fork, explicit C11 threads also have pedagogical value —
  you see every lock. The cost (paradigm split, sanitizer-gate scope, churn) outweighs the RAII win.
- **Higher-value alternatives to a rewrite:** (a) this doc; (b) a tiny C `SCOPED_LOCK`-style helper or
  just disciplined single-return lock/unlock; (c) fixing the verified defects (the `db_worker_start`
  signature) and making `ENABLE_NO_THREADS` either work or be removed. None require C++.

If a future goal makes C++ the primary language anyway, revisit — the RAII lock argument is the thing to
weigh. Until then, the understanding gap is closed by documentation, not translation.

## Cross-links

- `tasks/reference/architecture-overview.md` — the whole-engine map (this doc is the threading deep-dive).
- `tasks/explain-threading-and-cpp-decision.md` — the task that produced this doc (archived on completion).
