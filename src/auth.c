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

#include "auth.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POST_LENGTH 1024
#define MAX_RESPONSE_LENGTH 1024

size_t write_function(char *data, size_t size, size_t count, void *arg) {
  const size_t length = size * count;
  char *const dst = (char *)arg;
  char *const src = malloc(length + 1);
  memcpy(src, data, length);
  src[length] = '\0';
  strncat(dst, src, MAX_RESPONSE_LENGTH - strlen(dst) - 1);
  free(src);
  return length;
}

int get_access_token(char *result, int length, char *username,
                     char *identity_token) {
  static char url[] = "https://craft.michaelfogleman.com/api/1/identity";
  strncpy(result, "", length);
  CURL *curl = curl_easy_init();
  if (curl) {
    char post[MAX_POST_LENGTH] = {0};
    char response[MAX_RESPONSE_LENGTH] = {0};
    long http_code = 0;
    snprintf(post, MAX_POST_LENGTH, "username=%s&identity_token=%s", username,
             identity_token);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    CURLcode code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (code == CURLE_OK && http_code == 200) {
      strncpy(result, response, length);
      return 1;
    }
  }
  return 0;
}
