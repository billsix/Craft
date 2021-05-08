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

#ifndef _util_h_
#define _util_h_

#define PI 3.14159265359
#define DEGREES(radians) ((radians)*180 / PI)
#define RADIANS(degrees) ((degrees)*PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN_NUMBER(a, b) ((a) < (b) ? (a) : (b))
#define MAX_NUMBER(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

#if DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

typedef struct {
  unsigned int fps;
  unsigned int frames;
  double since;
} FPS;

int rand_int(int n);
double rand_double();
void update_fps(FPS *fps);
char *load_file(const char *path);
void flip_image_vertical(unsigned char *data, unsigned int width,
                         unsigned int height);
char *tokenize(char *str, const char *delim, char **key);
int char_width(char input);
int string_width(const char *input);
int wrap(const char *input, int max_width, char *output, int max_length);
float *malloc_faces(int components, int faces);

#ifdef __cplusplus
#define BEGIN_C_DECL extern "C" {
#define END_C_DECL }
#else
#define BEGIN_C_DECL
#define END_C_DECL
#endif

#endif
