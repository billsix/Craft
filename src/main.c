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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <GLFW/glfw3.h>

#include "config.h"

#include "tinycthread.h"

#include "sign.h"

#include "map.h"

#include "main.h"

#include "auth.h"
#include "client.h"
#include "cube.h"
#include "db.h"
#include "gl_render.h"
#include "item.h"
#include "matrix.h"
#include "noise.h"
#include "util.h"

#include "gui.h"

#include "vulkan_render.h"

#include "world.h"
#include <curl/curl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#elif CMAKE_CONFIG
// do nothing
#endif

Model model;
Model *g = &model;

bool escape_pressed = false;
bool do_render_chunks = true;
bool do_render_signs = true;
bool do_render_sky = true;
bool do_render_wireframe = true;
bool do_render_text = true;
bool do_render_item = true;
bool do_render_plant = true;
bool do_render_cube = true;
bool do_render_crosshairs = true;

// Interface for multiple rendering backends
// currently, only OpenGL3.3 core profile is supported.
//
// The event loop is intended to be independent of rendering backend.
// I intend to support Vulkan and Metal.
//
// I removed GLuint, GLbool, etc, and used uint32_t, etc,
// by looking up in the OpenGL standard what the minimal
// number of bits required is.
// I did this with the intent of making this interface portable
// towards those other framewarks.
// TODO - because of the above comments, this may have been
// imprudent, and I will need to reassess later as I understand
// those frameworks better.
struct graphics_renderer {
  void (*viewport)(uint32_t x_min, uint32_t x_max, uint32_t y_min,
                   uint32_t y_max);
  void (*clear_depth_buffer)();
  void (*clear_color_buffer)();
  void (*enable_scissor_test)();
  void (*disable_scissor_test)();
  void (*scissor)(uint32_t x_min, uint32_t y_min, uint32_t x_width,
                  uint32_t y_height);
  uint32_t (*gen_buffer)(size_t size, const float *const data);
  void (*del_buffer)(uint32_t buffer);
  uint32_t (*gen_faces)(int components, int faces, float *data);
  uint32_t (*make_shader)(uint32_t type, const char *source);
  uint32_t (*load_shader)(uint32_t type, const char *path);
  uint32_t (*make_program)(uint32_t shader1, uint32_t shader2);
  uint32_t (*load_program)(const char *path1, const char *path2);
  void (*load_png_texture)(const char *file_name);
  int (*graphics_loader_init)();
  void (*initiliaze_global_state)();
  void (*initiliaze_textures)();
  void (*setup_render_chunks)(
      const float *matrix,
      const PositionAndOrientation *const positionAndOrientation, float light);
  void (*render_chunk)(const Chunk *const chunk);
  void (*draw_triangles_3d_text)(uint32_t buffer, int count);
  void (*setup_render_signs)(const float *const matrix);
  void (*render_signs)(const Chunk *const chunk);
  void (*render_sign)(const float *const matrix, int x, int y, int z, int face);
  void (*setup_render_players)(
      const float *const matrix,
      const PositionAndOrientation *const positionAndOrientation);
  void (*render_player)(const Player *const other_player);
  void (*render_sky)(uint32_t buffer, const float *const matrix);
  void (*draw_lines)(uint32_t buffer, int components, int count);
  void (*render_wireframe)(const float *const matrix, int hx, int hy, int hz);
  void (*render_text)(const float *const matrix, int justify, float x, float y,
                      float n, const char *const text);
  void (*render_item)(const float *const matrix);
  void (*render_plant)(uint32_t plant_buffer);
  void (*render_cube)(uint32_t cube_buffer);
  void (*render_crosshairs)(uint32_t crosshair_buffer,
                            const float *const matrix);
};

#ifdef ENABLE_OPENGL_CORE_PROFILE_RENDERER
// the OpenGL 3.3 Core Profile renderer
struct graphics_renderer gl_renderer = {
    .viewport = gl_viewport,
    .clear_depth_buffer = gl_clear_depth_buffer,
    .clear_color_buffer = gl_clear_color_buffer,
    .enable_scissor_test = gl_enable_scissor_test,
    .disable_scissor_test = gl_disable_scissor_test,
    .scissor = gl_scissor,
    .gen_buffer = gl_gen_buffer,
    .del_buffer = gl_del_buffer,
    .gen_faces = gl_gen_faces,
    .make_shader = gl_make_shader,
    .load_shader = gl_load_shader,
    .make_program = gl_make_program,
    .load_program = gl_load_program,
    .load_png_texture = gl_load_png_texture,
    .graphics_loader_init = gl_graphics_loader_init,
    .initiliaze_global_state = gl_initiliaze_global_state,
    .initiliaze_textures = gl_initiliaze_textures,
    .setup_render_chunks = gl_setup_render_chunks,
    .render_chunk = gl_render_chunk,
    .draw_triangles_3d_text = gl_draw_triangles_3d_text,
    .setup_render_signs = gl_setup_render_signs,
    .render_signs = gl_render_signs,
    .render_sign = gl_render_sign,
    .setup_render_players = gl_setup_render_players,
    .render_player = gl_render_player,
    .render_sky = gl_render_sky,
    .draw_lines = gl_draw_lines,
    .render_wireframe = gl_render_wireframe,
    .render_text = gl_render_text,
    .render_item = gl_render_item,
    .render_plant = gl_render_plant,
    .render_cube = gl_render_cube,
    .render_crosshairs = gl_render_crosshairs,
};
#endif

#ifdef ENABLE_VULKAN_RENDERER
// the Vulkan renderer, currently empty
struct graphics_renderer vulkan_renderer = {
    .viewport = vulkan_viewport,
    .clear_depth_buffer = vulkan_clear_depth_buffer,
    .clear_color_buffer = vulkan_clear_color_buffer,
    .enable_scissor_test = vulkan_enable_scissor_test,
    .disable_scissor_test = vulkan_disable_scissor_test,
    .scissor = vulkan_scissor,
    .gen_buffer = vulkan_gen_buffer,
    .del_buffer = vulkan_del_buffer,
    .gen_faces = vulkan_gen_faces,
    .make_shader = vulkan_make_shader,
    .load_shader = vulkan_load_shader,
    .make_program = vulkan_make_program,
    .load_program = vulkan_load_program,
    .load_png_texture = vulkan_load_png_texture,
    .graphics_loader_init = vulkan_graphics_loader_init,
    .initiliaze_global_state = vulkan_initiliaze_global_state,
    .initiliaze_textures = vulkan_initiliaze_textures,
    .setup_render_chunks = vulkan_setup_render_chunks,
    .render_chunk = vulkan_render_chunk,
    .draw_triangles_3d_text = vulkan_draw_triangles_3d_text,
    .setup_render_signs = vulkan_setup_render_signs,
    .render_signs = vulkan_render_signs,
    .render_sign = vulkan_render_sign,
    .setup_render_players = vulkan_setup_render_players,
    .render_player = vulkan_render_player,
    .render_sky = vulkan_render_sky,
    .draw_lines = vulkan_draw_lines,
    .render_wireframe = vulkan_render_wireframe,
    .render_text = vulkan_render_text,
    .render_item = vulkan_render_item,
    .render_plant = vulkan_render_plant,
    .render_cube = vulkan_render_cube,
    .render_crosshairs = vulkan_render_crosshairs,
};
#endif

// needs to be initialized before the event loop begins
struct graphics_renderer renderer;

int chunked(float x) { return floorf(roundf(x) / CHUNK_SIZE); }

float time_of_day() {
  if (g->day_length <= 0) {
    return 0.5;
  }
  float t;
  t = glfwGetTime();
  t = t / g->day_length;
  t = t - (int)t;
  return t;
}

void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) {
  const float m = cosf(ry);
  *vx = cosf(rx - RADIANS(90)) * m;
  *vy = sinf(ry);
  *vz = sinf(rx - RADIANS(90)) * m;
}

uint32_t gen_sky_buffer() {
  float data[12288];
  make_sphere(data, 1, 3);
  return (*renderer.gen_buffer)(sizeof(data), data);
}

uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) {
  float *const data = malloc_faces(10, 6);
  make_player(data, x, y, z, rx, ry);
  return gl_gen_faces(10, 6, data);
}

Player *find_player(int id) {
  for (int i = 0; i < g->player_count; i++) {
    Player *player = g->players + i;
    if (player->id == id) {
      return player;
    }
  }
  return 0;
}

void update_player(Player *player, float x, float y, float z, float rx,
                   float ry, int interpolate) {
  if (interpolate) {
    PositionAndOrientation *positionAndOrientation1 =
        &player->positionAndOrientation1;
    PositionAndOrientation *positionAndOrientation2 =
        &player->positionAndOrientation2;
    memcpy(positionAndOrientation1, positionAndOrientation2,
           sizeof(PositionAndOrientation));
    positionAndOrientation2->x = x;
    positionAndOrientation2->y = y;
    positionAndOrientation2->z = z;
    positionAndOrientation2->rx = rx;
    positionAndOrientation2->ry = ry;
    positionAndOrientation2->t = glfwGetTime();
    if (positionAndOrientation2->rx - positionAndOrientation1->rx > PI) {
      positionAndOrientation1->rx += 2 * PI;
    }
    if (positionAndOrientation1->rx - positionAndOrientation2->rx > PI) {
      positionAndOrientation1->rx -= 2 * PI;
    }
  } else {
    PositionAndOrientation *positionAndOrientation =
        &player->positionAndOrientation;
    positionAndOrientation->x = x;
    positionAndOrientation->y = y;
    positionAndOrientation->z = z;
    positionAndOrientation->rx = rx;
    positionAndOrientation->ry = ry;
    (*renderer.del_buffer)(player->buffer);
    player->buffer =
        gen_player_buffer(positionAndOrientation->x, positionAndOrientation->y,
                          positionAndOrientation->z, positionAndOrientation->rx,
                          positionAndOrientation->ry);
  }
}

Player *player_crosshair(Player *player) {
  Player *result = 0;
  float threshold = RADIANS(5);
  float best = 0;
  for (int i = 0; i < g->player_count; i++) {
    Player *other_player = g->players + i;
    if (other_player == player) {
      continue;
    }

    float player_other_player_distance;
    {
      // initialize player_other_player_distance
      const PositionAndOrientation *const positionAndOrientation1 =
          &player->positionAndOrientation;
      const PositionAndOrientation *const positionAndOrientation2 =
          &other_player->positionAndOrientation;
      const float x = positionAndOrientation2->x - positionAndOrientation1->x;
      const float y = positionAndOrientation2->y - positionAndOrientation1->y;
      const float z = positionAndOrientation2->z - positionAndOrientation1->z;
      player_other_player_distance = sqrtf(x * x + y * y + z * z);
    }

    float player_crosshair_distance;
    { // initialize player crosshair distance
      PositionAndOrientation *positionAndOrientation1 =
          &player->positionAndOrientation;
      PositionAndOrientation *positionAndOrientation2 =
          &other_player->positionAndOrientation;
      float vx, vy, vz;
      get_sight_vector(positionAndOrientation1->rx, positionAndOrientation1->ry,
                       &vx, &vy, &vz);
      vx *= player_other_player_distance;
      vy *= player_other_player_distance;
      vz *= player_other_player_distance;
      float px, py, pz;
      px = positionAndOrientation1->x + vx;
      py = positionAndOrientation1->y + vy;
      pz = positionAndOrientation1->z + vz;
      float x = positionAndOrientation2->x - px;
      float y = positionAndOrientation2->y - py;
      float z = positionAndOrientation2->z - pz;
      player_crosshair_distance = sqrtf(x * x + y * y + z * z);
    }
    if (player_other_player_distance < 96 &&
        player_crosshair_distance / player_other_player_distance < threshold) {
      if (best == 0 || player_other_player_distance < best) {
        best = player_other_player_distance;
        result = other_player;
      }
    }
  }
  return result;
}

