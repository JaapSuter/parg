#pragma once

#if defined(__APPLE_CC__)
#if defined(GLFW_INCLUDE_GLCOREARB)
#include <OpenGL/gl3.h>
#elif !defined(GLFW_INCLUDE_NONE)
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#endif
#else
#if defined(GLFW_INCLUDE_GLCOREARB)
#include <GL/glcorearb.h>
#elif defined(GLFW_INCLUDE_ES1)
#include <GLES/gl.h>
#elif defined(GLFW_INCLUDE_ES2)
#include <GLES2/gl2.h>
#elif defined(GLFW_INCLUDE_ES3)
#include <GLES3/gl3.h>
#elif !defined(GLFW_INCLUDE_NONE)
#include <GL/gl.h>
#endif
#endif

void glGenerateMipmap(GLenum target);

GLuint par_buffer_gpu_handle(par_buffer*);
GLuint par_shader_attrib_get(par_token);
GLint par_shader_uniform_get(par_token);

extern int _par_depthtest;