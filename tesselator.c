#include "tesselator.h"
#include "common.h"

Tesselator* Tesselator_create(void) {
    Tesselator* t = (Tesselator*)malloc(sizeof(Tesselator));
    if (t) {
        t->vertexBuffer = (float*)malloc(300000 * sizeof(float));
        t->texCoordBuffer = (float*)malloc(200000 * sizeof(float));
        t->colorBuffer = (float*)malloc(300000 * sizeof(float));
        t->vertices = 0;
        t->u = 0.0f;
        t->v = 0.0f;
        t->r = 1.0f;
        t->g = 1.0f;
        t->b = 1.0f;
        t->hasColor = false;
        t->hasTexture = false;
    }
    return t;
}

void Tesselator_destroy(Tesselator* self) {
    if (self) {
        free(self->vertexBuffer);
        free(self->texCoordBuffer);
        free(self->colorBuffer);
        free(self);
    }
}

void Tesselator_clear(Tesselator* self) {
    if (self) {
        self->vertices = 0;
    }
}

void Tesselator_flush(Tesselator* self) {
    if (!self || self->vertices == 0) return;

#if defined(__WII__)
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, self->vertexBuffer);

    if (self->hasTexture) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, self->texCoordBuffer);
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (self->hasColor) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0, self->colorBuffer);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    static unsigned short* static_indices = NULL;
    static int static_indices_capacity = 0;

    int indexCount = (self->vertices / 4) * 6;
    if (indexCount > static_indices_capacity) {
        int new_capacity = indexCount + 1024;
        unsigned short* new_indices = (unsigned short*)realloc(static_indices, new_capacity * sizeof(unsigned short));
        if (new_indices) {
            static_indices = new_indices;
            for (int i = static_indices_capacity; i < new_capacity; i += 6) {
                int v = (i / 6) * 4;
                if (i + 0 < new_capacity) static_indices[i + 0] = v + 0;
                if (i + 1 < new_capacity) static_indices[i + 1] = v + 1;
                if (i + 2 < new_capacity) static_indices[i + 2] = v + 2;
                if (i + 3 < new_capacity) static_indices[i + 3] = v + 0;
                if (i + 4 < new_capacity) static_indices[i + 4] = v + 2;
                if (i + 5 < new_capacity) static_indices[i + 5] = v + 3;
            }
            static_indices_capacity = new_capacity;
        }
    }
    if (static_indices && indexCount > 0) {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, static_indices);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
#else
    glBegin(GL_QUADS);
    for (int i = 0; i < self->vertices; i++) {
        if (self->hasColor) {
            glColor3f(self->colorBuffer[i * 3 + 0], self->colorBuffer[i * 3 + 1], self->colorBuffer[i * 3 + 2]);
        }
        if (self->hasTexture) {
            glTexCoord2f(self->texCoordBuffer[i * 2 + 0], self->texCoordBuffer[i * 2 + 1]);
        }
        glVertex3f(self->vertexBuffer[i * 3 + 0], self->vertexBuffer[i * 3 + 1], self->vertexBuffer[i * 3 + 2]);
    }
    glEnd();
#endif

    Tesselator_clear(self);
}

void Tesselator_init(Tesselator* self) {
    if (!self) return;
    Tesselator_clear(self);
    self->hasColor = false;
    self->hasTexture = false;
}

void Tesselator_tex(Tesselator* self, float u, float v) {
    if (!self) return;
    self->hasTexture = true;
    self->u = u;
    self->v = v;
}

void Tesselator_color(Tesselator* self, float r, float g, float b) {
    if (!self) return;
    self->hasColor = true;
    self->r = r;
    self->g = g;
    self->b = b;
}

void Tesselator_vertex(Tesselator* self, float x, float y, float z) {
    if (!self) return;

    self->vertexBuffer[self->vertices * 3 + 0] = x;
    self->vertexBuffer[self->vertices * 3 + 1] = y;
    self->vertexBuffer[self->vertices * 3 + 2] = z;

    if (self->hasTexture) {
        self->texCoordBuffer[self->vertices * 2 + 0] = self->u;
        self->texCoordBuffer[self->vertices * 2 + 1] = self->v;
    }

    if (self->hasColor) {
        self->colorBuffer[self->vertices * 3 + 0] = self->r;
        self->colorBuffer[self->vertices * 3 + 1] = self->g;
        self->colorBuffer[self->vertices * 3 + 2] = self->b;
    }

    self->vertices++;
    if (self->vertices == MAX_VERTICES) {
        Tesselator_flush(self);
    }
}