Chunk *find_chunk(int p, int q) {
  for (int i = 0; i < g->chunk_count; i++) {
    Chunk *chunk = g->chunks + i;
    if (chunk->p == p && chunk->q == q) {
      return chunk;
    }
  }
  return 0;
}

int chunk_distance(const Chunk *const chunk, int p, int q) {
  const int dp = ABS(chunk->p - p);
  const int dq = ABS(chunk->q - q);
  return MAX(dp, dq);
}

int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) {
  const int x = p * CHUNK_SIZE - 1;
  const int z = q * CHUNK_SIZE - 1;
  const int d = CHUNK_SIZE + 1;
  float points[8][3] = {{x + 0, miny, z + 0}, {x + d, miny, z + 0},
                        {x + 0, miny, z + d}, {x + d, miny, z + d},
                        {x + 0, maxy, z + 0}, {x + d, maxy, z + 0},
                        {x + 0, maxy, z + d}, {x + d, maxy, z + d}};
  const int n = g->ortho ? 4 : 6;
  for (int i = 0; i < n; i++) {
    int in = 0;
    int out = 0;
    for (int j = 0; j < 8; j++) {
      float d = planes[i][0] * points[j][0] + planes[i][1] * points[j][1] +
                planes[i][2] * points[j][2] + planes[i][3];
      if (d < 0) {
        out++;
      } else {
        in++;
      }
      if (in && out) {
        break;
      }
    }
    if (in == 0) {
      return 0;
    }
  }
  return 1;
}

int highest_block(float x, float z) {
  int result = -1;
  const int nx = roundf(x);
  const int nz = roundf(z);
  const int p = chunked(x);
  const int q = chunked(z);
  const Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    const Map *const map = &chunk->map;
    for (unsigned int i = 0; i <= map->mask; i++) {
      const MapEntry *const entry = map->data + i;
      if (EMPTY_ENTRY(entry)) {
        continue;
      }
      const int ex = entry->e.x + map->dx;
      const int ey = entry->e.y + map->dy;
      const int ez = entry->e.z + map->dz;
      const int ew = entry->e.w;
      if (is_obstacle(ew) && ex == nx && ez == nz) {
        result = MAX(result, ey);
      }
    }
  }
  return result;
}

int _hit_test(const Map *const map, float max_distance, int previous, float x,
              float y, float z, float vx, float vy, float vz, int *hx, int *hy,
              int *hz) {
  const int m = 32;
  int px = 0;
  int py = 0;
  int pz = 0;
  for (int i = 0; i < max_distance * m; i++) {
    const int nx = roundf(x);
    const int ny = roundf(y);
    const int nz = roundf(z);
    if (nx != px || ny != py || nz != pz) {
      int hw = map_get(map, nx, ny, nz);
      if (hw > 0) {
        if (previous) {
          *hx = px;
          *hy = py;
          *hz = pz;
        } else {
          *hx = nx;
          *hy = ny;
          *hz = nz;
        }
        return hw;
      }
      px = nx;
      py = ny;
      pz = nz;
    }
    x += vx / m;
    y += vy / m;
    z += vz / m;
  }
  return 0;
}

int hit_test(int previous, float x, float y, float z, float rx, float ry,
             int *bx, int *by, int *bz) {
  int result = 0;
  float best = 0;
  const int p = chunked(x);
  const int q = chunked(z);
  float vx, vy, vz;
  { get_sight_vector(rx, ry, &vx, &vy, &vz); }
  for (int i = 0; i < g->chunk_count; i++) {
    const Chunk *const chunk = g->chunks + i;
    if (chunk_distance(chunk, p, q) > 1) {
      continue;
    }
    int hx, hy, hz;
    int hw =
        _hit_test(&chunk->map, 8, previous, x, y, z, vx, vy, vz, &hx, &hy, &hz);
    if (hw > 0) {
      const float d =
          sqrtf(powf(hx - x, 2) + powf(hy - y, 2) + powf(hz - z, 2));
      if (best == 0 || d < best) {
        best = d;
        *bx = hx;
        *by = hy;
        *bz = hz;
        result = hw;
      }
    }
  }
  return result;
}

int hit_test_face(const Player *const player, int *x, int *y, int *z,
                  int *face) {
  const PositionAndOrientation *const positionAndOrientation =
      &player->positionAndOrientation;
  int w = hit_test(0, positionAndOrientation->x, positionAndOrientation->y,
                   positionAndOrientation->z, positionAndOrientation->rx,
                   positionAndOrientation->ry, x, y, z);
  if (is_obstacle(w)) {
    int hx, hy, hz;
    hit_test(1, positionAndOrientation->x, positionAndOrientation->y,
             positionAndOrientation->z, positionAndOrientation->rx,
             positionAndOrientation->ry, &hx, &hy, &hz);
    const int dx = hx - *x;
    const int dy = hy - *y;
    const int dz = hz - *z;
    if (dx == -1 && dy == 0 && dz == 0) {
      *face = 0;
      return 1;
    }
    if (dx == 1 && dy == 0 && dz == 0) {
      *face = 1;
      return 1;
    }
    if (dx == 0 && dy == 0 && dz == -1) {
      *face = 2;
      return 1;
    }
    if (dx == 0 && dy == 0 && dz == 1) {
      *face = 3;
      return 1;
    }
    if (dx == 0 && dy == 1 && dz == 0) {
      int degrees = roundf(DEGREES(atan2f(positionAndOrientation->x - hx,
                                          positionAndOrientation->z - hz)));
      {
        if (degrees < 0) {
          degrees += 360;
        }
      }
      const int top = ((degrees + 45) / 90) % 4;
      *face = 4 + top;
      return 1;
    }
  }
  return 0;
}

int player_intersects_block(int height, float x, float y, float z, int hx,
                            int hy, int hz) {
  const int nx = roundf(x);
  const int ny = roundf(y);
  const int nz = roundf(z);
  for (int i = 0; i < height; i++) {
    if (nx == hx && ny - i == hy && nz == hz) {
      return 1;
    }
  }
  return 0;
}

int _gen_sign_buffer(float *data, float x, float y, float z, int face,
                     const char *text) {
  static const int glyph_dx[8] = {0, 0, -1, 1, 1, 0, -1, 0};
  static const int glyph_dz[8] = {1, -1, 0, 0, 0, -1, 0, 1};
  static const int line_dx[8] = {0, 0, 0, 0, 0, 1, 0, -1};
  static const int line_dy[8] = {-1, -1, -1, -1, 0, 0, 0, 0};
  static const int line_dz[8] = {0, 0, 0, 0, 1, 0, -1, 0};
  if (face < 0 || face >= 8) {
    return 0;
  }
  int count = 0;
  const float max_width = 64;
  const float line_height = 1.25;
  char lines[1024];
  int rows = wrap(text, max_width, lines, 1024);
  { rows = MIN(rows, 5); }
  const int dx = glyph_dx[face];
  const int dz = glyph_dz[face];
  const int ldx = line_dx[face];
  const int ldy = line_dy[face];
  const int ldz = line_dz[face];
  const float n = 1.0 / (max_width / 10);
  float sx = x - n * (rows - 1) * (line_height / 2) * ldx;
  float sy = y - n * (rows - 1) * (line_height / 2) * ldy;
  float sz = z - n * (rows - 1) * (line_height / 2) * ldz;
  char *key;
  char *line = tokenize(lines, "\n", &key);
  while (line) {
    const int length = strlen(line);
    int line_width = string_width(line);
    { line_width = MIN(line_width, max_width); }
    float rx = sx - dx * line_width / max_width / 2;
    float ry = sy;
    float rz = sz - dz * line_width / max_width / 2;
    for (int i = 0; i < length; i++) {
      const int width = char_width(line[i]);
      line_width -= width;
      if (line_width < 0) {
        break;
      }
      rx += dx * width / max_width / 2;
      rz += dz * width / max_width / 2;
      if (line[i] != ' ') {
        make_character_3d(data + count * 30, rx, ry, rz, n / 2, face, line[i]);
        count++;
      }
      rx += dx * width / max_width / 2;
      rz += dz * width / max_width / 2;
    }
    sx += n * line_height * ldx;
    sy += n * line_height * ldy;
    sz += n * line_height * ldz;
    line = tokenize(NULL, "\n", &key);
    rows--;
    if (rows <= 0) {
      break;
    }
  }
  return count;
}

int has_lights(const Chunk *const chunk) {
  if (!SHOW_LIGHTS) {
    return 0;
  }
  for (int dp = -1; dp <= 1; dp++) {
    for (int dq = -1; dq <= 1; dq++) {
      const Chunk *const other_chunk =
          (dp || dq) ? find_chunk(chunk->p + dp, chunk->q + dq) : chunk;
      if (!other_chunk) {
        continue;
      }
      const Map *const map = &other_chunk->lights;
      if (map->size) {
        return 1;
      }
    }
  }
  return 0;
}

void dirty_chunk(Chunk *const chunk) {
  chunk->dirty = 1;
  if (has_lights(chunk)) {
    for (int dp = -1; dp <= 1; dp++) {
      for (int dq = -1; dq <= 1; dq++) {
        Chunk *const other_chunk = find_chunk(chunk->p + dp, chunk->q + dq);
        if (other_chunk) {
          other_chunk->dirty = 1;
        }
      }
    }
  }
}

void occlusion(char neighbors[27], char lights[27], float shades[27],
               float ao[6][4], float light[6][4]) {
  static const int lookup3[6][4][3] = {
      {{0, 1, 3}, {2, 1, 5}, {6, 3, 7}, {8, 5, 7}},
      {{18, 19, 21}, {20, 19, 23}, {24, 21, 25}, {26, 23, 25}},
      {{6, 7, 15}, {8, 7, 17}, {24, 15, 25}, {26, 17, 25}},
      {{0, 1, 9}, {2, 1, 11}, {18, 9, 19}, {20, 11, 19}},
      {{0, 3, 9}, {6, 3, 15}, {18, 9, 21}, {24, 15, 21}},
      {{2, 5, 11}, {8, 5, 17}, {20, 11, 23}, {26, 17, 23}}};
  static const int lookup4[6][4][4] = {
      {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}, {4, 5, 7, 8}},
      {{18, 19, 21, 22}, {19, 20, 22, 23}, {21, 22, 24, 25}, {22, 23, 25, 26}},
      {{6, 7, 15, 16}, {7, 8, 16, 17}, {15, 16, 24, 25}, {16, 17, 25, 26}},
      {{0, 1, 9, 10}, {1, 2, 10, 11}, {9, 10, 18, 19}, {10, 11, 19, 20}},
      {{0, 3, 9, 12}, {3, 6, 12, 15}, {9, 12, 18, 21}, {12, 15, 21, 24}},
      {{2, 5, 11, 14}, {5, 8, 14, 17}, {11, 14, 20, 23}, {14, 17, 23, 26}}};
  static const float curve[4] = {0.0, 0.25, 0.5, 0.75};
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      const int corner = neighbors[lookup3[i][j][0]];
      const int side1 = neighbors[lookup3[i][j][1]];
      const int side2 = neighbors[lookup3[i][j][2]];
      const int value = side1 && side2 ? 3 : corner + side1 + side2;
      float shade_sum = 0;
      float light_sum = 0;
      const int is_light = lights[13] == 15;
      for (int k = 0; k < 4; k++) {
        shade_sum += shades[lookup4[i][j][k]];
        light_sum += lights[lookup4[i][j][k]];
      }
      if (is_light) {
        light_sum = 15 * 4 * 10;
      }
      const float total = curve[value] + shade_sum / 4.0;
      ao[i][j] = MIN(total, 1.0);
      light[i][j] = light_sum / 15.0 / 4.0;
    }
  }
}

#define XZ_SIZE (CHUNK_SIZE * 3 + 2)
#define XZ_LO (CHUNK_SIZE)
#define XZ_HI (CHUNK_SIZE * 2 + 1)
#define Y_SIZE 258
#define XYZ(x, y, z) ((y)*XZ_SIZE * XZ_SIZE + (x)*XZ_SIZE + (z))
#define XZ(x, z) ((x)*XZ_SIZE + (z))

