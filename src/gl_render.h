#ifndef _gl_render_h_
#define _gl_render_h_

#include "auth.h"
#include "client.h"
#include "config.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "lodepng.h"
#include "main.h"
#include "map.h"
#include "matrix.h"
#include "noise.h"
#include "sign.h"
#include "tinycthread.h"
#include "util.h"
#include "world.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>

void gl_viewport(uint32_t x_min, uint32_t x_max, uint32_t y_min,
                 uint32_t y_max);

void gl_clear_depth_buffer();
void gl_clear_color_buffer();

void gl_enable_scissor_test();
void gl_disable_scissor_test();

void gl_scissor(uint32_t x_min, uint32_t y_min, uint32_t x_width,
                uint32_t y_height);

uint32_t gl_gen_buffer(size_t size, float *data);
void gl_del_buffer(uint32_t buffer);
uint32_t gl_gen_faces(int components, int faces, float *data);
uint32_t gl_make_shader(uint32_t type, const char *source);
uint32_t gl_load_shader(uint32_t type, const char *path);
uint32_t gl_make_program(uint32_t shader1, uint32_t shader2);
uint32_t gl_load_program(const char *path1, const char *path2);
void gl_load_png_texture(const char *file_name);

int gl_graphics_loader_init();
void gl_initiliaze_global_state();
void gl_initiliaze_textures();

void gl_setup_render_chunks(float *matrix,
                            PositionAndOrientation *positionAndOrientation,
                            float light);

void gl_render_chunk(Chunk *chunk);

void gl_draw_triangles_3d_text(uint32_t buffer, int count);

void gl_setup_render_signs(float *matrix);

void gl_render_signs(Chunk *chunk);

void gl_render_sign(float *matrix, int x, int y, int z, int face);

void gl_setup_render_players(float *matrix,
                             PositionAndOrientation *positionAndOrientation);

void gl_render_player(Player *other_player);

void gl_render_sky(uint32_t buffer, float *matrix);

void gl_draw_lines(uint32_t buffer, int components, int count);

void gl_render_wireframe(float *matrix, int hx, int hy, int hz);

void gl_render_text(float *matrix, int justify, float x, float y, float n,
                    char *text);

void gl_render_item(float *matrix);

void gl_render_plant(uint32_t plant_buffer);

void gl_render_cube(uint32_t cube_buffer);

void gl_render_crosshairs(uint32_t crosshair_buffer, float *matrix);

#endif
