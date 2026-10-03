#pragma once

// Web and Android both render with OpenGL ES 3 and have no GLEW.
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#define CANIS_GLES 1
#include <GLES3/gl3.h>
#include <GLES3/gl3platform.h>
const static char* OPENGLVERSION = "#version 300 es";
#else
#define CANIS_GLES 0
#include <GL/glew.h>
//#include <SDL3/SDL_opengl.h>
//#include <SDL3/SDL_opengl_glext.h>
const static char* OPENGLVERSION = "#version 330 core";
#endif
