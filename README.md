## Craft

A voxel ("Minecraft-like") game in **C with modern OpenGL** (shaders) — just a
few thousand lines of first-party code. Cross-platform (Linux, macOS, Windows),
with optional Python-based multiplayer.

This is **William Emerison Six's fork** of **Michael Fogleman's Craft**
(<http://www.michaelfogleman.com/craft/>,
<https://github.com/fogleman/Craft>). On top of upstream it adds:

* a **pluggable render backend** — the working OpenGL 3.3 core-profile renderer
  and an **in-progress Vulkan** renderer, selected at build time;
* **Dear ImGui** integration for in-game UI;
* refactors that split the renderer out of `main.c` behind a backend-agnostic
  API (`gl_render.*` / `vulkan_render.*`), plus regularly-refreshed vendored
  dependencies.

![Screenshot](https://i.imgur.com/SH7wcas.png)

### Features

* Simple but nice-looking terrain generation using Perlin / simplex noise.
* More than 10 types of blocks, easily extended.
* Plants (grass, flowers, trees, …) and transparency (glass).
* Simple clouds, day/night cycle, and a textured sky dome.
* World changes persisted in a sqlite3 database.
* Ambient occlusion, frustum culling, and exposed-face-only meshing.
* Multiplayer support via a Python server.

### Install dependencies

#### Linux (Fedora)

    sudo dnf install cmake gcc gcc-c++ glfw-devel libcurl-devel sqlite-devel \
                     mesa-libGL-devel libXi-devel libXcursor-devel
    # for the Vulkan backend (optional): sudo dnf install vulkan-loader-devel vulkan-headers

#### Linux (Ubuntu/Debian)

    sudo apt-get install cmake build-essential libglfw3-dev libcurl4-openssl-dev \
                         libsqlite3-dev xorg-dev
    # for the Vulkan backend (optional): sudo apt-get install libvulkan-dev vulkan-headers

On Linux, GLFW / CURL / sqlite3 are taken from the system (via `pkg-config`);
`gl3w`, `imgui`, `lodepng`, `noise`, and `tinycthread` are bundled under `deps/`.

#### macOS

    brew install cmake

#### Windows

Install [CMake](https://cmake.org/download/) and Visual Studio.

### Compile and run

#### Linux / macOS

The `Makefile` is a thin wrapper over CMake:

    git clone https://github.com/billsix/Craft.git
    cd Craft
    export CC=clang CXX=clang++      # gcc works too
    make debug
    ./debugBuildInstall/bin/craft

`make release` builds an optimized tree under `releaseBuildInstall/`. Both
wrappers build the **OpenGL** backend and install the binary plus its
shaders/textures.

To configure CMake directly (e.g. to try the Vulkan backend or other options):

    cmake -S. -Bbuild -DENABLE_VULKAN_RENDERER=YES
    cmake --build build

#### Windows

Use a Visual Studio developer command prompt at the project root:

    buildDebug.bat       # or buildRelease.bat

Open the generated solution under `build/`, set `craft` as the startup project,
and run.

### Render backends & build options

Pick **exactly one** renderer. CMake compiles only the selected backend's
sources (`gl_render.c` + `gui-gl.cpp`, or `vulkan_render.c` + `gui-vulkan.cpp`).

| CMake option                            | Default | Effect                                              |
|-----------------------------------------|---------|-----------------------------------------------------|
| `ENABLE_OPENGL_CORE_PROFILE_RENDERER`   | `ON`    | OpenGL 3.3+ core-profile renderer (complete)        |
| `ENABLE_VULKAN_RENDERER`                | `OFF`   | Vulkan renderer (**work in progress** — see `TODO.md`) |
| `ENABLE_PYTHON`                         | `OFF`   | build the bundled Python (Win/macOS)                |
| `ENABLE_ONLY_RENDER_ONE_CHUNK`          | `OFF`   | debug: render a single chunk, to visualize a chunk  |
| `ENABLE_NO_THREADS`                     | `OFF`   | single-threaded chunk generation (old-hardware FPS) |

Debug builds load shaders/textures from the source tree; Release builds load them
from the install prefix (`share/craft/`), so run a Release build from its
install.

### Multiplayer

The original hosted server (`craft.michaelfogleman.com`) is gone; you self-host.

#### Client

Connect with command-line arguments or the in-game command:

    ./craft [HOST [PORT]]      # default port 4080
    /online [HOST [PORT]]

#### Server

The server is Python, but needs a compiled shared library so it generates terrain
identically to the client:

    cmake -S. -Bbuild && cmake --build build      # builds libworld.so
    cp build/libworld.so .                         # world.py loads ./libworld.so
    python server.py [HOST [PORT]]

> Note: `server.py` is legacy (Python-2-flavored `SocketServer` usage) and may
> need a Python-3 port before it runs. `world.py` (terrain via `ctypes`) and
> `builder.py` (scripted block placement) accompany it.

### Controls

- WASD to move; Space to jump.
- Left Click destroys a block; Right Click (or Cmd + Left Click) creates one.
- Ctrl + Right Click toggles a block as a light source.
- 1–9 select the block to place; E / R cycle the block type.
- Tab toggles walking / flying.
- ZXCVBN move along exact XYZ axes; Left Shift to zoom.
- F toggles orthographic mode.
- O observes players in the main view; P in a picture-in-picture inset.
- T to chat; `/` to enter a command; `` ` `` to write on a block (signs).
- Arrow keys emulate mouse movement; Enter emulates a mouse click.

### Chat commands

    /goto [NAME]      teleport to another user (random if NAME omitted)
    /list             list connected users
    /login NAME       switch registered username (re-contacts login server)
    /logout           become a guest
    /offline [FILE]   switch to offline mode (FILE save, defaults to "craft")
    /online HOST [PORT]   connect to a server
    /pq P Q           teleport to chunk (P, Q)
    /spawn            teleport to spawn

### Implementation details

#### Terrain generation

Generated from deterministic simplex noise seeded by position, so a given
location always generates the same way. The world is split into 32×32-block
chunks in the XZ plane (Y is up), making it effectively "infinite" (floating
point precision is the limit at extreme X/Z) and easy to stream — only visible
chunks are queried from the database.

#### Rendering

Only **exposed faces** are meshed (the vast majority of blocks are hidden); each
chunk keeps a one-block overlap with its neighbors so it knows which perimeter
faces are exposed. Only **visible chunks** are drawn (naive frustum culling). A
chunk's vertex buffer is fully regenerated when any block in it changes. Text is
a bitmap atlas (two triangles per glyph). Modern OpenGL only — no fixed-function;
VBOs for position/normal/UV and GLSL shaders. The simple models (cubes, planes)
are generated in code (`cube.c`); matrices come from `matrix.c`. Transparency in
glass and plants is done by discarding magenta pixels in the fragment shader.

In this fork all of the above is reached through a backend-agnostic API
(`gl_render.h` / `vulkan_render.h`); `main.c` does not call OpenGL directly, so a
second backend (Vulkan, eventually Metal) can be dropped in by implementing the
same functions. Dear ImGui (`gui.cpp` + a per-backend glue file) draws the UI.

#### Database

User changes are stored as a delta in a sqlite database — the default world is
generated, then user edits applied on load. The main `block` table is
`(p, q, x, y, z, w)`: `(p, q)` is the chunk, `(x, y, z)` the position, `w` the
block type (0 = air). In memory, chunks store blocks in a hash map (`map.c`):
`(x, y, z) → w`. Y is limited to `0 ≤ y < 256`; `y = 0` blocks can't be destroyed
(so you can't fall out of the world).

#### Multiplayer protocol

Plain sockets, a line-based ASCII protocol: each line is a command code plus
comma-separated args. The client requests a chunk with `C,p,q,key`; the server
replies with block updates `B,p,q,x,y,z,w` and a new cache key `K,p,q,key` (the
key lets the server send only changes since the client last asked). Player
positions are `P,pid,x,y,z,rx,ry`; the client interpolates the last two updates
for smoother motion and sends its own position at most every 0.1 s. Client-side
sqlite caching runs on a background thread, batched in a transaction committed
every few seconds, fed by a ring buffer (`ring.c`).

#### Collision & hit testing

Hit testing ray-casts from the player along the sight vector (step rate trades
accuracy for cost). Collision keeps the player a fixed distance from obstacle
blocks; clouds and plants aren't obstacles, so you pass through them.

#### Sky dome & ambient occlusion

A textured sky dome encodes time-of-day on the texture's X axis; the block
shaders also sample it to pick a fog color. Ambient occlusion follows
<http://0fps.wordpress.com/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/>.

### Dependencies

`gl3w` (OpenGL extension loading), `glfw` (windowing), `curl` (HTTPS auth),
`lodepng` (PNG textures), `sqlite3` (save data), `tinycthread` (threads), `imgui`
(UI), and `noise` (simplex noise). On Linux, GLFW/CURL/sqlite3 come from the
system; the rest are vendored under `deps/`.

### License

MIT — Michael Fogleman (2013) and William Emerison Six (2020). See `LICENSE.md`.
