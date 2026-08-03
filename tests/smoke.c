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

/*
 * Sanitizer smoke harness for Craft's GL-free core.
 *
 * This is NOT part of the game binary. It drives the non-GL, non-GPU logic of
 * src/ (world-gen, the voxel hash map, ring buffer, sqlite persistence,
 * cube/plant/text mesh geometry, 4x4 matrix math, item tables, sign list, and
 * the pure util helpers) with adversarial inputs so that the address and
 * undefined-behaviour sanitizers have code paths to exercise. Any trap (SIGILL
 * from UBSan trap mode) or abort (ASan) fails the build gate. See
 * github.com/billsix/Craft tasks/add-sanitizer-gate.md.
 *
 * The render/input/networking code (main.c, gl_render.c, client.c, auth.c,
 * gui*) needs a display and a GPU context and is out of scope here.
 */

/* for nanosleep under -std=c11 */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* db.h references Map and SignList, so their headers must precede it. */
#include "map.h"
#include "sign.h"

#include "cube.h"
#include "db.h"
#include "item.h"
#include "matrix.h"
#include "ring.h"
#include "util.h"
#include "world.h"

/*
 * Keep leak detection off: several src/ units (the map, the db worker's
 * prepared statements, item tables) legitimately hold allocations for process
 * lifetime, and this harness intentionally exercises alloc-heavy paths without
 * chasing every free. The gate is for *corruption* (overflow / use-after-free /
 * double-free) and undefined behaviour, not unfreed-at-exit memory.
 * Guarded so non-ASan builds ignore it.
 */
#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
#if !defined(__SANITIZE_ADDRESS__)
#if __has_feature(address_sanitizer)
#define CRAFT_ASAN 1
#endif
#else
#define CRAFT_ASAN 1
#endif
#endif

#if defined(CRAFT_ASAN)
const char *__asan_default_options(void) { return "detect_leaks=0"; }
#endif

/* Sink so the optimizer cannot elide the geometry generators. */
static volatile float g_sink = 0.0f;
static void consume(const float *data, int count) {
  float acc = 0.0f;
  for (int i = 0; i < count; i++) {
    acc += data[i];
  }
  g_sink += acc;
}

/* world_func that stores into a Map, mirroring how the game fills a chunk. */
static void world_into_map(int x, int y, int z, int w, void *arg) {
  Map *map = (Map *)arg;
  /* Block coords are reduced into the map's local [0,255] window the same way
   * the game does when creating a chunk. */
  map_set(map, x & 0xff, y & 0xff, z & 0xff, w ? w : 1);
}

static void exercise_map(void) {
  Map map;
  /* Deliberately small initial mask so map_grow rehashing runs many times. */
  map_alloc(&map, 0, 0, 0, 0x7);
  for (int x = 0; x < 40; x++) {
    for (int z = 0; z < 40; z++) {
      map_set(&map, x, (x + z) % 32, z, 1 + ((x + z) % 15));
    }
  }
  /* Overwrite an existing entry (the overwrite branch of map_set). */
  map_set(&map, 5, 5, 5, 7);
  map_set(&map, 5, 5, 5, 0); /* w==0 on existing entry: no-op branch */
  /* Hits and out-of-window misses. */
  long checksum = 0;
  for (int x = -4; x < 44; x++) {
    for (int z = -4; z < 44; z++) {
      checksum += map_get(&map, x, (x + z) % 32, z);
    }
  }
  if (checksum < 0) {
    printf("map checksum %ld\n", checksum);
  }
  Map copy;
  map_copy(&copy, &map);
  map_free(&copy);
  map_free(&map);
}

static void exercise_world(void) {
  Map map;
  map_alloc(&map, 0, 0, 0, 0xfff);
  for (int p = -1; p <= 1; p++) {
    for (int q = -1; q <= 1; q++) {
      create_world(p, q, world_into_map, &map);
    }
  }
  map_free(&map);
}

static void exercise_ring(void) {
  Ring ring;
  /* Small capacity so ring_grow triggers. */
  ring_alloc(&ring, 4);
  for (int i = 0; i < 50; i++) {
    switch (i % 5) {
      case 0:
        ring_put_block(&ring, i, i, i, i, i, i % 16);
        break;
      case 1:
        ring_put_light(&ring, i, i, i, i, i, i % 16);
        break;
      case 2:
        ring_put_key(&ring, i, i, i);
        break;
      case 3:
        ring_put_commit(&ring);
        break;
      default:
        ring_put_exit(&ring);
        break;
    }
  }
  (void)ring_size(&ring);
  (void)ring_empty(&ring);
  (void)ring_full(&ring);
  RingEntry e;
  int drained = 0;
  while (ring_get(&ring, &e)) {
    drained++;
  }
  if (drained < 0) {
    printf("ring drained %d\n", drained);
  }
  ring_free(&ring);
}

