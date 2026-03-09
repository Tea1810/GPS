// =============================================================================
//  OpenGL Beginner Scene – "World in a Cube"
//  Course: Graphics Processing Systems
//
//  Requirements:
//    - GLFW3       (window + input)
//    - GLAD        (function loader)  – generated for OpenGL 3.3 core
//    - GLM         (mathematics)
//    - A C++17 compiler
//
//  What this project does:
//    1. Draws a large skybox cube around the whole scene
//       (sky-blue top, mountain-silhouette sides, horizon gradient)
//    2. Draws a textured ground plane (grass-green checkerboard)
//    3. Draws a simple terrain area made of a grid of quads
//       with height variation (hills)
//    4. Basic camera – use W/A/S/D to move, mouse to look around
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  WINDOW SETTINGS
// ─────────────────────────────────────────────────────────────────────────────
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const char* WINDOW_TITLE = "OpenGL Scene – World in a Cube";

// ─────────────────────────────────────────────────────────────────────────────
//  CAMERA (simple first-person)
// ─────────────────────────────────────────────────────────────────────────────
glm::vec3 cameraPos = glm::vec3(0.0f, 2.5f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;  // horizontal angle
float pitch = -10.0f; // vertical angle (slight downward tilt)
float fov = 60.0f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ─────────────────────────────────────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int compileShader(const char* vertSrc, const char* fragSrc);
unsigned int createProceduralTexture_Grass(int w, int h);
unsigned int createProceduralTexture_Sky(int w, int h);
unsigned int createProceduralTexture_Mountain(int w, int h);

void buildSkyboxMesh(unsigned int& VAO, unsigned int& VBO, int& vertCount);
void buildGroundMesh(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO,
    int& indexCount);
void buildTerrainMesh(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO,
    int& indexCount, int gridX, int gridZ);

// ─────────────────────────────────────────────────────────────────────────────
//  GLSL SHADERS  (vertex + fragment bundled as raw strings)
// ─────────────────────────────────────────────────────────────────────────────

// -- Textured shader (used for ground + skybox walls) -------------------------
const char* texVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* texFragSrc = R"(
#version 330 core
in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;
uniform vec4      colorTint;   // multiply color (use vec4(1) for no tint)

void main()
{
    FragColor = texture(ourTexture, TexCoord) * colorTint;
}
)";

// -- Terrain shader (vertex-colored height map) --------------------------------
const char* terrainVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertColor = aColor;
}
)";

const char* terrainFragSrc = R"(
#version 330 core
in  vec3 vertColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertColor, 1.0);
}
)";

