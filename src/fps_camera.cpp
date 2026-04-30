#include "fps_camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

FPSCamera::FPSCamera(const glm::vec3& position)
    : m_Position(position)
    , m_Yaw(-90.0f)
    , m_Pitch(0.0f) {
    updateVectors();
}

void FPSCamera::setPosition(const glm::vec3& pos) {
    m_Position = pos;
}

void FPSCamera::processMouseMovement(float xoffset, float yoffset) {
    m_Yaw   += xoffset * m_Sensitivity;
    m_Pitch += yoffset * m_Sensitivity;

    if (m_Pitch >  89.0f) m_Pitch =  89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    updateVectors();
}

glm::mat4 FPSCamera::getViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void FPSCamera::updateVectors() {
    glm::vec3 forward;
    forward.x = std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
    forward.y = std::sin(glm::radians(m_Pitch));
    forward.z = std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));

    m_Forward = glm::normalize(forward);
    m_Right   = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_Up      = glm::normalize(glm::cross(m_Right,   m_Forward));
}
