# Upgrade the container base from Fedora 44 to Fedora 45 (done)

Done 2026-11-05 (William Emerison Six <billsix@gmail.com>). Part of a fleet-wide
Fedora 44 -> 45 bump.

## What changed

- `Dockerfile`: `registry.fedoraproject.org/fedora:44` -> `fedora:45`.

No source changes. `make image` green nested on the 45 base -- Craft C and its
vendored source deps compiled clean under GCC 16 / clang LLVM 23; base packages
present on Fedora 45. Image `localhost/craft:latest`.
