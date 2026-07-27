// glad must come before any header that may pull in the system OpenGL headers.
#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "fps_camera.h"
#include "game_world.h"
#include "net/remote_player.h"
#include "player_controller.h"
#include "renderer.h"
#include "weapon_system.h"
#include "weapon_types.h"

#include <iostream>

namespace {

// Cube with per-vertex colours, used for both targets and the floor proxy.
constexpr float kCubeVertices[] = {
    // positions             // colours
    -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, 0.5f,  -0.5f, 1.0f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 0.0f, 1.0f, 1.0f,
    0.5f,  -0.5f, -0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -0.5f, -0.5f, 0.5f, 0.0f, 1.0f};

constexpr unsigned int kCubeIndices[] = {
    0, 1, 2, 2, 3, 0, // front
    1, 5, 6, 6, 2, 1, // right
    5, 4, 7, 7, 6, 5, // back
    4, 0, 3, 3, 7, 4, // left
    4, 5, 1, 1, 0, 4, // top
    3, 2, 6, 6, 7, 3  // bottom
};

constexpr float kFloorVertices[] = {
    // positions           // colours (grey)
    -50.0f, 0.0f, -50.0f, 0.55f, 0.55f, 0.60f, 50.0f,  0.0f, -50.0f, 0.55f, 0.55f, 0.60f,
    50.0f,  0.0f, 50.0f,  0.55f, 0.55f, 0.60f, -50.0f, 0.0f, 50.0f,  0.55f, 0.55f, 0.60f};

constexpr unsigned int kFloorIndices[] = {0, 1, 2, 2, 3, 0};

constexpr float kCrosshairVertices[]   = {
    // horizontal line
    -0.02f, 0.00f, 0.0f, 1.0f, 1.0f, 1.0f, 0.02f, 0.00f, 0.0f, 1.0f, 1.0f, 1.0f,
    // vertical line
    0.00f, -0.02f, 0.0f, 1.0f, 1.0f, 1.0f, 0.00f, 0.02f, 0.0f, 1.0f, 1.0f, 1.0f};

void setupColouredVertexLayout() {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // OpenGL wants an intptr-encoded offset here — the classic
    // vertex-attrib-pointer idiom. `reinterpret_cast` silences the
    // clang-tidy warnings that we're going int → void*.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

} // namespace

Renderer::Renderer() = default;

Renderer::~Renderer() {
    // Best-effort GL resource cleanup. Safe even if init failed.
    if (m_FloorEBO) glDeleteBuffers(1, &m_FloorEBO);
    if (m_FloorVBO) glDeleteBuffers(1, &m_FloorVBO);
    if (m_FloorVAO) glDeleteVertexArrays(1, &m_FloorVAO);

    if (m_CubeEBO) glDeleteBuffers(1, &m_CubeEBO);
    if (m_CubeVBO) glDeleteBuffers(1, &m_CubeVBO);
    if (m_CubeVAO) glDeleteVertexArrays(1, &m_CubeVAO);

    if (m_CrossVBO) glDeleteBuffers(1, &m_CrossVBO);
    if (m_CrossVAO) glDeleteVertexArrays(1, &m_CrossVAO);
}

bool Renderer::initialize(int framebufferWidth, int framebufferHeight) {
    m_Width  = framebufferWidth;
    m_Height = framebufferHeight;

    glViewport(0, 0, m_Width, m_Height);
    glEnable(GL_DEPTH_TEST);

    try {
        m_Shader = std::make_unique<Shader>("shaders/basic.vert", "shaders/basic.frag");
    } catch (...) {
        std::cerr << "[Renderer] Failed to load shaders\n";
        return false;
    }

    buildFloor();
    buildCube();
    buildCrosshair();
    return true;
}

void Renderer::onResize(int width, int height) {
    m_Width  = width;
    m_Height = height;
    glViewport(0, 0, width, height);
}

void Renderer::buildFloor() {
    glGenVertexArrays(1, &m_FloorVAO);
    glGenBuffers(1, &m_FloorVBO);
    glGenBuffers(1, &m_FloorEBO);

    glBindVertexArray(m_FloorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_FloorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kFloorVertices), kFloorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FloorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kFloorIndices), kFloorIndices, GL_STATIC_DRAW);
    setupColouredVertexLayout();
    glBindVertexArray(0);
}

