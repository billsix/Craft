#include "gl3_render.h"
#include "gl3w.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>

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
