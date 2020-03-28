cmake_minimum_required(VERSION 3.0)

project(craft LANGUAGES C)


# TODO - make the fetching and building of these static libraries
# possible on linux through an option
if(WIN32)

     set(BUILD_SHARED_LIBS OFF)
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
     # no console on start
     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

FILE(GLOB SOURCE_FILES src/*.c)

add_executable(
    craft
    ${SOURCE_FILES}
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c)


include_directories(deps/gl3w)
include_directories(deps/lodepng)
include_directories(deps/noise)
include_directories(deps/sqlite)
include_directories(deps/tinycthread)




target_include_directories(craft PUBLIC "${GLFW_SOURCE_DIR}/include" "${CURL_SOURCE_DIR}/include")
if(WIN32)
    target_link_libraries(craft libcurl glfw opengl32)
else()
    add_definitions(-std=c99)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(CURL REQUIRED libcurl)

    include_directories(${GLFW_INCLUDE_DIR})
    target_link_libraries(craft dl m pthread
        ${GLFW_LIBRARIES} ${CURL_LIBRARIES})
endif()

# add configuration for VS
if(WIN32)
     configure_file(craft.vcxproj.user.in ${CMAKE_BINARY_DIR}/craft.vcxproj.user @ONLY)
endif()
# copy assets over to build dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders) 
