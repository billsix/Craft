cmake_minimum_required(VERSION 4.0)

project(craft LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GL3W=1)


option(ENABLE_OPENGL_CORE_PROFILE_RENDERER "Build the OpenGL Core Profile 3.3+ Renderer" ON)
option(ENABLE_VULKAN_RENDERER "Build the Vulkan Renderer" OFF)
option(ENABLE_PYTHON "Build Python" OFF)
option(ENABLE_ONLY_RENDER_ONE_CHUNK "Only render one chunk" OFF)
option(ENABLE_NO_THREADS "Don't use threads.  This increases the framerate on old hardware significantly, at the cost of poor user experience" OFF)


if(ENABLE_OPENGL_CORE_PROFILE_RENDERER)
  set(IMGUI_RENDERERS
    deps/gl3w/src/gl3w.c
    deps/imgui/backends/imgui_impl_opengl3.cpp
    src/gl_render.c
    src/gui-gl.cpp
    )
endif()

if(ENABLE_VULKAN_RENDERER)
  set(IMGUI_RENDERERS
    deps/imgui/backends/imgui_impl_vulkan.cpp
    src/vulkan_render.c
    src/gui-vulkan.cpp
    )
endif()



# set SOURCE_FILES to all of the c files
set(SOURCE_FILES
  src/auth.c
  src/auth.h
  src/client.c
  src/client.h
  src/cube.c
  src/cube.h
  src/db.c
  src/db.h
  src/item.c
  src/item.h
  src/main.c
  src/main.h
  src/map.c
  src/map.h
  src/matrix.c
  src/matrix.h
  src/ring.c
  src/ring.h
  src/sign.c
  src/sign.h
  src/util.c
  src/util.h
  src/world.c
  src/world.h
)
set(CPLUSPLUS_FILES
  src/gui.h
  src/gui.cpp
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug mode")
  set(RESOURCE_PATH "${CMAKE_SOURCE_DIR}")
ELSE(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Release mode")
  set(RESOURCE_PATH "${CMAKE_INSTALL_PREFIX}/share/craft/")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

configure_file(src/config.h.in src/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src
  deps/imgui/
  deps/imgui/backends/
  )

if(WIN32)
  set(SQLITE_SOURCE "deps/sqlite/sqlite3.c")
elseif(APPLE)
  set(SQLITE_SOURCE "deps/sqlite/sqlite3.c")
else()
  set(SQLITE_SOURCE "")
endif()




# definition of the craft, and its dependent sources
add_executable(
    craft
    ${SOURCE_FILES}
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    ${SQLITE_SOURCE}
    deps/tinycthread/tinycthread.c
    deps/imgui/imgui.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    ${CPLUSPLUS_FILES}
    ${IMGUI_RENDERERS}
)

# build using the C11 standard
set_property(TARGET craft PROPERTY C_STANDARD 11)

# TODO - make the fetching and building of these static libraries
# possible on linux through an option
if(APPLE OR WIN32)
     set(BUILD_SHARED_LIBS OFF)
     if(WIN32)
         # no console on start
         target_link_options(craft PUBLIC "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
     endif(WIN32)
endif()



# add include directories for compilation
if(APPLE)
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    add_subdirectory(deps/glfw)
    if(ENABLE_PYTHON)
       add_subdirectory(deps/python)
    endif(ENABLE_PYTHON)

    include_directories(deps/glfw/include)
    include_directories(deps/sqlite)
endif(APPLE)
if(WIN32)
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    add_subdirectory(deps/glfw)
    if(ENABLE_PYTHON)
      add_subdirectory(deps/python)
    endif(ENABLE_PYTHON)
    include_directories(deps/glfw/include)
    add_subdirectory(deps/curl)
    include_directories(deps/curl/include)
    include_directories(deps/sqlite)
endif(WIN32)


include_directories(deps/gl3w/include)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/tinycthread)
include_directories(deps/imgui)

if(ENABLE_VULKAN_RENDERER)
    # Libraries
    find_package(Vulkan REQUIRED)
    add_dependencies(craft Vulkan::Vulkan)
    set(VULKAN_LIBRARIES "Vulkan::Vulkan")

else()
    set(VULKAN_LIBRARIES "")
    set(Vulkan_INCLUDE_DIR "")
endif()



# link against the fetched libraries
if(WIN32)
    target_link_libraries(craft libcurl glfw opengl32 ${VULKAN_LIBRARIES})
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(craft curl glfw ${GLFW_LIBRARIES})
else()
    # using PkgConfig to determine how to link against
    # the system's glfw and curl
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(CURL REQUIRED libcurl)
    pkg_check_modules(SQLITE REQUIRED sqlite3)

    include_directories(${GLFW_INCLUDE_DIR} ${Vulkan_INCLUDE_DIR})
    target_link_libraries(craft dl m pthread
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES} ${SQLITE_LIBRARIES} ${VULKAN_LIBRARIES})
endif()

add_library(world SHARED deps/noise/noise.c src/world.c)

# Install
install(TARGETS craft DESTINATION bin)
install(DIRECTORY textures/ DESTINATION share/craft/textures)
install(DIRECTORY shaders/ DESTINATION share/craft/shaders)
install(FILES world.py server.py builder.py DESTINATION server)
install(TARGETS world DESTINATION server)
