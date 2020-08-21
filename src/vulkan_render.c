/*
 * Copyright (C) 2020 William Emerison Six
 *
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

#include "config.h"

#ifdef VULKAN_ENABLED

#include "tinycthread.h"
#include <GLFW/glfw3.h>

#include "cube.h"
#include "lodepng.h"
#include "map.h"
#include "sign.h"
#include "util.h"

#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vulkan_render.h"

void vulkan_viewport(uint32_t x_min, uint32_t x_max, uint32_t y_min,
                     uint32_t y_max) {}

void vulkan_clear_depth_buffer() {}

void vulkan_clear_color_buffer() {}

void vulkan_enable_scissor_test() {}

void vulkan_disable_scissor_test() {}

void vulkan_scissor(uint32_t x_min, uint32_t y_min, uint32_t x_width,
                    uint32_t y_height) {}

uint32_t vulkan_gen_buffer(size_t size, float *data) { return 0; }

void vulkan_del_buffer(uint32_t buffer) {}

uint32_t vulkan_gen_faces(int components, int faces, float *data) { return 0; }

uint32_t vulkan_make_shader(uint32_t type, const char *source) { return 0; }

uint32_t vulkan_load_shader(uint32_t type, const char *path) { return 0; }

uint32_t vulkan_make_program(uint32_t shader1, uint32_t shader2) { return 0; }

uint32_t vulkan_load_program(const char *path1, const char *path2) { return 0; }

void vulkan_load_png_texture(const char *file_name) {}

int vulkan_graphics_loader_init() { return 0; }

void vulkan_initiliaze_global_state() {}

void vulkan_initiliaze_textures() {}

void vulkan_setup_render_chunks(float *matrix,
                                PositionAndOrientation *positionAndOrientation,
                                float light) {}

void vulkan_render_chunk(Chunk *chunk) {}

void vulkan_draw_triangles_3d_text(uint32_t buffer, int count) {}

void vulkan_setup_render_signs(float *matrix) {}

void vulkan_render_signs(Chunk *chunk) {}

void vulkan_render_sign(float *matrix, int x, int y, int z, int face) {}

void vulkan_setup_render_players(
    float *matrix, PositionAndOrientation *positionAndOrientation) {}

void vulkan_render_player(Player *other_player) {}

void vulkan_render_sky(uint32_t buffer, float *matrix) {}

void vulkan_draw_lines(uint32_t buffer, int components, int count) {}

void vulkan_render_wireframe(float *matrix, int hx, int hy, int hz) {}

void vulkan_render_text(float *matrix, int justify, float x, float y, float n,
                        char *text) {}

void vulkan_render_item(float *matrix) {}

void vulkan_render_plant(uint32_t plant_buffer) {}

void vulkan_render_cube(uint32_t cube_buffer) {}

void vulkan_render_crosshairs(uint32_t crosshair_buffer, float *matrix) {}

#endif
