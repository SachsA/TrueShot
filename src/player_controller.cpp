#include "player_controller.h"
#include "fps_camera.h"
#include <algorithm>
#include <iostream>
#include <cmath>

PlayerController::PlayerController(FPSCamera* camera)
    : m_Camera(camera) {
    m_State.position = glm::vec3(0.0f, Physics::PLAYER_HEIGHT, 3.0f);
    m_Camera->setPosition(m_State.position);
}

void PlayerController::update(float deltaTime) {
    // Update input timing
    updateInputTiming(deltaTime);
    
    // Fixed timestep physics
    m_TimeAccumulator += deltaTime;
    
    while (m_TimeAccumulator >= Physics::FIXED_TIMESTEP) {
        updatePhysics(Physics::FIXED_TIMESTEP);
        m_TimeAccumulator -= Physics::FIXED_TIMESTEP;
    }
    
    m_Camera->setPosition(m_State.position);
    m_Input.reset();
}

void PlayerController::processInput(GLFWwindow* window, float deltaTime) {
    // Store previous jump state
    bool wasJumping = m_Input.jump;
    
    // Movement input (WASD)
    m_Input.moveInput = glm::vec2(0.0f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_Input.moveInput.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_Input.moveInput.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_Input.moveInput.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_Input.moveInput.x += 1.0f;
    
    // Normalize diagonal movement
    if (glm::length(m_Input.moveInput) > 1.0f) {
        m_Input.moveInput = glm::normalize(m_Input.moveInput);
    }
    
    // Jump input avec timing
    bool jumpCurrently = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    m_Input.jumpPressed = jumpCurrently && !wasJumping;
    m_Input.jumpReleased = !jumpCurrently && wasJumping;
    m_Input.jump = jumpCurrently;
    
    if (m_Input.jumpPressed) {
        m_State.wishJump = true;
    }
}

void PlayerController::processMouseInput(float xOffset, float yOffset) {
    m_Input.mouseInput = glm::vec2(xOffset, yOffset);
    
    // Smooth mouse delta pour strafe efficiency calculation
    m_Input.mouseDelta = m_Input.mouseDelta * 0.8f + m_Input.mouseInput * 0.2f;
    
    if (m_Camera) {
        m_Camera->processMouseMovement(xOffset, yOffset);
    }
}

void PlayerController::updateInputTiming(float deltaTime) {
    if (m_Input.jump) {
        m_Input.jumpHoldTime += deltaTime;
    } else {
        m_Input.jumpHoldTime = 0.0f;
    }
}

void PlayerController::updatePhysics(float deltaTime) {
    // Store previous state
    m_State.previousVelocity = m_State.velocity;
    m_State.wasOnGround = m_State.onGround;
    
    // Update ground state
    updateGroundState();
    
    // Handle jump avec timing optimisé
    if (m_State.wishJump) {
        handleBunnyHop();
        m_State.wishJump = false;
    }
    
    // Apply movement
    if (m_State.onGround) {
        handleGroundMovement(deltaTime);
    } else {
        handleAirMovement(deltaTime);
        m_State.airTime += deltaTime;
    }
    
    // Reset air time when landing
    if (!m_State.wasOnGround && m_State.onGround) {
        m_State.airTime = 0.0f;
    }
    
    applyGravity(deltaTime);
    applyMovement(deltaTime);
    
    // Update performance metrics
    glm::vec3 horizontalVel = glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z);
    m_State.speed = glm::length(horizontalVel);
    m_State.maxSpeed = std::max(m_State.maxSpeed, m_State.speed);
    m_State.strafeEfficiency = calculateStrafeEfficiency();
}

void PlayerController::handleAirMovement(float deltaTime) {
    glm::vec3 wishDir = calculateWishDirection();
    
    if (glm::length(wishDir) > 0.0f) {
        optimizeAirMovement(wishDir, deltaTime);
    }
    
    // Air friction minimal
    glm::vec3 horizontalVel = glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z);
    horizontalVel *= (1.0f - Physics::AIR_FRICTION * deltaTime);
    m_State.velocity.x = horizontalVel.x;
    m_State.velocity.z = horizontalVel.z;
}

