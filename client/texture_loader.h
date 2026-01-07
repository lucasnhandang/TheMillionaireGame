// Lightweight OpenGL texture loader using stb_image for PNG/JPG.
// Linux/Ubuntu friendly. Header-only dependency lives in third_party/stb_image.h
//
// Usage:
//   #include "texture_loader.h"
//   GLuint tex = 0; int w = 0, h = 0;
//   if (LoadTextureFromFile("client/assets/icon.png", &tex, &w, &h)) { ... }
//
#pragma once

#include <string>

#ifdef _WIN32
    #include <GL/gl.h>
#elif __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

// Returns true on success. On success, out_texture must be deleted with glDeleteTextures(1, &tex).
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

// Helper that tries multiple candidate paths (first that exists wins).
bool LoadTextureFromAny(const std::initializer_list<std::string>& candidates,
                        GLuint* out_texture, int* out_width, int* out_height);