void light_fill(const char *const opaque, char *light, int x, int y, int z,
                int w, int force) {
  if (x + w < XZ_LO || z + w < XZ_LO) {
    return;
  }
  if (x - w > XZ_HI || z - w > XZ_HI) {
    return;
  }
  if (y < 0 || y >= Y_SIZE) {
    return;
  }
  if (light[XYZ(x, y, z)] >= w) {
    return;
  }
  if (!force && opaque[XYZ(x, y, z)]) {
    return;
  }
  light[XYZ(x, y, z)] = w--;
  light_fill(opaque, light, x - 1, y, z, w, 0);
  light_fill(opaque, light, x + 1, y, z, w, 0);
  light_fill(opaque, light, x, y - 1, z, w, 0);
  light_fill(opaque, light, x, y + 1, z, w, 0);
  light_fill(opaque, light, x, y, z - 1, w, 0);
  light_fill(opaque, light, x, y, z + 1, w, 0);
}

void compute_chunk(WorkerItem *item) {
  char *const opaque = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
  char *const light = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
  char *const highest = (char *)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

  const int ox = item->p * CHUNK_SIZE - CHUNK_SIZE - 1;
  const int oy = -1;
  const int oz = item->q * CHUNK_SIZE - CHUNK_SIZE - 1;

  // check for lights
  int has_light = 0;
  if (SHOW_LIGHTS) {
    for (int a = 0; a < 3; a++) {
      for (int b = 0; b < 3; b++) {
        Map *map = item->light_maps[a][b];
        if (map && map->size) {
          has_light = 1;
        }
      }
    }
  }

  // populate opaque array
  for (int a = 0; a < 3; a++) {
    for (int b = 0; b < 3; b++) {
      const Map *const map = item->block_maps[a][b];
      if (!map) {
        continue;
      }
      for (unsigned int i = 0; i <= map->mask; i++) {
        const MapEntry *const entry = map->data + i;
        if (EMPTY_ENTRY(entry)) {
          continue;
        }
        const int ex = entry->e.x + map->dx;
        const int ey = entry->e.y + map->dy;
        const int ez = entry->e.z + map->dz;
        const int ew = entry->e.w;
        const int x = ex - ox;
        const int y = ey - oy;
        const int z = ez - oz;
        const int w = ew;
        // TODO: this should be unnecessary
        if (x < 0 || y < 0 || z < 0) {
          continue;
        }
        if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE) {
          continue;
        }
        // END TODO
        opaque[XYZ(x, y, z)] = !is_transparent(w);
        if (opaque[XYZ(x, y, z)]) {
          highest[XZ(x, z)] = MAX(highest[XZ(x, z)], y);
        }
      }
    }
  }

  // flood fill light intensities
  if (has_light) {
    for (int a = 0; a < 3; a++) {
      for (int b = 0; b < 3; b++) {
        const Map *const map = item->light_maps[a][b];
        if (!map) {
          continue;
        }
        for (unsigned int i = 0; i <= map->mask; i++) {
          const MapEntry *const entry = map->data + i;
          if (EMPTY_ENTRY(entry)) {
            continue;
          }
          const int ex = entry->e.x + map->dx;
          const int ey = entry->e.y + map->dy;
          const int ez = entry->e.z + map->dz;
          const int ew = entry->e.w;
          const int x = ex - ox;
          const int y = ey - oy;
          const int z = ez - oz;
          light_fill(opaque, light, x, y, z, ew, 1);
        }
      }
    }
  }

  const Map *const map = item->block_maps[1][1];

  // count exposed faces
  int miny = 256;
  int maxy = 0;
  int faces = 0;
  for (unsigned int i = 0; i <= map->mask; i++) {
    const MapEntry *const entry = map->data + i;
    if (EMPTY_ENTRY(entry)) {
      continue;
    }
    const int ex = entry->e.x + map->dx;
    const int ey = entry->e.y + map->dy;
    const int ez = entry->e.z + map->dz;
    const int ew = entry->e.w;
    if (ew <= 0) {
      continue;
    }
    const int x = ex - ox;
    const int y = ey - oy;
    const int z = ez - oz;
    const int f1 = !opaque[XYZ(x - 1, y, z)];
    const int f2 = !opaque[XYZ(x + 1, y, z)];
    const int f3 = !opaque[XYZ(x, y + 1, z)];
    const int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
    const int f5 = !opaque[XYZ(x, y, z - 1)];
    const int f6 = !opaque[XYZ(x, y, z + 1)];
    const int total = f1 + f2 + f3 + f4 + f5 + f6;
    if (total == 0) {
      continue;
    }
    miny = MIN(miny, ey);
    maxy = MAX(maxy, ey);
    faces += is_plant(ew) ? 4 : total;
  }

  // generate geometry
  float *const data = malloc_faces(10, faces);
  int offset = 0;
  for (unsigned int i = 0; i <= map->mask; i++) {
    const MapEntry *const entry = map->data + i;
    if (EMPTY_ENTRY(entry)) {
      continue;
    }
    const int ex = entry->e.x + map->dx;
    const int ey = entry->e.y + map->dy;
    const int ez = entry->e.z + map->dz;
    const int ew = entry->e.w;
    if (ew <= 0) {
      continue;
    }
    const int x = ex - ox;
    const int y = ey - oy;
    const int z = ez - oz;
    const int f1 = !opaque[XYZ(x - 1, y, z)];
    const int f2 = !opaque[XYZ(x + 1, y, z)];
    const int f3 = !opaque[XYZ(x, y + 1, z)];
    const int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
    const int f5 = !opaque[XYZ(x, y, z - 1)];
    const int f6 = !opaque[XYZ(x, y, z + 1)];
    const int total = f1 + f2 + f3 + f4 + f5 + f6;
    if (total == 0) {
      continue;
    }
    char neighbors[27] = {0};
    char lights[27] = {0};
    float shades[27] = {0};
    int index = 0;
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        for (int dz = -1; dz <= 1; dz++) {
          neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
          lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
          shades[index] = 0;
          if (y + dy <= highest[XZ(x + dx, z + dz)]) {
            for (int oy = 0; oy < 8; oy++) {
              if (opaque[XYZ(x + dx, y + dy + oy, z + dz)]) {
                shades[index] = 1.0 - oy * 0.125;
                break;
              }
            }
          }
          index++;
        }
      }
    }
    float ao[6][4];
    float light[6][4];
    occlusion(neighbors, lights, shades, ao, light);
    if (is_plant(ew)) {
      float min_ao = 1;
      float max_light = 0;
      for (int a = 0; a < 6; a++) {
        for (int b = 0; b < 4; b++) {
          min_ao = MIN(min_ao, ao[a][b]);
          max_light = MAX(max_light, light[a][b]);
        }
      }
      float rotation = simplex2(ex, ez, 4, 0.5, 2) * 360;
      make_plant(data + offset, min_ao, max_light, ex, ey, ez, 0.5, ew,
                 rotation);
    } else {
      make_cube(data + offset, ao, light, f1, f2, f3, f4, f5, f6, ex, ey, ez,
                0.5, ew);
    }
    offset += (is_plant(ew) ? 4 : total) * 60;
  }

  free(opaque);
  free(light);
  free(highest);

  item->miny = miny;
  item->maxy = maxy;
  item->faces = faces;
  item->data = data;
}

void generate_chunk(Chunk *const chunk, WorkerItem *const item) {
  chunk->miny = item->miny;
  chunk->maxy = item->maxy;
  chunk->faces = item->faces;
  (*renderer.del_buffer)(chunk->buffer);
  chunk->buffer = (*renderer.gen_faces)(10, item->faces, item->data);

  // generate sign buffer
  const SignList *const signs = &chunk->signs;

  // first pass - count characters
  int maxFaces = 0;
  for (int i = 0; i < signs->size; i++) {
    const Sign *const e = signs->data + i;
    maxFaces += strlen(e->text);
  }

  // second pass - generate geometry
  float *const data = malloc_faces(5, maxFaces);
  int faces = 0;
  for (int i = 0; i < signs->size; i++) {
    const Sign *const e = signs->data + i;
    faces +=
        _gen_sign_buffer(data + faces * 30, e->x, e->y, e->z, e->face, e->text);
  }

  (*renderer.del_buffer)(chunk->sign_buffer);
  chunk->sign_buffer = (*renderer.gen_faces)(5, faces, data);
  chunk->sign_faces = faces;
}

void gen_chunk_buffer(Chunk *const chunk) {
  WorkerItem _item;
  WorkerItem *const item = &_item;
  {
    item->p = chunk->p;
    item->q = chunk->q;
  }
  for (int dp = -1; dp <= 1; dp++) {
    for (int dq = -1; dq <= 1; dq++) {
      Chunk *const other_chunk =
          (dp || dq) ? find_chunk(chunk->p + dp, chunk->q + dq) : chunk;
      if (other_chunk) {
        item->block_maps[dp + 1][dq + 1] = &other_chunk->map;
        item->light_maps[dp + 1][dq + 1] = &other_chunk->lights;
      } else {
        item->block_maps[dp + 1][dq + 1] = 0;
        item->light_maps[dp + 1][dq + 1] = 0;
      }
    }
  }
  compute_chunk(item);
  generate_chunk(chunk, item);
  chunk->dirty = 0;
}

void map_set_func(int x, int y, int z, int w, void *arg) {
  Map *map = (Map *)arg;
  map_set(map, x, y, z, w);
}

void load_chunk(WorkerItem *item) {
  const int p = item->p;
  const int q = item->q;
  Map *const block_map = item->block_maps[1][1];
  Map *const light_map = item->light_maps[1][1];
  create_world(p, q, map_set_func, block_map);
  db_load_blocks(block_map, p, q);
  db_load_lights(light_map, p, q);
}

void request_chunk(int p, int q) {
  const int key = db_get_key(p, q);
  client_chunk(p, q, key);
}

void init_chunk(Chunk *chunk, int p, int q) {
  // initilize the chunk's values
  {
    chunk->p = p;
    chunk->q = q;
    chunk->faces = 0;
    chunk->sign_faces = 0;
    chunk->buffer = 0;
    chunk->sign_buffer = 0;
  }
  dirty_chunk(chunk);
  SignList *const signs = &chunk->signs;
  sign_list_alloc(signs, 16);
  db_load_signs(signs, p, q);
  Map *const block_map = &chunk->map;
  Map *const light_map = &chunk->lights;
  const int dx = p * CHUNK_SIZE - 1;
  const int dy = 0;
  const int dz = q * CHUNK_SIZE - 1;
  map_alloc(block_map, dx, dy, dz, 0x7fff);
  map_alloc(light_map, dx, dy, dz, 0xf);
}

void create_chunk(Chunk *chunk, int p, int q) {
  init_chunk(chunk, p, q);

  WorkerItem _item;
  WorkerItem *item = &_item;
  {
    item->p = chunk->p;
    item->q = chunk->q;
    item->block_maps[1][1] = &chunk->map;
    item->light_maps[1][1] = &chunk->lights;
  }
  load_chunk(item);

  request_chunk(p, q);
}

void delete_chunks() {
  int count = g->chunk_count;
  PositionAndOrientation *const positionAndOrientation1 =
      &g->players->positionAndOrientation;
  PositionAndOrientation *const positionAndOrientation2 =
      &(g->players + g->observe1)->positionAndOrientation;
  PositionAndOrientation *const positionAndOrientation3 =
      &(g->players + g->observe2)->positionAndOrientation;
  PositionAndOrientation *const positionsAndOrientations[3] = {
      positionAndOrientation1, positionAndOrientation2,
      positionAndOrientation3};
  for (int i = 0; i < count; i++) {
    Chunk *const chunk = g->chunks + i;
    int delete = 1;
    for (int j = 0; j < 3; j++) {
      PositionAndOrientation *positionAndOrientation =
          positionsAndOrientations[j];
      const int p = chunked(positionAndOrientation->x);
      const int q = chunked(positionAndOrientation->z);
      if (chunk_distance(chunk, p, q) < g->delete_radius) {
        delete = 0;
        break;
      }
    }
    if (delete) {
      map_free(&chunk->map);
      map_free(&chunk->lights);
      sign_list_free(&chunk->signs);
      (*renderer.del_buffer)(chunk->buffer);
      (*renderer.del_buffer)(chunk->sign_buffer);
      Chunk *other_chunk = g->chunks + (--count);
      memcpy(chunk, other_chunk, sizeof(Chunk));
    }
  }
  g->chunk_count = count;
}

