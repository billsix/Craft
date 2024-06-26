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

#include "config.h"

#ifdef ENABLE_OPENGL_CORE_PROFILE_RENDERER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "GL/gl3w.h"

#include "tinycthread.h"
#include <GLFW/glfw3.h>

// N.B - this whitespace is required so that clang-format
// does not break the build.

#include "cube.h"
#include "lodepng.h"
#include "map.h"
#include "sign.h"
#include "util.h"

#include <stdbool.h>

#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl_render.h"

// TEXTURE ids
uint32_t texture, font, sky, sign;

Block_Attributes block_attrib;
Line_Attributes line_attrib;
Text_Attributes text_attrib;
Sky_Attributes sky_attrib;

void gl_viewport(GLint x_min, GLint y_min, GLsizei x_width, GLsizei y_width) {
  glViewport(x_min, y_min, x_width, y_width);
}

void gl_clear_depth_buffer() { glClear(GL_DEPTH_BUFFER_BIT); }

void gl_clear_color_buffer() { glClear(GL_COLOR_BUFFER_BIT); }

void gl_enable_scissor_test() { glEnable(GL_SCISSOR_TEST); }

void gl_disable_scissor_test() { glDisable(GL_SCISSOR_TEST); }

void gl_scissor(GLint x_min, GLint y_min, GLsizei x_width, GLsizei y_height) {
  glScissor(x_min, y_min, x_width, y_height);
}

GLuint gl_gen_buffer(GLsizei size, const float *const data) {
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return buffer;
}

void gl_del_buffer(GLuint buffer) { glDeleteBuffers(1, &buffer); }

GLuint gl_gen_faces(int components, int faces, float *data) {
  GLuint buffer = gl_gen_buffer(sizeof(float) * 6 * components * faces, data);
  free(data);
  return buffer;
}

GLuint gl_make_shader(GLenum type, const char *const source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int32_t status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    int32_t length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char *info = calloc(length, sizeof(char));
    glGetShaderInfoLog(shader, length, NULL, info);
    fprintf(stderr, "glCompileShader failed:\n%s\n", info);
    free(info);
  }
  return shader;
}

GLuint gl_load_shader(GLenum type, const char *const path) {
  char *data = load_file(path);
  GLuint result = gl_make_shader(type, data);
  free(data);
  return result;
}

GLuint gl_make_program(GLuint shader1, GLuint shader2) {
  GLuint program = glCreateProgram();
  glAttachShader(program, shader1);
  glAttachShader(program, shader2);
  glLinkProgram(program);
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int32_t length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char *info = calloc(length, sizeof(char));
    glGetProgramInfoLog(program, length, NULL, info);
    fprintf(stderr, "glLinkProgram failed: %s\n", info);
    free(info);
  }
  glDetachShader(program, shader1);
  glDetachShader(program, shader2);
  glDeleteShader(shader1);
  glDeleteShader(shader2);
  return program;
}

GLuint gl_load_program(const char *path1, const char *path2) {
  GLuint shader1 = gl_load_shader(GL_VERTEX_SHADER, path1),
         shader2 = gl_load_shader(GL_FRAGMENT_SHADER, path2),
         program = gl_make_program(shader1, shader2);
  return program;
}

void gl_load_png_texture(const char *const file_name) {
  unsigned int error;
  unsigned char *data;
  unsigned int width, height;
  error = lodepng_decode32_file(&data, &width, &height, file_name);
  if (error) {
    fprintf(stderr, "load_png_texture %s failed, error %u: %s\n", file_name,
            error, lodepng_error_text(error));
    exit(1);
  }
  flip_image_vertical(data, width, height);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  free(data);
}

void GLAPIENTRY GLMessageCallback(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar *message,
                                  const void *userParam) {
  char severity_name[100];
  memset(&severity_name, 0, sizeof(char) * sizeof(severity_name));
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: {
      const char name[] = "SEVERITY HIGH";
      strncpy(severity_name, name, strlen(name));
      break;
    }
    case GL_DEBUG_SEVERITY_MEDIUM: {
      const char name[] = "SEVERITY MED";
      strncpy(severity_name, name, strlen(name));
      break;
    }
    case GL_DEBUG_SEVERITY_LOW: {
      const char name[] = "SEVERITY LOW";
      strncpy(severity_name, name, strlen(name));
      break;
    }
    case GL_DEBUG_SEVERITY_NOTIFICATION: {
      const char name[] = "SEVERITY NOTIFICATION";
      strncpy(severity_name, name, strlen(name));
      break;
    }
  }
  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = %s, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type,
          severity_name, message);
}

