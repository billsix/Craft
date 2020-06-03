#include "gl3w.h"
#include "gl3_render.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>
#include <config.h>

// TEXTURE ids
uint32_t texture;
uint32_t font;
uint32_t sky;
uint32_t sign;

Block_Attributes block_attrib;
Line_Attributes line_attrib;
Text_Attributes text_attrib;
Sky_Attributes sky_attrib;

// TODO -- rename gen_buffer, and all other functions
// in this file, to have a gl3_ prefix
uint32_t gen_buffer(size_t size, float *data) {
    uint32_t buffer;
    glGenBuffers(1, &buffer);
    GL_DEBUG_ASSERT();
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GL_DEBUG_ASSERT();
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    GL_DEBUG_ASSERT();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_DEBUG_ASSERT();
    return buffer;
}

void del_buffer(uint32_t buffer) {
    glDeleteBuffers(1, &buffer);
    GL_DEBUG_ASSERT();
}

float *malloc_faces(int components, int faces) {
    return malloc(sizeof(float) * 6 * components * faces);
}

uint32_t gen_faces(int components, int faces, float *data) {
    uint32_t buffer = gen_buffer(
        sizeof(float) * 6 * components * faces, data);
    free(data);
    return buffer;
}

uint32_t make_shader(uint32_t type, const char *source) {
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    GL_DEBUG_ASSERT();
    glCompileShader(shader);
    GL_DEBUG_ASSERT();

    int32_t status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GL_DEBUG_ASSERT();
    if (status == GL_FALSE) {
        int32_t length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GL_DEBUG_ASSERT();
        char *info = calloc(length, sizeof(char));
        glGetShaderInfoLog(shader, length, NULL, info);
        GL_DEBUG_ASSERT();
        fprintf(stderr, "glCompileShader failed:\n%s\n", info);
        free(info);
    }
    return shader;
}

uint32_t load_shader(uint32_t type, const char *path) {
    char *data = load_file(path);
    uint32_t result = make_shader(type, data);
    free(data);
    return result;
}

uint32_t make_program(uint32_t shader1, uint32_t shader2) {
    uint32_t program = glCreateProgram();
    GL_DEBUG_ASSERT();
    glAttachShader(program, shader1);
    GL_DEBUG_ASSERT();
    glAttachShader(program, shader2);
    GL_DEBUG_ASSERT();
    glLinkProgram(program);
    GL_DEBUG_ASSERT();
    int32_t status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    GL_DEBUG_ASSERT();
    if (status == GL_FALSE) {
        int32_t length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GL_DEBUG_ASSERT();
        char *info = calloc(length, sizeof(char));
        glGetProgramInfoLog(program, length, NULL, info);
        GL_DEBUG_ASSERT();
        fprintf(stderr, "glLinkProgram failed: %s\n", info);
        free(info);
    }
    glDetachShader(program, shader1);
    GL_DEBUG_ASSERT();
    glDetachShader(program, shader2);
    GL_DEBUG_ASSERT();
    glDeleteShader(shader1);
    GL_DEBUG_ASSERT();
    glDeleteShader(shader2);
    GL_DEBUG_ASSERT();
    return program;
}

uint32_t load_program(const char *path1, const char *path2) {
    uint32_t shader1 = load_shader(GL_VERTEX_SHADER, path1);
    uint32_t shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
    uint32_t program = make_program(shader1, shader2);
    return program;
}

void load_png_texture(const char *file_name) {
    unsigned int error;
    unsigned char *data;
    unsigned int width, height;
    error = lodepng_decode32_file(&data, &width, &height, file_name);
    if (error) {
        fprintf(stderr, "load_png_texture %s failed, error %u: %s\n", file_name, error, lodepng_error_text(error));
        exit(1);
    }
    flip_image_vertical(data, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data);
    GL_DEBUG_ASSERT();
    free(data);
}

int gl3_graphics_loader_init(){
  if (gl3w_init()) {
    return -1;
  }
  printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION),
         glGetString(GL_SHADING_LANGUAGE_VERSION));
  if (!gl3w_is_supported(3, 3)) {
    fprintf(stderr, "OpenGL 3.3 not supported\n");
    return -1;
  }
}


