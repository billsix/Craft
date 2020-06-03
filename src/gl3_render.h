#ifndef _gl3_render_h_
#define _gl3_render_h_

#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "auth.h"
#include "client.h"
#include "config.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "map.h"
#include "matrix.h"
#include "noise.h"
#include "sign.h"
#include "tinycthread.h"
#include "util.h"
#include "world.h"
#include "main.h"
#include "util.h"
#include "lodepng.h"

void gl3_viewport(uint32_t x_min, uint32_t x_max, uint32_t y_min, uint32_t y_max);

void gl3_clear_depth_buffer();
void gl3_clear_color_buffer();

void gl3_enable_scissor_test();
void gl3_disable_scissor_test();

void gl3_scissor(uint32_t x_min, uint32_t y_min, uint32_t x_width, uint32_t y_height);

uint32_t gen_buffer(size_t size, float *data);
void del_buffer(uint32_t buffer);
float *malloc_faces(int components, int faces);
uint32_t gen_faces(int components, int faces, float *data);
uint32_t make_shader(uint32_t type, const char *source);
uint32_t load_shader(uint32_t type, const char *path);
uint32_t make_program(uint32_t shader1, uint32_t shader2);
uint32_t load_program(const char *path1, const char *path2);
void load_png_texture(const char *file_name);

int gl3_graphics_loader_init();
void gl3_initiliaze_global_state();
void gl3_initiliaze_textures();

void gl3_setup_render_chunks(float *matrix, State * s, float light);

void gl3_render_chunk(Chunk *chunk);

void gl3_draw_triangles_3d_text(uint32_t buffer, int count);

void gl3_setup_render_signs(float *matrix);

void gl3_render_signs(Chunk *chunk);

void gl3_render_sign(float *matrix, int x, int y, int z, int face);

void gl3_setup_render_players(float *matrix, State *s);

void gl3_render_player(Player * other_player);

void gl3_render_sky(uint32_t buffer, float *matrix);

void gl3_draw_lines(uint32_t buffer, int components, int count);

void gl3_render_wireframe(float *matrix, int hx, int hy, int hz);

// Uncomment the following flag to debug OpenGL calls
// #define GLDEBUG 1

void gl3_render_text(float *matrix, int justify, float x, float y, float n, char *text);

void gl3_render_item(float *matrix);

void gl3_render_plant(uint32_t plant_buffer);

void gl3_render_cube(uint32_t cube_buffer);

void gl3_render_crosshairs(uint32_t crosshair_buffer, float *matrix);

#define NOOP() (void)0

#ifdef GLDEBUG
#define GL_DEBUG_ASSERT() {                                             \
    uint32_t error = glGetError();                                      \
    if (error != GL_NO_ERROR)                                           \
      {                                                                 \
        switch(error){                                                  \
        case GL_INVALID_ENUM:                                           \
          printf("An unacceptable value is specified for an enumerated " \
                 "argument. The offending command is ignored and has no " \
                 "other side effect than to set the error flag.");      \
          break;                                                        \
        case GL_INVALID_VALUE:                                          \
          printf("A numeric argument is out of range. The offending "   \
                 "command is ignored and has no other side effect than " \
                 "to set the error flag.");                             \
          break;                                                        \
        case GL_INVALID_OPERATION:                                      \
          printf("The specified operation is not allowed in the current " \
                 "state. The offending command is ignored and has no "  \
                 "other side effect than to set the error flag.");      \
          break;                                                        \
        case GL_INVALID_FRAMEBUFFER_OPERATION:                          \
          printf("The framebuffer object is not complete. The offending " \
                 "command is ignored and has no other side effect than " \
                 "to set the error flag.");                             \
          break;                                                        \
        case GL_OUT_OF_MEMORY:                                          \
          printf("There is not enough memory left to execute the "      \
                 "command. The state of the GL is undefined, except for " \
                 "the state of the error flags, after this error is "   \
                 "recorded.");                                          \
          break;                                                        \
        case GL_STACK_UNDERFLOW:                                        \
          printf("An attempt has been made to perform an operation that " \
                 "would cause an internal stack to underflow.");        \
          break;                                                        \
        case GL_STACK_OVERFLOW:                                         \
          printf("An attempt has been made to perform an operation that " \
                 "would cause an internal stack to overflow. ");        \
          break;                                                        \
        }                                                               \
        printf ("%d \n", error);                                        \
        assert (0);                                                     \
      }                                                                 \
  }
#else
#define GL_DEBUG_ASSERT() NOOP()
#endif


#endif
