cmake_minimum_required(VERSION 3.0)

# project name, using just the C language
project(craft LANGUAGES C)

# TODO - make the fetching and building of these static libraries
# possible on linux through an option
if(WIN32)
     # build static libraries, not dynamic
     set(BUILD_SHARED_LIBS OFF)
     # get the sources for curl
     include(FetchContent)
     FetchContent_Declare(
       CURL
       GIT_REPOSITORY https://github.com/curl/curl.git
       GIT_TAG        curl-7_69_1
     )
     FetchContent_MakeAvailable(CURL)
     # get the sources for glfw
     include(FetchContent)
     FetchContent_Declare(
       GLFW
       GIT_REPOSITORY https://github.com/glfw/glfw.git
       GIT_TAG        3.3.2
     )
     FetchContent_MakeAvailable(GLFW)
     # no console on start
     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
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
include_directories(deps/gl3w)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/sqlite)
include_directories(deps/tinycthread)

# link against the fetched libraries
if(WIN32)
    target_link_libraries(craft libcurl glfw opengl32)
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

# copy assets over to build dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

install(TARGETS craft DESTINATION bin)