// =============================================================================
//  MAIN
// =============================================================================
int main()
{
    // ── Init GLFW ─────────────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture mouse

    // ── Load GLAD ─────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ── Compile shaders ───────────────────────────────────────────────────────
    unsigned int texShader = compileShader(texVertSrc, texFragSrc);
    unsigned int terrainShader = compileShader(terrainVertSrc, terrainFragSrc);

    // ── Generate procedural textures ──────────────────────────────────────────
    unsigned int grassTex = createProceduralTexture_Grass(256, 256);
    unsigned int skyTex = createProceduralTexture_Sky(512, 256);
    unsigned int mountainTex = createProceduralTexture_Mountain(512, 256);

    // ── Build meshes ──────────────────────────────────────────────────────────
    // Skybox
    unsigned int skyVAO, skyVBO;
    int skyVertCount;
    buildSkyboxMesh(skyVAO, skyVBO, skyVertCount);

    // Ground plane
    unsigned int groundVAO, groundVBO, groundEBO;
    int groundIndexCount;
    buildGroundMesh(groundVAO, groundVBO, groundEBO, groundIndexCount);

    // Terrain grid  (30 x 30 quads)
    unsigned int terrainVAO, terrainVBO, terrainEBO;
    int terrainIndexCount;
    buildTerrainMesh(terrainVAO, terrainVBO, terrainEBO, terrainIndexCount,
        30, 30);

    // ── Render loop ───────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear
        glClearColor(0.52f, 0.80f, 0.92f, 1.0f); // fallback sky color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Matrices (shared)
        glm::mat4 view = glm::lookAt(cameraPos,
            cameraPos + cameraFront,
            cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 200.0f);

        // ── Draw SKYBOX ───────────────────────────────────────────────────────
        // We disable depth writing so the sky is always "behind" everything.
        glDepthMask(GL_FALSE);
        glUseProgram(texShader);

        glm::mat4 skyModel = glm::mat4(1.0f);
        // Centre the skybox on the camera so it never moves away
        skyModel = glm::translate(skyModel, cameraPos);
        skyModel = glm::scale(skyModel, glm::vec3(100.0f));

        glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(skyModel));
        glUniformMatrix4fv(glGetUniformLocation(texShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(texShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform4f(glGetUniformLocation(texShader, "colorTint"), 1, 1, 1, 1);

        glBindVertexArray(skyVAO);
        // Draw sky faces with different textures:
        //   We stored faces as: top(6), front/back/left/right(6 each), bottom(6)
        //   skyVertCount = 36 vertices total (6 faces × 6 verts)

        // Top face  → sky texture
        glBindTexture(GL_TEXTURE_2D, skyTex);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Side faces (4 × 6 = 24 verts) → mountain/horizon texture
        glBindTexture(GL_TEXTURE_2D, mountainTex);
        glDrawArrays(GL_TRIANGLES, 6, 24);

        // Bottom face → grass (hidden underground but keeps it clean)
        glBindTexture(GL_TEXTURE_2D, grassTex);
        glDrawArrays(GL_TRIANGLES, 30, 6);

        glDepthMask(GL_TRUE);

        // ── Draw GROUND ───────────────────────────────────────────────────────
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(texShader, "model"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform4f(glGetUniformLocation(texShader, "colorTint"), 1, 1, 1, 1);

        glBindTexture(GL_TEXTURE_2D, grassTex);
        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0);

        // ── Draw TERRAIN ──────────────────────────────────────────────────────
        glUseProgram(terrainShader);
        glm::mat4 terrainModel = glm::mat4(1.0f);
        // Place terrain slightly in front of centre
        terrainModel = glm::translate(terrainModel, glm::vec3(-7.5f, 0.0f, -7.5f));

        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, glm::value_ptr(terrainModel));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteBuffers(1, &groundEBO);
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
    glDeleteProgram(texShader);
    glDeleteProgram(terrainShader);

    glfwTerminate();
    return 0;
}

// =============================================================================
//  INPUT / CALLBACKS
// =============================================================================
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    // Keep camera above ground
    if (cameraPos.y < 0.5f) cameraPos.y = 0.5f;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    float xoffset = (xpos - lastX) * 0.1f;
    float yoffset = (lastY - ypos) * 0.1f; // reversed: y goes bottom-to-top
    lastX = xpos; lastY = ypos;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 10.0f) fov = 10.0f;
    if (fov > 90.0f) fov = 90.0f;
}

// =============================================================================
//  SHADER HELPERS
// =============================================================================
unsigned int compileShader(const char* vertSrc, const char* fragSrc)
{
    // Vertex shader
    unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);
    int ok; char log[512];
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(vert, 512, nullptr, log);
        std::cerr << "VERTEX SHADER:\n" << log << "\n";
    }

    // Fragment shader
    unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(frag, 512, nullptr, log);
        std::cerr << "FRAGMENT SHADER:\n" << log << "\n";
    }

    // Link
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "PROGRAM LINK:\n" << log << "\n";
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// =============================================================================
//  PROCEDURAL TEXTURES  (generated on CPU, uploaded to GPU)
//  No external image files required!
// =============================================================================

// Helper: upload an RGBA pixel array to a GL texture
unsigned int uploadTexture(std::vector<unsigned char>& data, int w, int h)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    // Repeat wrapping + linear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// Grass: dark-green/light-green checkerboard with subtle noise
unsigned int createProceduralTexture_Grass(int w, int h)
{
    std::vector<unsigned char> data(w * h * 4);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            bool checker = ((x / 16) + (y / 16)) % 2 == 0;
            // Add a tiny pseudo-random brightness variation
            float noise = 0.85f + 0.15f * (float)((x * 7 + y * 13) % 17) / 16.0f;
            unsigned char r = (unsigned char)((checker ? 34 : 50) * noise);
            unsigned char g = (unsigned char)((checker ? 120 : 160) * noise);
            unsigned char b = (unsigned char)((checker ? 24 : 40) * noise);
            data[idx + 0] = r; data[idx + 1] = g;
            data[idx + 2] = b; data[idx + 3] = 255;
        }
    }
    return uploadTexture(data, w, h);
}