int gl_graphics_loader_init() {
  int return_code = -1;  // error
  if (gl3wInit()) {
    goto exit;
  }
  printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION),
         glGetString(GL_SHADING_LANGUAGE_VERSION));
  if (gl3wIsSupported(3, 3)) {
    fprintf(stderr, "OpenGL 3.3 is supported\n");
    return_code = 0;
  }
  if (gl3wIsSupported(4, 3)) {
    fprintf(stderr, "OpenGL 4.3 is supported\n");

#if NDEBUG
#else
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
#endif
    return_code = 0;
  }

exit:
  return return_code;
}

void gl_initiliaze_global_state() {
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLogicOp(GL_INVERT);
  glClearColor(0, 0, 0, 1);

#define SHADER_DIR RESOURCE_PATH "/shaders/"

  int32_t program = gl_load_program(SHADER_DIR "block_vertex.glsl",
                                    SHADER_DIR "block_fragment.glsl");
  // initiliaze shaders
  block_attrib = (Block_Attributes){
      .program = program,
      .position = glGetAttribLocation(program, "position"),
      .normal = glGetAttribLocation(program, "normal"),
      .uv = glGetAttribLocation(program, "uv"),
      .matrix = glGetUniformLocation(program, "matrix"),
      .sampler = glGetUniformLocation(program, "sampler"),
      .camera = glGetUniformLocation(program, "camera"),
      .timer = glGetUniformLocation(program, "timer"),
      .sky_sampler = glGetUniformLocation(program, "sky_sampler"),
      .daylight = glGetUniformLocation(program, "daylight"),
      .fog_distance = glGetUniformLocation(program, "fog_distance"),
      .ortho = glGetUniformLocation(program, "ortho"),
      .enable_ambient_occlusion =
          glGetUniformLocation(program, "enable_ambient_occlusion")};

  program = gl_load_program(SHADER_DIR "line_vertex.glsl",
                            SHADER_DIR "line_fragment.glsl");
  line_attrib =
      (Line_Attributes){.program = program,
                        .position = glGetAttribLocation(program, "position"),
                        .matrix = glGetUniformLocation(program, "matrix")};

  program = gl_load_program(SHADER_DIR "text_vertex.glsl",
                            SHADER_DIR "text_fragment.glsl");
  text_attrib =
      (Text_Attributes){.program = program,
                        .position = glGetAttribLocation(program, "position"),
                        .uv = glGetAttribLocation(program, "uv"),
                        .matrix = glGetUniformLocation(program, "matrix"),
                        .sampler = glGetUniformLocation(program, "sampler"),
                        .is_sign = glGetUniformLocation(program, "is_sign")};

  program = gl_load_program(SHADER_DIR "sky_vertex.glsl",
                            SHADER_DIR "sky_fragment.glsl");
  sky_attrib =
      (Sky_Attributes){.program = program,
                       .position = glGetAttribLocation(program, "position"),
                       .normal = glGetAttribLocation(program, "normal"),
                       .uv = glGetAttribLocation(program, "uv"),
                       .matrix = glGetUniformLocation(program, "matrix"),
                       .sampler = glGetUniformLocation(program, "sampler"),
                       .timer = glGetUniformLocation(program, "timer")};
}

void gl_initiliaze_textures() {
#define TEXTURE_DIR RESOURCE_PATH "/textures/"
  glActiveTexture(GL_TEXTURE0);
  // LOAD TEXTURES //
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl_load_png_texture(TEXTURE_DIR "texture.png");

  glGenTextures(1, &font);
  glBindTexture(GL_TEXTURE_2D, font);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_load_png_texture(TEXTURE_DIR "font.png");

  glGenTextures(1, &sky);
  glBindTexture(GL_TEXTURE_2D, sky);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl_load_png_texture(TEXTURE_DIR "sky.png");

  glGenTextures(1, &sign);
  glBindTexture(GL_TEXTURE_2D, sign);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl_load_png_texture(TEXTURE_DIR "sign.png");
}

void gl_setup_render_chunks(
    const float *const matrix,
    const PositionAndOrientation *const positionAndOrientation, float light) {
  glUseProgram(block_attrib.program);
  glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
  glUniform3f(block_attrib.camera, positionAndOrientation->x,
              positionAndOrientation->y, positionAndOrientation->z);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(block_attrib.sampler, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, sky);
  glUniform1i(block_attrib.sky_sampler, 1);
  glUniform1f(block_attrib.daylight, light);
  glUniform1f(block_attrib.fog_distance, (float)g->render_radius * CHUNK_SIZE);
  glUniform1i(block_attrib.ortho, g->ortho);
  glUniform1i(block_attrib.enable_ambient_occlusion, enable_ambient_occlusion);
  glUniform1f(block_attrib.timer, time_of_day());
}

