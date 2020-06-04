## TODO

### Vulkan

* Modify CMake to allow building with Vulkan or with OpenGL
* Get Vulkan ot actually run and open up a black window
** Modify the window creation/initilization to support running Vulkan
** Create a vulkan_renderer.c and .h
** Implement all of the functions defined in main.c's graphics_renderer struct, with no implementation
** Using a CMake configuration, run the blank vulkan app
** Render the sky only using vulkan
** Render the sky and blocks using Vulkan
** Implement the rest.  The event loop in main.c shows everything needing to be done.

### Metal

* Modify CMake to allow building with Vulkan, Metal, or with OpenGL
* Get Metal ot actually run and open up a black window
** Modify the window creation/initilization to support running Metal
** Create a metal_renderer.c and .h
** Implement all of the functions defined in main.c's graphics_renderer struct, with no implementation
** Using a CMake configuration, run the blank metal app
** Render the sky only using metal
** Render the sky and blocks using Metal
** Implement the rest.  The event loop in main.c shows everything needing to be done.
