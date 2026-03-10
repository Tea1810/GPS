#include <GL/freeglut.h>
#include <cmath>
#include <vector>

// Camera state
static float camX = 0.0f, camY = 2.0f, camZ = 10.0f;
static float camYaw = 180.0f, camPitch = -10.0f;

static GLuint texGrass = 0, texHorizon = 0;

GLuint uploadTexture(std::vector<unsigned char>& pixels, int w, int h)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return id;
}

// Blotchy green pattern using sine waves
GLuint makeTexture_Grass(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            float bx = sinf(x * 0.18f) * cosf(y * 0.22f);
            float by = sinf(x * 0.31f + 1.1f) * sinf(y * 0.27f + 0.7f);
            float blend = (bx + by + 2.0f) / 4.0f;
            pixels[idx + 0] = (unsigned char)(40  + 20 * blend);
            pixels[idx + 1] = (unsigned char)(110 + 50 * blend);
            pixels[idx + 2] = (unsigned char)(20  + 10 * blend);
        }
    }
    return uploadTexture(pixels, w, h);
}

// Brown ground strip at the bottom, blue mountain silhouette, grey sky above
GLuint makeTexture_Horizon(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);

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
                // Ground strip – brown dirt
                pixels[idx + 0] = 101; pixels[idx + 1] = 67; pixels[idx + 2] = 33;
            }
            else if (ty < mh) {
                // Mountain body – blue, lighter at base, deeper at peak
                float fog = 1.0f - (mh - ty) / mh;
                pixels[idx + 0] = (unsigned char)(30  + 150 * fog);
                pixels[idx + 1] = (unsigned char)(100 + 120 * fog);
                pixels[idx + 2] = (unsigned char)(200 +  55 * fog);
            }
            else {
                // Sky – grey, darker near horizon, lighter at top
                float skyT = (ty - mh) / (1.0f - mh);
                pixels[idx + 0] = (unsigned char)(90 + 70 * skyT);
                pixels[idx + 1] = (unsigned char)(90 + 70 * skyT);
                pixels[idx + 2] = (unsigned char)(95 + 75 * skyT);
            }
        }
    }
    return uploadTexture(pixels, w, h);
}

void drawSkybox()
{
    float s = 200.0f;

    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(camX, camY, camZ);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Top + four sides – horizon texture
    glBindTexture(GL_TEXTURE_2D, texHorizon);

    glBegin(GL_QUADS); // Top
    glTexCoord2f(0,0); glVertex3f(-s, s,-s);
    glTexCoord2f(1,1); glVertex3f( s, s,-s);
    glTexCoord2f(1,1); glVertex3f( s, s, s);
    glTexCoord2f(0,1); glVertex3f(-s, s, s);
    glEnd();

    glBegin(GL_QUADS); // Front (south)
    glTexCoord2f(0,0); glVertex3f(-s,-s, s);
    glTexCoord2f(1,0); glVertex3f( s,-s, s);
    glTexCoord2f(1,1); glVertex3f( s, s, s);
    glTexCoord2f(0,1); glVertex3f(-s, s, s);
    glEnd();

    glBegin(GL_QUADS); // Back (north)
    glTexCoord2f(0,0); glVertex3f( s,-s,-s);
    glTexCoord2f(1,0); glVertex3f(-s,-s,-s);
    glTexCoord2f(1,1); glVertex3f(-s, s,-s);
    glTexCoord2f(0,1); glVertex3f( s, s,-s);
    glEnd();

    glBegin(GL_QUADS); // Left (west)
    glTexCoord2f(0,0); glVertex3f(-s,-s,-s);
    glTexCoord2f(1,0); glVertex3f(-s,-s, s);
    glTexCoord2f(1,1); glVertex3f(-s, s, s);
    glTexCoord2f(0,1); glVertex3f(-s, s,-s);
    glEnd();

    glBegin(GL_QUADS); // Right (east)
    glTexCoord2f(0,0); glVertex3f( s,-s, s);
    glTexCoord2f(1,0); glVertex3f( s,-s,-s);
    glTexCoord2f(1,1); glVertex3f( s, s,-s);
    glTexCoord2f(0,1); glVertex3f( s, s, s);
    glEnd();

    // Bottom – grass texture
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(-s,-s, s);
    glTexCoord2f(1,0); glVertex3f( s,-s, s);
    glTexCoord2f(1,1); glVertex3f( s,-s,-s);
    glTexCoord2f(0,1); glVertex3f(-s,-s,-s);
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
}

// Large flat grass quad at Y=0
void drawGround()
{
    float half = 50.0f, tile = 20.0f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    glTexCoord2f(0,    tile); glVertex3f(-half, 0.0f, -half);
    glTexCoord2f(tile, tile); glVertex3f( half, 0.0f, -half);
    glTexCoord2f(tile, 0);    glVertex3f( half, 0.0f,  half);
    glTexCoord2f(0,    0);    glVertex3f(-half, 0.0f,  half);
    glEnd();
}

// Flat brown rectangle centred on the origin, 15x15 world units
void drawTerrain()
{
    float half = 7.5f;
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.40f, 0.26f, 0.13f);
    glBegin(GL_QUADS);
    glVertex3f(-half, 0.01f, -half);
    glVertex3f( half, 0.01f, -half);
    glVertex3f( half, 0.01f,  half);
    glVertex3f(-half, 0.01f,  half);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawRad   = camYaw   * 3.14159f / 180.0f;
    float pitchRad = camPitch * 3.14159f / 180.0f;
    float dirX = sinf(yawRad) * cosf(pitchRad);
    float dirY = sinf(pitchRad);
    float dirZ = cosf(yawRad) * cosf(pitchRad);

    gluLookAt(camX, camY, camZ,
              camX + dirX, camY + dirY, camZ + dirZ,
              0.0f, 1.0f, 0.0f);

    drawSkybox();
    drawGround();
    drawTerrain();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y)
{
    float speed = 0.3f;
    float yawRad = camYaw * 3.14159f / 180.0f;
    switch (key) {
    case 'w': case 'W': camX += sinf(yawRad) * speed; camZ += cosf(yawRad) * speed; break;
    case 's': case 'S': camX -= sinf(yawRad) * speed; camZ -= cosf(yawRad) * speed; break;
    case 'a': case 'A': camX -= cosf(yawRad) * speed; camZ += sinf(yawRad) * speed; break;
    case 'd': case 'D': camX += cosf(yawRad) * speed; camZ -= sinf(yawRad) * speed; break;
    case 27:            glutLeaveMainLoop(); break;
    }
    if (camY < 0.5f) camY = 0.5f;
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:  camYaw   -= 3.0f; break;
    case GLUT_KEY_RIGHT: camYaw   += 3.0f; break;
    case GLUT_KEY_UP:    camPitch += 2.0f; break;
    case GLUT_KEY_DOWN:  camPitch -= 2.0f; break;
    }
    if (camPitch >  89.0f) camPitch =  89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
    glutPostRedisplay();
}

void idle() { glutPostRedisplay(); }

// =============================================================================
//  MAIN
// =============================================================================

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("OpenGL Scene – World in a Cube");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.12f, 0.39f, 0.78f, 1.0f);

    texGrass   = makeTexture_Grass(256, 256);
    texHorizon = makeTexture_Horizon(698, 256);

    glutMainLoop();
    return 0;
}
