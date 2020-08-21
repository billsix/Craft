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

#ifndef _ring_h_
#define _ring_h_

typedef enum { BLOCK, LIGHT, KEY, COMMIT, EXIT } RingEntryType;

typedef struct {
  RingEntryType type;
  int p;
  int q;
  int x;
  int y;
  int z;
  int w;
  int key;
} RingEntry;

typedef struct {
  unsigned int capacity;
  unsigned int start;
  unsigned int end;
  RingEntry *data;
} Ring;

void ring_alloc(Ring *ring, int capacity);
void ring_free(Ring *ring);
int ring_empty(Ring *ring);
int ring_full(Ring *ring);
int ring_size(Ring *ring);
void ring_grow(Ring *ring);
void ring_put(Ring *ring, RingEntry *entry);
void ring_put_block(Ring *ring, int p, int q, int x, int y, int z, int w);
void ring_put_light(Ring *ring, int p, int q, int x, int y, int z, int w);
void ring_put_key(Ring *ring, int p, int q, int key);
void ring_put_commit(Ring *ring);
void ring_put_exit(Ring *ring);
int ring_get(Ring *ring, RingEntry *entry);

#endif
