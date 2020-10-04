/*
 * Copyright (C) 2013 Michael Fogleman
 *               2020 William Emerison Six
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _gl_render_h_
#define _gl_render_h_

void gl_viewport(uint32_t x_min, uint32_t x_max, uint32_t y_min,
                 uint32_t y_max);

void gl_clear_depth_buffer();
void gl_clear_color_buffer();

void gl_enable_scissor_test();
void gl_disable_scissor_test();

void gl_scissor(uint32_t x_min, uint32_t y_min, uint32_t x_width,
                uint32_t y_height);

uint32_t gl_gen_buffer(size_t size, const float * const data);
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

void gl_setup_render_chunks(const float *matrix,
                            const PositionAndOrientation * const positionAndOrientation,
                            float light);

void gl_render_chunk(const Chunk * const chunk);

void gl_draw_triangles_3d_text(uint32_t buffer, int count);

void gl_setup_render_signs(const float * const matrix);

void gl_render_signs(const Chunk * const chunk);

void gl_render_sign(const float * const matrix, int x, int y, int z, int face);

void gl_setup_render_players(const float * const matrix,
                             const PositionAndOrientation * const positionAndOrientation);

void gl_render_player(const Player * const other_player);

void gl_render_sky(uint32_t buffer, const float * const matrix);

void gl_draw_lines(uint32_t buffer, int components, int count);

void gl_render_wireframe(const float * const matrix, int hx, int hy, int hz);

void gl_render_text(const float * const matrix, int justify, float x, float y, float n,
                    const char * const text);

void gl_render_item(const float * const matrix);

void gl_render_plant(uint32_t plant_buffer);

void gl_render_cube(uint32_t cube_buffer);

void gl_render_crosshairs(uint32_t crosshair_buffer, const float * const matrix);

#endif