void gl3_initiliaze_global_state(){
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLogicOp(GL_INVERT);
  glClearColor(0, 0, 0, 1);

#define SHADER_DIR RESOURCE_PATH "/share/craft/shaders/"

  int32_t program = load_program(SHADER_DIR "block_vertex.glsl",
                               SHADER_DIR "block_fragment.glsl");
  // initiliaze shaders
  block_attrib = (Block_Attributes)
    {
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
     .ortho = glGetUniformLocation(program, "ortho")
    };

  program = load_program(SHADER_DIR "line_vertex.glsl", SHADER_DIR "line_fragment.glsl");
  line_attrib = (Line_Attributes)
    {
     .program = program,
     .position = glGetAttribLocation(program, "position"),
     .matrix = glGetUniformLocation(program, "matrix")
    };

  program = load_program(
                         SHADER_DIR "text_vertex.glsl", SHADER_DIR "text_fragment.glsl");
  text_attrib = (Text_Attributes)
    {
     .program = program,
     .position = glGetAttribLocation(program, "position"),
     .uv = glGetAttribLocation(program, "uv"),
     .matrix = glGetUniformLocation(program, "matrix"),
     .sampler = glGetUniformLocation(program, "sampler"),
     .is_sign = glGetUniformLocation(program, "is_sign")
    };

  program = load_program(
                         SHADER_DIR "sky_vertex.glsl", SHADER_DIR "sky_fragment.glsl");
  sky_attrib = (Sky_Attributes)
    {
     .program = program,
     .position = glGetAttribLocation(program, "position"),
     .normal = glGetAttribLocation(program, "normal"),
     .uv = glGetAttribLocation(program, "uv"),
     .matrix = glGetUniformLocation(program, "matrix"),
     .sampler = glGetUniformLocation(program, "sampler"),
     .timer = glGetUniformLocation(program, "timer")
    };
}

void gl3_initiliaze_textures(){
#define TEXTURE_DIR RESOURCE_PATH "/share/craft/textures/"
  glActiveTexture(GL_TEXTURE0);
  // LOAD TEXTURES //
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  load_png_texture(TEXTURE_DIR "texture.png");

  glGenTextures(1, &font);
  glBindTexture(GL_TEXTURE_2D, font);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  load_png_texture(TEXTURE_DIR "font.png");

  glGenTextures(1, &sky);
  glBindTexture(GL_TEXTURE_2D, sky);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  load_png_texture(TEXTURE_DIR "sky.png");

  glGenTextures(1, &sign);
  glBindTexture(GL_TEXTURE_2D, sign);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  load_png_texture(TEXTURE_DIR "sign.png");
}

void gl3_setup_render_chunks(float *matrix, State *s, float light){
    glUseProgram(block_attrib.program);
    GL_DEBUG_ASSERT();
    glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
    GL_DEBUG_ASSERT();
    glUniform3f(block_attrib.camera, s->x, s->y, s->z);
    GL_DEBUG_ASSERT();
    glActiveTexture(GL_TEXTURE0);
    GL_DEBUG_ASSERT();
    glBindTexture(GL_TEXTURE_2D, texture);
    GL_DEBUG_ASSERT();
    glUniform1i(block_attrib.sampler, 0);
    GL_DEBUG_ASSERT();
    glActiveTexture(GL_TEXTURE1);
    GL_DEBUG_ASSERT();
    glBindTexture(GL_TEXTURE_2D, sky);
    glUniform1i(block_attrib.sky_sampler, 1);
    GL_DEBUG_ASSERT();
    glUniform1f(block_attrib.daylight, light);
    GL_DEBUG_ASSERT();
    glUniform1f(block_attrib.fog_distance, g->render_radius * CHUNK_SIZE);
    GL_DEBUG_ASSERT();
    glUniform1i(block_attrib.ortho, g->ortho);
    GL_DEBUG_ASSERT();
    glUniform1f(block_attrib.timer, time_of_day());
    GL_DEBUG_ASSERT();
}

void gl3_render_chunk(Chunk *chunk){
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 6

  // draw chunk
  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, chunk->buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_TRIANGLES, 0, chunk->faces * 6);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();
  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();


}

