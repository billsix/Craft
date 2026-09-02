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

void create_world(int chunk_x, int chunk_z, world_func func, void *arg) {
  // chunk_x, chunk_z: this chunk's column indices along world X and Z.
  // local_x, local_z: the block's offset within the chunk (0..CHUNK_SIZE-1),
  //   plus a one-block pad on each side (hence -pad .. CHUNK_SIZE+pad).
  // x, y, z below are the resulting world block coordinates (y is up).
  int pad = 1;
  for (int local_x = -pad; local_x < CHUNK_SIZE + pad; local_x++) {
    for (int local_z = -pad; local_z < CHUNK_SIZE + pad; local_z++) {
      // +1 for real interior blocks; -1 marks the 1-block pad that reports a
      // neighbouring chunk's edge (callers absorb the sign via ABS()).
      int boundary_sign = 1;
      if (local_x < 0 || local_z < 0 || local_x >= CHUNK_SIZE ||
          local_z >= CHUNK_SIZE) {
        boundary_sign = -1;
      }
      int x = chunk_x * CHUNK_SIZE + local_x;
      int z = chunk_z * CHUNK_SIZE + local_z;
      float base_noise = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
      float amplitude_noise = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
      int height_scale = amplitude_noise * 32 + 16;
      int terrain_height = base_noise * height_scale;
      int w = 1;
      int sea_level = 12;
      if (terrain_height <= sea_level) {
        terrain_height = sea_level;
        w = 2;
      }
      // sand and grass terrain
      for (int y = 0; y < terrain_height; y++) {
        func(x, y, z, w * boundary_sign, arg);
      }
      if (w == 1) {
        if (SHOW_PLANTS) {
          // grass
          if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
            func(x, terrain_height, z, 17 * boundary_sign, arg);
          }
          // flowers
          if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
            int flower = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
            func(x, terrain_height, z, flower * boundary_sign, arg);
          }
        }
        // trees
        int place_tree = SHOW_TREES;
        if (local_x - 4 < 0 || local_z - 4 < 0 || local_x + 4 >= CHUNK_SIZE ||
            local_z + 4 >= CHUNK_SIZE) {
          place_tree = 0;
        }
        if (place_tree && simplex2(x, z, 6, 0.5, 2) > 0.84) {
          for (int y = terrain_height + 3; y < terrain_height + 8; y++) {
            for (int ox = -3; ox <= 3; ox++) {
              for (int oz = -3; oz <= 3; oz++) {
                int dist_sq = (ox * ox) + (oz * oz) +
                              (y - (terrain_height + 4)) *
                                  (y - (terrain_height + 4));
                if (dist_sq < 11) {
                  func(x + ox, y, z + oz, 15, arg);
                }
              }
            }
          }
          for (int y = terrain_height; y < terrain_height + 7; y++) {
            func(x, y, z, 5, arg);
          }
        }
      }
      // clouds
      if (SHOW_CLOUDS) {
        for (int y = 64; y < 72; y++) {
          if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
            func(x, y, z, 16 * boundary_sign, arg);
          }
        }
      }
    }
  }
}
