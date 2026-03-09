#include <GL/freeglut.h>
#include <cmath>
#include <vector>

static int windowWidth = 1280;
static int windowHeight = 720;

static float camX = 0.0f;
static float camY = 2.0f;
static float camZ = 10.0f;
static float camYaw = 180.0f; // horizontal angle (degrees)
static float camPitch = -10.0f;  // vertical angle (degrees)

static GLuint texGrass = 0;
static GLuint texSky = 0;
static GLuint texMountain = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  TERRAIN DATA  (generated once, stored as a grid of heights)
// ─────────────────────────────────────────────────────────────────────────────
static const int GRID = 30;         // 30×30 quads
static const float CELL = 0.5f;     // world size of each cell
static float terrainH[GRID + 1][GRID + 1]; // height at each grid point

// ─────────────────────────────────────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────────────────────────────────────
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void idle();

GLuint makeTexture_Grass(int w, int h);
GLuint makeTexture_Sky(int w, int h);
GLuint makeTexture_Mountain(int w, int h);

void drawSkybox();
void drawGround();
void drawTerrain();

// =============================================================================
//  MAIN
// =============================================================================
int main(int argc, char** argv)
{
    // ── freeGLUT init (same as Course 1 example) ──────────────────────────────
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("OpenGL Scene – World in a Cube");

    // ── Callbacks ─────────────────────────────────────────────────────────────
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    // ── OpenGL state ──────────────────────────────────────────────────────────
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.12f, 0.39f, 0.78f, 1.0f); // deep sky blue fallback

    // ── Generate procedural textures ──────────────────────────────────────────
    texGrass = makeTexture_Grass(256, 256);
    texSky = makeTexture_Sky(512, 256);
    texMountain = makeTexture_Mountain(512, 256);

    // ── Pre-compute terrain heights ───────────────────────────────────────────
    for (int iz = 0; iz <= GRID; iz++) {
        for (int ix = 0; ix <= GRID; ix++) {
            float wx = ix * CELL;
            float wz = iz * CELL;
            // Layered sine waves → rolling hills
            terrainH[ix][iz] =
                0.6f * sinf(wx * 0.8f) * cosf(wz * 0.6f)
                + 0.3f * sinf(wx * 1.7f + 0.5f) * sinf(wz * 1.4f)
                + 0.2f * cosf(wx * 3.1f) * sinf(wz * 2.9f + 1.0f);
        }
    }

    glutMainLoop();
    return 0;
}

// =============================================================================
//  DISPLAY CALLBACK
// =============================================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── Set up camera (View matrix) ───────────────────────────────────────────
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Convert yaw + pitch to a look-at target
    float yawRad = camYaw * 3.14159f / 180.0f;
    float pitchRad = camPitch * 3.14159f / 180.0f;
    float dirX = sinf(yawRad) * cosf(pitchRad);
    float dirY = sinf(pitchRad);
    float dirZ = cosf(yawRad) * cosf(pitchRad);

    gluLookAt(
        camX, camY, camZ,                          // eye position
        camX + dirX, camY + dirY, camZ + dirZ,     // look-at target
        0.0f, 1.0f, 0.0f                           // up vector
    );

    // ── Draw scene elements ───────────────────────────────────────────────────
    drawSkybox();
    drawGround();
    drawTerrain();

    glutSwapBuffers();
}

// =============================================================================
//  RESHAPE CALLBACK  (same pattern as Course 1 PDF)
// =============================================================================
void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 0.1, 500.0);

    glMatrixMode(GL_MODELVIEW);
}

// =============================================================================
//  INPUT
// =============================================================================
void keyboard(unsigned char key, int x, int y)
{
    float speed = 0.3f;
    float yawRad = camYaw * 3.14159f / 180.0f;

    switch (key) {
    case 'w': case 'W':
        camX += sinf(yawRad) * speed;
        camZ += cosf(yawRad) * speed;
        break;
    case 's': case 'S':
        camX -= sinf(yawRad) * speed;
        camZ -= cosf(yawRad) * speed;
        break;
    case 'a': case 'A':
        camX -= cosf(yawRad) * speed;
        camZ += sinf(yawRad) * speed;
        break;
    case 'd': case 'D':
        camX += cosf(yawRad) * speed;
        camZ -= sinf(yawRad) * speed;
        break;
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    }
    // Keep camera above ground
    if (camY < 0.5f) camY = 0.5f;
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:  camYaw -= 3.0f; break;
    case GLUT_KEY_RIGHT: camYaw += 3.0f; break;
    case GLUT_KEY_UP:    camPitch += 2.0f; break;
    case GLUT_KEY_DOWN:  camPitch -= 2.0f; break;
    }
    // Clamp pitch so we can't flip upside down
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
    glutPostRedisplay();
}

