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

#ifndef _cube_h_
#define _cube_h_

void make_cube(float *data, float ambient_occlusion[6][4], float light[6][4],
               int left, int right, int top, int bottom, int front, int back,
               float x, float y, float z, float n, int w);

void make_plant(float *data, float ambient_occlusion, float light, float px,
                float py, float pz, float n, int w, float rotation);

void make_player(float *data, float x, float y, float z, float rx, float ry);

void make_cube_wireframe(float *data, float x, float y, float z, float n);

void make_character(float *data, float x, float y, float n, float m, char c);

void make_character_3d(float *data, float x, float y, float z, float n,
                       int face, char c);

void make_sphere(float *data, float r, int detail);

#endif