void delete_all_chunks() {
  for (int i = 0; i < g->chunk_count; i++) {
    Chunk *const chunk = g->chunks + i;
    map_free(&chunk->map);
    map_free(&chunk->lights);
    sign_list_free(&chunk->signs);
    (*renderer.del_buffer)(chunk->buffer);
    (*renderer.del_buffer)(chunk->sign_buffer);
  }
  g->chunk_count = 0;
}

void check_workers() {
  for (int i = 0; i < WORKERS; i++) {
    Worker *const worker = g->workers + i;
    mtx_lock(&worker->mtx);
    if (worker->state == WORKER_DONE) {
      WorkerItem *const item = &worker->item;
      Chunk *const chunk = find_chunk(item->p, item->q);
      if (chunk) {
        if (item->load) {
          Map *block_map = item->block_maps[1][1];
          Map *light_map = item->light_maps[1][1];
          map_free(&chunk->map);
          map_free(&chunk->lights);
          map_copy(&chunk->map, block_map);
          map_copy(&chunk->lights, light_map);
          request_chunk(item->p, item->q);
        }
        generate_chunk(chunk, item);
      }
      for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 3; b++) {
          Map *const block_map = item->block_maps[a][b];
          Map *const light_map = item->light_maps[a][b];
          if (block_map) {
            map_free(block_map);
            free(block_map);
          }
          if (light_map) {
            map_free(light_map);
            free(light_map);
          }
        }
      }
      worker->state = WORKER_IDLE;
    }
    mtx_unlock(&worker->mtx);
  }
}

void force_chunks(Player *player) {
  const PositionAndOrientation *const positionAndOrientation =
      &player->positionAndOrientation;
  const int p = chunked(positionAndOrientation->x);
  const int q = chunked(positionAndOrientation->z);
  const int r = 1;
  for (int dp = -r; dp <= r; dp++) {
    for (int dq = -r; dq <= r; dq++) {
      const int a = p + dp;
      const int b = q + dq;
      Chunk *chunk = find_chunk(a, b);
      if (chunk) {
        if (chunk->dirty) {
          gen_chunk_buffer(chunk);
        }
      } else if (g->chunk_count < MAX_CHUNKS) {
        chunk = g->chunks + g->chunk_count++;
        create_chunk(chunk, a, b);
        gen_chunk_buffer(chunk);
      }
    }
  }
}

void ensure_chunks_worker(Player *player, Worker *worker) {
  const PositionAndOrientation *const positionAndOrientation =
      &player->positionAndOrientation;
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);
  float planes[6][4];
  frustum_planes(planes, g->render_radius, matrix);
  const int p = chunked(positionAndOrientation->x);
  const int q = chunked(positionAndOrientation->z);
  const int r = g->create_radius;
  const int start = 0x0fffffff;
  int best_score = start;
  int best_a = 0;
  int best_b = 0;
  for (int dp = -r; dp <= r; dp++) {
    for (int dq = -r; dq <= r; dq++) {
      const int a = p + dp;
      const int b = q + dq;
      const int index = (ABS(a) ^ ABS(b)) % WORKERS;
      if (index != worker->index) {
        continue;
      }
      Chunk *chunk = find_chunk(a, b);
      if (chunk && !chunk->dirty) {
        continue;
      }
      const int distance = MAX(ABS(dp), ABS(dq));
      const int invisible = !chunk_visible(planes, a, b, 0, 256);
      int priority = 0;
      if (chunk) {
        priority = chunk->buffer && chunk->dirty;
      }
      int score = (invisible << 24) | (priority << 16) | distance;
      if (score < best_score) {
        best_score = score;
        best_a = a;
        best_b = b;
      }
    }
  }
  if (best_score == start) {
    return;
  }
  const int a = best_a;
  const int b = best_b;
  int load = 0;
  Chunk *chunk = find_chunk(a, b);
  if (!chunk) {
    load = 1;
    if (g->chunk_count < MAX_CHUNKS) {
      chunk = g->chunks + g->chunk_count++;
      init_chunk(chunk, a, b);
    } else {
      return;
    }
  }
  WorkerItem *const item = &worker->item;
  {
    item->p = chunk->p;
    item->q = chunk->q;
    item->load = load;
  }
  for (int dp = -1; dp <= 1; dp++) {
    for (int dq = -1; dq <= 1; dq++) {
      Chunk *other_chunk = chunk;
      if (dp || dq) {
        other_chunk = find_chunk(chunk->p + dp, chunk->q + dq);
      }
      if (other_chunk) {
        Map *const block_map = malloc(sizeof(Map));
        {
          // initialize the map
          map_copy(block_map, &other_chunk->map);
        }
        Map *const light_map = malloc(sizeof(Map));
        {
          // initialize the map
          map_copy(light_map, &other_chunk->lights);
        }
        item->block_maps[dp + 1][dq + 1] = block_map;
        item->light_maps[dp + 1][dq + 1] = light_map;
      } else {
        item->block_maps[dp + 1][dq + 1] = 0;
        item->light_maps[dp + 1][dq + 1] = 0;
      }
    }
  }
  chunk->dirty = 0;
  worker->state = WORKER_BUSY;
  cnd_signal(&worker->cnd);
}

void ensure_chunks(Player *player) {
  check_workers();
  force_chunks(player);
  for (int i = 0; i < WORKERS; i++) {
    Worker *const worker = g->workers + i;
    mtx_lock(&worker->mtx);
    if (worker->state == WORKER_IDLE) {
      ensure_chunks_worker(player, worker);
    }
    mtx_unlock(&worker->mtx);
  }
}

int worker_run(void *arg) {
  Worker *const worker = (Worker *)arg;
  int running = 1;
  while (running) {
    mtx_lock(&worker->mtx);
    while (worker->state != WORKER_BUSY) {
      cnd_wait(&worker->cnd, &worker->mtx);
    }
    mtx_unlock(&worker->mtx);
    WorkerItem *item = &worker->item;
    if (item->load) {
      load_chunk(item);
    }
    compute_chunk(item);
    mtx_lock(&worker->mtx);
    worker->state = WORKER_DONE;
    mtx_unlock(&worker->mtx);
  }
  return 0;
}

void unset_sign(int x, int y, int z) {
  const int p = chunked(x);
  const int q = chunked(z);
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    SignList *const signs = &chunk->signs;
    if (sign_list_remove_all(signs, x, y, z)) {
      chunk->dirty = 1;
      db_delete_signs(x, y, z);
    }
  } else {
    db_delete_signs(x, y, z);
  }
}

void unset_sign_face(int x, int y, int z, int face) {
  const int p = chunked(x);
  const int q = chunked(z);
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    SignList *const signs = &chunk->signs;
    if (sign_list_remove(signs, x, y, z, face)) {
      chunk->dirty = 1;
      db_delete_sign(x, y, z, face);
    }
  } else {
    db_delete_sign(x, y, z, face);
  }
}

void _set_sign(int p, int q, int x, int y, int z, int face, const char *text,
               int dirty) {
  if (strlen(text) == 0) {
    unset_sign_face(x, y, z, face);
    return;
  }
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    SignList *const signs = &chunk->signs;
    sign_list_add(signs, x, y, z, face, text);
    if (dirty) {
      chunk->dirty = 1;
    }
  }
  db_insert_sign(p, q, x, y, z, face, text);
}

void set_sign(int x, int y, int z, int face, const char *text) {
  const int p = chunked(x);
  const int q = chunked(z);
  _set_sign(p, q, x, y, z, face, text, 1);
  client_sign(x, y, z, face, text);
}

void toggle_light(int x, int y, int z) {
  const int p = chunked(x);
  const int q = chunked(z);
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    Map *map = &chunk->lights;
    const int w = map_get(map, x, y, z) ? 0 : 15;
    map_set(map, x, y, z, w);
    db_insert_light(p, q, x, y, z, w);
    client_light(x, y, z, w);
    dirty_chunk(chunk);
  }
}

void set_light(int p, int q, int x, int y, int z, int w) {
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    Map *const map = &chunk->lights;
    if (map_set(map, x, y, z, w)) {
      dirty_chunk(chunk);
      db_insert_light(p, q, x, y, z, w);
    }
  } else {
    db_insert_light(p, q, x, y, z, w);
  }
}

void _set_block(int p, int q, int x, int y, int z, int w, int dirty) {
  Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    Map *map = &chunk->map;
    if (map_set(map, x, y, z, w)) {
      if (dirty) {
        dirty_chunk(chunk);
      }
      db_insert_block(p, q, x, y, z, w);
    }
  } else {
    db_insert_block(p, q, x, y, z, w);
  }
  if (w == 0 && chunked(x) == p && chunked(z) == q) {
    unset_sign(x, y, z);
    set_light(p, q, x, y, z, 0);
  }
}

/*
 * TODO - figure out what these values are
 * I assume that x y z are positions within
 * a map, and w looks like it set's the type
 * of block.  I had first assumed that the
 * w was homogeneous coordinates
 */
void set_block(int x, int y, int z, int w) {
  const int p = chunked(x);
  const int q = chunked(z);
  _set_block(p, q, x, y, z, w, 1);
  for (int dx = -1; dx <= 1; dx++) {
    for (int dz = -1; dz <= 1; dz++) {
      if (dx == 0 && dz == 0) {
        continue;
      }
      if (dx && chunked(x + dx) == p) {
        continue;
      }
      if (dz && chunked(z + dz) == q) {
        continue;
      }
      _set_block(p + dx, q + dz, x, y, z, -w, 1);
    }
  }
  client_block(x, y, z, w);
}

void record_block(int x, int y, int z, int w) {
  memcpy(&g->block1, &g->block0, sizeof(Block));
  g->block0.x = x;
  g->block0.y = y;
  g->block0.z = z;
  g->block0.w = w;
}

int get_block(int x, int y, int z) {
  const int p = chunked(x);
  const int q = chunked(z);
  const Chunk *const chunk = find_chunk(p, q);
  if (chunk) {
    const Map *const map = &chunk->map;
    return map_get(map, x, y, z);
  }
  return 0;
}

void builder_block(int x, int y, int z, int w) {
  if (y <= 0 || y >= 256) {
    return;
  }
  if (is_destructable(get_block(x, y, z))) {
    set_block(x, y, z, 0);
  }
  if (w) {
    set_block(x, y, z, w);
  }
}

int render_chunks(Player *player) {
  int result = 0;
  const PositionAndOrientation *const positionAndOrientation =
      &player->positionAndOrientation;
  ensure_chunks(player);
  const int p = chunked(positionAndOrientation->x);
  const int q = chunked(positionAndOrientation->z);

  float light;
  // initilize light
  {
    // daylight
    float timer = time_of_day();
    if (timer < 0.5) {
      float t = (timer - 0.25) * 100;
      light = 1 / (1 + powf(2, -t));
    } else {
      float t = (timer - 0.85) * 100;
      light = 1 - 1 / (1 + powf(2, -t));
    }
  }
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);
  float planes[6][4];
  frustum_planes(planes, g->render_radius, matrix);

  gl_setup_render_chunks(matrix, positionAndOrientation, light);

  // N.B.
  // To See what a chunk is, change this loop to
  // only iterate once
