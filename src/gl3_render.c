#include "gl3_render.h"
#include "gl3w.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

GLuint gen_buffer(GLsizei size, GLfloat *data) {
    GLuint buffer;
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

void del_buffer(GLuint buffer) {
    glDeleteBuffers(1, &buffer);
    GL_DEBUG_ASSERT();
}

GLfloat *malloc_faces(int components, int faces) {
    return malloc(sizeof(GLfloat) * 6 * components * faces);
}

GLuint gen_faces(int components, int faces, GLfloat *data) {
    GLuint buffer = gen_buffer(
        sizeof(GLfloat) * 6 * components * faces, data);
    free(data);
    return buffer;
}

GLuint make_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    GL_DEBUG_ASSERT();
    glCompileShader(shader);
    GL_DEBUG_ASSERT();

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GL_DEBUG_ASSERT();
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GL_DEBUG_ASSERT();
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetShaderInfoLog(shader, length, NULL, info);
        GL_DEBUG_ASSERT();
        fprintf(stderr, "glCompileShader failed:\n%s\n", info);
        free(info);
    }
    return shader;
}

GLuint load_shader(GLenum type, const char *path) {
    char *data = load_file(path);
    GLuint result = make_shader(type, data);
    free(data);
    return result;
}

GLuint make_program(GLuint shader1, GLuint shader2) {
    GLuint program = glCreateProgram();
    GL_DEBUG_ASSERT();
    glAttachShader(program, shader1);
    GL_DEBUG_ASSERT();
    glAttachShader(program, shader2);
    GL_DEBUG_ASSERT();
    glLinkProgram(program);
    GL_DEBUG_ASSERT();
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    GL_DEBUG_ASSERT();
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GL_DEBUG_ASSERT();
        GLchar *info = calloc(length, sizeof(GLchar));
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

GLuint load_program(const char *path1, const char *path2) {
    GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1);
    GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2);
    GLuint program = make_program(shader1, shader2);
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
