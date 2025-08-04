#pragma once

#include <glm/glm.hpp>

// Constantes physiques optimisées pour strafe jumping
namespace Physics {
    // Movement constants
    const float GRAVITY = 800.0f;                  // units/sec²
    const float JUMP_IMPULSE = 301.993377f;        // Exact CS value
    
    // Ground movement
    const float MAX_GROUND_SPEED = 250.0f;         // units/sec
    const float GROUND_ACCELERATION = 10.0f;
    const float GROUND_FRICTION = 4.0f;
    
    // Air movement (optimisé pour bunny hop)
    const float AIR_MAX_SPEED = 30.0f;             // Max speed from WASD in air
    const float AIR_ACCELERATION = 12.0f;          // Air strafe acceleration
    const float AIR_FRICTION = 0.25f;              // Minimal air friction
    
    // Strafe jumping optimizations
    const float OPTIMAL_STRAFE_ANGLE = 30.0f;      // Degrees - optimal mouse/movement angle
    const float MAX_AIR_SPEED_CAP = 3000.0f;       // Absolute maximum (pour éviter les bugs)
    const float BHOP_SPEED_LOSS = 0.95f;           // 5% loss on bad landing
    
    // Fixed timestep pour consistency
    const float TICK_RATE = 64.0f;                 // 64 tick/sec
    const float FIXED_TIMESTEP = 1.0f / TICK_RATE;
    
    // Ground detection améliorée
    const float GROUND_TRACE_DISTANCE = 2.0f;      
    const float PLAYER_HEIGHT = 1.8f;
    const float PLAYER_RADIUS = 0.3f;
    const float GROUND_TOLERANCE = 0.1f;           // Tolerance pour "on ground"
    
    // Collision improvements
    const float WALL_BOUNCE_FACTOR = 0.8f;         // Bounce off walls
    const float MIN_WALL_SPEED = 50.0f;            // Min speed to bounce
}

struct MovementState {
    glm::vec3 position{0.0f, Physics::PLAYER_HEIGHT, 0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 previousVelocity{0.0f};   // Pour détecter les changements
    
    bool onGround = false;
    bool wishJump = false;
    bool wasOnGround = false;           // Previous frame ground state
    
    float surfaceFriction = 1.0f;
    float airTime = 0.0f;               // Time spent in air (pour bhop timing)
    
    // Performance metrics
    float speed = 0.0f;                 // Current horizontal speed
    float maxSpeed = 0.0f;              // Max speed reached
    float strafeEfficiency = 0.0f;      // How good is current strafe (0-1)
    int consecutiveHops = 0;            // Bhop combo counter
    
    // Collision info
    bool hitWall = false;
    glm::vec3 wallNormal{0.0f};
};

struct MovementInput {
    glm::vec2 moveInput{0.0f};      // WASD normalized (-1 to 1)
    glm::vec2 mouseInput{0.0f};     // Mouse delta this frame
    glm::vec2 mouseDelta{0.0f};     // Smoothed mouse delta
    bool jump = false;
    bool crouch = false;
    
    // Input timing (important pour bhop)
    float jumpHoldTime = 0.0f;      // How long jump has been held
    bool jumpPressed = false;       // Jump pressed this frame
    bool jumpReleased = false;      // Jump released this frame
    
    void reset() {
        mouseInput = glm::vec2(0.0f);
        jumpPressed = false;
        jumpReleased = false;
    }
};