void idle()
{
    glutPostRedisplay();
}

// =============================================================================
//  PROCEDURAL TEXTURES
//  All textures are generated as pixel arrays on the CPU, then uploaded to GPU.
//  This means no image files are needed.
// =============================================================================

// Helper: upload an RGB pixel array and return a texture ID
GLuint uploadTexture(std::vector<unsigned char>& pixels, int w, int h)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Upload pixels to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
        GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Texture filtering and wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return id;
}

// ── Grass texture: natural green blotchy pattern ──────────────────────────────
GLuint makeTexture_Grass(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            // Smooth organic blotches using sine waves
            float bx = sinf(x * 0.18f) * cosf(y * 0.22f);
            float by = sinf(x * 0.31f + 1.1f) * sinf(y * 0.27f + 0.7f);
            float blend = (bx + by + 2.0f) / 4.0f; // 0..1
            // Natural grass green: medium → bright green
            pixels[idx + 0] = (unsigned char)(40 + 20 * blend); // R – low
            pixels[idx + 1] = (unsigned char)(110 + 50 * blend); // G – dominant
            pixels[idx + 2] = (unsigned char)(20 + 10 * blend); // B – very low
        }
    }
    return uploadTexture(pixels, w, h);
}

// 25002500 Sky texture: deep blue at top fading to light blue at horizon 25002500250025002500250025002500250025002500250025002500250025002500
GLuint makeTexture_Sky(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);

    // Simple mountain height function using overlapping sine waves
    auto mountainHeight = [](float nx) -> float {
        float h1 = 0.35f + 0.20f * sinf(nx * 6.28f * 2.0f);
        float h2 = 0.30f + 0.15f * sinf(nx * 6.28f * 5.0f + 1.2f);
        float h3 = 0.25f + 0.10f * sinf(nx * 6.28f * 11.0f + 0.5f);
        return (h1 + h2 + h3) / 3.0f;
        };

    for (int y = 0; y < h; y++) {
        float ty = 1.0f - (float)y / (h - 1); // 0=bottom, 1=top
        for (int x = 0; x < w; x++) {
            float nx = (float)x / (w - 1);
            float mh = mountainHeight(nx);
            int idx = (y * w + x) * 3;

            if (ty < 0.08f) {
                // Ground strip 2013 earthy brown (dirt)
                pixels[idx + 0] = 101; pixels[idx + 1] = 67; pixels[idx + 2] = 33;
            }
            else if (ty < mh) {
                // Mountain body 2013 natural grey (dark base 2192 lighter near peak)
                float fog = 1.0f - (mh - ty) / mh;
                pixels[idx + 0] = (unsigned char)(90 + 70 * fog);  // R grey
                pixels[idx + 1] = (unsigned char)(90 + 70 * fog);  // G grey
                pixels[idx + 2] = (unsigned char)(95 + 75 * fog);  // B slight blue tint at peak
            }
            else {
                // Sky above mountains 2013 matches sky texture (deep blue 2192 light blue)
                float skyT = (ty - mh) / (1.0f - mh);
                pixels[idx + 0] = (unsigned char)(30 + 150 * (1.0f - skyT));
                pixels[idx + 1] = (unsigned char)(100 + 120 * (1.0f - skyT));
                pixels[idx + 2] = (unsigned char)(200 + 55 * (1.0f - skyT));
            }
        }
    }
    return uploadTexture(pixels, w, h);
}

// ── Mountain texture: sky gradient + mountain silhouette + ground strip ───────
GLuint makeTexture_Mountain(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);
    for (int y = 0; y < h; y++) {
        float t = (float)y / (h - 1); // 0=top, 1=bottom(horizon)
        unsigned char r = (unsigned char)(30 + 150 * t);
        unsigned char g = (unsigned char)(100 + 120 * t);
        unsigned char b = (unsigned char)(200 + 55 * t);
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            pixels[idx + 0] = r; pixels[idx + 1] = g; pixels[idx + 2] = b;
        }
    }
    return uploadTexture(pixels, w, h);
}

// =============================================================================
//  DRAWING FUNCTIONS
// =============================================================================