// Sky: vertical gradient from light-cyan at top to pale-blue at horizon
unsigned int createProceduralTexture_Sky(int w, int h)
{
    std::vector<unsigned char> data(w * h * 4);
    for (int y = 0; y < h; y++) {
        float t = (float)y / (h - 1); // 0 = top, 1 = bottom (horizon)
        // Top: deep sky blue  →  Bottom: pale/white horizon
        unsigned char r = (unsigned char)(100 + 155 * t);
        unsigned char g = (unsigned char)(160 + 80 * t);
        unsigned char b = (unsigned char)(220 + 35 * t);
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            data[idx + 0] = r; data[idx + 1] = g; data[idx + 2] = b; data[idx + 3] = 255;
        }
    }
    return uploadTexture(data, w, h);
}

// Mountains: horizon band + mountain silhouette + sky above
unsigned int createProceduralTexture_Mountain(int w, int h)
{
    std::vector<unsigned char> data(w * h * 4);

    // Simple "mountain" height function using sin waves
    auto mountainHeight = [&](float nx) -> float {
        // nx in [0,1], returns normalized height [0,1]
        float h1 = 0.35f + 0.20f * sinf(nx * 6.28f * 2.0f);
        float h2 = 0.30f + 0.15f * sinf(nx * 6.28f * 5.0f + 1.2f);
        float h3 = 0.25f + 0.10f * sinf(nx * 6.28f * 11.0f + 0.5f);
        return (h1 + h2 + h3) / 3.0f; // average, roughly [0.2 – 0.55]
        };

    for (int y = 0; y < h; y++) {
        float ty = 1.0f - (float)y / (h - 1); // 0=bottom, 1=top of texture
        for (int x = 0; x < w; x++) {
            float nx = (float)x / (w - 1);
            float mh = mountainHeight(nx);          // mountain peak height
            int idx = (y * w + x) * 4;

            if (ty < 0.08f) {
                // Ground strip – dark green
                data[idx + 0] = 30; data[idx + 1] = 100; data[idx + 2] = 20; data[idx + 3] = 255;
            }
            else if (ty < mh) {
                // Mountain body – grey/purple
                float fog = 1.0f - (mh - ty) / mh; // lighter near peak
                unsigned char mr = (unsigned char)(80 + 60 * fog);
                unsigned char mg = (unsigned char)(70 + 50 * fog);
                unsigned char mb = (unsigned char)(90 + 70 * fog);
                data[idx + 0] = mr; data[idx + 1] = mg; data[idx + 2] = mb; data[idx + 3] = 255;
            }
            else {
                // Sky gradient above mountains
                float skyT = (ty - mh) / (1.0f - mh);
                unsigned char r = (unsigned char)(200 - 100 * skyT);
                unsigned char g = (unsigned char)(220 - 60 * skyT);
                unsigned char b = (unsigned char)(240 - 20 * skyT);
                data[idx + 0] = r; data[idx + 1] = g; data[idx + 2] = b; data[idx + 3] = 255;
            }
        }
    }
    return uploadTexture(data, w, h);
}

// =============================================================================
//  MESH BUILDERS
// =============================================================================

