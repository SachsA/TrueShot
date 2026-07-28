#pragma once

#include <glm/glm.hpp>

// A simple AABB target the player can shoot at.
// Targets carry a hitbox, current HP, and a small respawn timer
// so the practice range is always populated.
struct Target {
    glm::vec3 position{0.0f};
    glm::vec3 halfExtents{0.5f}; // AABB half-size (used for hit detection)
    float spinSpeed = 0.5f;      // Rotation speed (visual only)
    float scale     = 1.0f;      // Visual scale

    // Gameplay state
    float maxHp        = 100.0f;
    float hp           = 100.0f;
    bool alive         = true;
    float respawnTimer = 0.0f; // Counts up while dead
    float respawnDelay = 2.5f; // Seconds before coming back

    // Score awarded when destroyed.
    int scoreValue = 100;

    // True if a ray (origin, normalized direction) intersects this target.
    // On hit, fills outDistance and outHitPoint (world-space).
    bool intersectRay(const glm::vec3& origin, const glm::vec3& dir, float maxDistance,
                      float& outDistance, glm::vec3& outHitPoint) const;
};
