#include "fps_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

FPSCamera::FPSCamera(glm::vec3 position)
    : m_Position(position), m_Yaw(-90.0f), m_Pitch(0.0f)
{
    updateVectors();
}

void FPSCamera::setPosition(const glm::vec3& pos) {
    m_Position = pos;
}

void FPSCamera::processMouseMovement(float xoffset, float yoffset) {
    float sensitivity = 0.1f;
    m_Yaw += xoffset * sensitivity;
    m_Pitch += yoffset * sensitivity;

    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    updateVectors();
}

glm::vec3 FPSCamera::getForward() const {
    return m_Forward;
}

glm::mat4 FPSCamera::getViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void FPSCamera::updateVectors() {
    glm::vec3 forward;
    forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    forward.y = sin(glm::radians(m_Pitch));
    forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Forward = glm::normalize(forward);
    m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0,1,0)));
    m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
}
