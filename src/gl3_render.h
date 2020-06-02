#ifndef _gl3_render_h_
#define _gl3_render_h_

#include "gl3w.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "util.h"
#include "lodepng.h"

uint32_t gen_buffer(size_t size, float *data);
void del_buffer(uint32_t buffer);
float *malloc_faces(int components, int faces);
uint32_t gen_faces(int components, int faces, float *data);
uint32_t make_shader(uint32_t type, const char *source);
uint32_t load_shader(uint32_t type, const char *path);
uint32_t make_program(uint32_t shader1, uint32_t shader2);
uint32_t load_program(const char *path1, const char *path2);
void load_png_texture(const char *file_name);

// Uncomment the following flag to debug OpenGL calls
// #define GLDEBUG 1

#define NOOP() (void)0

#ifdef GLDEBUG
#define GL_DEBUG_ASSERT() { \
  uint32_t error = glGetError(); \
  if (error != GL_NO_ERROR) \
    {                                                                   \
      switch(error){                                                    \
      case GL_INVALID_ENUM:                                             \
        printf("An unacceptable value is specified for an enumerated "  \
               "argument. The offending command is ignored and has no " \
               "other side effect than to set the error flag.");        \
        break;                                                          \
      case GL_INVALID_VALUE:                                            \
        printf("A numeric argument is out of range. The offending "     \
               "command is ignored and has no other side effect than "  \
               "to set the error flag.");                               \
        break;                                                          \
      case GL_INVALID_OPERATION:                                        \
        printf("The specified operation is not allowed in the current " \
               "state. The offending command is ignored and has no "    \
               "other side effect than to set the error flag.");        \
        break;                                                          \
      case GL_INVALID_FRAMEBUFFER_OPERATION:                            \
        printf("The framebuffer object is not complete. The offending " \
               "command is ignored and has no other side effect than "  \
               "to set the error flag.");                               \
        break;                                                          \
      case GL_OUT_OF_MEMORY:                                            \
        printf("There is not enough memory left to execute the "        \
               "command. The state of the GL is undefined, except for " \
               "the state of the error flags, after this error is "     \
               "recorded."); \
        break;                                                          \
      case GL_STACK_UNDERFLOW:                                          \
        printf("An attempt has been made to perform an operation that " \
               "would cause an internal stack to underflow.");          \
        break;                                                          \
      case GL_STACK_OVERFLOW:                                           \
        printf("An attempt has been made to perform an operation that " \
               "would cause an internal stack to overflow. ");          \
        break;                                                          \
      }                                                                 \
      printf ("%d \n", error);                                                \
      assert (0);                                                       \
    }                                                                   \
  }
#else
#define GL_DEBUG_ASSERT() NOOP()
#endif


#endif
