# Explain Craft's threading model; decide whether C++ threads aid understanding

**Status:** DONE 2026-08-27 (William Emerison Six <billsix@gmail.com>) — deliverable produced.

## Outcome

Read the threading code directly (verified `file:line`) and wrote **`tasks/reference/threading.md`**,
covering: the tinycthread (C11) primitive layer; **subsystem 1** — the 4-worker chunk pool (the
IDLE→BUSY→DONE handoff, and the two invariants that make per-worker locks sufficient: coordinate
sharding `(ABS(a)^ABS(b))%WORKERS` and main-thread-sole-ownership of `g->chunks`); **subsystem 2** — the
single DB commit thread (SPSC ring-buffer queue, 5 s commits, with a flagged caveat that the two mutexes
guard the *queue* and *reads* but not the shared sqlite *handle*); and the `ENABLE_NO_THREADS` finding.

**C++ decision: NO — keep C11 + tinycthread.** The hard part is the invariants and the state-machine
handoff, which documentation (not a language switch) clarifies. C++ RAII locks / `jthread` / condvar
predicates would genuinely reduce the manual lock-pairing footgun, but the cost — dragging the clean,
sanitizer-tested C core into C++/`extern "C"` — outweighs it. Full reasoning + the trade-off recorded in
`threading.md` ("Decision" section).

**Notable finding:** `ENABLE_NO_THREADS` (a maintainer-added learning-time flag, not upstream) **does not
work as written** — the spawn is guarded but `ensure_chunks` still marks chunks clean-but-unmeshed and
signals nonexistent workers, so only the 3×3 around the player builds. "Should it work or be removed?" is
carried in the bugs/cleanup task.

Original goal + plan are in git history; this is the lean archived record per convention.
