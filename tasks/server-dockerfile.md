# Dockerfile for the Craft server

**Status:** blocked
**Priority:** 7
**Difficulty:** 3
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** (1) `revive-python-server.md` — the server must run under Py3 before it can be
containerized; (2) maintainer answers the Open questions below.
**Recheck:** `revive-python-server.md` is done AND the Open questions below are answered
(maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"Make dockerfile for server, and one for client."* This task is the
**server** half. The **client** half is folded into the existing `add-container-template.md` (the
Fedora+Podman dev container already builds/runs the C client) — see the cross-reference there.

## Context (investigation 2026-08-27)

- No Dockerfile exists anywhere in the repo.
- The server is `server.py` + `world.py` + `libworld.so` (the `world` CMake target, `CMakeLists.txt:194`,
  `:200-201`). It is currently broken (mixed Py2/Py3) — see `revive-python-server.md`.
- The client container is the dev-container task `add-container-template.md`; this server task must not
  duplicate it (see Q2).

## Plan (draft — after the server runs under Py3)

- [ ] Author a Dockerfile that builds `libworld.so` and runs the Py3 server.
- [ ] Decide separate images vs one multi-stage (Q1); reconcile with the client dev-container (Q2).

## Open questions

1. **Two separate images or one multi-stage** for server + client?
2. **Does "client" here mean the existing dev-container** (`add-container-template.md`), so this task is
   *only* the server image — or a separate client *runtime* image distinct from the dev container?
   *(Recommend: this task = server image only; client stays the dev-container task, avoiding duplication.)*
