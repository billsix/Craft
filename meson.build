project('craft', ['c', 'cpp'],
  version: '1.0',
  # gnu11/gnu++17 (GNU extensions on) matches CMake's default C_EXTENSIONS=ON;
  # strict c11 hides pthread_rwlock_t etc. behind feature-test macros.
  default_options: ['c_std=gnu11', 'cpp_std=gnu++17'])

# NOTE: Meson port of the CMake build (Linux client first; Windows/macOS -- which
# vendor glfw/curl/sqlite via deps/ subprojects and use the .bat files -- are
# deferred and still described by CMakeLists.txt). deps/ stays vendored and is
# compiled straight into the targets, exactly as the CMake build does.

cc = meson.get_compiler('c')

# ---- renderer selection (mirrors ENABLE_OPENGL / ENABLE_VULKAN; exactly one) ----
opengl = get_option('opengl')
vulkan = get_option('vulkan')
if opengl and vulkan
  error('pick exactly one renderer: -Dopengl=true (default) OR -Dvulkan=true')
endif
if not opengl and not vulkan
  error('one renderer must be enabled: -Dopengl=true or -Dvulkan=true')
endif

add_project_arguments('-DIMGUI_IMPL_OPENGL_LOADER_GL3W=1', language: ['c', 'cpp'])

# ---- generated config.h (from src/config.h.meson.in) ----
conf = configuration_data()
if get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
  conf.set_quoted('RESOURCE_PATH', meson.project_source_root())
else
  conf.set_quoted('RESOURCE_PATH', get_option('prefix') / get_option('datadir') / 'craft')
endif
conf.set_quoted('CMAKE_SYSTEM_VERSION', host_machine.system())  # legacy name the source reads
conf.set('ENABLE_OPENGL_CORE_PROFILE_RENDERER', opengl)
conf.set('ENABLE_VULKAN_RENDERER', vulkan)
conf.set('ENABLE_ONLY_RENDER_ONE_CHUNK', get_option('only_render_one_chunk'))
conf.set('ENABLE_NO_THREADS', get_option('no_threads'))
config_h = configure_file(input: 'src/config.h.meson.in', output: 'config.h', configuration: conf)

# ---- include paths (include_directories('.') also adds the build dir, where
#      config.h lands, so `#include "config.h"` resolves) ----
inc = include_directories(
  '.',
  'src',
  'deps/gl3w/include',
  'deps/lodepng',
  'deps/noise',
  'deps/tinycthread',
  'deps/imgui',
  'deps/imgui/backends',
)

# ---- dependencies (Linux path: system glfw3/libcurl/sqlite3 + GL/dl/m/threads) ----
glfw_dep   = dependency('glfw3')
curl_dep   = dependency('libcurl')
sqlite_dep = dependency('sqlite3')
gl_dep     = dependency('gl')
thread_dep = dependency('threads')
m_dep      = cc.find_library('m', required: false)
dl_dep     = cc.find_library('dl', required: false)

craft_deps = [glfw_dep, curl_dep, sqlite_dep, gl_dep, thread_dep, m_dep, dl_dep]
if vulkan
  craft_deps += dependency('vulkan')
endif

# ---- source lists ----
craft_sources = files(
  'src/auth.c', 'src/client.c', 'src/cube.c', 'src/db.c', 'src/item.c',
  'src/main.c', 'src/map.c', 'src/matrix.c', 'src/ring.c', 'src/sign.c',
  'src/util.c', 'src/world.c',
  'src/gui.cpp',
  # vendored, compiled straight in (sqlite is the system lib on Linux)
  'deps/lodepng/lodepng.c',
  'deps/noise/noise.c',
  'deps/tinycthread/tinycthread.c',
  'deps/imgui/imgui.cpp',
  'deps/imgui/imgui_demo.cpp',
  'deps/imgui/imgui_draw.cpp',
  'deps/imgui/imgui_widgets.cpp',
  'deps/imgui/imgui_tables.cpp',
  'deps/imgui/backends/imgui_impl_glfw.cpp',
)

if opengl
  renderer_sources = files(
    'deps/gl3w/src/gl3w.c',
    'deps/imgui/backends/imgui_impl_opengl3.cpp',
    'src/gl_render.c',
    'src/gui-gl.cpp',
  )
else
  renderer_sources = files(
    'deps/imgui/backends/imgui_impl_vulkan.cpp',
    'src/vulkan_render.c',
    'src/gui-vulkan.cpp',
  )
endif

executable('craft',
  craft_sources, renderer_sources, config_h,
  include_directories: inc,
  dependencies: craft_deps,
  install: true,
  install_dir: get_option('bindir'))

# ---- the `world` shared library the Python server loads ----
shared_library('world',
  files('deps/noise/noise.c', 'src/world.c'), config_h,
  include_directories: inc,
  dependencies: [m_dep],
  install: true,
  install_dir: 'server')

# ---- resources + server scripts ----
install_subdir('textures', install_dir: get_option('datadir') / 'craft')
install_subdir('shaders', install_dir: get_option('datadir') / 'craft')
install_data('world.py', 'server.py', 'builder.py', install_dir: 'server')

# ---- sanitizer gate: craft_smoke over the GL-free core (Linux/clang) ----
san_kind = get_option('sanitizer_gate')
if san_kind != 'off'
  if host_machine.system() != 'linux'
    error('sanitizer_gate is Linux-only')
  endif
  if san_kind == 'address'
    san_args = ['-fsanitize=address', '-fno-omit-frame-pointer', '-g']
  else
    # trap mode: UB -> ud2 -> SIGILL, the reliable pass/fail gate
    san_args = ['-fsanitize=undefined', '-fsanitize-trap=undefined',
                '-fno-omit-frame-pointer', '-g']
  endif
  # noise + tinycthread compiled WITHOUT the sanitizer so their benign UB can't trip it
  gate_deps = static_library('craft_gate_deps',
    files('deps/noise/noise.c', 'deps/tinycthread/tinycthread.c'),
    include_directories: include_directories('deps/noise', 'deps/tinycthread'))
  executable('craft_smoke',
    files('tests/smoke.c', 'src/world.c', 'src/map.c', 'src/ring.c', 'src/db.c',
          'src/cube.c', 'src/item.c', 'src/matrix.c', 'src/sign.c', 'src/util.c'),
    config_h,
    include_directories: inc,
    c_args: san_args,
    link_args: san_args,
    link_with: gate_deps,
    dependencies: [m_dep, dl_dep, thread_dep, sqlite_dep, glfw_dep])
endif