void gl3_draw_triangles_3d_text(uint32_t buffer, int count){
  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(text_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(text_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(text_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 5, 0);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(text_attrib.uv, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 5, (GLvoid *)(sizeof(float) * 3));
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_TRIANGLES, 0, count);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(text_attrib.position);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(text_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_setup_render_signs(float *matrix){
  glUseProgram(text_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, sign);
  GL_DEBUG_ASSERT();
  glUniform1i(text_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE1);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, font);
  GL_DEBUG_ASSERT();
  glUniform1i(text_attrib.is_sign, 1);
  GL_DEBUG_ASSERT();
}

void gl3_render_signs(Chunk *chunk){
  // draw signs
  glEnable(GL_POLYGON_OFFSET_FILL);
  GL_DEBUG_ASSERT();
  glPolygonOffset(-8, -1024);
  GL_DEBUG_ASSERT();
  // TODO
  //  figure out why sign_buffer can ever
  //  be 0
  if(chunk->sign_buffer != 0){
    gl3_draw_triangles_3d_text(chunk->sign_buffer, chunk->sign_faces * 6);
  }
  glDisable(GL_POLYGON_OFFSET_FILL);
  GL_DEBUG_ASSERT();

}

void gl3_render_sign(float *matrix, int x, int y, int z, int face){
  glUseProgram(text_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, sign);
  GL_DEBUG_ASSERT();
  glUniform1i(text_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE1);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, font);
  GL_DEBUG_ASSERT();
  glUniform1i(text_attrib.is_sign, 1);
  GL_DEBUG_ASSERT();
  char text[MAX_SIGN_LENGTH];
  strncpy(text, g->typing_buffer + 1, MAX_SIGN_LENGTH);
  text[MAX_SIGN_LENGTH - 1] = '\0';
  float *data = malloc_faces(5, strlen(text));
  int length = _gen_sign_buffer(data, x, y, z, face, text);
  uint32_t buffer = gen_faces(5, length, data);

  // draw sign
  glEnable(GL_POLYGON_OFFSET_FILL);
  GL_DEBUG_ASSERT();
  glPolygonOffset(-8, -1024);
  gl3_draw_triangles_3d_text(buffer, length * 6);
  GL_DEBUG_ASSERT();
  glDisable(GL_POLYGON_OFFSET_FILL);
  GL_DEBUG_ASSERT();

  del_buffer(buffer);
}


void gl3_setup_render_players(float *matrix, State *s){
  glUseProgram(block_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glUniform3f(block_attrib.camera, s->x, s->y, s->z);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, texture);
  GL_DEBUG_ASSERT();
  glUniform1i(block_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  glUniform1f(block_attrib.timer, time_of_day());
  GL_DEBUG_ASSERT();
}

void gl3_render_player(Player *other_player){
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 36

  // draw player
  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, other_player->buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_TRIANGLES, 0, 36);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();
  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_render_sky(uint32_t buffer, float *matrix){
  glUseProgram(sky_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(sky_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, sky);
  GL_DEBUG_ASSERT();
  glUniform1i(sky_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  glUniform1f(sky_attrib.timer, time_of_day());
  GL_DEBUG_ASSERT();

  // draw sky
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // TODO - remove magic numbers, like 512 * 3

  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(sky_attrib.position);
  GL_DEBUG_ASSERT();
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if((int) sky_attrib.normal >= 0) {
    glEnableVertexAttribArray(sky_attrib.normal);
    GL_DEBUG_ASSERT();
  }
  glEnableVertexAttribArray(sky_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(sky_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 8, 0);
  GL_DEBUG_ASSERT();
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if((int) sky_attrib.normal >= 0) {
    glVertexAttribPointer(sky_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 8, (GLvoid *)(sizeof(float) * 3));
    GL_DEBUG_ASSERT();
  }
  glVertexAttribPointer(sky_attrib.uv, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 8, (GLvoid *)(sizeof(float) * 6));
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_TRIANGLES, 0, 512 * 3);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(sky_attrib.position);
  GL_DEBUG_ASSERT();
  // TODO
  // Figure out why I have to do this check.
  // If I don't, I will get an OpenGL error
  if((int) sky_attrib.normal >= 0) {
    glDisableVertexAttribArray(sky_attrib.normal);
    GL_DEBUG_ASSERT();
  }
  glDisableVertexAttribArray(sky_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_draw_lines(uint32_t buffer, int components, int count){

  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(line_attrib.position);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(
                        line_attrib.position, components, GL_FLOAT, GL_FALSE, 0, 0);
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_LINES, 0, count);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(line_attrib.position);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_render_wireframe(float *matrix, int hx, int hy, int hz){
  glUseProgram(line_attrib.program);
  GL_DEBUG_ASSERT();
  glLineWidth(1);
  GL_DEBUG_ASSERT();
  glEnable(GL_COLOR_LOGIC_OP);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(line_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  uint32_t wireframe_buffer;
  {
    // initilize wireframe_buffer
    float data[72];
    make_cube_wireframe(data, hx, hy, hz, 0.53);
    wireframe_buffer = gen_buffer(sizeof(data), data);
  }
  gl3_draw_lines(wireframe_buffer, 3, 24);
  del_buffer(wireframe_buffer);
  glDisable(GL_COLOR_LOGIC_OP);
  GL_DEBUG_ASSERT();
}

void gl3_render_text(float *matrix, int justify, float x, float y, float n, char *text){
  glUseProgram(text_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(text_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, font);
  GL_DEBUG_ASSERT();
  glUniform1i(text_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  // extra1 = is_sign
  glUniform1i(text_attrib.is_sign, 0);
  GL_DEBUG_ASSERT();
  int length = strlen(text);
  x -= n * justify * (length - 1) / 2;
  uint32_t text_buffer;
  {
    // initialize text_buffer
    const int length = strlen(text);
    float *data = malloc_faces(4, length);
    for (int i = 0; i < length; i++) {
      make_character(data + i * 24, x, y, n / 2, n, text[i]);
      x += n;
    }
    text_buffer = gen_faces(4, length, data);
  }
  // draw text
  glEnable(GL_BLEND);
  GL_DEBUG_ASSERT();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GL_DEBUG_ASSERT();

  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, text_buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(text_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(text_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(text_attrib.position, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, 0);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(text_attrib.uv, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, (GLvoid *)(sizeof(float) * 2));
  GL_DEBUG_ASSERT();
  glDrawArrays(GL_TRIANGLES, 0, length * 6);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(text_attrib.position);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(text_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();

  GL_DEBUG_ASSERT();
  glDisable(GL_BLEND);
  GL_DEBUG_ASSERT();

  del_buffer(text_buffer);
}


void gl3_render_item(float *matrix){
  glUseProgram(block_attrib.program);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(block_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();
  glUniform3f(block_attrib.camera, 0, 0, 5);
  GL_DEBUG_ASSERT();
  glActiveTexture(GL_TEXTURE0);
  GL_DEBUG_ASSERT();
  glBindTexture(GL_TEXTURE_2D, texture);
  GL_DEBUG_ASSERT();
  glUniform1i(block_attrib.sampler, 0);
  GL_DEBUG_ASSERT();
  glUniform1f(block_attrib.timer, time_of_day());
  GL_DEBUG_ASSERT();
}

void gl3_render_plant(uint32_t plant_buffer){
  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, plant_buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  GL_DEBUG_ASSERT();

  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
  GL_DEBUG_ASSERT();

  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  GL_DEBUG_ASSERT();

  glDrawArrays(GL_TRIANGLES, 0, 24);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();

  glDisableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();

  glDisableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_render_cube(uint32_t cube_buffer){
  // TODO -
  // make and initilize the VAO once at initilization time.
  // only thing that should happen here
  // is 1) binding the vao, 2) binding the vbo,
  // 3) setting the vertexattribpointers
  // 4) draw arrays 5) cleanup
  // also, remove magic numbers, like 36


  uint32_t vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
  glBindVertexArray(vertexArrayID );
  GL_DEBUG_ASSERT();

  glBindBuffer(GL_ARRAY_BUFFER, cube_buffer);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();
  glEnableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glVertexAttribPointer(block_attrib.position, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, 0);
  GL_DEBUG_ASSERT();

  glVertexAttribPointer(block_attrib.normal, 3, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
  GL_DEBUG_ASSERT();

  glVertexAttribPointer(block_attrib.uv, 4, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
  GL_DEBUG_ASSERT();

  glDrawArrays(GL_TRIANGLES, 0, 36);
  GL_DEBUG_ASSERT();
  glDisableVertexAttribArray(block_attrib.position);
  GL_DEBUG_ASSERT();

  glDisableVertexAttribArray(block_attrib.normal);
  GL_DEBUG_ASSERT();

  glDisableVertexAttribArray(block_attrib.uv);
  GL_DEBUG_ASSERT();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_DEBUG_ASSERT();

  glDeleteVertexArrays(1, &vertexArrayID);
  GL_DEBUG_ASSERT();
}

void gl3_render_crosshairs(uint32_t crosshair_buffer, float *matrix){
  glUseProgram(line_attrib.program);
  GL_DEBUG_ASSERT();
  // TODO -
  //  do something with this, remove it, etc
  //  commented out because in core profile,
  //  a linewidth greater than 1.0 results
  //  in an invalid value error
  //glLineWidth(4.0 * ((float)g->scale));
  //GL_DEBUG_ASSERT();
  glEnable(GL_COLOR_LOGIC_OP);
  GL_DEBUG_ASSERT();
  glUniformMatrix4fv(line_attrib.matrix, 1, GL_FALSE, matrix);
  GL_DEBUG_ASSERT();

  gl3_draw_lines(crosshair_buffer, 2, 4);
}
