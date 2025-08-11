#pragma once

#include "physics_types.h"
#include "audio_system.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class FPSCamera;

class PlayerController {
public:
    PlayerController(FPSCamera* camera);

    // Main update function - fixed timestep
    void update(float deltaTime);
    
    // Input processing
    void processInput(GLFWwindow* window, float deltaTime);
    void processMouseInput(float xOffset, float yOffset);

    // Getters
    glm::vec3 getPosition() const { return m_State.position; }
    glm::vec3 getVelocity() const { return m_State.velocity; }
    float getSpeed() const { return m_State.speed; }
    bool isOnGround() const { return m_State.onGround; }

    // Setters
    void setAudioSystem(AudioSystem* audioSystem) { m_AudioSystem = audioSystem; }
    
    // Debug info
    const MovementState& getMovementState() const { return m_State; }

private:
    // Core systems
    void updatePhysics(float deltaTime);
    void updateGroundState();
    void applyMovement(float deltaTime);
    
    // Movement types
    void handleGroundMovement(float deltaTime);
    void handleAirMovement(float deltaTime);
    void handleJump();
    
    // Physics helpers
    glm::vec3 calculateWishDirection() const;
    void accelerate(const glm::vec3& wishDir, float wishSpeed, float acceleration, float deltaTime);
    void applyFriction(float deltaTime);
    void applyGravity(float deltaTime);
    
    // Strafe jumping optimizations
    float calculateStrafeEfficiency() const;
    void optimizeAirMovement(const glm::vec3& wishDir, float deltaTime);
    void handleBunnyHop();
    
    // Collision améliorée
    bool checkGroundCollision(const glm::vec3& position) const;
    glm::vec3 resolveCollisions(const glm::vec3& position);
    void handleWallCollision(const glm::vec3& wallNormal);
    
    // Input processing amélioré
    void updateInputTiming(float deltaTime);

private:
    MovementState m_State;
    MovementInput m_Input;
    FPSCamera* m_Camera;
    AudioSystem* m_AudioSystem = nullptr;

    // Pour le timing des sauts et autres mécaniques
    float m_GameTime = 0.0f;

    // Footstep tracking
    float m_LastFootstepTime = 0.0f;
    glm::vec3 m_LastFootstepPos{0.0f};
    
    // Fixed timestep accumulator
    float m_TimeAccumulator = 0.0f;
};
