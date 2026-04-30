#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Free-look first-person camera. Owns yaw/pitch and a cached
// orthonormal basis (forward/right/up).
class FPSCamera {
public:
    explicit FPSCamera(const glm::vec3& position);

    // Mouse delta -> yaw/pitch update. Pitch is clamped to ±89°.
    void processMouseMovement(float xoffset, float yoffset);

    void setPosition(const glm::vec3& pos);
    void setSensitivity(float s)  { m_Sensitivity = s; }

    glm::vec3 getPosition()   const { return m_Position; }
    glm::vec3 getForward()    const { return m_Forward; }
    glm::vec3 getRight()      const { return m_Right;   }
    glm::vec3 getUp()         const { return m_Up;      }
    float     getYaw()        const { return m_Yaw;     }
    float     getPitch()      const { return m_Pitch;   }
    glm::mat4 getViewMatrix() const;

private:
    void updateVectors();

    glm::vec3 m_Position;
    float     m_Yaw;
    float     m_Pitch;
    float     m_Sensitivity = 0.1f;

    glm::vec3 m_Forward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Right  {1.0f, 0.0f,  0.0f};
    glm::vec3 m_Up     {0.0f, 1.0f,  0.0f};
};
