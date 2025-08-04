#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class FPSCamera {
public:
    FPSCamera(glm::vec3 position);

    void processMouseMovement(float xoffset, float yoffset);
    void setPosition(const glm::vec3& pos);
    glm::vec3 getForward() const;
    glm::mat4 getViewMatrix() const;

private:
    glm::vec3 m_Position;
    float m_Yaw, m_Pitch;
    glm::vec3 m_Forward, m_Right, m_Up;

    void updateVectors();
};
