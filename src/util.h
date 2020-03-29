#ifndef _util_h_
#define _util_h_

#include "gl3w.h"
#include <GLFW/glfw3.h>
#include "config.h"
#include <assert.h>

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
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

GLuint gen_buffer(GLsizei size, GLfloat *data);
void del_buffer(GLuint buffer);
GLfloat *malloc_faces(int components, int faces);
GLuint gen_faces(int components, int faces, GLfloat *data);
GLuint make_shader(GLenum type, const unsigned char *source);
GLuint load_shader(GLenum type, const unsigned char *png);
GLuint make_program(GLuint shader1, GLuint shader2);
GLuint load_program(const unsigned char *vertex_shader,
                    const unsigned char *fragment_shader);
void load_png_texture(const unsigned char *png, unsigned size);
char *tokenize(char *str, const char *delim, char **key);
int char_width(char input);
int string_width(const char *input);
int wrap(const char *input, int max_width, char *output, int max_length);

// Uncomment the following flag to debug OpenGL calls
// #define GLDEBUG 1

#define NOOP() (void)0

#ifdef GLDEBUG
#define GL_DEBUG_ASSERT() { \
  GLenum error = glGetError(); \
  if (error != GL_NO_ERROR) \
    {                                                                   \
      switch(error){                                                    \
      case GL_INVALID_ENUM:                                             \
        printf("An unacceptable value is specified for an enumerated "  \
               "argument. The offending command is ignored and has no " \
               "other side effect than to set the error flag.");        \
        break;                                                          \
      case GL_INVALID_VALUE:                                            \
        printf("A numeric argument is out of range. The offending "     \
               "command is ignored and has no other side effect than "  \
               "to set the error flag.");                               \
        break;                                                          \
      case GL_INVALID_OPERATION:                                        \
        printf("The specified operation is not allowed in the current " \
               "state. The offending command is ignored and has no "    \
               "other side effect than to set the error flag.");        \
        break;                                                          \
      case GL_INVALID_FRAMEBUFFER_OPERATION:                            \
        printf("The framebuffer object is not complete. The offending " \
               "command is ignored and has no other side effect than "  \
               "to set the error flag.");                               \
        break;                                                          \
      case GL_OUT_OF_MEMORY:                                            \
        printf("There is not enough memory left to execute the "        \
               "command. The state of the GL is undefined, except for " \
               "the state of the error flags, after this error is "     \
               "recorded."); \
        break;                                                          \
      case GL_STACK_UNDERFLOW:                                          \
        printf("An attempt has been made to perform an operation that " \
               "would cause an internal stack to underflow.");          \
        break;                                                          \
      case GL_STACK_OVERFLOW:                                           \
        printf("An attempt has been made to perform an operation that " \
               "would cause an internal stack to overflow. ");          \
        break;                                                          \
      }                                                                 \
      printf ("%d \n", error);                                                \
      assert (0);                                                       \
    }                                                                   \
  }
#else
#define GL_DEBUG_ASSERT() NOOP()
#endif

#endif
