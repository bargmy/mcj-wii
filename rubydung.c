#include "common.h"
#include <time.h>
#include <math.h>
#include "timer.h"
#include "textures.h"
#include "tile.h"
#include "level.h"
#include "level_renderer.h"
#include "player.h"
#include "character.h"
#include "hit_result.h"
#include "input.h"
#include "font.h"

// Force high-performance GPU driver selection on AMD and NVIDIA laptops
#ifdef _WIN32
#include <windows.h>
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

// Global/static variables
static int width = 1024;
static int height = 768;
static float fogColor[4];
static Timer gameTimer;
static int currentFPS = 0;
static Level* level = NULL;
static LevelRenderer* levelRenderer = NULL;
static Player* player = NULL;

#define MAX_ZOMBIES 100
static Zombie* zombies[MAX_ZOMBIES];
static int zombieCount = 0;

static unsigned int selectBuffer[2000];
static HitResult* hitResult = NULL;
static bool hasHitResult = false;
static HitResult activeHitResult;

// Mouse callback variables
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;
static bool firstMouse = true;

static void moveCameraToPlayer(float a) {
    glTranslatef(0.0f, 0.0f, -0.3f);
    glRotatef(player->base.xRot, 1.0f, 0.0f, 0.0f);
    glRotatef(player->base.yRot, 0.0f, 1.0f, 0.0f);
    float x = player->base.xo + (player->base.x - player->base.xo) * a;
    float y = player->base.yo + (player->base.y - player->base.yo) * a;
    float z = player->base.zo + (player->base.z - player->base.zo) * a;
    glTranslatef(-x, -y, -z);
}

static void setupCamera(float a) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0f, (float)width / (float)height, 0.05f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    moveCameraToPlayer(a);
}

static void setupPickCamera(float a, int x, int y) {
#ifdef ANDROID_PORT
    (void)a; (void)x; (void)y;
#else
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluPickMatrix((float)x, (float)y, 5.0f, 5.0f, viewport);
    gluPerspective(70.0f, (float)width / (float)height, 0.05f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    moveCameraToPlayer(a);
#endif
}

static void pick(float a) {
    float x = player->base.xo + (player->base.x - player->base.xo) * a;
    float y = player->base.yo + (player->base.y - player->base.yo) * a;
    float z = player->base.zo + (player->base.z - player->base.zo) * a;

    float xRotRad = player->base.xRot * (float)M_PI / 180.0f;
    float yRotRad = player->base.yRot * (float)M_PI / 180.0f;

    float dx = sinf(yRotRad) * cosf(xRotRad);
    float dy = -sinf(xRotRad);
    float dz = -cosf(yRotRad) * cosf(xRotRad);

    float maxDist = 5.0f;
    float step = 0.05f;
    float cx = x;
    float cy = y;
    float cz = z;

    bool hit = false;
    for (float d = 0.0f; d < maxDist; d += step) {
        cx = x + dx * d;
        cy = y + dy * d;
        cz = z + dz * d;

        int ix = (int)floorf(cx);
        int iy = (int)floorf(cy);
        int iz = (int)floorf(cz);

        if (Level_isSolidTile(level, ix, iy, iz)) {
            int f = -1;
            float px = cx - dx * step;
            float py = cy - dy * step;
            float pz = cz - dz * step;
            int pix = (int)floorf(px);
            int piy = (int)floorf(py);
            int piz = (int)floorf(pz);

            if (pix < ix) f = 4;
            else if (pix > ix) f = 5;
            else if (piy < iy) f = 0;
            else if (piy > iy) f = 1;
            else if (piz < iz) f = 2;
            else if (piz > iz) f = 3;
            else f = 1;

            activeHitResult = HitResult_create(ix, iy, iz, 0, f);
            hitResult = &activeHitResult;
            hasHitResult = true;
            hit = true;
            break;
        }
    }

    if (!hit) {
        hitResult = NULL;
        hasHitResult = false;
    }
}
#if !defined(ANDROID_PORT) && !defined(__WII__)
// Mouse button handler
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        MobileUI_handleTouch(button, (float)x, (float)y, true, width, height);
    } else if (action == GLFW_RELEASE) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        MobileUI_handleTouch(button, (float)x, (float)y, false, width, height);
    }

    // Logic for breaking/placing blocks
    if (action == GLFW_PRESS && hitResult != NULL) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT || GlobalInput.attack) {
            Level_setTile(level, hitResult->x, hitResult->y, hitResult->z, 0);
            GlobalInput.attack = false;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT || GlobalInput.place) {
            int x = hitResult->x;
            int y = hitResult->y;
            int z = hitResult->z;
            if (hitResult->f == 0) y--;
            if (hitResult->f == 1) y++;
            if (hitResult->f == 2) z--;
            if (hitResult->f == 3) z++;
            if (hitResult->f == 4) x--;
            if (hitResult->f == 5) x++;
            Level_setTile(level, x, y, z, 1);
            GlobalInput.place = false;
        }
    }
}