// ── SKYBOX ────────────────────────────────────────────────────────────────────
// A large cube centred on the camera. We disable depth writes so it is always
// drawn "behind" all other geometry.
void drawSkybox()
{
    float s = 200.0f; // half-size of the skybox

    // Reset vertex color to white so textures show their true colors
    // (terrain uses glColor3f which would otherwise tint the skybox)
    glColor3f(1.0f, 1.0f, 1.0f);

    // Centre the box on the camera every frame
    glPushMatrix();
    glTranslatef(camX, camY, camZ);

    // Disable depth writing – skybox is always "behind" everything
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // ── TOP FACE – sky texture ────────────────────────────────────────────────
    glBindTexture(GL_TEXTURE_2D, texSky);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    // ── BOTTOM FACE – grass texture ───────────────────────────────────────────
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, -s);
    glEnd();

    // ── SIDE FACES – mountain/horizon texture ─────────────────────────────────
    // UV: U goes left to right, V=0 at bottom of wall (ground), V=1 at top (sky)
    glBindTexture(GL_TEXTURE_2D, texMountain);

    // Front face (south)
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);  // bottom-left  V=0 (ground)
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);  // bottom-right V=0 (ground)
    glTexCoord2f(1, 1); glVertex3f(s, s, s);  // top-right    V=1 (sky)
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);  // top-left     V=1 (sky)
    glEnd();

    // Back face (north)
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    // Left face (west)
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glEnd();

    // Right face (east)
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    // Re-enable depth for the rest of the scene
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();

    // Reset color to white for subsequent textured draws
    glColor3f(1.0f, 1.0f, 1.0f);
}

// ── GROUND PLANE ──────────────────────────────────────────────────────────────
// A large flat quad at Y=0, tiled with the grass texture.
void drawGround()
{
    float half = 50.0f;  // extends ±50 units from centre
    float tile = 20.0f;  // UV repeats 20× so the grass pattern tiles nicely

    glColor3f(1.0f, 1.0f, 1.0f); // reset color so texture shows correctly
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    glTexCoord2f(0, tile); glVertex3f(-half, 0.0f, -half);
    glTexCoord2f(tile, tile); glVertex3f(half, 0.0f, -half);
    glTexCoord2f(tile, 0); glVertex3f(half, 0.0f, half);
    glTexCoord2f(0, 0); glVertex3f(-half, 0.0f, half);
    glEnd();
}

// ── TERRAIN ───────────────────────────────────────────────────────────────────
// A 30×30 grid of quads with sine-wave height variation.
// Each vertex is colored green→brown based on its height.
void drawTerrain()
{
    // Place the terrain slightly offset from centre so it sits in the scene
    glPushMatrix();
    glTranslatef(-7.5f, 0.0f, -7.5f);

    // Disable texturing for this mesh – we use vertex colors instead
    glDisable(GL_TEXTURE_2D);

    for (int iz = 0; iz < GRID; iz++) {
        for (int ix = 0; ix < GRID; ix++) {

            // Heights of the four corners of this quad
            float h00 = terrainH[ix][iz];
            float h10 = terrainH[ix + 1][iz];
            float h01 = terrainH[ix][iz + 1];
            float h11 = terrainH[ix + 1][iz + 1];

            // World positions
            float x0 = ix * CELL, x1 = (ix + 1) * CELL;
            float z0 = iz * CELL, z1 = (iz + 1) * CELL;

            // Color by height:
            //   low  (valleys) → brown dirt   (0.40, 0.26, 0.13)
            //   mid  (slopes)  → grass green  (0.13, 0.55, 0.13)
            //   high (peaks)   → rocky grey   (0.55, 0.55, 0.55)
            auto heightColor = [](float h) {
                float t = (h + 0.8f) / 1.6f; // normalise to [0,1]
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                if (t < 0.4f) {
                    // brown dirt at low points
                    float s = t / 0.4f;
                    glColor3f(0.40f - 0.05f * s, 0.26f + 0.10f * s, 0.13f);
                }
                else if (t < 0.7f) {
                    // grass green on mid slopes
                    float s = (t - 0.4f) / 0.3f;
                    glColor3f(0.13f + 0.10f * s, 0.55f - 0.05f * s, 0.08f);
                }
                else {
                    // rocky grey at peaks
                    float s = (t - 0.7f) / 0.3f;
                    glColor3f(0.23f + 0.32f * s, 0.50f + 0.05f * s, 0.08f + 0.12f * s);
                }
                };

            // Draw as two triangles (same as GL_QUADS but explicit)
            glBegin(GL_TRIANGLES);
            // Triangle 1
            heightColor(h00); glVertex3f(x0, h00, z0);
            heightColor(h10); glVertex3f(x1, h10, z0);
            heightColor(h01); glVertex3f(x0, h01, z1);
            // Triangle 2
            heightColor(h10); glVertex3f(x1, h10, z0);
            heightColor(h11); glVertex3f(x1, h11, z1);
            heightColor(h01); glVertex3f(x0, h01, z1);
            glEnd();
        }
    }

    // Re-enable texturing for anything drawn after terrain
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}