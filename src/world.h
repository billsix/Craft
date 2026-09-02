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

#ifndef _world_h_
#define _world_h_

/*
 * Callback invoked once per generated block: (world_x, world_y, world_z,
 * block_type, arg). A negative block_type marks a block on the one-block pad
 * that belongs to a neighbouring chunk; callers absorb the sign with ABS().
 */
typedef void (*world_func)(int, int, int, int, void *);

/*
 * Generate the terrain for the chunk at column (chunk_x, chunk_z) - chunk
 * indices along world X and Z respectively, each chunk being CHUNK_SIZE
 * blocks square - emitting every block through func.
 */
void create_world(int chunk_x, int chunk_z, world_func func, void *arg);

#endif