#ifdef ENABLE_ONLY_RENDER_ONE_CHUNK
  for (int i = 0; i < 1; i++) {
#else
  for (int i = 0; i < g->chunk_count; i++) {
#endif
    const Chunk *const chunk = g->chunks + i;
    if (chunk_distance(chunk, p, q) > g->render_radius) {
      continue;
    }
    if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy)) {
      continue;
    }
    if (chunk->buffer == 0) {
      continue;
    }

    (*renderer.render_chunk)(chunk);

    result += chunk->faces;
  }
  return result;
}

void draw_triangles_3d_text(uint32_t buffer, int count) {
  gl_draw_triangles_3d_text(buffer, count);
}

void render_signs(Player *player) {
  PositionAndOrientation *positionAndOrientation =
      &player->positionAndOrientation;
  int p = chunked(positionAndOrientation->x);
  int q = chunked(positionAndOrientation->z);
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);
  float planes[6][4];
  frustum_planes(planes, g->render_radius, matrix);

  gl_setup_render_signs(matrix);

  for (int i = 0; i < g->chunk_count; i++) {
    Chunk *chunk = g->chunks + i;
    if (chunk_distance(chunk, p, q) > g->sign_radius) {
      continue;
    }
    if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy)) {
      continue;
    }
    (*renderer.render_signs)(chunk);
  }
}

void render_sign(Player *player) {
  if (!g->typing || g->typing_buffer[0] != CRAFT_KEY_SIGN) {
    return;
  }
  int x, y, z, face;
  if (!hit_test_face(player, &x, &y, &z, &face)) {
    return;
  }
  PositionAndOrientation *positionAndOrientation =
      &player->positionAndOrientation;
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);

  (*renderer.render_sign)(matrix, x, y, z, face);
}

void render_players(Player *player) {
  PositionAndOrientation *positionAndOrientation =
      &player->positionAndOrientation;
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);

  gl_setup_render_players(matrix, positionAndOrientation);

  for (int i = 0; i < g->player_count; i++) {
    Player *other_player = g->players + i;
    if (other_player != player) {
      // draw player
      if (other_player->buffer == 0) {
        continue;
      }
      (*renderer.render_player)(other_player);
    }
  }
}

void render_sky(Player *player, uint32_t buffer) {
  PositionAndOrientation *positionAndOrientation =
      &player->positionAndOrientation;
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, 0, 0, 0,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                0, g->render_radius);

  (*renderer.render_sky)(buffer, matrix);
}

void draw_lines(uint32_t buffer, int components, int count) {
  gl_draw_lines(buffer, components, count);
}

void render_wireframe(Player *player) {
  PositionAndOrientation *positionAndOrientation =
      &player->positionAndOrientation;
  float matrix[16];
  set_matrix_3d(matrix, g->width, g->height, positionAndOrientation->x,
                positionAndOrientation->y, positionAndOrientation->z,
                positionAndOrientation->rx, positionAndOrientation->ry, g->fov,
                g->ortho, g->render_radius);
  int hx, hy, hz;
  int hw = hit_test(0, positionAndOrientation->x, positionAndOrientation->y,
                    positionAndOrientation->z, positionAndOrientation->rx,
                    positionAndOrientation->ry, &hx, &hy, &hz);
  if (is_obstacle(hw)) {
    (*renderer.render_wireframe)(matrix, hx, hy, hz);
  }
}

void render_text(int justify, float x, float y, float n, char *text) {
  if (!do_render_text) {
    return;
  }

  float matrix[16];
  set_matrix_2d(matrix, g->width, g->height);
  (*renderer.render_text)(matrix, justify, x, y, n, text);
}

void add_message(const char *text) {
  printf("%s\n", text);
  snprintf(g->messages[g->message_index], MAX_TEXT_LENGTH, "%s", text);
  g->message_index = (g->message_index + 1) % MAX_MESSAGES;
}

void login() {
  char username[128] = {0};
  char identity_token[128] = {0};
  char access_token[128] = {0};
  if (db_auth_get_selected(username, 128, identity_token, 128)) {
    printf("Contacting login server for username: %s\n", username);
    if (get_access_token(access_token, 128, username, identity_token)) {
      printf("Successfully authenticated with the login server\n");
      client_login(username, access_token);
    } else {
      printf("Failed to authenticate with the login server\n");
      client_login("", "");
    }
  } else {
    printf("Logging in anonymously\n");
    client_login("", "");
  }
}

void copy() {
  memcpy(&g->copy0, &g->block0, sizeof(Block));
  memcpy(&g->copy1, &g->block1, sizeof(Block));
}

void paste() {
  Block *c1 = &g->copy1;
  Block *c2 = &g->copy0;
  Block *p1 = &g->block1;
  Block *p2 = &g->block0;
  int scx = SIGN(c2->x - c1->x);
  int scz = SIGN(c2->z - c1->z);
  int spx = SIGN(p2->x - p1->x);
  int spz = SIGN(p2->z - p1->z);
  int oy = p1->y - c1->y;
  int dx = ABS(c2->x - c1->x);
  int dz = ABS(c2->z - c1->z);
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x <= dx; x++) {
      for (int z = 0; z <= dz; z++) {
        int w = get_block(c1->x + x * scx, y, c1->z + z * scz);
        builder_block(p1->x + x * spx, y + oy, p1->z + z * spz, w);
      }
    }
  }
}

void array(Block *b1, Block *b2, int xc, int yc, int zc) {
  if (b1->w != b2->w) {
    return;
  }
  int w = b1->w;
  int dx = b2->x - b1->x;
  int dy = b2->y - b1->y;
  int dz = b2->z - b1->z;
  xc = dx ? xc : 1;
  yc = dy ? yc : 1;
  zc = dz ? zc : 1;
  for (int i = 0; i < xc; i++) {
    int x = b1->x + dx * i;
    for (int j = 0; j < yc; j++) {
      int y = b1->y + dy * j;
      for (int k = 0; k < zc; k++) {
        int z = b1->z + dz * k;
        builder_block(x, y, z, w);
      }
    }
  }
}

void cube(Block *b1, Block *b2, int fill) {
  if (b1->w != b2->w) {
    return;
  }
  int w = b1->w;
  int x1 = MIN(b1->x, b2->x);
  int y1 = MIN(b1->y, b2->y);
  int z1 = MIN(b1->z, b2->z);
  int x2 = MAX(b1->x, b2->x);
  int y2 = MAX(b1->y, b2->y);
  int z2 = MAX(b1->z, b2->z);
  int a = (x1 == x2) + (y1 == y2) + (z1 == z2);
  for (int x = x1; x <= x2; x++) {
    for (int y = y1; y <= y2; y++) {
      for (int z = z1; z <= z2; z++) {
        if (!fill) {
          int n = 0;
          n += x == x1 || x == x2;
          n += y == y1 || y == y2;
          n += z == z1 || z == z2;
          if (n <= a) {
            continue;
          }
        }
        builder_block(x, y, z, w);
      }
    }
  }
}

void sphere(Block *center, int radius, int fill, int fx, int fy, int fz) {
  static const float offsets[8][3] = {{-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5},
                                      {-0.5, 0.5, -0.5},  {-0.5, 0.5, 0.5},
                                      {0.5, -0.5, -0.5},  {0.5, -0.5, 0.5},
                                      {0.5, 0.5, -0.5},   {0.5, 0.5, 0.5}};
  int cx = center->x;
  int cy = center->y;
  int cz = center->z;
  int w = center->w;
  for (int x = cx - radius; x <= cx + radius; x++) {
    if (fx && x != cx) {
      continue;
    }
    for (int y = cy - radius; y <= cy + radius; y++) {
      if (fy && y != cy) {
        continue;
      }
      for (int z = cz - radius; z <= cz + radius; z++) {
        if (fz && z != cz) {
          continue;
        }
        int inside = 0;
        int outside = fill;
        for (int i = 0; i < 8; i++) {
          float dx = x + offsets[i][0] - cx;
          float dy = y + offsets[i][1] - cy;
          float dz = z + offsets[i][2] - cz;
          float d = sqrtf(dx * dx + dy * dy + dz * dz);
          if (d < radius) {
            inside = 1;
          } else {
            outside = 1;
          }
        }
        if (inside && outside) {
          builder_block(x, y, z, w);
        }
      }
    }
  }
}

void cylinder(Block *b1, Block *b2, int radius, int fill) {
  if (b1->w != b2->w) {
    return;
  }
  int w = b1->w;
  int x1 = MIN(b1->x, b2->x);
  int y1 = MIN(b1->y, b2->y);
  int z1 = MIN(b1->z, b2->z);
  int x2 = MAX(b1->x, b2->x);
  int y2 = MAX(b1->y, b2->y);
  int z2 = MAX(b1->z, b2->z);
  int fx = x1 != x2;
  int fy = y1 != y2;
  int fz = z1 != z2;
  if (fx + fy + fz != 1) {
    return;
  }
  Block block = {x1, y1, z1, w};
  if (fx) {
    for (int x = x1; x <= x2; x++) {
      block.x = x;
      sphere(&block, radius, fill, 1, 0, 0);
    }
  }
  if (fy) {
    for (int y = y1; y <= y2; y++) {
      block.y = y;
      sphere(&block, radius, fill, 0, 1, 0);
    }
  }
  if (fz) {
    for (int z = z1; z <= z2; z++) {
      block.z = z;
      sphere(&block, radius, fill, 0, 0, 1);
    }
  }
}

void tree(Block *block) {
  int bx = block->x;
  int by = block->y;
  int bz = block->z;
  for (int y = by + 3; y < by + 8; y++) {
    for (int dx = -3; dx <= 3; dx++) {
      for (int dz = -3; dz <= 3; dz++) {
        int dy = y - (by + 4);
        int d = (dx * dx) + (dy * dy) + (dz * dz);
        if (d < 11) {
          builder_block(bx + dx, y, bz + dz, 15);
        }
      }
    }
  }
  for (int y = by; y < by + 7; y++) {
    builder_block(bx, y, bz, 5);
  }
}

