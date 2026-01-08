#include "texture_loader.h"
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#ifdef TEXTURE_LOADER_ENABLE_STB
#  define TEXTURE_LOADER_HAVE_STB 1
#else
#  define TEXTURE_LOADER_HAVE_STB 0
#endif

#if TEXTURE_LOADER_HAVE_STB
    #define STB_IMAGE_IMPLEMENTATION
    #define STBI_ONLY_PNG
    #define STBI_ONLY_JPEG
    #define STBI_NO_HDR
    #define STBI_NO_LINEAR
    #include "stb_image.h"
#endif

static bool file_exists(const char* path) {
	struct stat st;
	return ::stat(path, &st) == 0 && (st.st_mode & S_IFREG) != 0;
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	if (!filename || !out_texture || !out_width || !out_height) return false;
	if (!file_exists(filename)) {
		fprintf(stderr, "[TEXTURE] File not found: %s\n", filename);
		return false;
	}
	fprintf(stderr, "[TEXTURE] Loading texture from: %s\n", filename);

#if !TEXTURE_LOADER_HAVE_STB
    // stb_image not available: gracefully fail so caller can fallback to text buttons.
    return false;
#else
	int image_width = 0;
	int image_height = 0;
	int channels = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
	if (image_data == nullptr) {
		return false;
	}

	GLuint image_texture = 0;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;
	return true;
#endif
}

bool LoadTextureFromAny(const std::initializer_list<std::string>& candidates,
                        GLuint* out_texture, int* out_width, int* out_height)
{
	for (const auto& p : candidates) {
		fprintf(stderr, "[TEXTURE] Trying path: %s\n", p.c_str());
		if (LoadTextureFromFile(p.c_str(), out_texture, out_width, out_height)) {
			fprintf(stderr, "[TEXTURE] Successfully loaded from: %s\n", p.c_str());
			return true;
		}
	}
	fprintf(stderr, "[TEXTURE] Failed to load texture from all candidate paths\n");
	return false;
}


