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

#include "ring.h"
#include <stdlib.h>
#include <string.h>

void ring_alloc(Ring *ring, int capacity) {
  ring->capacity = capacity;
  ring->start = 0;
  ring->end = 0;
  ring->data = (RingEntry *)calloc(capacity, sizeof(RingEntry));
}

void ring_free(Ring *ring) { free(ring->data); }

int ring_empty(Ring *ring) { return ring->start == ring->end; }

int ring_full(Ring *ring) {
  return ring->start == (ring->end + 1) % ring->capacity;
}

int ring_size(Ring *ring) {
  if (ring->end >= ring->start) {
    return ring->end - ring->start;
  } else {
    return ring->capacity - (ring->start - ring->end);
  }
}

void ring_grow(Ring *ring) {
  Ring new_ring;
  RingEntry entry;
  ring_alloc(&new_ring, ring->capacity * 2);
  while (ring_get(ring, &entry)) {
    ring_put(&new_ring, &entry);
  }
  free(ring->data);
  ring->capacity = new_ring.capacity;
  ring->start = new_ring.start;
  ring->end = new_ring.end;
  ring->data = new_ring.data;
}

void ring_put(Ring *ring, RingEntry *entry) {
  if (ring_full(ring)) {
    ring_grow(ring);
  }
  RingEntry *e = ring->data + ring->end;
  memcpy(e, entry, sizeof(RingEntry));
  ring->end = (ring->end + 1) % ring->capacity;
}

void ring_put_block(Ring *ring, int p, int q, int x, int y, int z, int w) {
  RingEntry entry;
  entry.type = BLOCK;
  entry.p = p;
  entry.q = q;
  entry.x = x;
  entry.y = y;
  entry.z = z;
  entry.w = w;
  ring_put(ring, &entry);
}

void ring_put_light(Ring *ring, int p, int q, int x, int y, int z, int w) {
  RingEntry entry;
  entry.type = LIGHT;
  entry.p = p;
  entry.q = q;
  entry.x = x;
  entry.y = y;
  entry.z = z;
  entry.w = w;
  ring_put(ring, &entry);
}

void ring_put_key(Ring *ring, int p, int q, int key) {
  RingEntry entry;
  entry.type = KEY;
  entry.p = p;
  entry.q = q;
  entry.key = key;
  ring_put(ring, &entry);
}

void ring_put_commit(Ring *ring) {
  RingEntry entry;
  entry.type = COMMIT;
  ring_put(ring, &entry);
}

void ring_put_exit(Ring *ring) {
  RingEntry entry;
  entry.type = EXIT;
  ring_put(ring, &entry);
}

int ring_get(Ring *ring, RingEntry *entry) {
  if (ring_empty(ring)) {
    return 0;
  }
  RingEntry *e = ring->data + ring->start;
  memcpy(entry, e, sizeof(RingEntry));
  ring->start = (ring->start + 1) % ring->capacity;
  return 1;
}
