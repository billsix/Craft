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

#include "map.h"
#include <stdlib.h>
#include <string.h>

/*
 * Hash an integer.
 *
 * https://en.wikipedia.org/wiki/Hash_function
 */
int hash_int(int key) {
  /*
   * Thomas Wang's integer hash. The shifts and the multiply are meant to wrap
   * modulo 2^32, so compute in unsigned to keep that well-defined (a signed
   * left-shift of a negative value and a signed multiply overflow are both
   * undefined behaviour). Only the low bits are used, via `& map->mask`.
   */
  unsigned int h = (unsigned int)key;
  h = ~h + (h << 15);
  h = h ^ (h >> 12);
  h = h + (h << 2);
  h = h ^ (h >> 4);
  h = h * 2057;
  h = h ^ (h >> 16);
  return (int)h;
}

/*
 * Hash a triplet of x y and z
 *
 * https://en.wikipedia.org/wiki/Hash_function
 */
int hash(int x, int y, int z) {
  x = hash_int(x);
  y = hash_int(y);
  z = hash_int(z);
  return x ^ y ^ z;
}

/*
 * Allocate memory on the heap for a Map's data.
 * Initialize the struct.
 *
 */
void map_alloc(Map *map, int origin_x, int origin_y, int origin_z, int mask) {
  map->origin_x = origin_x;
  map->origin_y = origin_y;
  map->origin_z = origin_z;
  map->mask = mask;
  map->size = 0;
  map->data = (MapEntry *)calloc(map->mask + 1, sizeof(MapEntry));
}

/*
 * Free the map's data from the heap.
 */
void map_free(Map *map) { free(map->data); }

/*
 * Copy the source map into a destination map.
 * The map data needs to be heap allocated.
 */
void map_copy(Map *dst, Map *src) {
  dst->origin_x = src->origin_x;
  dst->origin_y = src->origin_y;
  dst->origin_z = src->origin_z;
  dst->mask = src->mask;
  dst->size = src->size;
  dst->data = (MapEntry *)calloc(dst->mask + 1, sizeof(MapEntry));
  memcpy(dst->data, src->data, (dst->mask + 1) * sizeof(MapEntry));
}

/*
 * Store block type w at world coordinate (x, y, z). Returns 1 if the map
 * changed, 0 if not.
 *
 * The world coordinate is converted to storage-local coordinates by
 * subtracting the map's origin, then linear-probed into the table. Setting
 * w == 0 on a key that is not present is a no-op (there is no real deletion).
 */
int map_set(Map *map, int x, int y, int z, int w) {
  unsigned int index = hash(x, y, z) & map->mask;
  x -= map->origin_x;
  y -= map->origin_y;
  z -= map->origin_z;
  MapEntry *entry = map->data + index;
  int overwrite = 0;
  while (!EMPTY_ENTRY(entry)) {
    if (entry->e.x == x && entry->e.y == y && entry->e.z == z) {
      overwrite = 1;
      break;
    }
    index = (index + 1) & map->mask;
    entry = map->data + index;
  }
  if (overwrite) {
    if (entry->e.w != w) {
      entry->e.w = w;
      return 1;
    }
  } else if (w) {
    entry->e.x = x;
    entry->e.y = y;
    entry->e.z = z;
    entry->e.w = w;
    map->size++;
    if (map->size * 2 > map->mask) {
      map_grow(map);
    }
    return 1;
  }
  return 0;
}

int map_get(const Map *const map, int x, int y, int z) {
  unsigned int index = hash(x, y, z) & map->mask;
  x -= map->origin_x;
  y -= map->origin_y;
  z -= map->origin_z;
  if (x < 0 || x > 255) return 0;
  if (y < 0 || y > 255) return 0;
  if (z < 0 || z > 255) return 0;
  MapEntry *entry = map->data + index;
  while (!EMPTY_ENTRY(entry)) {
    if (entry->e.x == x && entry->e.y == y && entry->e.z == z) {
      return entry->e.w;
    }
    index = (index + 1) & map->mask;
    entry = map->data + index;
  }
  return 0;
}

void map_grow(Map *map) {
  Map new_map;
  new_map.origin_x = map->origin_x;
  new_map.origin_y = map->origin_y;
  new_map.origin_z = map->origin_z;
  new_map.mask = (map->mask << 1) | 1;
  new_map.size = 0;
  new_map.data = (MapEntry *)calloc(new_map.mask + 1, sizeof(MapEntry));
  for (unsigned int i = 0; i <= map->mask; i++) {
    MapEntry *entry = map->data + i;
    if (EMPTY_ENTRY(entry)) {
      continue;
    }
    int ex = entry->e.x + map->origin_x;
    int ey = entry->e.y + map->origin_y;
    int ez = entry->e.z + map->origin_z;
    int ew = entry->e.w;
    map_set(&new_map, ex, ey, ez, ew);
  }
  free(map->data);
  map->mask = new_map.mask;
  map->size = new_map.size;
  map->data = new_map.data;
}
