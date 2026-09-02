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

#ifndef _map_h_
#define _map_h_

#define EMPTY_ENTRY(entry) ((entry)->value == 0)

/*
 * One slot in the voxel hash map: a single stored block.
 *
 * The union lets a slot be tested for "empty" with one 32-bit compare
 * (`value == 0`, see EMPTY_ENTRY) while normally being read field-by-field.
 * The `e` fields are:
 *   x, y, z - the block's coordinates *local to this map*, i.e. the world
 *             coordinate minus the map's origin (see Map below). They are one
 *             byte each, so a map addresses a 256x256x256 window (chunks only
 *             ever use a small corner of it).
 *   w       - the block type ("what" it is: grass, sand, stone, ...). Signed,
 *             because world generation marks a neighbouring chunk's edge
 *             blocks with a negative type; callers take ABS(w) to recover it.
 */
typedef union {
  unsigned int value;
  struct {
    unsigned char x;
    unsigned char y;
    unsigned char z;
    char w;
  } e;
} MapEntry;

/*
 * The voxel hash map: an open-addressing (linear-probe) hash table from a
 * block coordinate (x, y, z) to its block type w. One Map holds one chunk's
 * blocks (Chunk.map) or its light levels (Chunk.lights).
 *
 * origin_x/origin_y/origin_z are the map's world-space origin. Stored entry
 * coordinates are *relative to this origin*, so map_set/map_get subtract the
 * origin to turn a world coordinate into the local 0..255 byte stored in the
 * entry, and map_grow adds it back to recover the world coordinate. For a
 * chunk at column (p, q) the game sets origin_x = p*CHUNK_SIZE - 1,
 * origin_y = 0, origin_z = q*CHUNK_SIZE - 1 (the -1 is a one-block pad so a
 * chunk can also hold its neighbours' edge blocks). Keeping only the low
 * bytes is what lets an entry pack into 32 bits.
 *
 * mask is (power-of-two - 1); index = hash(x,y,z) & mask. size is the entry
 * count; the table doubles (map_grow) once it is half full.
 */
typedef struct {
  int origin_x;
  int origin_y;
  int origin_z;
  unsigned int mask;
  unsigned int size;
  MapEntry *data;
} Map;

void map_alloc(Map *map, int origin_x, int origin_y, int origin_z, int mask);
void map_free(Map *map);
void map_copy(Map *dst, Map *src);
void map_grow(Map *map);
int map_set(Map *map, int x, int y, int z, int w);
int map_get(const Map *const map, int x, int y, int z);

#endif