void gl_render_chunk(const Chunk *const chunk) {
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 6

  // draw chunk
  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);
  glBindBuffer(GL_ARRAY_BUFFER, chunk->buffer);
  glEnableVertexAttribArray(block_attrib.position);
  glEnableVertexAttribArray(block_attrib.normal);
  glEnableVertexAttribArray(block_attrib.uv);
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  glDrawArrays(GL_TRIANGLES, 0, chunk->faces * 6);
  glDisableVertexAttribArray(block_attrib.position);
  glDisableVertexAttribArray(block_attrib.normal);
  glDisableVertexAttribArray(block_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_draw_triangles_3d_text(GLuint buffer, int count) {
  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableVertexAttribArray(text_attrib.position);
  glEnableVertexAttribArray(text_attrib.uv);
  glVertexAttribPointer(text_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 5, 0);
  glVertexAttribPointer(text_attrib.uv, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 5, (GLvoid *)(sizeof(float) * 3));
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(text_attrib.position);
  glDisableVertexAttribArray(text_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_setup_render_signs(const float *const matrix) {
  glUseProgram(text_attrib.program);
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sign);
  glUniform1i(text_attrib.sampler, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, font);
  glUniform1i(text_attrib.is_sign, 1);
}

void gl_render_signs(const Chunk *const chunk) {
  // draw signs
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-8, -1024);
  // TODO
  //  figure out why sign_buffer can ever
  //  be 0
  if (chunk->sign_buffer != 0) {
    gl_draw_triangles_3d_text(chunk->sign_buffer, chunk->sign_faces * 6);
  }
  glDisable(GL_POLYGON_OFFSET_FILL);
}

void gl_render_sign(const float *const matrix, int x, int y, int z, int face) {
  glUseProgram(text_attrib.program);
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sign);
  glUniform1i(text_attrib.sampler, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, font);
  glUniform1i(text_attrib.is_sign, 1);
  char text[MAX_SIGN_LENGTH];
  strncpy(text, g->typing_buffer + 1, MAX_SIGN_LENGTH);
  text[MAX_SIGN_LENGTH - 1] = '\0';
  float *data = malloc_faces(5, (int)strlen(text));
  int length = _gen_sign_buffer(data, (float)x, (float)y, (float)z, face, text);
  GLuint buffer = gl_gen_faces(5, length, data);

  // draw sign
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-8, -1024);
  gl_draw_triangles_3d_text(buffer, length * 6);
  glDisable(GL_POLYGON_OFFSET_FILL);

  gl_del_buffer(buffer);
}

void gl_setup_render_players(
    const float *const matrix,
    const PositionAndOrientation *const positionAndOrientation) {
  glUseProgram(block_attrib.program);
  glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
  glUniform3f(block_attrib.camera, positionAndOrientation->x,
              positionAndOrientation->y, positionAndOrientation->z);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(block_attrib.sampler, 0);
  glUniform1i(block_attrib.enable_ambient_occlusion, enable_ambient_occlusion);
  glUniform1f(block_attrib.timer, time_of_day());
}