void parse_command(const char *buffer, int forward) {
  char username[128] = {0};
  char token[128] = {0};
  char server_addr[MAX_ADDR_LENGTH];
  int server_port = DEFAULT_PORT;
  char filename[MAX_PATH_LENGTH];
  int radius, count, xc, yc, zc;
  if (sscanf(buffer, "/identity %128s %128s", username, token) == 2) {
    db_auth_set(username, token);
    add_message("Successfully imported identity token!");
    login();
  } else if (strcmp(buffer, "/logout") == 0) {
    db_auth_select_none();
    login();
  } else if (sscanf(buffer, "/login %128s", username) == 1) {
    if (db_auth_select(username)) {
      login();
    } else {
      add_message("Unknown username.");
    }
  } else if (sscanf(buffer, "/online %128s %d", server_addr, &server_port) >=
             1) {
    g->mode_changed = 1;
    g->mode = MODE_ONLINE;
    strncpy(g->server_addr, server_addr, MAX_ADDR_LENGTH);
    g->server_port = server_port;
    snprintf(g->db_path, MAX_PATH_LENGTH, "cache.%s.%d.db", g->server_addr,
             g->server_port);
  } else if (sscanf(buffer, "/offline %128s", filename) == 1) {
    g->mode_changed = 1;
    g->mode = MODE_OFFLINE;
    snprintf(g->db_path, MAX_PATH_LENGTH, "%s.db", filename);
  } else if (strcmp(buffer, "/offline") == 0) {
    g->mode_changed = 1;
    g->mode = MODE_OFFLINE;
    snprintf(g->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
  } else if (sscanf(buffer, "/view %d", &radius) == 1) {
    if (radius >= 1 && radius <= 24) {
      g->create_radius = radius;
      g->render_radius = radius;
      g->delete_radius = radius + 4;
    } else {
      add_message("Viewing distance must be between 1 and 24.");
    }
  } else if (strcmp(buffer, "/copy") == 0) {
    copy();
  } else if (strcmp(buffer, "/paste") == 0) {
    paste();
  } else if (strcmp(buffer, "/tree") == 0) {
    tree(&g->block0);
  } else if (sscanf(buffer, "/array %d %d %d", &xc, &yc, &zc) == 3) {
    array(&g->block1, &g->block0, xc, yc, zc);
  } else if (sscanf(buffer, "/array %d", &count) == 1) {
    array(&g->block1, &g->block0, count, count, count);
  } else if (strcmp(buffer, "/fcube") == 0) {
    cube(&g->block0, &g->block1, 1);
  } else if (strcmp(buffer, "/cube") == 0) {
    cube(&g->block0, &g->block1, 0);
  } else if (sscanf(buffer, "/fsphere %d", &radius) == 1) {
    sphere(&g->block0, radius, 1, 0, 0, 0);
  } else if (sscanf(buffer, "/sphere %d", &radius) == 1) {
    sphere(&g->block0, radius, 0, 0, 0, 0);
  } else if (sscanf(buffer, "/fcirclex %d", &radius) == 1) {
    sphere(&g->block0, radius, 1, 1, 0, 0);
  } else if (sscanf(buffer, "/circlex %d", &radius) == 1) {
    sphere(&g->block0, radius, 0, 1, 0, 0);
  } else if (sscanf(buffer, "/fcircley %d", &radius) == 1) {
    sphere(&g->block0, radius, 1, 0, 1, 0);
  } else if (sscanf(buffer, "/circley %d", &radius) == 1) {
    sphere(&g->block0, radius, 0, 0, 1, 0);
  } else if (sscanf(buffer, "/fcirclez %d", &radius) == 1) {
    sphere(&g->block0, radius, 1, 0, 0, 1);
  } else if (sscanf(buffer, "/circlez %d", &radius) == 1) {
    sphere(&g->block0, radius, 0, 0, 0, 1);
  } else if (sscanf(buffer, "/fcylinder %d", &radius) == 1) {
    cylinder(&g->block0, &g->block1, radius, 1);
  } else if (sscanf(buffer, "/cylinder %d", &radius) == 1) {
    cylinder(&g->block0, &g->block1, radius, 0);
  } else if (forward) {
    client_talk(buffer);
  }
}

void on_light() {
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  int hx, hy, hz;
  int hw = hit_test(0, positionAndOrientation->x, positionAndOrientation->y,
                    positionAndOrientation->z, positionAndOrientation->rx,
                    positionAndOrientation->ry, &hx, &hy, &hz);
  if (hy > 0 && hy < 256 && is_destructable(hw)) {
    toggle_light(hx, hy, hz);
  }
}

void on_left_click() {
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  int hx, hy, hz;
  int hw = hit_test(0, positionAndOrientation->x, positionAndOrientation->y,
                    positionAndOrientation->z, positionAndOrientation->rx,
                    positionAndOrientation->ry, &hx, &hy, &hz);
  if (hy > 0 && hy < 256 && is_destructable(hw)) {
    set_block(hx, hy, hz, 0);
    record_block(hx, hy, hz, 0);
    if (is_plant(get_block(hx, hy + 1, hz))) {
      set_block(hx, hy + 1, hz, 0);
    }
  }
}

void on_right_click() {
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  int hx, hy, hz;
  int hw = hit_test(1, positionAndOrientation->x, positionAndOrientation->y,
                    positionAndOrientation->z, positionAndOrientation->rx,
                    positionAndOrientation->ry, &hx, &hy, &hz);
  if (hy > 0 && hy < 256 && is_obstacle(hw)) {
    if (!player_intersects_block(2, positionAndOrientation->x,
                                 positionAndOrientation->y,
                                 positionAndOrientation->z, hx, hy, hz)) {
      set_block(hx, hy, hz, items[g->item_index]);
      record_block(hx, hy, hz, items[g->item_index]);
    }
  }
}

void on_middle_click() {
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  int hx, hy, hz;
  const int hw =
      hit_test(0, positionAndOrientation->x, positionAndOrientation->y,
               positionAndOrientation->z, positionAndOrientation->rx,
               positionAndOrientation->ry, &hx, &hy, &hz);
  for (int i = 0; i < item_count; i++) {
    if (items[i] == hw) {
      g->item_index = i;
      break;
    }
  }
}

void on_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
  const int control = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
  if (action == GLFW_RELEASE) {
    return;
  }
  if (key == GLFW_KEY_BACKSPACE) {
    if (g->typing) {
      int n = strlen(g->typing_buffer);
      if (n > 0) {
        g->typing_buffer[n - 1] = '\0';
      }
    }
  }
  if (action != GLFW_PRESS) {
    return;
  }
  if (key == GLFW_KEY_ESCAPE) {
    escape_pressed = true;
    const int exclusive =
        glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

    if (g->typing) {
      g->typing = 0;
    } else if (exclusive) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
  if (key == GLFW_KEY_ENTER) {
    if (g->typing) {
      if (mods & GLFW_MOD_SHIFT) {
        int n = strlen(g->typing_buffer);
        if (n < MAX_TEXT_LENGTH - 1) {
          g->typing_buffer[n] = '\r';
          g->typing_buffer[n + 1] = '\0';
        }
      } else {
        g->typing = 0;
        if (g->typing_buffer[0] == CRAFT_KEY_SIGN) {
          Player *player = g->players;
          int x, y, z, face;
          if (hit_test_face(player, &x, &y, &z, &face)) {
            set_sign(x, y, z, face, g->typing_buffer + 1);
          }
        } else if (g->typing_buffer[0] == '/') {
          parse_command(g->typing_buffer, 1);
        } else {
          client_talk(g->typing_buffer);
        }
      }
    } else {
      if (control) {
        on_right_click();
      } else {
        on_left_click();
      }
    }
  }
  if (control && key == 'V') {
    const char *buffer = glfwGetClipboardString(window);
    if (g->typing) {
      g->suppress_char = 1;
      strncat(g->typing_buffer, buffer,
              MAX_TEXT_LENGTH - strlen(g->typing_buffer) - 1);
    } else {
      parse_command(buffer, 0);
    }
  }
  if (!g->typing) {
    if (key == CRAFT_KEY_FLY) {
      g->flying = !g->flying;
    }
    if (key >= '1' && key <= '9') {
      g->item_index = key - '1';
    }
    if (key == '0') {
      g->item_index = 9;
    }
    if (key == CRAFT_KEY_ITEM_NEXT) {
      g->item_index = (g->item_index + 1) % item_count;
    }
    if (key == CRAFT_KEY_ITEM_PREV) {
      g->item_index--;
      if (g->item_index < 0) {
        g->item_index = item_count - 1;
      }
    }
    if (key == CRAFT_KEY_OBSERVE) {
      g->observe1 = (g->observe1 + 1) % g->player_count;
    }
    if (key == CRAFT_KEY_OBSERVE_INSET) {
      g->observe2 = (g->observe2 + 1) % g->player_count;
    }
  }
}

void on_char(GLFWwindow *window, unsigned int u) {
  if (g->suppress_char) {
    g->suppress_char = 0;
    return;
  }
  if (g->typing) {
    if (u >= 32 && u < 128) {
      char c = (char)u;
      int n = strlen(g->typing_buffer);
      if (n < MAX_TEXT_LENGTH - 1) {
        g->typing_buffer[n] = c;
        g->typing_buffer[n + 1] = '\0';
      }
    }
  } else {
    if (u == CRAFT_KEY_CHAT) {
      g->typing = 1;
      g->typing_buffer[0] = '\0';
    }
    if (u == CRAFT_KEY_COMMAND) {
      g->typing = 1;
      g->typing_buffer[0] = '/';
      g->typing_buffer[1] = '\0';
    }
    if (u == CRAFT_KEY_SIGN) {
      g->typing = 1;
      g->typing_buffer[0] = CRAFT_KEY_SIGN;
      g->typing_buffer[1] = '\0';
    }
  }
}

void on_scroll(GLFWwindow *window, double xdelta, double ydelta) {
  static double ypos = 0;
  ypos += ydelta;
  if (ypos < -SCROLL_THRESHOLD) {
    g->item_index = (g->item_index + 1) % item_count;
    ypos = 0;
  }
  if (ypos > SCROLL_THRESHOLD) {
    g->item_index--;
    if (g->item_index < 0) {
      g->item_index = item_count - 1;
    }
    ypos = 0;
  }
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
  const int control = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
  const int exclusive =
      glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
  if (action != GLFW_PRESS) {
    return;
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (exclusive) {
      if (control) {
        on_right_click();
      } else {
        on_left_click();
      }
    } else {
      // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (exclusive) {
      if (control) {
        on_light();
      } else {
        on_right_click();
      }
    }
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (exclusive) {
      on_middle_click();
    }
  }
}

void handle_orientation_input() {
  int exclusive =
      glfwGetInputMode(g->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
  static double px = 0;
  static double py = 0;
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  if (exclusive && (px || py)) {
    double mx, my;
    glfwGetCursorPos(g->window, &mx, &my);
    float m = 0.0025;
    positionAndOrientation->rx += (mx - px) * m;
    if (INVERT_MOUSE) {
      positionAndOrientation->ry += (my - py) * m;
    } else {
      positionAndOrientation->ry -= (my - py) * m;
    }

    // handle controller input
    {

      // get input from controller 1
      int count;
      const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);

      // 2 is left trigger
      if (count >= 5) {
        if (fabsf(axes[3]) > 0.19) {
          positionAndOrientation->rx += 0.05 * axes[3];
        }
        if (fabsf(axes[4]) > 0.19) {
          positionAndOrientation->ry += -0.05 * axes[4];
        }
      }
    }

    if (positionAndOrientation->rx < 0) {
      positionAndOrientation->rx += RADIANS(360);
    }
    if (positionAndOrientation->rx >= RADIANS(360)) {
      positionAndOrientation->rx -= RADIANS(360);
    }
    positionAndOrientation->ry = MAX(positionAndOrientation->ry, -RADIANS(90));
    positionAndOrientation->ry = MIN(positionAndOrientation->ry, RADIANS(90));
    px = mx;
    py = my;
  } else {
    glfwGetCursorPos(g->window, &px, &py);
  }
}

void handle_movement(double dt) {
  static float dy = 0;
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  int sz = 0;
  int sx = 0;
  if (!g->typing) {
    float m = dt * 1.0;
    g->ortho = glfwGetKey(g->window, CRAFT_KEY_ORTHO) ? 64 : 0;
    g->fov = glfwGetKey(g->window, CRAFT_KEY_ZOOM) ? 15 : 65;
    if (glfwGetKey(g->window, CRAFT_KEY_FORWARD))
      sz--;
    if (glfwGetKey(g->window, CRAFT_KEY_BACKWARD))
      sz++;
    if (glfwGetKey(g->window, CRAFT_KEY_LEFT))
      sx--;
    if (glfwGetKey(g->window, CRAFT_KEY_RIGHT))
      sx++;
    if (glfwGetKey(g->window, GLFW_KEY_LEFT))
      positionAndOrientation->rx -= m;
    if (glfwGetKey(g->window, GLFW_KEY_RIGHT))
      positionAndOrientation->rx += m;
    if (glfwGetKey(g->window, GLFW_KEY_UP))
      positionAndOrientation->ry += m;
    if (glfwGetKey(g->window, GLFW_KEY_DOWN))
      positionAndOrientation->ry -= m;
    // handle controller input
    {
      // get input from controller 1
      int count;
      const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);

      if (count >= 1 && fabsf(axes[0]) > 0.19) {
        // vertical
        sx += axes[0] * 50.0;
      }
      if (count >= 2 && fabsf(axes[1]) > 0.19) {
        // horizontal
        sz += axes[1] * 50.0;
      }
    }
  }
  // motion vector vx, vy, vz
  float vx = 0.0, vy = 0.0, vz = 0.0;
  const int motion_key_was_pressed = sz || sx;
  // initiize motion vector if a movement key was pressed
  if (motion_key_was_pressed) {
    float strafe = atan2f(sz, sx);
    if (g->flying) {
      float m = cosf(positionAndOrientation->ry);
      float y = sinf(positionAndOrientation->ry);
      if (sx) {
        if (!sz) {
          y = 0;
        }
        m = 1;
      }
      if (sz > 0) {
        y = -y;
      }
      vx = cosf(positionAndOrientation->rx + strafe) * m;
      vy = y;
      vz = sinf(positionAndOrientation->rx + strafe) * m;
    } else {
      vx = cosf(positionAndOrientation->rx + strafe);
      vy = 0;
      vz = sinf(positionAndOrientation->rx + strafe);
    }
  }

  if (!g->typing) {
    if (glfwGetKey(g->window, CRAFT_KEY_JUMP)) {
      if (g->flying) {
        vy = 1;
      } else if (dy == 0) {
        dy = 8;
      }
    }
  }
  float speed = g->flying ? 20 : 5;
  int estimate =
      roundf(sqrtf(powf(vx * speed, 2) + powf(vy * speed + ABS(dy) * 2, 2) +
                   powf(vz * speed, 2)) *
             dt * 8);
  int step = MAX(8, estimate);
  float ut = dt / step;
  vx = vx * ut * speed;
  vy = vy * ut * speed;
  vz = vz * ut * speed;
  for (int i = 0; i < step; i++) {
    if (g->flying) {
      dy = 0;
    } else {
      dy -= ut * 25;
      dy = MAX(dy, -250);
    }
    positionAndOrientation->x += vx;
    positionAndOrientation->y += vy + dy * ut;
    positionAndOrientation->z += vz;
    int collide = 0; // false
    {
      // set collide to true, TODO explain
      // why it would be set to true
      int p = chunked(positionAndOrientation->x);
      int q = chunked(positionAndOrientation->z);
      Chunk *chunk = find_chunk(p, q);
      if (chunk) {
        Map *map = &chunk->map;
        int nx = roundf(positionAndOrientation->x);
        int ny = roundf(positionAndOrientation->y);
        int nz = roundf(positionAndOrientation->z);
        float px = positionAndOrientation->x - nx;
        float py = positionAndOrientation->y - ny;
        float pz = positionAndOrientation->z - nz;
        float pad = 0.25;
        const int height = 2;
        for (int dy = 0; dy < height; dy++) {
          if (px < -pad && is_obstacle(map_get(map, nx - 1, ny - dy, nz))) {
            positionAndOrientation->x = nx - pad;
          }
          if (px > pad && is_obstacle(map_get(map, nx + 1, ny - dy, nz))) {
            positionAndOrientation->x = nx + pad;
          }
          if (py < -pad && is_obstacle(map_get(map, nx, ny - dy - 1, nz))) {
            positionAndOrientation->y = ny - pad;
            collide = 1;
          }
          if (py > pad && is_obstacle(map_get(map, nx, ny - dy + 1, nz))) {
            positionAndOrientation->y = ny + pad;
            collide = 1;
          }
          if (pz < -pad && is_obstacle(map_get(map, nx, ny - dy, nz - 1))) {
            positionAndOrientation->z = nz - pad;
          }
          if (pz > pad && is_obstacle(map_get(map, nx, ny - dy, nz + 1))) {
            positionAndOrientation->z = nz + pad;
          }
        }
      }
    }
    if (collide) {
      // TODO - perhaps do the assigment to dy where collide
      // is being set to 1
      dy = 0;
    }
  }
  if (positionAndOrientation->y < 0) {
    positionAndOrientation->y =
        highest_block(positionAndOrientation->x, positionAndOrientation->z) + 2;
  }
}

