# Revive the Python server (Py2→Py3), especially in a container

**Status:** blocked
**Priority:** 6
**Difficulty:** 4
**Started:** 2026-08-27 (William Emerison Six <billsix@gmail.com>)
**Blocked on:** maintainer answers the Open question below (multiplayer auth in scope, or local-only?).
**Recheck:** the Open question below is answered (maintainer-gated; `/recheck-blocked` surfaces it).

## Goal

Maintainer's idea, verbatim: *"See if the python server can get working again, especially in a
container."*

Get Craft's Python server running again under Python 3, ideally in a container.

## Context (investigation 2026-08-27)

- `server.py` (25 KB) is **verifiably broken — mixed Py2 + Py3**: it imports Py3 modules (`queue`,
  `socketserver`, `requests` — `server.py:24-25`) but uses Py2 syntax/APIs: `print line`
  (`server.py:83`), `SocketServer.ThreadingMixIn`/`BaseRequestHandler` (`server.py:114,118`), and
  `print >> sys.stderr` (`server.py:660,676`). `CLAUDE.md:184-186` already flags this.
- `world.py` ctypes-loads `libworld.so`, built by the `world` CMake target (`CMakeLists.txt:194`).
- **Prerequisite for the server Dockerfile** (`server-dockerfile.md`, bullet 4): fix the server first,
  then containerize it.

## Plan (draft)

- [ ] Port `server.py` to Python 3 (fix `print`, `SocketServer`→`socketserver`, stderr prints).
- [ ] Confirm `world.py` loads `libworld.so` built from the `world` target.
- [ ] Get a server instance up locally; scope multiplayer/auth per Q1.
- [ ] Hand off to `server-dockerfile.md` for containerization.

## Open questions

1. **Scope** — is the **multiplayer auth** path in scope (the old auth URL is dead, `CLAUDE.md:186`), or
   just getting a **local, offline-mode** server up? This bounds the effort heavily. *(Recommend:
   local/offline server-up first; treat auth/multiplayer as a separate follow-on.)*