// Keyboard key handler
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
    
    if (key == GLFW_KEY_W) GlobalInput.forward = pressed;
    if (key == GLFW_KEY_S) GlobalInput.back = pressed;
    if (key == GLFW_KEY_A) GlobalInput.left = pressed;
    if (key == GLFW_KEY_D) GlobalInput.right = pressed;
    if (key == GLFW_KEY_SPACE) GlobalInput.jump = pressed;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ENTER) {
            Level_save(level);
            printf("Level saved successfully.\n");
        }
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
}

// Window size changed handler
static void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}
#endif

void initGame(void* window) {
    (void)window;
    int col = 920330;
    float fr = 0.5f;
    float fg = 0.8f;
    float fb = 1.0f;

    fogColor[0] = (float)(col >> 16 & 0xFF) / 255.0f;
    fogColor[1] = (float)(col >> 8 & 0xFF) / 255.0f;
    fogColor[2] = (float)(col & 0xFF) / 255.0f;
    fogColor[3] = 1.0f;

#if !defined(ANDROID_PORT) && !defined(__WII__)
    glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
#endif
    glViewport(0, 0, width, height);

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(fr, fg, fb, 0.0f);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    Tile_init();
    Input_init();

    level = Level_create(256, 256, 64);
    levelRenderer = LevelRenderer_create(level);
    player = Player_create(level);

    for (int i = 0; i < 100; i++) {
        zombies[zombieCount++] = Zombie_create(level, 128.0f, 0.0f, 128.0f);
    }

    printf("Pre-building chunks... ");
    fflush(stdout);
    for (int i = 0; i < levelRenderer->chunkCount; i++) {
        Chunk_render(levelRenderer->chunks[i], 0);
    }
    printf("Done!\n");
}

static u32 wii_buttons_down = 0;