void PlayerController::optimizeAirMovement(const glm::vec3& wishDir, float deltaTime) {
    // Calcul de l'angle entre wish direction et velocity
    glm::vec3 horizontalVel = glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z);
    
    if (glm::length(horizontalVel) < 1.0f) {
        // Si on a pas de vitesse, acceleration normale
        accelerate(wishDir, Physics::AIR_MAX_SPEED, Physics::AIR_ACCELERATION, deltaTime);
        return;
    }
    
    glm::vec3 velDir = glm::normalize(horizontalVel);
    float angle = glm::degrees(acos(glm::clamp(glm::dot(wishDir, velDir), -1.0f, 1.0f)));
    
    // Optimal strafe jumping: 30-45 degrees
    if (angle >= 20.0f && angle <= 60.0f) {
        // Bonus acceleration pour bon angle
        float angleFactor = 1.0f - abs(angle - Physics::OPTIMAL_STRAFE_ANGLE) / 30.0f;
        angleFactor = std::max(0.5f, angleFactor);
        
        float effectiveAccel = Physics::AIR_ACCELERATION * (1.0f + angleFactor * 0.5f);
        accelerate(wishDir, Physics::AIR_MAX_SPEED, effectiveAccel, deltaTime);
    } else {
        // Acceleration normale
        accelerate(wishDir, Physics::AIR_MAX_SPEED, Physics::AIR_ACCELERATION, deltaTime);
    }
    
    // Cap absolu pour éviter les bugs
    float currentSpeed = glm::length(glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z));
    if (currentSpeed > Physics::MAX_AIR_SPEED_CAP) {
        float factor = Physics::MAX_AIR_SPEED_CAP / currentSpeed;
        m_State.velocity.x *= factor;
        m_State.velocity.z *= factor;
    }
}

void PlayerController::handleBunnyHop() {
    if (m_State.onGround) {
        // Perfect bhop: jump immediately
        m_State.velocity.y = Physics::JUMP_IMPULSE;
        m_State.onGround = false;
        m_State.consecutiveHops++;
        
        std::cout << "Bhop #" << m_State.consecutiveHops 
                  << " | Speed: " << m_State.speed 
                  << " | Efficiency: " << (m_State.strafeEfficiency * 100.0f) << "%" << std::endl;
    } else if (m_State.airTime < 0.1f) {
        // Pre-speed: jump buffering
        m_State.wishJump = true; // Keep trying
    }
}

float PlayerController::calculateStrafeEfficiency() const {
    if (glm::length(m_Input.moveInput) < 0.1f || glm::length(m_Input.mouseDelta) < 0.1f) {
        return 0.0f;
    }
    
    // Efficiency based on mouse movement vs keyboard input sync
    glm::vec3 horizontalVel = glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z);
    if (glm::length(horizontalVel) < 10.0f) return 0.0f;
    
    // Simplified efficiency calculation
    float mouseSpeed = glm::length(m_Input.mouseDelta);
    float keyboardInput = glm::length(m_Input.moveInput);
    
    if (mouseSpeed > 0.1f && keyboardInput > 0.1f) {
        return std::min(1.0f, (mouseSpeed * keyboardInput) / 10.0f);
    }
    
    return 0.0f;
}

void PlayerController::handleGroundMovement(float deltaTime) {
    glm::vec3 wishDir = calculateWishDirection();
    float wishSpeed = Physics::MAX_GROUND_SPEED;
    
    // Reset consecutive hops if we stay on ground too long
    if (m_State.wasOnGround && m_State.onGround) {
        static float groundTime = 0.0f;
        groundTime += deltaTime;
        if (groundTime > 0.2f) { // 200ms tolerance
            m_State.consecutiveHops = 0;
            groundTime = 0.0f;
        }
    }
    
    // Apply friction si pas de mouvement
    if (glm::length(m_Input.moveInput) < 0.1f) {
        applyFriction(deltaTime);
    }
    
    // Accelerate
    if (glm::length(wishDir) > 0.0f) {
        accelerate(wishDir, wishSpeed, Physics::GROUND_ACCELERATION, deltaTime);
    }
}

glm::vec3 PlayerController::calculateWishDirection() const {
    if (glm::length(m_Input.moveInput) < 0.1f) {
        return glm::vec3(0.0f);
    }
    
    glm::vec3 forward = m_Camera->getForward();
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    
    glm::vec3 wishDir = forward * m_Input.moveInput.y + right * m_Input.moveInput.x;
    
    if (glm::length(wishDir) > 0.0f) {
        wishDir = glm::normalize(wishDir);
    }
    
    return wishDir;
}