// ── SKYBOX ────────────────────────────────────────────────────────────────────
// A unit cube (centred at origin) with inward-facing normals.
// Layout per vertex: x y z  u v  (5 floats)
// Face order: top(0–5), front(6–11), back(12–17), left(18–23), right(24–29), bottom(30–35)
void buildSkyboxMesh(unsigned int& VAO, unsigned int& VBO, int& vertCount)
{
    // Each face is 2 triangles = 6 vertices
    // Note: UVs are written so the texture appears "right side up" inside the cube
    float verts[] = {
        // ── TOP (sky) ──────────────────────────────────────────────
        -1, 1,-1,  0,1,   1, 1,-1,  1,1,   1, 1, 1,  1,0,
         1, 1, 1,  1,0,  -1, 1, 1,  0,0,  -1, 1,-1,  0,1,
         // ── FRONT (south wall, mountain) ──────────────────────────
         -1,-1, 1,  0,0,   1,-1, 1,  1,0,   1, 1, 1,  1,1,
          1, 1, 1,  1,1,  -1, 1, 1,  0,1,  -1,-1, 1,  0,0,
          // ── BACK ──────────────────────────────────────────────────
           1,-1,-1,  0,0,  -1,-1,-1,  1,0,  -1, 1,-1,  1,1,
          -1, 1,-1,  1,1,   1, 1,-1,  0,1,   1,-1,-1,  0,0,
          // ── LEFT ──────────────────────────────────────────────────
          -1,-1,-1,  0,0,  -1,-1, 1,  1,0,  -1, 1, 1,  1,1,
          -1, 1, 1,  1,1,  -1, 1,-1,  0,1,  -1,-1,-1,  0,0,
          // ── RIGHT ─────────────────────────────────────────────────
           1,-1, 1,  0,0,   1,-1,-1,  1,0,   1, 1,-1,  1,1,
           1, 1,-1,  1,1,   1, 1, 1,  0,1,   1,-1, 1,  0,0,
           // ── BOTTOM (ground/grass) ─────────────────────────────────
           -1,-1,-1,  0,1,   1,-1,-1,  1,1,   1,-1, 1,  1,0,
            1,-1, 1,  1,0,  -1,-1, 1,  0,0,  -1,-1,-1,  0,1,
    };
    vertCount = 36;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── GROUND PLANE ──────────────────────────────────────────────────────────────
// A large flat quad (Y = 0) tiled with the grass texture.
void buildGroundMesh(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO,
    int& indexCount)
{
    float half = 50.0f;   // extends ±50 units
    float tile = 20.0f;  // UV tiles so the grass pattern repeats nicely

    float verts[] = {
        // x       y    z       u      v
        -half,  0.0f, -half,  0.0f,   tile,
         half,  0.0f, -half,  tile,   tile,
         half,  0.0f,  half,  tile,   0.0f,
        -half,  0.0f,  half,  0.0f,   0.0f,
    };
    unsigned int indices[] = { 0,1,2,  2,3,0 };
    indexCount = 6;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── TERRAIN GRID ──────────────────────────────────────────────────────────────
// Generates a gridX × gridZ quad mesh with sine-wave height variation.
// Vertices are colored green→brown based on height (low=green, high=brown/snow).
void buildTerrainMesh(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO,
    int& indexCount, int gridX, int gridZ)
{
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    float cellSize = 0.5f;  // world-space size of each grid cell

    auto heightAt = [](float x, float z) -> float {
        // Layered sine waves produce rolling hills
        return  0.6f * sinf(x * 0.8f) * cosf(z * 0.6f)
            + 0.3f * sinf(x * 1.7f + 0.5f) * sinf(z * 1.4f)
            + 0.2f * cosf(x * 3.1f) * sinf(z * 2.9f + 1.0f);
        };

    // Build vertices
    for (int iz = 0; iz <= gridZ; iz++) {
        for (int ix = 0; ix <= gridX; ix++) {
            float wx = ix * cellSize;
            float wz = iz * cellSize;
            float wy = heightAt(wx, wz);

            // Color based on height: low=dark green, mid=light green, high=brown
            float t = (wy + 0.8f) / 1.6f; // normalise roughly to [0,1]
            t = std::max(0.0f, std::min(1.0f, t));
            float r, g, b;
            if (t < 0.5f) {
                // dark green → light green
                float s = t / 0.5f;
                r = 0.10f + 0.10f * s;
                g = 0.40f + 0.20f * s;
                b = 0.05f;
            }
            else {
                // light green → sandy brown
                float s = (t - 0.5f) / 0.5f;
                r = 0.20f + 0.45f * s;
                g = 0.60f - 0.20f * s;
                b = 0.05f + 0.10f * s;
            }

            // Position (x, y, z)
            vertices.push_back(wx);
            vertices.push_back(wy);
            vertices.push_back(wz);
            // Color (r, g, b)
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
        }
    }

    // Build indices (two triangles per quad)
    for (int iz = 0; iz < gridZ; iz++) {
        for (int ix = 0; ix < gridX; ix++) {
            unsigned int tl = iz * (gridX + 1) + ix;
            unsigned int tr = tl + 1;
            unsigned int bl = tl + (gridX + 1);
            unsigned int br = bl + 1;
            // Triangle 1
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            // Triangle 2
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }
    indexCount = (int)indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    // Position (location 0) – 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color (location 1) – 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}