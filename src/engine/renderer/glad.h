#ifndef GLAD_H
#define GLAD_H

#include <GLFW/glfw3.h>

// OpenGL function pointer types
typedef void (*GLADloadproc)(const char* name);

// GLAD function
int gladLoadGLLoader(GLADloadproc load);

// OpenGL constants
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_INT 0x1405
#define GL_DEPTH_TEST 0x0B71
#define GL_LESS 0x0201
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_FLOAT 0x1406
#define GL_FALSE 0

// OpenGL function declarations
extern "C" {
    unsigned int glCreateShader(unsigned int type);
    void glShaderSource(unsigned int shader, int count, const char* const* string, const int* length);
    void glCompileShader(unsigned int shader);
    void glGetShaderiv(unsigned int shader, unsigned int pname, int* params);
    void glGetShaderInfoLog(unsigned int shader, int bufSize, int* length, char* infoLog);
    unsigned int glCreateProgram(void);
    void glAttachShader(unsigned int program, unsigned int shader);
    void glLinkProgram(unsigned int program);
    void glGetProgramiv(unsigned int program, unsigned int pname, int* params);
    void glGetProgramInfoLog(unsigned int program, int bufSize, int* length, char* infoLog);
    void glDeleteShader(unsigned int shader);
    void glDeleteProgram(unsigned int program);
    void glGenVertexArrays(int n, unsigned int* arrays);
    void glBindVertexArray(unsigned int array);
    void glGenBuffers(int n, unsigned int* buffers);
    void glBindBuffer(unsigned int target, unsigned int buffer);
    void glBufferData(unsigned int target, long long size, const void* data, unsigned int usage);
    void glVertexAttribPointer(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void* pointer);
    void glEnableVertexAttribArray(unsigned int index);
    void glUseProgram(unsigned int program);
    void glUniformMatrix4fv(int location, int count, unsigned char transpose, const float* value);
    void glUniform3fv(int location, int count, const float* value);
    void glUniform1f(int location, float v0);
    int glGetUniformLocation(unsigned int program, const char* name);
    void glDrawElements(unsigned int mode, int count, unsigned int type, const void* indices);
    void glClearColor(float red, float green, float blue, float alpha);
    void glClear(unsigned int mask);
    void glEnable(unsigned int cap);
    void glDepthFunc(unsigned int func);
    void glDeleteBuffers(int n, const unsigned int* buffers);
    void glDeleteVertexArrays(int n, const unsigned int* arrays);
}

#endif // GLAD_H