void parse_buffer(char *buffer) {
  Player *me = g->players;
  PositionAndOrientation *positionAndOrientation =
      &g->players->positionAndOrientation;
  char *key;
  char *line = tokenize(buffer, "\n", &key);
  while (line) {
    int pid;
    float ux, uy, uz, urx, ury;
    if (sscanf(line, "U,%d,%f,%f,%f,%f,%f", &pid, &ux, &uy, &uz, &urx, &ury) ==
        6) {
      me->id = pid;
      positionAndOrientation->x = ux;
      positionAndOrientation->y = uy;
      positionAndOrientation->z = uz;
      positionAndOrientation->rx = urx;
      positionAndOrientation->ry = ury;
      force_chunks(me);
      if (uy == 0) {
        positionAndOrientation->y = highest_block(positionAndOrientation->x,
                                                  positionAndOrientation->z) +
                                    2;
      }
    }
    int bp, bq, bx, by, bz, bw;
    if (sscanf(line, "B,%d,%d,%d,%d,%d,%d", &bp, &bq, &bx, &by, &bz, &bw) ==
        6) {
      _set_block(bp, bq, bx, by, bz, bw, 0);
      if (player_intersects_block(2, positionAndOrientation->x,
                                  positionAndOrientation->y,
                                  positionAndOrientation->z, bx, by, bz)) {
        positionAndOrientation->y = highest_block(positionAndOrientation->x,
                                                  positionAndOrientation->z) +
                                    2;
      }
    }
    if (sscanf(line, "L,%d,%d,%d,%d,%d,%d", &bp, &bq, &bx, &by, &bz, &bw) ==
        6) {
      set_light(bp, bq, bx, by, bz, bw);
    }
    float px, py, pz, prx, pry;
    if (sscanf(line, "P,%d,%f,%f,%f,%f,%f", &pid, &px, &py, &pz, &prx, &pry) ==
        6) {
      Player *player = find_player(pid);
      if (!player && g->player_count < MAX_PLAYERS) {
        player = g->players + g->player_count;
        g->player_count++;
        player->id = pid;
        player->buffer = 0;
        snprintf(player->name, MAX_NAME_LENGTH, "player%d", pid);
        update_player(player, px, py, pz, prx, pry, 1); // twice
      }
      if (player) {
        update_player(player, px, py, pz, prx, pry, 1);
      }
    }
    if (sscanf(line, "D,%d", &pid) == 1) {
      // delete player
      Player *player = find_player(pid);
      if (player) {
        int count = g->player_count;
        (*renderer.del_buffer)(player->buffer);
        Player *other_player = g->players + (--count);
        memcpy(player, other_player, sizeof(Player));
        g->player_count = count;
      }
    }
    int kp, kq, kk;
    if (sscanf(line, "K,%d,%d,%d", &kp, &kq, &kk) == 3) {
      db_set_key(kp, kq, kk);
    }
    if (sscanf(line, "R,%d,%d", &kp, &kq) == 2) {
      Chunk *chunk = find_chunk(kp, kq);
      if (chunk) {
        dirty_chunk(chunk);
      }
    }
    double elapsed;
    int day_length;
    if (sscanf(line, "E,%lf,%d", &elapsed, &day_length) == 2) {
      glfwSetTime(fmod(elapsed, day_length));
      g->day_length = day_length;
      g->time_changed = 1;
    }
    if (line[0] == 'T' && line[1] == ',') {
      char *text = line + 2;
      add_message(text);
    }
    char format[64];
    snprintf(format, sizeof(format), "N,%%d,%%%ds", MAX_NAME_LENGTH - 1);
    char name[MAX_NAME_LENGTH];
    if (sscanf(line, format, &pid, name) == 2) {
      Player *player = find_player(pid);
      if (player) {
        strncpy(player->name, name, MAX_NAME_LENGTH);
      }
    }
    snprintf(format, sizeof(format), "S,%%d,%%d,%%d,%%d,%%d,%%d,%%%d[^\n]",
             MAX_SIGN_LENGTH - 1);
    int face;
    char text[MAX_SIGN_LENGTH] = {0};
    if (sscanf(line, format, &bp, &bq, &bx, &by, &bz, &face, text) >= 6) {
      _set_sign(bp, bq, bx, by, bz, face, text, 0);
    }
    line = tokenize(NULL, "\n", &key);
  }
}

void reset_model() {
  memset(g->chunks, 0, sizeof(Chunk) * MAX_CHUNKS);
  g->chunk_count = 0;
  memset(g->players, 0, sizeof(Player) * MAX_PLAYERS);
  g->player_count = 0;
  g->observe1 = 0;
  g->observe2 = 0;
  g->flying = 0;
  g->item_index = 0;
  memset(g->typing_buffer, 0, sizeof(char) * MAX_TEXT_LENGTH);
  g->typing = 0;
  memset(g->messages, 0, sizeof(char) * MAX_MESSAGES * MAX_TEXT_LENGTH);
  g->message_index = 0;
  g->day_length = DAY_LENGTH;
  glfwSetTime(g->day_length / 3.0);
  g->time_changed = 1;
}

