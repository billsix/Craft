cmake_minimum_required(VERSION 3.0)

# project name, using just the C language
project(craft LANGUAGES C)

# TODO - make the fetching and building of these static libraries
# possible on linux through an option
if(APPLE OR WIN32)
     set(BUILD_SHARED_LIBS OFF)
     if(WIN32)
         # no console on start
         set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
     endif(WIN32)
endif()

# set SOURCE_FILES to all of the c files
FILE(GLOB SOURCE_FILES src/*.c)

# definition of the craft, and its dependent sources
add_executable(
    craft
    ${SOURCE_FILES}
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c)

# build using the C99 standard
set_property(TARGET craft PROPERTY C_STANDARD 99)

# add include directories for compilation
if(APPLE)
    add_subdirectory(deps/glfw)
    include_directories(deps/glfw/include)
endif(APPLE)
if(WIN32)
    add_subdirectory(deps/glfw)
    include_directories(deps/glfw/include)
    add_subdirectory(deps/curl)
    include_directories(deps/curl/include)
endif(WIN32)

include_directories(deps/gl3w)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/sqlite)
include_directories(deps/tinycthread)

add_definitions(-DTEXTURE_DIR="${CMAKE_INSTALL_PREFIX}/share/craft/textures/")
add_definitions(-DSHADER_DIR="${CMAKE_INSTALL_PREFIX}/share/craft/shaders/")


# link against the fetched libraries
if(WIN32)
    target_link_libraries(craft libcurl glfw opengl32 )
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(craft curl glfw ${GLFW_LIBRARIES})
else()
    # using PkgConfig to determine how to link against
    # the system's glfw and curl
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(CURL REQUIRED libcurl)

    include_directories(${GLFW_INCLUDE_DIR})
    target_link_libraries(craft dl m pthread
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(WIN32)
    # copy assets over to build dir
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                           ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                           ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
endif(WIN32)

# Install
install(TARGETS craft DESTINATION bin)
install(DIRECTORY textures/ DESTINATION share/craft/textures)
install(DIRECTORY shaders/ DESTINATION share/craft/shaders)
