#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Weapons {
    // Weapon categories
    enum class WeaponType {
        RIFLE,      // AK, M4, etc.
        SMG,        // MP9, MAC10, etc.
        SNIPER,     // AWP, Scout, etc.
        PISTOL,     // Glock, USP, Deagle, etc.
        SHOTGUN,    // Nova, XM1014, etc.
        LMG         // Negev, M249, etc.
    };

    // Firing modes
    enum class FireMode {
        SEMI_AUTO,      // One shot per click
        FULL_AUTO,      // Hold to spray
        BURST,          // 3-round burst
        BOLT_ACTION     // Manual cycling
    };

    // Weapon states
    enum class WeaponState {
        IDLE,
        FIRING,
        RELOADING,
        DRAWING,
        HOLSTERING,
        INSPECTING
    };

    // Recoil pattern point
    struct RecoilPoint {
        glm::vec2 offset;       // X = horizontal, Y = vertical
        float timeOffset;       // When this point is reached
        float resetSpeed;       // How fast to return to center
    };

    // Weapon statistics
    struct WeaponStats {
        // Damage
        float baseDamage = 30.0f;           // Base damage at optimal range
        float headshotMultiplier = 4.0f;    // Headshot damage multiplier
        float chestMultiplier = 1.0f;       // Chest damage multiplier
        float limbMultiplier = 0.75f;       // Arms/legs damage multiplier
        
        // Range & falloff
        float optimalRange = 30.0f;         // Range where damage is 100%
        float maxRange = 100.0f;            // Range where damage becomes minimal
        float minDamagePercent = 0.2f;      // Minimum damage % at max range
        
        // Accuracy
        float baseSpread = 0.1f;            // Base inaccuracy (degrees)
        float movingSpread = 0.3f;          // Additional spread when moving
        float jumpingSpread = 1.0f;         // Additional spread when airborne
        float crouchingSpread = -0.05f;     // Spread reduction when crouching
        
        // Recoil
        float recoilMagnitude = 1.0f;       // Overall recoil strength
        float recoilRecovery = 8.0f;        // How fast recoil recovers
        float recoilRandomness = 0.1f;      // Random variation in recoil
        
        // Fire rate
        float fireRate = 600.0f;            // Rounds per minute
        FireMode fireMode = FireMode::FULL_AUTO;
        
        // Ammo
        int magazineSize = 30;
        int reserveAmmo = 90;
        float reloadTime = 2.5f;            // Full reload time
        float tacticalReloadTime = 2.0f;    // Reload with bullet in chamber
        
        // Movement
        float movementSpeedMultiplier = 0.9f; // Speed when holding this weapon
        float adsSpeedMultiplier = 0.3f;    // Speed when aiming down sights
        
        // ADS (Aim Down Sights)
        float adsTime = 0.25f;              // Time to fully ADS
        float adsSpreadReduction = 0.7f;    // Spread reduction when ADS
        float adsFOVMultiplier = 0.6f;      // FOV zoom when ADS
    };

    // Weapon configuration
    struct WeaponConfig {
        std::string name;
        WeaponType type;
        WeaponStats stats;
        std::vector<RecoilPoint> recoilPattern;
        
        // Audio
        std::string fireSound;
        std::string reloadSound;
        std::string drawSound;
        
        // Visual
        std::string viewModel;
        std::string worldModel;
        glm::vec3 muzzleOffset = glm::vec3(0.0f, 0.0f, 1.0f);
    };
}

// Weapon state for runtime
struct WeaponState {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 velocity{0.0f};
    
    // Ammo state
    int currentAmmo = 0;
    int reserveAmmo = 0;
    bool chamberedRound = false;
    
    // Timing
    float lastFireTime = 0.0f;
    float reloadStartTime = 0.0f;
    float drawStartTime = 0.0f;
    
    // Recoil state
    glm::vec2 currentRecoil{0.0f};      // Current recoil offset
    glm::vec2 targetRecoil{0.0f};       // Target recoil from pattern
    int shotsFired = 0;                 // Shots in current burst
    float lastShotTime = 0.0f;
    
    // ADS state
    bool isAiming = false;
    float adsProgress = 0.0f;           // 0.0 = hip fire, 1.0 = fully ADS
    
    // State machine
    Weapons::WeaponState state = Weapons::WeaponState::IDLE;
    float stateTimer = 0.0f;
};

// Hit result information
struct HitResult {
    bool hit = false;
    glm::vec3 hitPoint{0.0f};
    glm::vec3 hitNormal{0.0f};
    float distance = 0.0f;
    float damage = 0.0f;
    
    // Hit location for damage calculation
    enum HitLocation {
        HEAD,
        CHEST,
        STOMACH,
        ARM_LEFT,
        ARM_RIGHT,
        LEG_LEFT,
        LEG_RIGHT
    } hitLocation = CHEST;
    
    // Target info (for multiplayer)
    int targetId = -1;
    bool isHeadshot = false;
};

// Shooting input state
struct ShootingInput {
    bool primaryFire = false;       // Mouse1 / trigger
    bool secondaryFire = false;     // Mouse2 / ADS
    bool reload = false;            // R key
    bool inspect = false;           // F key
    
    // Input timing
    bool primaryPressed = false;    // Pressed this frame
    bool primaryReleased = false;   // Released this frame
    bool reloadPressed = false;     // Reload pressed this frame
    
    void reset() {
        primaryPressed = false;
        primaryReleased = false;
        reloadPressed = false;
    }
};
