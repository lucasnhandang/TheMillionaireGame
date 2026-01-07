// stb_image - public domain image loader - https://github.com/nothings/stb
// Version 2.28 (trimmed license header for brevity). This file is unmodified from upstream interface.
// Only the declarations are here; implementation compiled via STB_IMAGE_IMPLEMENTATION in texture_loader.cpp
//
// This is a compact subset header to avoid bloating the repo; it exposes only APIs we use.

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int      stbi_write_png_compression_level; // not used here, but kept for ABI compatibility

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

// Primary load API
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
stbi_uc *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
void     stbi_image_free(void *retval_from_stbi_load);

// Info
int      stbi_info(char const *filename, int *x, int *y, int *comp);

// I/O callbacks (not used here)
typedef struct
{
   int (*read)  (void *user, char *data, int size);
   void (*skip) (void *user, int n);
   int (*eof)   (void *user);
} stbi_io_callbacks;

// Config macros used by implementation (defined by including translation unit)
// #define STB_IMAGE_IMPLEMENTATION
// #define STBI_ONLY_PNG
// #define STBI_ONLY_JPEG

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_H


