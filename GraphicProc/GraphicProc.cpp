#include <GL/freeglut.h>
#include <cmath>
#include <vector>

// Camera state
static float camX = 0.0f, camY = 2.0f, camZ = 10.0f;
static float camYaw = 180.0f, camPitch = -10.0f;

static GLuint texGrass = 0, texHorizon = 0, texAsphalt = 0;

// Oval circuit parameters
const int   TRACK_SEGS    = 72;
const float TRACK_OUTER_A = 22.0f;  // outer ellipse X radius
const float TRACK_OUTER_B = 16.0f;  // outer ellipse Z radius
const float TRACK_INNER_A = 15.0f;  // inner ellipse X radius
const float TRACK_INNER_B =  9.0f;  // inner ellipse Z radius

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
        float ty = 1.0f - (float)y / (h - 1);
        for (int x = 0; x < w; x++) {
            float nx = (float)x / (w - 1);
            float mh = mountainHeight(nx);
            int idx = (y * w + x) * 3;

            if (ty < 0.08f) {
                pixels[idx + 0] = 101; pixels[idx + 1] = 67; pixels[idx + 2] = 33;
            }
            else if (ty < mh) {
                float fog = 1.0f - (mh - ty) / mh;
                pixels[idx + 0] = (unsigned char)(30  + 150 * fog);
                pixels[idx + 1] = (unsigned char)(100 + 120 * fog);
                pixels[idx + 2] = (unsigned char)(200 +  55 * fog);
            }
            else {
                float skyT = (ty - mh) / (1.0f - mh);
                pixels[idx + 0] = (unsigned char)(90 + 70 * skyT);
                pixels[idx + 1] = (unsigned char)(90 + 70 * skyT);
                pixels[idx + 2] = (unsigned char)(95 + 75 * skyT);
            }
        }
    }
    return uploadTexture(pixels, w, h);
}