void Renderer::buildCube() {
    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);
    glGenBuffers(1, &m_CubeEBO);

    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kCubeIndices), kCubeIndices, GL_STATIC_DRAW);
    setupColouredVertexLayout();
    glBindVertexArray(0);
}

void Renderer::buildCrosshair() {
    glGenVertexArrays(1, &m_CrossVAO);
    glGenBuffers(1, &m_CrossVBO);

    glBindVertexArray(m_CrossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CrossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCrosshairVertices), kCrosshairVertices, GL_STATIC_DRAW);
    setupColouredVertexLayout();
    glBindVertexArray(0);
}

void Renderer::render(const FPSCamera& camera, const GameWorld& world, const WeaponSystem& weapons,
                      const PlayerController& /*player*/, float gameTime,
                      const Net::RemotePlayerRegistry* remotes) {
    glClearColor(0.05f, 0.10f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_Shader) return;
    m_Shader->use();

    // FOV with smooth ADS interpolation.
    float fov = 75.0f;
    if (weapons.isAiming() && weapons.getCurrentWeapon()) {
        const float adsFOV   = fov * weapons.getCurrentWeapon()->stats.adsFOVMultiplier;
        const float progress = weapons.getWeaponState().adsProgress;
        fov                  = glm::mix(fov, adsFOV, progress);
    }

    const float aspect         = (m_Height > 0) ? float(m_Width) / float(m_Height) : 16.0f / 9.0f;
    const glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 200.0f);
    const glm::mat4 view       = camera.getViewMatrix();

    m_Shader->setMat4("projection", projection);
    m_Shader->setMat4("view", view);

    drawFloor(view, projection);
    drawTargets(world, view, projection, gameTime);
    if (remotes) {
        drawRemotePlayers(*remotes, view, projection, gameTime);
    }
    drawCrosshair();
}

void Renderer::drawFloor(const glm::mat4& /*view*/, const glm::mat4& /*proj*/) {
    m_Shader->setMat4("model", glm::mat4(1.0f));
    glBindVertexArray(m_FloorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Renderer::drawTargets(const GameWorld& world, const glm::mat4& /*view*/,
                           const glm::mat4& /*proj*/, float gameTime) {
    glBindVertexArray(m_CubeVAO);
    for (const Target& t : world.targets()) {
        if (!t.alive) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model           = glm::translate(model, t.position);
        model           = glm::rotate(model, gameTime * t.spinSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
        model           = glm::scale(model, glm::vec3(t.scale));

        m_Shader->setMat4("model", model);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Renderer::drawRemotePlayers(const Net::RemotePlayerRegistry& remotes,
                                 const glm::mat4& /*view*/, const glm::mat4& /*proj*/,
                                 float gameTime) {
    // Remote players are drawn as upright cubes at the interpolated server
    // position. Phase 1.6 is intentionally placeholder geometry — proper
    // character models land in the rendering phase.
    glBindVertexArray(m_CubeVAO);
    for (const auto& [id, player] : remotes.players()) {
        glm::vec3 pos;
        float yaw = 0.0f, pitch = 0.0f;
        uint8_t flags = 0;
        if (!player.sample(static_cast<double>(gameTime), pos, yaw, pitch, flags)) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model           = glm::translate(model, pos);
        model           = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model           = glm::scale(model, glm::vec3(0.8f, 1.8f, 0.4f));

        m_Shader->setMat4("model", model);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Renderer::drawCrosshair() {
    // Render crosshair in NDC, ignoring depth and the active view/projection.
    glDisable(GL_DEPTH_TEST);

    const glm::mat4 identity(1.0f);
    m_Shader->setMat4("projection", identity);
    m_Shader->setMat4("view", identity);
    m_Shader->setMat4("model", identity);

    glBindVertexArray(m_CrossVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}
