# Explain Craft's threading model; decide whether C++ threads aid understanding

**Status:** blocked
**Priority:** 6
**Difficulty:** 3
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (deliverable type: doc vs migration vs go/no-go).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Explain threading. Decide if using C++ just for the threading would be
advantageous for understanding."*

Document how Craft's threading works, and decide whether rewriting the threading in C++ (`std::thread`)
would make it easier to understand.

## Context (investigation 2026-08-27)

- Threading uses **tinycthread** (C11 `<threads.h>`-style API). Worker pool `WORKERS 4` (`main.h:35`,
  array `main.h:161`); spawned at `main.c:2437-2443` (`thrd_create(&worker->thrd, worker_run, worker)`);
  `worker_run` loop at `main.c:1125`; mutexes `mtx_lock/unlock` throughout (e.g. `main.c:956,1116`).
- A background **sqlite commit thread** with its own mutexes at `db.c:48-50`.
- Single-threaded opt-out `ENABLE_NO_THREADS` at `main.c:2434`.
- **C++17 is already in the build** (imgui/gui glue), so `std::thread` is available with no new toolchain.

## Plan (draft)

- [ ] "Explain" half → a `tasks/reference/threading.md` reference doc: the worker pool, the work item
      ring, the db commit thread, the locks, and the `ENABLE_NO_THREADS` path — `file:line`-anchored.
- [ ] "Decide" half → a decision record (can live in the same doc): would `std::thread`/`std::mutex`/
      `std::jthread` read more clearly than tinycthread here, weighed against staying C-only? Recommend.

## Open questions

1. **Deliverable** — is the ask (a) a written explanation only (reference doc), (b) an actual
   tinycthread→`std::thread` migration, or (c) just a go/no-go recommendation on the C++ switch? These
   are very different scopes. *(Recommend: (a)+(c) first — write the explanation and a recommendation;
   defer any migration to a follow-on task.)*
