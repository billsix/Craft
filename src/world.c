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

#include "world.h"
#include "config.h"
#include "noise.h"

void create_world(int p, int q, world_func func, void *arg) {
  int pad = 1;
  for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
    for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
      int flag = 1;
      if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
        flag = -1;
      }
      int x = p * CHUNK_SIZE + dx;
      int z = q * CHUNK_SIZE + dz;
      float f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
      float g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
      int mh = g * 32 + 16;
      int h = f * mh;
      int w = 1;
      int t = 12;
      if (h <= t) {
        h = t;
        w = 2;
      }
      // sand and grass terrain
      for (int y = 0; y < h; y++) {
        func(x, y, z, w * flag, arg);
      }
      if (w == 1) {
        if (SHOW_PLANTS) {
          // grass
          if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
            func(x, h, z, 17 * flag, arg);
          }
          // flowers
          if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
            int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
            func(x, h, z, w * flag, arg);
          }
        }
        // trees
        int ok = SHOW_TREES;
        if (dx - 4 < 0 || dz - 4 < 0 || dx + 4 >= CHUNK_SIZE ||
            dz + 4 >= CHUNK_SIZE) {
          ok = 0;
        }
        if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
          for (int y = h + 3; y < h + 8; y++) {
            for (int ox = -3; ox <= 3; ox++) {
              for (int oz = -3; oz <= 3; oz++) {
                int d = (ox * ox) + (oz * oz) + (y - (h + 4)) * (y - (h + 4));
                if (d < 11) {
                  func(x + ox, y, z + oz, 15, arg);
                }
              }
            }
          }
          for (int y = h; y < h + 7; y++) {
            func(x, y, z, 5, arg);
          }
        }
      }
      // clouds
      if (SHOW_CLOUDS) {
        for (int y = 64; y < 72; y++) {
          if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
            func(x, y, z, 16 * flag, arg);
          }
        }
      }
    }
  }
}