void PlayerController::accelerate(const glm::vec3& wishDir, float wishSpeed, float acceleration, float deltaTime) {
    float currentSpeed = glm::dot(m_State.velocity, wishDir);
    float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;
    
    float accelSpeed = acceleration * wishSpeed * deltaTime;
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }
    
    m_State.velocity += wishDir * accelSpeed;
}

void PlayerController::applyFriction(float deltaTime) {
    glm::vec3 horizontalVel = glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z);
    float speed = glm::length(horizontalVel);
    
    if (speed < 0.1f) {
        m_State.velocity.x = 0.0f;
        m_State.velocity.z = 0.0f;
        return;
    }
    
    float friction = Physics::GROUND_FRICTION * m_State.surfaceFriction;
    float control = std::max(speed, Physics::GROUND_FRICTION);
    float drop = control * friction * deltaTime;
    
    float newSpeed = std::max(0.0f, speed - drop);
    if (newSpeed != speed) {
        newSpeed /= speed;
        m_State.velocity.x *= newSpeed;
        m_State.velocity.z *= newSpeed;
    }
}

void PlayerController::applyGravity(float deltaTime) {
    if (!m_State.onGround) {
        m_State.velocity.y -= Physics::GRAVITY * deltaTime;
    }
}

void PlayerController::updateGroundState() {
    m_State.onGround = checkGroundCollision(m_State.position);
    
    if (m_State.onGround && m_State.velocity.y <= 0.0f) {
        m_State.velocity.y = 0.0f;
        m_State.position.y = Physics::PLAYER_HEIGHT;
    }
}

void PlayerController::applyMovement(float deltaTime) {
    glm::vec3 newPosition = m_State.position + m_State.velocity * deltaTime;
    newPosition = resolveCollisions(newPosition);
    m_State.position = newPosition;
}

bool PlayerController::checkGroundCollision(const glm::vec3& position) const {
    return position.y <= Physics::PLAYER_HEIGHT + Physics::GROUND_TOLERANCE;
}

glm::vec3 PlayerController::resolveCollisions(const glm::vec3& position) {
    glm::vec3 resolvedPos = position;
    m_State.hitWall = false;
    
    // Ground collision
    if (resolvedPos.y < Physics::PLAYER_HEIGHT) {
        resolvedPos.y = Physics::PLAYER_HEIGHT;
    }
    
    // Wall collision avec bounce
    if (resolvedPos.x < -45.0f || resolvedPos.x > 45.0f || 
        resolvedPos.z < -45.0f || resolvedPos.z > 45.0f) {
        
        glm::vec3 wallNormal(0.0f);
        
        // Determine wall normal
        if (resolvedPos.x < -45.0f) {
            wallNormal = glm::vec3(1.0f, 0.0f, 0.0f);
            resolvedPos.x = -45.0f;
        } else if (resolvedPos.x > 45.0f) {
            wallNormal = glm::vec3(-1.0f, 0.0f, 0.0f);
            resolvedPos.x = 45.0f;
        }
        
        if (resolvedPos.z < -45.0f) {
            wallNormal += glm::vec3(0.0f, 0.0f, 1.0f);
            resolvedPos.z = -45.0f;
        } else if (resolvedPos.z > 45.0f) {
            wallNormal += glm::vec3(0.0f, 0.0f, -1.0f);
            resolvedPos.z = 45.0f;
        }
        
        if (glm::length(wallNormal) > 0.0f) {
            wallNormal = glm::normalize(wallNormal);
            handleWallCollision(wallNormal);
        }
    }
    
    return resolvedPos;
}

void PlayerController::handleWallCollision(const glm::vec3& wallNormal) {
    m_State.hitWall = true;
    m_State.wallNormal = wallNormal;
    
    // Bounce off wall si assez rapide
    float speed = glm::length(glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z));
    if (speed > Physics::MIN_WALL_SPEED) {
        glm::vec3 reflection = m_State.velocity - 2.0f * glm::dot(m_State.velocity, wallNormal) * wallNormal;
        m_State.velocity = reflection * Physics::WALL_BOUNCE_FACTOR;
        
        std::cout << "Wall bounce! Speed: " << speed << " → " << glm::length(glm::vec3(m_State.velocity.x, 0.0f, m_State.velocity.z)) << std::endl;
    }
}
