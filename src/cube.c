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

#include "cube.h"
#include "item.h"
#include "matrix.h"
#include "util.h"
#include <math.h>

static void make_cube_faces(float *data, float ambient_occlusion[6][4],
                            float light[6][4], int left, int right, int top,
                            int bottom, int front, int back, int wleft,
                            int wright, int wtop, int wbottom, int wfront,
                            int wback, float x, float y, float z, float n) {
  static const float positions[6][4][3] =
      {{{-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1}},
       {{+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}},
       {{-1, +1, -1}, {-1, +1, +1}, {+1, +1, -1}, {+1, +1, +1}},
       {{-1, -1, -1}, {-1, -1, +1}, {+1, -1, -1}, {+1, -1, +1}},
       {{-1, -1, -1}, {-1, +1, -1}, {+1, -1, -1}, {+1, +1, -1}},
       {{-1, -1, +1}, {-1, +1, +1}, {+1, -1, +1}, {+1, +1, +1}}},
                     normals[6][3] = {{-1, 0, 0}, {+1, 0, 0}, {0, +1, 0},
                                      {0, -1, 0}, {0, 0, -1}, {0, 0, +1}},
                     uvs[6][4][2] = {{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
                                     {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
                                     {{0, 1}, {0, 0}, {1, 1}, {1, 0}},
                                     {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
                                     {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
                                     {{1, 0}, {1, 1}, {0, 0}, {0, 1}}};

  static const float indices[6][6] = {{0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3}, {0, 3, 1, 0, 2, 3}},
                     flipped[6][6] = {{0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1},
                                      {0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1},
                                      {0, 1, 2, 1, 3, 2}, {0, 2, 1, 2, 3, 1}};
  float *d = data;
  const float s = 0.0625, a = 0 + 1 / 2048.0, b = s - 1 / 2048.0;
  const int faces[6] = {left, right, top, bottom, front, back},
            tiles[6] = {wleft, wright, wtop, wbottom, wfront, wback};
  for (int i = 0; i < 6; i++) {
    if (faces[i] == 0) {
      continue;
    }
    const float du = (tiles[i] % 16) * s, dv = (tiles[i] / 16) * s;
    const int flip = ambient_occlusion[i][0] + ambient_occlusion[i][3] >
                     ambient_occlusion[i][1] + ambient_occlusion[i][2];
    for (int v = 0; v < 6; v++) {
      int j = flip ? flipped[i][v] : indices[i][v];
      *(d++) = x + n * positions[i][j][0];
      *(d++) = y + n * positions[i][j][1];
      *(d++) = z + n * positions[i][j][2];
      *(d++) = normals[i][0];
      *(d++) = normals[i][1];
      *(d++) = normals[i][2];
      *(d++) = du + (uvs[i][j][0] ? b : a);
      *(d++) = dv + (uvs[i][j][1] ? b : a);
      *(d++) = ambient_occlusion[i][j];
      *(d++) = light[i][j];
    }
  }
}

void make_cube(float *data, float ambient_occlusion[6][4], float light[6][4],
               int left, int right, int top, int bottom, int front, int back,
               float x, float y, float z, float n, int w) {
  int wleft = blocks[w][0], wright = blocks[w][1], wtop = blocks[w][2],
      wbottom = blocks[w][3], wfront = blocks[w][4], wback = blocks[w][5];
  make_cube_faces(data, ambient_occlusion, light, left, right, top, bottom,
                  front, back, wleft, wright, wtop, wbottom, wfront, wback, x,
                  y, z, n);
}

void make_plant(float *data, float ambient_occlusion, float light, float px,
                float py, float pz, float n, int w, float rotation) {
  static const float positions[4][4][3] =
      {{{0, -1, -1}, {0, -1, +1}, {0, +1, -1}, {0, +1, +1}},
       {{0, -1, -1}, {0, -1, +1}, {0, +1, -1}, {0, +1, +1}},
       {{-1, -1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0}},
       {{-1, -1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0}}},
                     normals[4][3] = {{-1, 0, 0},
                                      {+1, 0, 0},
                                      {0, 0, -1},
                                      {0, 0, +1}},
                     uvs[4][4][2] = {{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
                                     {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
                                     {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
                                     {{1, 0}, {1, 1}, {0, 0}, {0, 1}}},
                     indices[4][6] = {{0, 3, 2, 0, 1, 3},
                                      {0, 3, 1, 0, 2, 3},
                                      {0, 3, 2, 0, 1, 3},
                                      {0, 3, 1, 0, 2, 3}};
  float *d = data;
  const float s = 0.0625, a = 0, b = s, du = (plants[w] % 16) * s,
              dv = (plants[w] / 16) * s;
  for (int i = 0; i < 4; i++) {
    for (int v = 0; v < 6; v++) {
      const int j = indices[i][v];
      *(d++) = n * positions[i][j][0];
      *(d++) = n * positions[i][j][1];
      *(d++) = n * positions[i][j][2];
      *(d++) = normals[i][0];
      *(d++) = normals[i][1];
      *(d++) = normals[i][2];
      *(d++) = du + (uvs[i][j][0] ? b : a);
      *(d++) = dv + (uvs[i][j][1] ? b : a);
      *(d++) = ambient_occlusion;
      *(d++) = light;
    }
  }
  float ma[16], mb[16];
  mat_identity(ma);
  mat_rotate(mb, 0, 1, 0, RADIANS(rotation));
  mat_multiply(ma, mb, ma);
  mat_apply(data, ma, 24, 3, 10);
  mat_translate(mb, px, py, pz);
  mat_multiply(ma, mb, ma);
  mat_apply(data, ma, 24, 0, 10);
}

void make_player(float *data, float x, float y, float z, float rx, float ry) {
  float ambient_occlusion[6][4] = {0},
        light[6][4] = {{0.8, 0.8, 0.8, 0.8}, {0.8, 0.8, 0.8, 0.8},
                       {0.8, 0.8, 0.8, 0.8}, {0.8, 0.8, 0.8, 0.8},
                       {0.8, 0.8, 0.8, 0.8}, {0.8, 0.8, 0.8, 0.8}};
  make_cube_faces(data, ambient_occlusion, light, 1, 1, 1, 1, 1, 1, 226, 224,
                  241, 209, 225, 227, 0, 0, 0, 0.4);
  float ma[16], mb[16];
  mat_identity(ma);
  mat_rotate(mb, 0, 1, 0, rx);
  mat_multiply(ma, mb, ma);
  mat_rotate(mb, cosf(rx), 0, sinf(rx), -ry);
  mat_multiply(ma, mb, ma);
  mat_apply(data, ma, 36, 3, 10);
  mat_translate(mb, x, y, z);
  mat_multiply(ma, mb, ma);
  mat_apply(data, ma, 36, 0, 10);
}

void make_cube_wireframe(float *data, float x, float y, float z, float n) {
  static const float positions[8][3] = {
      {-1, -1, -1}, {-1, -1, +1}, {-1, +1, -1}, {-1, +1, +1},
      {+1, -1, -1}, {+1, -1, +1}, {+1, +1, -1}, {+1, +1, +1}};
  static const int indices[24] = {0, 1, 0, 2, 0, 4, 1, 3, 1, 5, 2, 3,
                                  2, 6, 3, 7, 4, 5, 4, 6, 5, 7, 6, 7};
  float *d = data;
  for (int i = 0; i < 24; i++) {
    int j = indices[i];
    *(d++) = x + n * positions[j][0];
    *(d++) = y + n * positions[j][1];
    *(d++) = z + n * positions[j][2];
  }
}

void make_character(float *data, float x, float y, float n, float m, char c) {
  float *d = data;
  const float s = 0.0625, a = s, b = s * 2;
  const int w = c - 32;
  const float du = (w % 16) * a, dv = 1 - (w / 16) * b - b;
  *(d++) = x - n;
  *(d++) = y - m;
  *(d++) = du + 0;
  *(d++) = dv;
  *(d++) = x + n;
  *(d++) = y - m;
  *(d++) = du + a;
  *(d++) = dv;
  *(d++) = x + n;
  *(d++) = y + m;
  *(d++) = du + a;
  *(d++) = dv + b;
  *(d++) = x - n;
  *(d++) = y - m;
  *(d++) = du + 0;
  *(d++) = dv;
  *(d++) = x + n;
  *(d++) = y + m;
  *(d++) = du + a;
  *(d++) = dv + b;
  *(d++) = x - n;
  *(d++) = y + m;
  *(d++) = du + 0;
  *(d++) = dv + b;
}

void make_character_3d(float *data, float x, float y, float z, float n,
                       int face, char c) {
  static const float positions[8][6][3] = {{{0, -2, -1},
                                            {0, +2, +1},
                                            {0, +2, -1},
                                            {0, -2, -1},
                                            {0, -2, +1},
                                            {0, +2, +1}},
                                           {{0, -2, -1},
                                            {0, +2, +1},
                                            {0, -2, +1},
                                            {0, -2, -1},
                                            {0, +2, -1},
                                            {0, +2, +1}},
                                           {{-1, -2, 0},
                                            {+1, +2, 0},
                                            {+1, -2, 0},
                                            {-1, -2, 0},
                                            {-1, +2, 0},
                                            {+1, +2, 0}},
                                           {{-1, -2, 0},
                                            {+1, -2, 0},
                                            {+1, +2, 0},
                                            {-1, -2, 0},
                                            {+1, +2, 0},
                                            {-1, +2, 0}},
                                           {{-1, 0, +2},
                                            {+1, 0, +2},
                                            {+1, 0, -2},
                                            {-1, 0, +2},
                                            {+1, 0, -2},
                                            {-1, 0, -2}},
                                           {{-2, 0, +1},
                                            {+2, 0, -1},
                                            {-2, 0, -1},
                                            {-2, 0, +1},
                                            {+2, 0, +1},
                                            {+2, 0, -1}},
                                           {{+1, 0, +2},
                                            {-1, 0, -2},
                                            {-1, 0, +2},
                                            {+1, 0, +2},
                                            {+1, 0, -2},
                                            {-1, 0, -2}},
                                           {{+2, 0, -1},
                                            {-2, 0, +1},
                                            {+2, 0, +1},
                                            {+2, 0, -1},
                                            {-2, 0, -1},
                                            {-2, 0, +1}}};
  static const float uvs[8][6][2] = {
      {{0, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}},
      {{1, 0}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}},
      {{1, 0}, {0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1}},
      {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}},
      {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}},
      {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}},
      {{0, 1}, {1, 0}, {1, 1}, {0, 1}, {0, 0}, {1, 0}}};
  static const float offsets[8][3] = {
      {-1, 0, 0}, {+1, 0, 0}, {0, 0, -1}, {0, 0, +1},
      {0, +1, 0}, {0, +1, 0}, {0, +1, 0}, {0, +1, 0},
  };
  float *d = data;
  float s = 0.0625;
  const float pu = s / 5, pv = s / 2.5, u1 = pu, v1 = pv, u2 = s - pu,
              v2 = s * 2 - pv, p = 0.5;
  const int w = c - 32;
  const float du = (w % 16) * s, dv = 1 - (w / 16 + 1) * s * 2;
  x += p * offsets[face][0];
  y += p * offsets[face][1];
  z += p * offsets[face][2];
  for (int i = 0; i < 6; i++) {
    *(d++) = x + n * positions[face][i][0];
    *(d++) = y + n * positions[face][i][1];
    *(d++) = z + n * positions[face][i][2];
    *(d++) = du + (uvs[face][i][0] ? u2 : u1);
    *(d++) = dv + (uvs[face][i][1] ? v2 : v1);
  }
}

int _make_sphere(float *data, float r, int detail, const float *const a,
                 const float *const b, const float *const c,
                 const float *const ta, const float *const tb,
                 const float *const tc) {
  if (detail == 0) {
    float *d = data;
    *(d++) = a[0] * r;
    *(d++) = a[1] * r;
    *(d++) = a[2] * r;
    *(d++) = a[0];
    *(d++) = a[1];
    *(d++) = a[2];
    *(d++) = ta[0];
    *(d++) = ta[1];
    *(d++) = b[0] * r;
    *(d++) = b[1] * r;
    *(d++) = b[2] * r;
    *(d++) = b[0];
    *(d++) = b[1];
    *(d++) = b[2];
    *(d++) = tb[0];
    *(d++) = tb[1];
    *(d++) = c[0] * r;
    *(d++) = c[1] * r;
    *(d++) = c[2] * r;
    *(d++) = c[0];
    *(d++) = c[1];
    *(d++) = c[2];
    *(d++) = tc[0];
    *(d++) = tc[1];
    return 1;
  } else {
    float ab[3], ac[3], bc[3];
    for (int i = 0; i < 3; i++) {
      ab[i] = (a[i] + b[i]) / 2;
      ac[i] = (a[i] + c[i]) / 2;
      bc[i] = (b[i] + c[i]) / 2;
    }
    normalize(ab + 0, ab + 1, ab + 2);
    normalize(ac + 0, ac + 1, ac + 2);
    normalize(bc + 0, bc + 1, bc + 2);
    float tab[2], tac[2], tbc[2];
    tab[0] = 0;
    tab[1] = 1 - acosf(ab[1]) / PI;
    tac[0] = 0;
    tac[1] = 1 - acosf(ac[1]) / PI;
    tbc[0] = 0;
    tbc[1] = 1 - acosf(bc[1]) / PI;
    int total = 0, n;
    n = _make_sphere(data, r, detail - 1, a, ab, ac, ta, tab, tac);
    total += n;
    data += n * 24;
    n = _make_sphere(data, r, detail - 1, b, bc, ab, tb, tbc, tab);
    total += n;
    data += n * 24;
    n = _make_sphere(data, r, detail - 1, c, ac, bc, tc, tac, tbc);
    total += n;
    data += n * 24;
    n = _make_sphere(data, r, detail - 1, ab, bc, ac, tab, tbc, tac);
    total += n;
    data += n * 24;
    return total;
  }
}

void make_sphere(float *data, float r, int detail) {
  // detail, triangles, floats
  // 0, 8, 192
  // 1, 32, 768
  // 2, 128, 3072
  // 3, 512, 12288
  // 4, 2048, 49152
  // 5, 8192, 196608
  // 6, 32768, 786432
  // 7, 131072, 3145728
  static const int indices[8][3] = {{4, 3, 0}, {1, 4, 0}, {3, 4, 5}, {4, 1, 5},
                                    {0, 3, 2}, {0, 2, 1}, {5, 2, 3}, {5, 1, 2}};
  static const float positions[6][3] = {{0, 0, -1}, {1, 0, 0}, {0, -1, 0},
                                        {-1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
                     uvs[6][3] = {{0, 0.5}, {0, 0.5}, {0, 0},
                                  {0, 0.5}, {0, 1},   {0, 0.5}};
  int total = 0;
  for (int i = 0; i < 8; i++) {
    int n = _make_sphere(data, r, detail, positions[indices[i][0]],
                         positions[indices[i][1]], positions[indices[i][2]],
                         uvs[indices[i][0]], uvs[indices[i][1]],
                         uvs[indices[i][2]]);
    total += n;
    data += n * 24;
  }
}