void gl_render_player(const Player *const other_player) {
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 36

  // draw player
  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);
  glBindBuffer(GL_ARRAY_BUFFER, other_player->buffer);
  glEnableVertexAttribArray(block_attrib.position);
  glEnableVertexAttribArray(block_attrib.normal);
  glEnableVertexAttribArray(block_attrib.uv);
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDisableVertexAttribArray(block_attrib.position);
  glDisableVertexAttribArray(block_attrib.normal);
  glDisableVertexAttribArray(block_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_render_sky(GLuint buffer, const float *const matrix) {
  glUseProgram(sky_attrib.program);
  glUniformMatrix4fv(sky_attrib.matrix, 1, GL_FALSE, matrix);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sky);
  glUniform1i(sky_attrib.sampler, 0);
  glUniform1f(sky_attrib.timer, time_of_day());

  // draw sky
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // TODO - remove magic numbers, like 512 * 3

  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableVertexAttribArray(sky_attrib.position);
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if ((int)sky_attrib.normal >= 0) {
    glEnableVertexAttribArray(sky_attrib.normal);
  }
  glEnableVertexAttribArray(sky_attrib.uv);
  glVertexAttribPointer(sky_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 8, 0);
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if ((int)sky_attrib.normal >= 0) {
    glVertexAttribPointer(sky_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 8, (GLvoid *)(sizeof(float) * 3));
  }
  glVertexAttribPointer(sky_attrib.uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8,
                        (GLvoid *)(sizeof(float) * 6));
  glDrawArrays(GL_TRIANGLES, 0, 512 * 3);
  glDisableVertexAttribArray(sky_attrib.position);
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if ((int)sky_attrib.normal >= 0) {
    glDisableVertexAttribArray(sky_attrib.normal);
  }
  glDisableVertexAttribArray(sky_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_draw_lines(GLuint buffer, int components, int count) {
  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableVertexAttribArray(line_attrib.position);
  glVertexAttribPointer(line_attrib.position, components, GL_FLOAT, GL_FALSE, 0,
                        0);
  glDrawArrays(GL_LINES, 0, count);
  glDisableVertexAttribArray(line_attrib.position);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_render_wireframe(const float *const matrix, int hx, int hy, int hz) {
  glUseProgram(line_attrib.program);
  glLineWidth(1);
  glEnable(GL_COLOR_LOGIC_OP);
  glUniformMatrix4fv(line_attrib.matrix, 1, GL_FALSE, matrix);
  GLuint wireframe_buffer;
  {
    // initilize wireframe_buffer
    float data[72];
    make_cube_wireframe(data, (float)hx, (float)hy, (float)hz, (float)0.53);
    wireframe_buffer = gl_gen_buffer(sizeof(data), data);
  }
  gl_draw_lines(wireframe_buffer, 3, 24);
  gl_del_buffer(wireframe_buffer);
  glDisable(GL_COLOR_LOGIC_OP);
}

void gl_render_text(const float *const matrix, int justify, float x, float y,
                    float n, const char *const text) {
  glUseProgram(text_attrib.program);
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, font);
  glUniform1i(text_attrib.sampler, 0);
  // extra1 = is_sign
  glUniform1i(text_attrib.is_sign, 0);
  int length = (int)strlen(text);
  x -= n * justify * (length - 1) / 2;
  GLuint text_buffer;
  {
    // initialize text_buffer
    const int length = (int)strlen(text);
    float *data = malloc_faces(4, length);
    for (int i = 0; i < length; i++) {
      make_character(data + i * 24, x, y, n / 2, n, text[i]);
      x += n;
    }
    text_buffer = gl_gen_faces(4, length, data);
  }
  // draw text
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  glBindBuffer(GL_ARRAY_BUFFER, text_buffer);
  glEnableVertexAttribArray(text_attrib.position);
  glEnableVertexAttribArray(text_attrib.uv);
  glVertexAttribPointer(text_attrib.position, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, 0);
  glVertexAttribPointer(text_attrib.uv, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, (GLvoid *)(sizeof(float) * 2));
  glDrawArrays(GL_TRIANGLES, 0, length * 6);
  glDisableVertexAttribArray(text_attrib.position);
  glDisableVertexAttribArray(text_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vertexArrayID);

  glDisable(GL_BLEND);

  gl_del_buffer(text_buffer);
}

void gl_render_item(const float *const matrix) {
  glUseProgram(block_attrib.program);
  glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
  glUniform3f(block_attrib.camera, 0, 0, 5);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(block_attrib.sampler, 0);
  glUniform1i(block_attrib.enable_ambient_occlusion, enable_ambient_occlusion);
  glUniform1f(block_attrib.timer, time_of_day());
}

void gl_render_plant(GLuint plant_buffer) {
  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);
  glBindBuffer(GL_ARRAY_BUFFER, plant_buffer);
  glEnableVertexAttribArray(block_attrib.position);
  glEnableVertexAttribArray(block_attrib.normal);
  glEnableVertexAttribArray(block_attrib.uv);
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  glDrawArrays(GL_TRIANGLES, 0, 24);
  glDisableVertexAttribArray(block_attrib.position);
  glDisableVertexAttribArray(block_attrib.normal);
  glDisableVertexAttribArray(block_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_render_cube(GLuint cube_buffer) {
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 36

  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);
  glBindBuffer(GL_ARRAY_BUFFER, cube_buffer);
  glEnableVertexAttribArray(block_attrib.position);
  glEnableVertexAttribArray(block_attrib.normal);
  glEnableVertexAttribArray(block_attrib.uv);
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDisableVertexAttribArray(block_attrib.position);
  glDisableVertexAttribArray(block_attrib.normal);
  glDisableVertexAttribArray(block_attrib.uv);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteVertexArrays(1, &vertexArrayID);
}

void gl_render_crosshairs(GLuint crosshair_buffer, const float *const matrix) {
  glUseProgram(line_attrib.program);
  // TODO -
  //  do something with this, remove it, etc
  //  commented out because in core profile,
  //  a linewidth greater than 1.0 results
  //  in an invalid value error
  // glLineWidth(4.0 * ((float)g->scale));
  glEnable(GL_COLOR_LOGIC_OP);
  glUniformMatrix4fv(line_attrib.matrix, 1, GL_FALSE, matrix);

  gl_draw_lines(crosshair_buffer, 2, 4);
  glDisable(GL_COLOR_LOGIC_OP);
}

#endif