static void exercise_sign(void) {
  SignList list;
  sign_list_alloc(&list, 2); /* small so sign_list_grow triggers */
  for (int i = 0; i < 20; i++) {
    char text[80];
    snprintf(text, sizeof(text), "sign number %d with some words to wrap", i);
    sign_list_add(&list, i, 0, i % 3, i % 6, text);
  }
  /* Add a duplicate coordinate (drives the remove-then-add path). */
  sign_list_add(&list, 1, 0, 1, 1, "overwrite");
  /* A text longer than MAX_SIGN_LENGTH to drive the strncpy truncation. */
  char longtext[256];
  memset(longtext, 'x', sizeof(longtext) - 1);
  longtext[sizeof(longtext) - 1] = '\0';
  sign_list_add(&list, 99, 99, 99, 0, longtext);
  (void)sign_list_remove(&list, 3, 0, 0, 3);
  (void)sign_list_remove_all(&list, 6, 0, 0);
  sign_list_free(&list);
}

static void exercise_db(void) {
  db_enable();
  int rc = db_init(":memory:");
  if (rc) {
    printf("db_init returned %d\n", rc);
    db_disable();
    return;
  }

  /* Synchronous paths: signs, auth, state. */
  for (int i = 0; i < 8; i++) {
    db_insert_sign(0, 0, i, 0, i, i % 6, "hello sign");
  }
  db_delete_sign(0, 0, 0, 0);
  db_delete_signs(0, 0, 3);

  db_auth_set("player", "identity-token-value");
  char uname[64], token[128];
  (void)db_auth_get("player", token, (int)sizeof(token));
  (void)db_auth_get_selected(uname, (int)sizeof(uname), token,
                             (int)sizeof(token));
  (void)db_auth_select("player");
  db_auth_select_none();

  db_save_state(1.5f, 2.5f, 3.5f, 0.25f, -0.5f);
  float x, y, z, rx, ry;
  (void)db_load_state(&x, &y, &z, &rx, &ry);

  /* Async paths through the ring / worker thread. */
  for (int i = 0; i < 16; i++) {
    db_insert_block(0, 0, i, i, i, 1 + (i % 15));
    db_insert_light(0, 0, i, i, i, i % 16);
    db_set_key(0, 0, i);
  }
  db_commit();
  /* Give the worker thread a moment to drain the ring so the load paths below
   * have rows to read (best effort; db_close joins the worker regardless). */
  struct timespec ts = {0, 100L * 1000L * 1000L};
  nanosleep(&ts, NULL);

  Map map;
  map_alloc(&map, 0, 0, 0, 0xfff);
  db_load_blocks(&map, 0, 0);
  db_load_lights(&map, 0, 0);
  map_free(&map);

  SignList signs;
  sign_list_alloc(&signs, 4);
  db_load_signs(&signs, 0, 0);
  sign_list_free(&signs);

  (void)db_get_key(0, 0);

  db_close();
  db_disable();

  /* db_init attaches 'auth.db' as a side file; clean it up. */
  remove("auth.db");
  remove("auth.db-journal");
}

static void exercise_item(void) {
  long acc = 0;
  /* Range includes the negative ids world-gen produces (w * flag). */
  for (int w = -64; w <= 64; w++) {
    acc += is_plant(w);
    acc += is_obstacle(w);
    acc += is_transparent(w);
    acc += is_destructable(w);
  }
  for (int i = 0; i < item_count; i++) {
    acc += items[i];
  }
  if (acc < 0) {
    printf("item acc %ld\n", acc);
  }
}

static void exercise_matrix(void) {
  float a[16], b[16], m[16];
  float vec[4] = {1, 2, 3, 1};
  float planes[6][4];
  mat_identity(a);
  mat_translate(b, 1, 2, 3);
  mat_multiply(m, a, b);
  mat_rotate(a, 0.3f, 0.6f, 0.1f, 1.2f);
  mat_multiply(m, a, b);
  mat_vec_multiply(vec, m, vec);
  mat_frustum(a, -1, 1, -1, 1, 0.1f, 100.0f);
  mat_perspective(b, 65.0f, 1.6f, 0.125f, 400.0f);
  mat_ortho(m, -10, 10, -10, 10, -1, 1);
  frustum_planes(planes, 10, b);
  set_matrix_2d(m, 1024, 768);
  set_matrix_3d(m, 1024, 768, 5, 20, 5, 0.1f, -0.2f, 65.0f, 0, 10);
  set_matrix_3d(m, 1024, 768, 5, 20, 5, 0.1f, -0.2f, 65.0f, 4, 10);
  set_matrix_item(m, 1024, 768, 1);
  float nx = 3, ny = 4, nz = 0;
  normalize(&nx, &ny, &nz);
  consume(m, 16);
  consume(planes[0], 4);
}