static void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (float)width, (float)height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);

    glColor3f(1.0f, 1.0f, 0.0f); // Yellow for high contrast
    glLineWidth(2.0f);

    char hudText[256];
    float px = player->base.x;
    float py = player->base.y;
    float pz = player->base.z;
    
    // Scale 3.0f is highly readable on 640x480 resolution
    sprintf(hudText, "POS: %.1f %.1f %.1f", px, py, pz);
    Font_drawString(hudText, 15.0f, 15.0f, 3.0f);

    sprintf(hudText, "ROT: %.1f %.1f", player->base.xRot, player->base.yRot);
    Font_drawString(hudText, 15.0f, 45.0f, 3.0f);

    if (hasHitResult) {
        sprintf(hudText, "HIT: %d %d %d Face: %d", hitResult->x, hitResult->y, hitResult->z, hitResult->f);
        Font_drawString(hudText, 15.0f, 75.0f, 3.0f);
    } else {
        Font_drawString("NO HIT TARGET", 15.0f, 75.0f, 3.0f);
    }

    sprintf(hudText, "BTNS DOWN: %08X", (unsigned int)wii_buttons_down);
    Font_drawString(hudText, 15.0f, 115.0f, 3.0f);

    sprintf(hudText, "ATTACK: %d  PLACE: %d", GlobalInput.attack, GlobalInput.place);
    Font_drawString(hudText, 15.0f, 155.0f, 3.0f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void tickGame() {
    for (int i = 0; i < zombieCount; i++) {
        Zombie_tick(zombies[i]);
    }
#ifdef __WII__
    Player_tick(player, NULL);
#else
    Player_tick(player, glfwGetCurrentContext());
#endif
    
    // Process actions
    if (GlobalInput.attack || GlobalInput.place) {
        // Trigger pick if we haven't already
        pick(1.0f);
        if (hasHitResult) {
            if (GlobalInput.attack) Level_setTile(level, hitResult->x, hitResult->y, hitResult->z, 0);
            if (GlobalInput.place) {
                int x = hitResult->x, y = hitResult->y, z = hitResult->z;
                if (hitResult->f == 0) y--; if (hitResult->f == 1) y++;
                if (hitResult->f == 2) z--; if (hitResult->f == 3) z++;
                if (hitResult->f == 4) x--; if (hitResult->f == 5) x++;
                Level_setTile(level, x, y, z, 1);
            }
        }
        GlobalInput.attack = false;
        GlobalInput.place = false;
    }
}

void renderGame(void* window, float a) {
#ifndef __WII__
    GLFWwindow* win = (GLFWwindow*)window;
    if (glfwGetInputMode(win, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        double mouseX, mouseY;
        glfwGetCursorPos(win, &mouseX, &mouseY);
        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }
        float dx = (float)(mouseX - lastMouseX);
        float dy = (float)(lastMouseY - mouseY);
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        Entity_turn(&player->base, dx, dy);
    }
#else
    (void)window;
#endif

    pick(a);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera(a);

#ifndef __WII__
    glEnable(GL_FOG);
    glFogf(GL_FOG_MODE, (float)GL_EXP);
    glFogf(GL_FOG_DENSITY, 0.2f);
    glFogfv(GL_FOG_COLOR, fogColor);
    glDisable(GL_FOG);
#endif

    LevelRenderer_render(levelRenderer, player, 0);
    for (int i = 0; i < zombieCount; i++) {
        Zombie_render(zombies[i], a);
    }

#ifndef __WII__
    glEnable(GL_FOG);
#endif
    LevelRenderer_render(levelRenderer, player, 1);
    glDisable(GL_TEXTURE_2D);

    if (hitResult != NULL) {
        LevelRenderer_renderHit(levelRenderer, *hitResult);
    }

#ifndef __WII__
    glDisable(GL_FOG);
#endif

}

void destroyGame() {
    Level_save(level);
    Player_destroy(player);
    LevelRenderer_destroy(levelRenderer);
    Level_destroy(level);

    for (int i = 0; i < zombieCount; i++) {
        Zombie_destroy(zombies[i]);
    }
    Textures_cleanup();
}

#ifdef __WII__
#include <fat.h>

// Compatibility wrappers for FreeGLUT link dependencies
u64 gettime(void) {
    u64 time;
    asm volatile("mftb %0" : "=r"(time));
    return time;
}

void DCStoreRange(void *startaddress, u32 len) {
    PPCDCacheStoreAsync(startaddress, len);
}

static long wiiLastTime = 0;
static int wiiFrames = 0;

static void wii_display_func(void) {
    // Scan and read inputs
    WPAD_ScanPads();
    u32 buttons_held = WPAD_ButtonsHeld(0);
    static u32 last_buttons_held = 0;
    u32 buttons_down = buttons_held & ~last_buttons_held;
    last_buttons_held = buttons_held;
    wii_buttons_down = buttons_down;

    if (buttons_down & WPAD_BUTTON_HOME) {
        destroyGame();
        exit(0);
    }

    expansion_t exp;
    WPAD_Expansion(0, &exp);

    float stickX = 0.0f;
    float stickY = 0.0f;
    if (exp.type == WPAD_EXP_NUNCHUK) {
        // Fallback to standard hardware neutral (128) if emulation calibration center is 0
        float cx = (exp.nunchuk.js.center.x == 0) ? 128.0f : (float)exp.nunchuk.js.center.x;
        float cy = (exp.nunchuk.js.center.y == 0) ? 128.0f : (float)exp.nunchuk.js.center.y;
        stickX = (float)exp.nunchuk.js.pos.x - cx;
        stickY = (float)exp.nunchuk.js.pos.y - cy;
        
        float maxVal = 100.0f;
        float deadZone = 15.0f;
        if (fabs(stickX) < deadZone) stickX = 0.0f;
        if (fabs(stickY) < deadZone) stickY = 0.0f;
        stickX = stickX / maxVal;
        stickY = stickY / maxVal;

        // Nunchuk analog stick controls camera looking/turning
        float dx = stickX * 30.0f;
        float dy = stickY * 30.0f;
        Entity_turn(&player->base, dx, dy);
    }

    // DPAD buttons control player movement
    GlobalInput.moveX = 0.0f;
    GlobalInput.moveY = 0.0f;
    if (buttons_held & WPAD_BUTTON_UP) GlobalInput.moveY = -1.0f;
    else if (buttons_held & WPAD_BUTTON_DOWN) GlobalInput.moveY = 1.0f;

    if (buttons_held & WPAD_BUTTON_LEFT) GlobalInput.moveX = -1.0f;
    else if (buttons_held & WPAD_BUTTON_RIGHT) GlobalInput.moveX = 1.0f;

    GlobalInput.jump = (exp.type == WPAD_EXP_NUNCHUK && (buttons_held & WPAD_NUNCHUK_BUTTON_C));
    if ((buttons_down & WPAD_BUTTON_A) || (exp.type == WPAD_EXP_NUNCHUK && (buttons_down & WPAD_NUNCHUK_BUTTON_Z))) {
        GlobalInput.attack = true;
    }
    if (buttons_down & WPAD_BUTTON_B) {
        GlobalInput.place = true;
    }

    Timer_advanceTime(&gameTimer);
    


    for (int i = 0; i < gameTimer.ticks; i++) {
        tickGame();
    }
    renderGame(NULL, gameTimer.a);
    
    // FPS tracking
    wiiFrames++;
    long now = (long)(Util_getTime() * 1000.0);
    if (wiiLastTime == 0) {
        wiiLastTime = now;
    }
    while (now >= wiiLastTime + 1000L) {
        currentFPS = wiiFrames;
        wiiLastTime += 1000L;
        wiiFrames = 0;
    }

    glutSwapBuffers();
}

static void wii_idle_func(void) {
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    fatInitDefault();
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(640, 480);
    glutCreateWindow("RubyDung Wii");

    // Initialize Wii controllers
    WPAD_Init();

    width = 640;
    height = 480;

    initGame(NULL);

    gameTimer = Timer_create(60.0f);

    glutDisplayFunc(wii_display_func);
    glutIdleFunc(wii_idle_func);

    glutMainLoop();

    destroyGame();
    return 0;
}
#else
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "RubyDung Android Port", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    
    glfwSwapInterval(1);

    initGame(window);

    gameTimer = Timer_create(60.0f);
    long lastTime = (long)(Util_getTime() * 1000.0);
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        Timer_advanceTime(&gameTimer);
        for (int i = 0; i < gameTimer.ticks; i++) {
            tickGame();
        }
        renderGame(window, gameTimer.a);
        frames++;
        long now = (long)(Util_getTime() * 1000.0);
        while (now >= lastTime + 1000L) {
            currentFPS = frames;
            Chunk_updates = 0;
            lastTime += 1000L;
            frames = 0;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyGame();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
#endif
