#pragma once

#include <glm/glm.hpp>

#include "shader.h"

#include <memory>

class FPSCamera;
class GameWorld;
class WeaponSystem;
class PlayerController;

// Owns all OpenGL resources (VAO/VBO/EBO + shader) and renders
// the scene each frame. Knows nothing about input, audio, or physics.
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // One-time GL setup. Returns false on failure.
    bool initialize(int framebufferWidth, int framebufferHeight);

    // Resize the GL viewport to match the new framebuffer.
    void onResize(int width, int height);

    // Draw a frame. The renderer pulls camera/world state from
    // its arguments — it never stores them long-term.
    void render(const FPSCamera& camera, const GameWorld& world, const WeaponSystem& weapons,
                const PlayerController& player, float gameTime);

    int width() const { return m_Width; }
    int height() const { return m_Height; }

private:
    void buildFloor();
    void buildCube();
    void buildCrosshair();

    void drawFloor(const glm::mat4& view, const glm::mat4& proj);
    void drawTargets(const GameWorld& world, const glm::mat4& view, const glm::mat4& proj,
                     float gameTime);
    void drawCrosshair();

    std::unique_ptr<Shader> m_Shader;

    unsigned int m_FloorVAO = 0, m_FloorVBO = 0, m_FloorEBO = 0;
    unsigned int m_CubeVAO = 0, m_CubeVBO = 0, m_CubeEBO = 0;
    unsigned int m_CrossVAO = 0, m_CrossVBO = 0;

    int m_Width  = 0;
    int m_Height = 0;
};