// 0 = inner edge, 1 = outer edge.
GLuint makeTexture_Asphalt(int w, int h)
{
    std::vector<unsigned char> pixels(w * h * 3);
    for (int y = 0; y < h; y++) {
        float fv = (float)y / (h - 1);   
        for (int x = 0; x < w; x++) {
            float fu = (float)x / (w - 1);
            int idx = (y * w + x) * 3;

            float n = sinf(x * 1.3f) * cosf(y * 0.9f)
                    + sinf(x * 2.7f + y * 1.1f) * 0.5f;
            unsigned char base = (unsigned char)(52 + 12 * n);
            pixels[idx + 0] = base;
            pixels[idx + 1] = base;
            pixels[idx + 2] = base;

            // Solid white edge lines
            if (fv < 0.05f || fv > 0.95f) {
                pixels[idx + 0] = 210;
                pixels[idx + 1] = 210;
                pixels[idx + 2] = 210;
            }
            //  yellow centre 
            else if (fv > 0.47f && fv < 0.53f) {
                if (fmodf(fu * 10.0f, 1.0f) < 0.55f) {
                    pixels[idx + 0] = 220;
                    pixels[idx + 1] = 200;
                    pixels[idx + 2] =  30;
                }
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

    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS); // Bottom
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

void drawCircuit()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texAsphalt);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Oval track rendered as a quad strip between inner and outer ellipses
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= TRACK_SEGS; i++) {
        float angle = 2.0f * 3.14159265f * i / TRACK_SEGS;
        float ca = cosf(angle), sa = sinf(angle);
        float u  = (float)i / TRACK_SEGS;

        glTexCoord2f(u, 0.0f);
        glVertex3f(TRACK_INNER_A * ca, 0.02f, TRACK_INNER_B * sa);

        glTexCoord2f(u, 1.0f);
        glVertex3f(TRACK_OUTER_A * ca, 0.02f, TRACK_OUTER_B * sa);
    }
    glEnd();

    // Start / finish line: 
    glDisable(GL_TEXTURE_2D);
    const int CHECK = 8;
    float ang0 = -0.04f, ang1 = 0.04f;
    float ca0 = cosf(ang0), sa0 = sinf(ang0);
    float ca1 = cosf(ang1), sa1 = sinf(ang1);

    for (int c = 0; c < CHECK; c++) {
        float t0 = (float) c      / CHECK;
        float t1 = (float)(c + 1) / CHECK;
        float ix0 = TRACK_INNER_A + (TRACK_OUTER_A - TRACK_INNER_A) * t0;
        float ix1 = TRACK_INNER_A + (TRACK_OUTER_A - TRACK_INNER_A) * t1;

        // Alternate black / white
        if (c % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
        else            glColor3f(0.05f, 0.05f, 0.05f);

        glBegin(GL_QUADS);
        glVertex3f(ix0 * ca0, 0.03f, ix0 * sa0);
        glVertex3f(ix1 * ca0, 0.03f, ix1 * sa0);
        glVertex3f(ix1 * ca1, 0.03f, ix1 * sa1);
        glVertex3f(ix0 * ca1, 0.03f, ix0 * sa1);
        glEnd();
    }
    glEnable(GL_TEXTURE_2D);
}

static void primCone(float x, float baseY, float z,
                     float radius, float height, int segs,
                     float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);
    const float PI2 = 2.0f * 3.14159265f;

    // Side
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, baseY + height, z);
    for (int i = 0; i <= segs; i++) {
        float a = PI2 * i / segs;
        glVertex3f(x + radius * cosf(a), baseY, z + radius * sinf(a));
    }
    glEnd();

    // Base cap
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, baseY, z);
    for (int i = segs; i >= 0; i--) {
        float a = PI2 * i / segs;
        glVertex3f(x + radius * cosf(a), baseY, z + radius * sinf(a));
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

static void primCylinder(float x, float baseY, float z,
                         float radius, float height, int segs,
                         float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);
    const float PI2 = 2.0f * 3.14159265f;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; i++) {
        float a  = PI2 * i / segs;
        float cx = radius * cosf(a);
        float cz = radius * sinf(a);
        glVertex3f(x + cx, baseY + height, z + cz);
        glVertex3f(x + cx, baseY,          z + cz);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

static void primBox(float cx, float baseY, float cz,
                    float w, float h, float d,
                    float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    float y0 = baseY,          y1 = baseY + h;
    float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;

    // front / back: full colour
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1);
    glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);

    glVertex3f(x1,y0,z0); glVertex3f(x0,y0,z0);
    glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);
    glEnd();

    // left / right: slightly darker
    glColor3f(r * 0.82f, g * 0.82f, b * 0.82f);
    glBegin(GL_QUADS);
    glVertex3f(x0,y0,z0); glVertex3f(x0,y0,z1);
    glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);

    glVertex3f(x1,y0,z1); glVertex3f(x1,y0,z0);
    glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1);
    glEnd();

    // top: slightly lighter
    glColor3f(r * 1.15f, g * 1.15f, b * 1.15f);
    glBegin(GL_QUADS);
    glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);
    glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

static void drawTree(float x, float z)
{
    primCylinder(x, 0.0f, z,  0.15f, 0.9f, 8,  0.40f, 0.25f, 0.10f);
    primCone    (x, 0.9f, z,  1.10f, 2.8f, 10, 0.10f, 0.52f, 0.10f);
    primCone    (x, 1.8f, z,  0.80f, 2.0f, 10, 0.12f, 0.58f, 0.12f);
}

static void drawBuilding(float x, float z, float w, float h, float d)
{
    primBox(x, 0.0f, z, w, h, d, 0.62f, 0.62f, 0.68f);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.12f, 0.18f, 0.35f);

    int cols = (int)(w / 1.2f) + 1;
    int rows = (int)(h / 1.5f);
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;

    float winW = 0.45f, winH = 0.55f;
    float colStep = w  / cols;
    float rowStep = h  / rows;

    for (int face = 0; face < 2; face++) {
        float fz = (face == 0) ? (z + d * 0.5f + 0.01f) : (z - d * 0.5f - 0.01f);
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                float wx = x - w * 0.5f + colStep * (col + 0.5f);
                float wy = rowStep * (row + 0.25f) + 0.3f;
                glBegin(GL_QUADS);
                glVertex3f(wx - winW*0.5f, wy,        fz);
                glVertex3f(wx + winW*0.5f, wy,        fz);
                glVertex3f(wx + winW*0.5f, wy + winH, fz);
                glVertex3f(wx - winW*0.5f, wy + winH, fz);
                glEnd();
            }
        }
    }
    glEnable(GL_TEXTURE_2D);
}

