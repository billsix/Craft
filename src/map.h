#ifndef _map_h_
#define _map_h_

#define EMPTY_ENTRY(entry) ((entry)->value == 0)

/*
 * TODO - figure out what this is
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
 * TODO - figure out what this is, definitively
 *
 * I assume Map represents the terrain,
 * where dx dy dz are relative to the Chunk,
 * which is a higher level structure than the map
 */
typedef struct {
  int dx;
  int dy;
  int dz;
  unsigned int mask;
  unsigned int size;
  MapEntry *data;
} Map;

void map_alloc(Map *map, int dx, int dy, int dz, int mask);
void map_free(Map *map);
void map_copy(Map *dst, Map *src);
void map_grow(Map *map);
int map_set(Map *map, int x, int y, int z, int w);
int map_get(Map *map, int x, int y, int z);

#endif