static void exercise_cube(void) {
  float ao[6][4] = {{0}};
  float light[6][4];
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      ao[i][j] = (float)((i + j) % 3) * 0.25f;
      light[i][j] = 0.8f;
    }
  }

  /* make_cube writes up to 6 faces * 6 verts * 10 floats = 360. */
  float *cube = malloc(sizeof(float) * 360);
  for (int i = 0; i < item_count; i++) {
    int w = items[i];
    if (w < 0 || w > 255) continue;
    make_cube(cube, ao, light, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0.5f, w);
    consume(cube, 360);
  }
  free(cube);

  /* make_plant writes 4 quads * 6 verts * 10 floats = 240. */
  float *plant = malloc(sizeof(float) * 240);
  int plant_ids[] = {17, 18, 19, 20, 21, 22, 23};
  for (unsigned i = 0; i < sizeof(plant_ids) / sizeof(int); i++) {
    make_plant(plant, 0.5f, 0.8f, 0, 0, 0, 0.5f, plant_ids[i], 45.0f);
    consume(plant, 240);
  }
  free(plant);

  /* make_player writes 6 faces * 60 = 360. */
  float *player = malloc(sizeof(float) * 360);
  make_player(player, 3, 4, 5, 0.4f, -0.2f);
  consume(player, 360);
  free(player);

  /* make_cube_wireframe writes 24 verts * 3 floats = 72. */
  float *wire = malloc(sizeof(float) * 72);
  make_cube_wireframe(wire, 1, 1, 1, 0.5f);
  consume(wire, 72);
  free(wire);

  /* make_character writes 6 verts * 4 floats = 24. */
  float *ch = malloc(sizeof(float) * 24);
  for (char c = 32; c < 127; c++) {
    make_character(ch, 10, 10, 4, 6, c);
    consume(ch, 24);
  }
  free(ch);

  /* make_character_3d writes 6 verts * 5 floats = 30. */
  float *ch3 = malloc(sizeof(float) * 30);
  for (int face = 0; face < 8; face++) {
    make_character_3d(ch3, 0, 0, 0, 0.4f, face, 'A' + (face % 26));
    consume(ch3, 30);
  }
  free(ch3);

  /* make_sphere at increasing detail; detail 3 => 512 triangles => 12288. */
  for (int detail = 0; detail <= 3; detail++) {
    int triangles = 8;
    for (int d = 0; d < detail; d++) triangles *= 4;
    int floats = triangles * 24;
    float *sphere = malloc(sizeof(float) * floats);
    make_sphere(sphere, 1.0f, detail);
    consume(sphere, floats);
    free(sphere);
  }
}

static void exercise_util(void) {
  /* Deterministic rng path. */
  srand(1234);
  long acc = 0;
  for (int i = 0; i < 100; i++) {
    acc += rand_int(37);
  }
  acc += (long)(rand_double() * 1000.0);

  /* Text width helpers over the printable range. */
  acc += string_width("Craft: a voxel engine in C");
  for (char c = 0; c >= 0 && c < 127; c++) {
    acc += char_width(c);
  }

  /* wrap() over multi-line, multi-word input into a generous buffer, the way
   * the game wraps chat / info text. */
  char out[1024];
  int lines = wrap(
      "the quick brown fox jumps over the lazy dog\nand then keeps on running "
      "across a very wide field of text that must be wrapped into lines",
      120, out, (int)sizeof(out));
  acc += lines;

  /* tokenize a copy in place. */
  char buf[64];
  strcpy(buf, "alpha,beta,,gamma");
  char *key;
  char *tok = tokenize(buf, ",", &key);
  while (tok) {
    acc += (long)strlen(tok);
    tok = tokenize(NULL, ",", &key);
  }

  /* flip_image_vertical on a small RGBA buffer (pure, no GL). */
  unsigned int w = 4, h = 3;
  unsigned char *img = malloc(w * h * 4);
  for (unsigned int i = 0; i < w * h * 4; i++) img[i] = (unsigned char)i;
  flip_image_vertical(img, w, h);
  acc += img[0];
  free(img);

  float *faces = malloc_faces(10, 12);
  faces[0] = 1.0f;
  faces[6 * 10 * 12 - 1] = 2.0f;
  consume(faces, 6 * 10 * 12);
  free(faces);

  if (acc == 0x7fffffff) {
    printf("util acc %ld\n", acc);
  }
}

int main(void) {
  printf("craft sanitizer smoke harness: start\n");
  exercise_map();
  exercise_world();
  exercise_ring();
  exercise_sign();
  exercise_item();
  exercise_matrix();
  exercise_cube();
  exercise_util();
  exercise_db();
  printf("craft sanitizer smoke harness: ok (sink=%f)\n", (double)g_sink);
  return 0;
}