// returns -1 on failure
int initialize_craft(int argc, char **argv) {
  // INITIALIZATION //
  curl_global_init(CURL_GLOBAL_DEFAULT);
  srand(time(NULL));
  rand();
  // WINDOW INITIALIZATION //
  if (!glfwInit()) {
    return -1;
  }
  // create window
  {
    // initialize g->window
    int window_width = WINDOW_WIDTH;
    int window_height = WINDOW_HEIGHT;
    GLFWmonitor *monitor = NULL;
    if (FULLSCREEN) {
      int mode_count;
      monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode *modes = glfwGetVideoModes(monitor, &mode_count);
      window_width = modes[mode_count - 1].width;
      window_height = modes[mode_count - 1].height;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    g->window =
        glfwCreateWindow(window_width, window_height, "Craft", monitor, NULL);
    if (!g->window) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
      g->window =
          glfwCreateWindow(window_width, window_height, "Craft", monitor, NULL);

      if (!g->window) {
        glfwTerminate();
        return -1;
      }
    }
  }

  glfwMakeContextCurrent(g->window);
  glfwSwapInterval(VSYNC);
  glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(g->window, on_key);
  glfwSetCharCallback(g->window, on_char);
  glfwSetMouseButtonCallback(g->window, on_mouse_button);
  glfwSetScrollCallback(g->window, on_scroll);

  if (-1 == gl_graphics_loader_init()) {
    return -1;
  }

  gl_initiliaze_global_state();

  gl_initiliaze_textures();

  // CHECK COMMAND LINE ARGUMENTS //
  if (argc == 2 || argc == 3) {
    g->mode = MODE_ONLINE;
    strncpy(g->server_addr, argv[1], MAX_ADDR_LENGTH);
    g->server_port = argc == 3 ? atoi(argv[2]) : DEFAULT_PORT;
    snprintf(g->db_path, MAX_PATH_LENGTH, "cache.%s.%d.db", g->server_addr,
             g->server_port);
  } else {
    g->mode = MODE_OFFLINE;
    snprintf(g->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
  }

  g->create_radius = CREATE_CHUNK_RADIUS;
  g->render_radius = RENDER_CHUNK_RADIUS;
  g->delete_radius = DELETE_CHUNK_RADIUS;
  g->sign_radius = RENDER_SIGN_RADIUS;

#ifdef ENABLE_NO_THREADS
#else
  // INITIALIZE WORKER THREADS
  for (int i = 0; i < WORKERS; i++) {
    Worker *worker = g->workers + i;
    worker->index = i;
    worker->state = WORKER_IDLE;
    mtx_init(&worker->mtx, mtx_plain);
    cnd_init(&worker->cnd);
    thrd_create(&worker->thrd, worker_run, worker);
  }
#endif
  return 0;
}

int main(int argc, char **argv) {

  if (-1 == initialize_craft(argc, argv)) {
    return -1;
  }

  // initialize IMGUI
  gui_init(g->window);

  // OUTER LOOP //
  int running = 1;
  while (running) {
    // DATABASE INITIALIZATION //
    if (g->mode == MODE_OFFLINE || USE_CACHE) {
      db_enable();
      if (db_init(g->db_path)) {
        return -1;
      }
      if (g->mode == MODE_ONLINE) {
        // TODO: support proper caching of signs (handle deletions)
        db_delete_all_signs();
      }
    }

    // CLIENT INITIALIZATION //
    if (g->mode == MODE_ONLINE) {
      client_enable();
      client_connect(g->server_addr, g->server_port);
      client_start();
      client_version(1);
      login();
    }

    // LOCAL VARIABLES //
    reset_model();
    FPS fps = {0, 0, 0};
    double last_commit = glfwGetTime();
    double last_update = glfwGetTime();

    // use the openGL3.3 renderer
    // TODO - make this configurable between OpenGL3.3 core profile,
    // Vulkan, and Apple'positionAndOrientation Metal
    renderer = gl_renderer;

    uint32_t sky_buffer = gen_sky_buffer();

    Player *me = g->players;
    PositionAndOrientation *positionAndOrientation =
        &g->players->positionAndOrientation;
    me->id = 0;
    me->name[0] = '\0';
    me->buffer = 0;
    g->player_count = 1;

    // LOAD STATE FROM DATABASE //
    int loaded =
        db_load_state(&positionAndOrientation->x, &positionAndOrientation->y,
                      &positionAndOrientation->z, &positionAndOrientation->rx,
                      &positionAndOrientation->ry);
    force_chunks(me);
    if (!loaded) {
      positionAndOrientation->y =
          highest_block(positionAndOrientation->x, positionAndOrientation->z) +
          2;
    }

    // BEGIN MAIN LOOP //
    double previous = glfwGetTime();

    // the event loop
    while (1) {
      // WINDOW SIZE AND SCALE //
      // set g->scale
      {
        int window_width, window_height;
        int buffer_width, buffer_height;
        glfwGetWindowSize(g->window, &window_width, &window_height);
        glfwGetFramebufferSize(g->window, &buffer_width, &buffer_height);
        int result = buffer_width / window_width;
        result = MAX(1, result);
        result = MIN(2, result);
        g->scale = result;
      }

      glfwGetFramebufferSize(g->window, &g->width, &g->height);
      (*renderer.viewport)(0, 0, g->width, g->height);

      bool show_gui_this_frame = escape_pressed;
      if (show_gui_this_frame) {
        gui_create_frame();
        gui_show_demo_window();
      }

      // FRAME RATE //
      if (g->time_changed) {
        g->time_changed = 0;
        last_commit = glfwGetTime();
        last_update = glfwGetTime();
        memset(&fps, 0, sizeof(fps));
      }
      update_fps(&fps);
      double now = glfwGetTime();
      double dt = now - previous;
      dt = MIN(dt, 0.2);
      dt = MAX(dt, 0.0);
      previous = now;

      // HANDLE PLAYER INPUT //
      handle_orientation_input();

      // HANDLE MOVEMENT //
      handle_movement(dt);

      // HANDLE DATA FROM SERVER //
      char *buffer = client_recv();
      if (buffer) {
        parse_buffer(buffer);
        free(buffer);
      }

      // FLUSH DATABASE //
      if (now - last_commit > COMMIT_INTERVAL) {
        last_commit = now;
        db_commit();
      }

      // SEND POSITION TO SERVER //
      if (now - last_update > 0.1) {
        last_update = now;
        client_position(positionAndOrientation->x, positionAndOrientation->y,
                        positionAndOrientation->z, positionAndOrientation->rx,
                        positionAndOrientation->ry);
      }

      // PREPARE TO RENDER //
      g->observe1 = g->observe1 % g->player_count;
      g->observe2 = g->observe2 % g->player_count;
      delete_chunks();
      (*renderer.del_buffer)(me->buffer);
      me->buffer = gen_player_buffer(
          positionAndOrientation->x, positionAndOrientation->y,
          positionAndOrientation->z, positionAndOrientation->rx,
          positionAndOrientation->ry);
      for (int i = 1; i < g->player_count; i++) {
        // interpolate playen
        {
          Player *player = g->players + i;
          PositionAndOrientation *positionAndOrientation1 =
              &player->positionAndOrientation1;
          PositionAndOrientation *positionAndOrientation2 =
              &player->positionAndOrientation2;
          float t1 = positionAndOrientation2->t - positionAndOrientation1->t;
          float t2 = glfwGetTime() - positionAndOrientation2->t;
          t1 = MIN(t1, 1);
          t1 = MAX(t1, 0.1);
          float p = MIN(t2 / t1, 1);
          update_player(
              player,
              positionAndOrientation1->x +
                  (positionAndOrientation2->x - positionAndOrientation1->x) * p,
              positionAndOrientation1->y +
                  (positionAndOrientation2->y - positionAndOrientation1->y) * p,
              positionAndOrientation1->z +
                  (positionAndOrientation2->z - positionAndOrientation1->z) * p,
              positionAndOrientation1->rx +
                  (positionAndOrientation2->rx - positionAndOrientation1->rx) *
                      p,
              positionAndOrientation1->ry +
                  (positionAndOrientation2->ry - positionAndOrientation1->ry) *
                      p,
              0);
        }
      }
      Player *player = g->players + g->observe1;

      // RENDER 3-D SCENE //

      (*renderer.clear_color_buffer)();
      (*renderer.clear_depth_buffer)();
      if (do_render_sky) {
        render_sky(player, sky_buffer);
      }
      (*renderer.clear_depth_buffer)();
      int face_count = 0; // default value
      if (do_render_chunks) {
        face_count = render_chunks(player);
      }
      render_signs(player);
      render_sign(player);
      render_players(player);
      if (SHOW_WIREFRAME) {
        if (do_render_wireframe) {
          render_wireframe(player);
        }
      }

      // RENDER HUD //
      (*renderer.clear_depth_buffer)();
      if (SHOW_CROSSHAIRS) {
        // render crosshairs
        float matrix[16];
        set_matrix_2d(matrix, g->width, g->height);
        uint32_t crosshair_buffer;
        {
          // initialize crosshair_buffer
          const int x = g->width / 2;
          const int y = g->height / 2;
          const int p = 10 * g->scale;
          float data[] = {x, y - p, x, y + p, x - p, y, x + p, y};
          crosshair_buffer = (*renderer.gen_buffer)(sizeof(data), data);
        }

        if (do_render_crosshairs) {
          (*renderer.render_crosshairs)(crosshair_buffer, matrix);
        }
        (*renderer.del_buffer)(crosshair_buffer);
      }
      if (SHOW_ITEM) {
        // render item
        float matrix[16];
        set_matrix_item(matrix, g->width, g->height, g->scale);

        if (do_render_item) {
          (*renderer.render_item)(matrix);
        }

        int w = items[g->item_index];
        if (is_plant(w)) {
          uint32_t plant_buffer;
          {
            // initialize plant buffer
            const float x = 0;
            const float y = 0;
            const float z = 0;
            const float n = 0.5;
            float *data = malloc_faces(10, 4);
            float ao = 0;
            float light = 1;
            make_plant(data, ao, light, x, y, z, n, w, 45);
            plant_buffer = (*renderer.gen_faces)(10, 4, data);
          }
          // TODO -
          // make and initilize the VAO once at initilization time.
          // only thing that should happen here
          // is 1) binding the vao, 2) binding the vbo,
          // 3) setting the vertexattribpointers
          // 4) draw arrays 5) cleanup
          // also, remove magic numbers, like 24

          // draw plant
          if (plant_buffer != 0) {
            if (do_render_plant) {
              (*renderer.render_plant)(plant_buffer);
            }
          }
          (*renderer.del_buffer)(plant_buffer);
        } else {
          uint32_t cube_buffer;
          {
            // initilize cube_buffer
            const float x = 0;
            const float y = 0;
            const float z = 0;
            const float n = 0.5;
            float *data = malloc_faces(10, 6);
            float ao[6][4] = {0};
            float light[6][4] = {{0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5},
                                 {0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5},
                                 {0.5, 0.5, 0.5, 0.5}, {0.5, 0.5, 0.5, 0.5}};
            make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
            cube_buffer = (*renderer.gen_faces)(10, 6, data);
          }
          // draw cube buffer
          if (cube_buffer != 0) {
            if (do_render_cube) {
              (*renderer.render_cube)(cube_buffer);
            }
          }
          (*renderer.del_buffer)(cube_buffer);
        }
      }

      // RENDER TEXT //
      char text_buffer[1024];
      float ts = 12 * g->scale;
      float tx = ts / 2;
      float ty = g->height - ts;
      if (SHOW_INFO_TEXT) {
        int hour = time_of_day() * 24;
        char am_pm = hour < 12 ? 'a' : 'p';
        hour = hour % 12;
        hour = hour ? hour : 12;
        snprintf(text_buffer, 1024,
                 "(%d, %d) (%.2f, %.2f, %.2f) [%d, %d, %d] %d%cm %dfps",
                 chunked(positionAndOrientation->x),
                 chunked(positionAndOrientation->z), positionAndOrientation->x,
                 positionAndOrientation->y, positionAndOrientation->z,
                 g->player_count, g->chunk_count, face_count * 2, hour, am_pm,
                 fps.fps);
        render_text(ALIGN_LEFT, tx, ty, ts, text_buffer);
        ty -= ts * 2;
      }
      if (SHOW_CHAT_TEXT) {
        for (int i = 0; i < MAX_MESSAGES; i++) {
          int index = (g->message_index + i) % MAX_MESSAGES;
          if (strlen(g->messages[index])) {
            render_text(ALIGN_LEFT, tx, ty, ts, g->messages[index]);
            ty -= ts * 2;
          }
        }
      }
      if (g->typing) {
        snprintf(text_buffer, 1024, "> %s", g->typing_buffer);
        render_text(ALIGN_LEFT, tx, ty, ts, text_buffer);
        ty -= ts * 2;
      }
      if (SHOW_PLAYER_NAMES) {
        if (player != me) {
          render_text(ALIGN_CENTER, g->width / 2, ts, ts, player->name);
        }
        Player *other_player = player_crosshair(player);
        if (other_player) {
          render_text(ALIGN_CENTER, g->width / 2, g->height / 2 - ts - 24, ts,
                      other_player->name);
        }
      }

      // RENDER PICTURE IN PICTURE //
      if (g->observe2) {
        player = g->players + g->observe2;

        int pw = 256 * g->scale;
        int ph = 256 * g->scale;
        int offset = 32 * g->scale;
        int pad = 3 * g->scale;
        int sw = pw + pad * 2;
        int sh = ph + pad * 2;

        (*renderer.enable_scissor_test)();
        (*renderer.scissor)(g->width - sw - offset + pad, offset - pad, sw, sh);
        (*renderer.clear_color_buffer)();
        (*renderer.disable_scissor_test)();
        (*renderer.clear_depth_buffer)();
        (*renderer.viewport)(g->width - pw - offset, offset, pw, ph);

        g->width = pw;
        g->height = ph;
        g->ortho = 0;
        g->fov = 65;

        if (do_render_sky) {
          render_sky(player, sky_buffer);
        }
        (*renderer.clear_depth_buffer)();
        if (do_render_chunks) {
          render_chunks(player);
        }
        render_signs(player);
        render_players(player);
        (*renderer.clear_depth_buffer)();
        if (SHOW_PLAYER_NAMES) {
          render_text(ALIGN_CENTER, pw / 2, ts, ts, player->name);
        }
      }

      if (show_gui_this_frame) {
        gui_render_frame();
      }

      // SWAP AND POLL //
      glfwSwapBuffers(g->window);
      glfwPollEvents();
      if (glfwWindowShouldClose(g->window)) {
        running = 0;
        break;
      }
      if (g->mode_changed) {
        g->mode_changed = 0;
        break;
      }
    }

    gui_cleanup();

    // SHUTDOWN //
    db_save_state(positionAndOrientation->x, positionAndOrientation->y,
                  positionAndOrientation->z, positionAndOrientation->rx,
                  positionAndOrientation->ry);
    db_close();
    db_disable();
    client_stop();
    client_disable();
    (*renderer.del_buffer)(sky_buffer);
    delete_all_chunks();
    // delete all players
    {
      for (int i = 0; i < g->player_count; i++) {
        Player *player = g->players + i;
        (*renderer.del_buffer)(player->buffer);
      }
      g->player_count = 0;
    }
  }

  glfwTerminate();
  curl_global_cleanup();
  return 0;
}
