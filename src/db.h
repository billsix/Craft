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

#ifndef _db_h_
#define _db_h_

void db_enable();
void db_disable();
int get_db_enabled();
int db_init(char *path);
void db_close();
void db_commit();
void db_auth_set(char *username, char *identity_token);
int db_auth_select(char *username);
void db_auth_select_none();
int db_auth_get(char *username, char *identity_token,
                int identity_token_length);
int db_auth_get_selected(char *username, int username_length,
                         char *identity_token, int identity_token_length);
void db_save_state(float x, float y, float z, float rx, float ry);
int db_load_state(float *x, float *y, float *z, float *rx, float *ry);
void db_insert_block(int p, int q, int x, int y, int z, int w);
void db_insert_light(int p, int q, int x, int y, int z, int w);
void db_insert_sign(int p, int q, int x, int y, int z, int face,
                    const char *text);
void db_delete_sign(int x, int y, int z, int face);
void db_delete_signs(int x, int y, int z);
void db_delete_all_signs();
void db_load_blocks(Map *map, int p, int q);
void db_load_lights(Map *map, int p, int q);
void db_load_signs(SignList *list, int p, int q);
int db_get_key(int p, int q);
void db_set_key(int p, int q, int key);
void db_worker_start();
void db_worker_stop();
int db_worker_run(void *arg);

#endif