static void drawGrandstand(float x, float z)
{
    primBox(x, 0.0f, z + 2.0f, 10.0f, 1.0f, 2.5f, 0.72f, 0.72f, 0.72f);
    primBox(x, 1.0f, z + 0.5f, 10.0f, 1.0f, 2.5f, 0.72f, 0.72f, 0.72f);
    primBox(x, 2.0f, z - 1.0f, 10.0f, 1.0f, 2.5f, 0.72f, 0.72f, 0.72f);
    primBox(x, 3.2f, z + 0.5f, 10.2f, 0.25f, 7.5f, 0.35f, 0.35f, 0.38f);
    for (int c = -1; c <= 1; c++) {
        primBox(x + c * 4.5f, 0.0f, z + 0.5f, 0.3f, 3.2f, 0.3f, 0.50f, 0.50f, 0.55f);
    }
}

static void drawTireBarrier(float x, float z)
{
    primCylinder(x, 0.0f, z, 0.45f, 0.45f, 10, 0.10f, 0.10f, 0.10f);
    primCylinder(x, 0.45f, z, 0.45f, 0.45f, 10, 0.13f, 0.13f, 0.13f);
}

static void drawLamppost(float x, float z)
{
    primCylinder(x,    0.0f, z, 0.06f, 5.0f, 6, 0.55f, 0.55f, 0.60f);
    // horizontal arm
    primBox     (x + 0.5f, 5.0f, z, 1.0f, 0.1f, 0.1f, 0.55f, 0.55f, 0.60f);
    // lamp head (yellow)
    primBox     (x + 1.05f, 4.85f, z, 0.35f, 0.22f, 0.22f, 1.0f, 0.88f, 0.20f);
}

static void drawPitWall(float x, float z, float length)
{
    // White pit wall with red stripe at top
    primBox(x, 0.0f, z, 0.30f, 0.75f, length, 0.92f, 0.92f, 0.92f);
    primBox(x, 0.75f, z, 0.30f, 0.20f, length, 0.85f, 0.10f, 0.10f);
}

void drawStaticObjects()
{
    drawTree(-30.0f,  0.0f);    //  west
    drawTree(-30.0f,  9.0f);    //  west-north
    drawTree(-30.0f, -9.0f);    //  west-south
    drawTree( 30.0f,  0.0f);    //  east
    drawTree(  0.0f, -22.0f);   //  south

    // 4 Buildings (safely outside the circuit) 
    drawBuilding(-38.0f,  0.0f,  5.0f, 9.0f, 6.0f);   //  west (tall)
    drawBuilding( 37.0f,  0.0f,  4.5f, 6.0f, 5.0f);   //  east (medium)
    drawBuilding( -9.0f, -28.0f, 5.0f, 5.0f, 5.0f);   //  south-west (short)
    drawBuilding(  9.0f, -28.0f, 5.0f, 4.0f, 5.0f);   //  south-east (short)

    //  Grandstand on the main straight 
    drawGrandstand(31.0f, 0.0f);                        // 

    //  Tire barriers near start/finish 
    drawTireBarrier(23.2f,  3.0f);                      // 
    drawTireBarrier(23.2f, -3.0f);                      // 

    //  Lamp posts around the circuit 
    drawLamppost(  0.0f,  19.0f);                       // north straight
    drawLamppost(  0.0f, -19.0f);                       // south straight
    drawLamppost(-23.5f,   0.0f);                       // west straight

    //  Pit-lane wall (infield, near east straight) 
    drawPitWall(13.5f, 0.0f, 10.0f);                   
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
    drawCircuit();
    drawStaticObjects();

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
    case 'w': camX += sinf(yawRad) * speed; camZ += cosf(yawRad) * speed; break;
    case 's': camX -= sinf(yawRad) * speed; camZ -= cosf(yawRad) * speed; break;
    case 'a': camX += cosf(yawRad) * speed; camZ -= sinf(yawRad) * speed; break;
    case 'd': camX -= cosf(yawRad) * speed; camZ += sinf(yawRad) * speed; break;
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

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("OpenGL Street Circuit");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.12f, 0.39f, 0.78f, 1.0f);

    texGrass   = makeTexture_Grass(256, 256);
    texHorizon = makeTexture_Horizon(512, 256);
    texAsphalt = makeTexture_Asphalt(512, 128);

    glutMainLoop();
    return 0;
}
