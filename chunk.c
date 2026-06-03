#include "chunk.h"
#include "textures.h"
#include "tile.h"
#include "common.h"

int Chunk_rebuiltThisFrame = 0;
int Chunk_updates = 0;

static Tesselator* sharedTesselator = NULL;

Chunk* Chunk_create(Level* level, int x0, int y0, int z0, int x1, int y1, int z1) {
    if (!sharedTesselator) {
        sharedTesselator = Tesselator_create();
    }

    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    if (chunk) {
        chunk->level = level;
        chunk->x0 = x0;
        chunk->y0 = y0;
        chunk->z0 = z0;
        chunk->x1 = x1;
        chunk->y1 = y1;
        chunk->z1 = z1;
        chunk->aabb = AABB_create((float)x0, (float)y0, (float)z0, (float)x1, (float)y1, (float)z1);
        chunk->dirty = true;
        
        chunk->vertexBuffers[0] = NULL;
        chunk->vertexBuffers[1] = NULL;
        chunk->texCoordBuffers[0] = NULL;
        chunk->texCoordBuffers[1] = NULL;
        chunk->vertexCounts[0] = 0;
        chunk->vertexCounts[1] = 0;
    }
    return chunk;
}

void Chunk_destroy(Chunk* self) {
    if (self) {
        for (int i = 0; i < 2; i++) {
            if (self->vertexBuffers[i]) free(self->vertexBuffers[i]);
            if (self->texCoordBuffers[i]) free(self->texCoordBuffers[i]);
        }
        free(self);
    }
}

static void Chunk_rebuild(Chunk* self, int layer) {
    if (Chunk_rebuiltThisFrame != 2) {
        self->dirty = false;
        Chunk_updates++;
        Chunk_rebuiltThisFrame++;

        Tesselator_init(sharedTesselator);

        for (int x = self->x0; x < self->x1; x++) {
            for (int y = self->y0; y < self->y1; y++) {
                for (int z = self->z0; z < self->z1; z++) {
                    if (Level_isTile(self->level, x, y, z)) {
                        int tex = (y == self->level->depth * 2 / 3) ? 0 : 1;
                        if (tex == 0) {
                            Tile_render(&Tile_rock, sharedTesselator, self->level, layer, x, y, z);
                        } else {
                            Tile_render(&Tile_grass, sharedTesselator, self->level, layer, x, y, z);
                        }
                    }
                }
            }
        }

        if (self->vertexBuffers[layer]) {
            free(self->vertexBuffers[layer]);
            self->vertexBuffers[layer] = NULL;
        }
        if (self->texCoordBuffers[layer]) {
            free(self->texCoordBuffers[layer]);
            self->texCoordBuffers[layer] = NULL;
        }

        self->vertexCounts[layer] = sharedTesselator->vertices;
        if (sharedTesselator->vertices > 0) {
            self->vertexBuffers[layer] = (float*)malloc(sharedTesselator->vertices * 3 * sizeof(float));
            memcpy(self->vertexBuffers[layer], sharedTesselator->vertexBuffer, sharedTesselator->vertices * 3 * sizeof(float));
            
            self->texCoordBuffers[layer] = (float*)malloc(sharedTesselator->vertices * 2 * sizeof(float));
            memcpy(self->texCoordBuffers[layer], sharedTesselator->texCoordBuffer, sharedTesselator->vertices * 2 * sizeof(float));
        }
    }
}

void Chunk_render(Chunk* self, int layer) {
    if (!self) return;
    if (self->dirty) {
        Chunk_rebuild(self, 0);
        Chunk_rebuild(self, 1);
    }
    
    if (self->vertexCounts[layer] > 0) {
        int id = Textures_loadTexture("terrain.png", GL_NEAREST);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, id);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        
        glVertexPointer(3, GL_FLOAT, 0, self->vertexBuffers[layer]);
        glTexCoordPointer(2, GL_FLOAT, 0, self->texCoordBuffers[layer]);
        
        int indexCount = (self->vertexCounts[layer] / 4) * 6;
        static unsigned short* indices = NULL;
        static int currentMaxIndices = 0;
        if (indexCount > currentMaxIndices) {
            indices = (unsigned short*)realloc(indices, indexCount * sizeof(unsigned short));
            for (int i = (currentMaxIndices/6)*6, v = (currentMaxIndices/6)*4; i < indexCount; i += 6, v += 4) {
                indices[i + 0] = v + 0;
                indices[i + 1] = v + 1;
                indices[i + 2] = v + 2;
                indices[i + 3] = v + 0;
                indices[i + 4] = v + 2;
                indices[i + 5] = v + 3;
            }
            currentMaxIndices = indexCount;
        }
        
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, indices);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
    }
}

void Chunk_setDirty(Chunk* self) {
    if (self) {
        self->dirty = true;
    }
}
