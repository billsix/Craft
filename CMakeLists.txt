cmake_minimum_required(VERSION 3.0)

project(craft)

include(FetchContent)
FetchContent_Declare(
  CURL
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        curl-7_69_1
)
FetchContent_MakeAvailable(CURL)

include(FetchContent)
FetchContent_Declare(
  GLFW
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.2
)
FetchContent_MakeAvailable(GLFW)

FILE(GLOB SOURCE_FILES src/*.c)

add_executable(
    craft
    ${SOURCE_FILES}
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c)

add_definitions(-std=c99 -O3)

#add_subdirectory(deps/glfw)
include_directories(deps/gl3w)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/sqlite)
include_directories(deps/tinycthread)

if(MINGW)
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
        "C:/Program Files/CURL/lib" "C:/Program Files (x86)/CURL/lib")
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
        "C:/Program Files/CURL/include" "C:/Program Files (x86)/CURL/include")
endif()

if(WIN32)
	target_include_directories(craft PUBLIC "${GLFW_SOURCE_DIR}/include" "${CURL_SOURCE_DIR}/include")
    target_link_libraries(craft libcurl glfw opengl32)
endif()
if(APPLE)
    target_link_libraries(craft glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(UNIX)
    target_link_libraries(craft dl glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(MINGW)
    target_link_libraries(craft ws2_32.lib glfw
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

if(WIN32)
configure_file(craft.vcxproj.user.in ${CMAKE_BINARY_DIR}/craft.vcxproj.user @ONLY) 
set(TEXTURES_DIR ${CMAKE_SOURCE_DIR}/textures)
STRING(REGEX REPLACE "/" "\\\\" TEXTURES_DIR ${TEXTURES_DIR})
set(SHADERS_DIR ${CMAKE_SOURCE_DIR}/shaders)
STRING(REGEX REPLACE "/" "\\\\" SHADERS_DIR ${SHADERS_DIR})
set(OUTPUT_COPY_DIR ${CMAKE_CURRENT_BINARY_DIR})
STRING(REGEX REPLACE "/" "\\\\" OUTPUT_COPY_DIR ${OUTPUT_COPY_DIR})
add_custom_command(TARGET craft POST_BUILD
    COMMAND xcopy /i /s /y "${TEXTURES_DIR}" "${OUTPUT_COPY_DIR}\\$<CONFIGURATION>\\textures"
)
add_custom_command(TARGET craft POST_BUILD
    COMMAND xcopy /i /s /y "${SHADERS_DIR}" "${OUTPUT_COPY_DIR}\\$<CONFIGURATION>\\shaders"
)
endif()