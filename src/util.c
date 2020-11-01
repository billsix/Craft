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

#include "util.h"
#include <GLFW/glfw3.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rand_int(int n) {
  int result;
  while (n <= (result = rand() / (RAND_MAX / n)))
    ;
  return result;
}

double rand_double() { return (double)rand() / (double)RAND_MAX; }

void update_fps(FPS *fps) {
  fps->frames++;
  const double now = glfwGetTime();
  const double elapsed = now - fps->since;
  if (elapsed >= 1) {
    fps->fps = round(fps->frames / elapsed);
    fps->frames = 0;
    fps->since = now;
  }
}

char *load_file(const char *path) {
  FILE *const file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "fopen %s failed: %d %s\n", path, errno, strerror(errno));
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  int length = ftell(file);
  rewind(file);
  char *const data = calloc(length + 1, sizeof(char));
  fread(data, 1, length, file);
  fclose(file);
  return data;
}

void flip_image_vertical(unsigned char *data, unsigned int width,
                         unsigned int height) {
  const unsigned int size = width * height * 4;
  const unsigned int stride = sizeof(char) * width * 4;
  unsigned char *const new_data = malloc(sizeof(unsigned char) * size);
  for (unsigned int i = 0; i < height; i++) {
    unsigned int j = height - i - 1;
    memcpy(new_data + j * stride, data + i * stride, stride);
  }
  memcpy(data, new_data, size);
  free(new_data);
}

char *tokenize(char *str, const char *delim, char **key) {
  char *result;
  if (str == NULL) {
    str = *key;
  }
  str += strspn(str, delim);
  if (*str == '\0') {
    return NULL;
  }
  result = str;
  str += strcspn(str, delim);
  if (*str) {
    *str++ = '\0';
  }
  *key = str;
  return result;
}

int char_width(char input) {
  static const int lookup[128] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 2, 4, 7, 6, 9, 7, 2, 3, 3, 4, 6,
      3, 5, 2, 7, 6, 3, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 6, 5, 7, 8, 6,
      6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 8, 8, 6, 6, 7, 6, 6, 6, 6, 8, 10,
      8, 6, 6, 3, 6, 3, 6, 6, 4, 7, 6, 6, 6, 6, 5, 6, 6, 2, 5, 5, 2, 9,
      6, 6, 6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 2, 5, 7, 0};
  return lookup[input];
}

int string_width(const char *input) {
  int result = 0;
  const int length = strlen(input);
  for (int i = 0; i < length; i++) {
    result += char_width(input[i]);
  }
  return result;
}

int wrap(const char *input, int max_width, char *output, int max_length) {
  *output = '\0';
  char *const text = malloc(sizeof(char) * (strlen(input) + 1));
  strcpy(text, input);
  const int space_width = char_width(' ');
  int line_number = 0;
  char *key1, *key2;
  char *line = tokenize(text, "\r\n", &key1);
  while (line) {
    int line_width = 0;
    char *token = tokenize(line, " ", &key2);
    while (token) {
      int token_width = string_width(token);
      if (line_width) {
        if (line_width + token_width > max_width) {
          line_width = 0;
          line_number++;
          strncat(output, "\n", max_length - strlen(output) - 1);
        } else {
          strncat(output, " ", max_length - strlen(output) - 1);
        }
      }
      strncat(output, token, max_length - strlen(output) - 1);
      line_width += token_width + space_width;
      token = tokenize(NULL, " ", &key2);
    }
    line_number++;
    strncat(output, "\n", max_length - strlen(output) - 1);
    line = tokenize(NULL, "\r\n", &key1);
  }
  free(text);
  return line_number;
}

float *malloc_faces(int components, int faces) {
  return (float *)malloc(sizeof(float) * 6 * components * faces);
}
