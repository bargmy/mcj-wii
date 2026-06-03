#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "textures.h"
#include "common.h"

#define MAX_CACHED_TEXTURES 32

typedef struct {
    char name[256];
    int id;
} TextureCacheEntry;

static TextureCacheEntry textureCache[MAX_CACHED_TEXTURES];
static int textureCacheCount = 0;

int Textures_loadTexture(const char* resourceName, int mode) {
    // Check if texture is already loaded
    for (int i = 0; i < textureCacheCount; i++) {
        if (strcmp(textureCache[i].name, resourceName) == 0) {
            return textureCache[i].id;
        }
    }

    // Prepare filename by stripping leading slash
    const char* filename = resourceName;
    if (filename[0] == '/') {
        filename++;
    }

    int width, height, channels;
    unsigned char* pixels = NULL;

#ifdef __WII__
    stbi_set_flip_vertically_on_load(1);
#else
    stbi_set_flip_vertically_on_load(0);
#endif

    // Load from embedded memory arrays to prevent disk overhead and ensure successful loads
    if (strstr(filename, "char.png") != NULL) {
        #include "char_png.h"
        pixels = stbi_load_from_memory(char_png, char_png_len, &width, &height, &channels, 4);
    } else if (strstr(filename, "terrain.png") != NULL) {
        #include "terrain_png.h"
        pixels = stbi_load_from_memory(terrain_png, terrain_png_len, &width, &height, &channels, 4);
    } else {
        pixels = stbi_load(filename, &width, &height, &channels, 4);
    }

    if (!pixels) {
        fprintf(stderr, "Failed to load texture: %s\n", filename);
        return 0;
    }

#ifdef __WII__
    // Manually flip decoded RGBA8 pixels vertically to guarantee correct orientation on Wii/Dolphin
    {
        int row_size = width * 4;
        unsigned char* temp_row = (unsigned char*)malloc(row_size);
        if (temp_row) {
            for (int y = 0; y < height / 2; y++) {
                unsigned char* row1 = pixels + y * row_size;
                unsigned char* row2 = pixels + (height - 1 - y) * row_size;
                memcpy(temp_row, row1, row_size);
                memcpy(row1, row2, row_size);
                memcpy(row2, temp_row, row_size);
            }
            free(temp_row);
        }
    }
#endif

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);

#ifdef __WII__
    // OpenGX / Wii texture load
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
    // Build mipmaps for desktop (equivalent to GLU.gluBuild2DMipmaps)
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif

    stbi_image_free(pixels);

    // Store in cache
    if (textureCacheCount < MAX_CACHED_TEXTURES) {
        strncpy(textureCache[textureCacheCount].name, resourceName, sizeof(textureCache[textureCacheCount].name) - 1);
        textureCache[textureCacheCount].id = (int)textureId;
        textureCacheCount++;
    }

    printf("%s -> %d\n", resourceName, textureId);
    return (int)textureId;
}

void Textures_cleanup(void) {
    for (int i = 0; i < textureCacheCount; i++) {
        GLuint id = textureCache[i].id;
        glDeleteTextures(1, &id);
    }
    textureCacheCount = 0;
}
