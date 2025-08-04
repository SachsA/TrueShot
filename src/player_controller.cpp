#include "player_controller.h"
#include "fps_camera.h"

PlayerController::PlayerController(FPSCamera* camera)
    : m_Position(0.0f, 1.8f, 3.0f), m_Speed(5.0f), m_Camera(camera) {}

void PlayerController::processInput(GLFWwindow* window, float deltaTime) {
    glm::vec3 forward = m_Camera->getForward();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        m_Position += forward * m_Speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_Position -= forward * m_Speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_Position -= right * m_Speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_Position += right * m_Speed * deltaTime;

    m_Camera->setPosition(m_Position);
}
