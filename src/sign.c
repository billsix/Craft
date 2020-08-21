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

#include "sign.h"
#include <stdlib.h>
#include <string.h>

void sign_list_alloc(SignList *list, int capacity) {
  list->capacity = capacity;
  list->size = 0;
  list->data = (Sign *)calloc(capacity, sizeof(Sign));
}

void sign_list_free(SignList *list) { free(list->data); }

void sign_list_grow(SignList *list) {
  SignList new_list;
  sign_list_alloc(&new_list, list->capacity * 2);
  memcpy(new_list.data, list->data, list->size * sizeof(Sign));
  free(list->data);
  list->capacity = new_list.capacity;
  list->data = new_list.data;
}

void _sign_list_add(SignList *list, Sign *sign) {
  if (list->size == list->capacity) {
    sign_list_grow(list);
  }
  Sign *e = list->data + list->size++;
  memcpy(e, sign, sizeof(Sign));
}

void sign_list_add(SignList *list, int x, int y, int z, int face,
                   const char *text) {
  sign_list_remove(list, x, y, z, face);
  Sign sign;
  sign.x = x;
  sign.y = y;
  sign.z = z;
  sign.face = face;
  strncpy(sign.text, text, MAX_SIGN_LENGTH);
  sign.text[MAX_SIGN_LENGTH - 1] = '\0';
  _sign_list_add(list, &sign);
}

int sign_list_remove(SignList *list, int x, int y, int z, int face) {
  int result = 0;
  for (int i = 0; i < list->size; i++) {
    Sign *e = list->data + i;
    if (e->x == x && e->y == y && e->z == z && e->face == face) {
      Sign *other = list->data + (--list->size);
      memcpy(e, other, sizeof(Sign));
      i--;
      result++;
    }
  }
  return result;
}

int sign_list_remove_all(SignList *list, int x, int y, int z) {
  int result = 0;
  for (int i = 0; i < list->size; i++) {
    Sign *e = list->data + i;
    if (e->x == x && e->y == y && e->z == z) {
      Sign *other = list->data + (--list->size);
      memcpy(e, other, sizeof(Sign));
      i--;
      result++;
    }
  }
  return result;
}
