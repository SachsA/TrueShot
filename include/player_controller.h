#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class FPSCamera;

class PlayerController {
public:
    PlayerController(FPSCamera* camera);

    void processInput(GLFWwindow* window, float deltaTime);

    glm::vec3 getPosition() const { return m_Position; }

private:
    glm::vec3 m_Position;
    float m_Speed;
    FPSCamera* m_Camera;
};
