#pragma once

#include "weapon_types.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <unordered_map>

class FPSCamera;
class PlayerController;
class AudioSystem;

class WeaponSystem {
public:
    WeaponSystem(FPSCamera* camera, PlayerController* player);
    ~WeaponSystem();

    // Main update loop
    void update(float deltaTime);
    
    // Input processing
    void processInput(GLFWwindow* window, float deltaTime);
    
    // Weapon management
    bool equipWeapon(const std::string& weaponName);
    void switchToWeapon(int slot);
    void dropWeapon();
    
    // Shooting mechanics
    bool canFire() const;
    void fire();
    HitResult performRaycast(const glm::vec3& origin, const glm::vec3& direction) const;
    float calculateDamage(const HitResult& hit) const;
    
    // Weapon states
    void startReload();
    void cancelReload();
    bool isReloading() const;
    void startADS();
    void stopADS();
    bool isAiming() const;
    
    // Getters
    const Weapons::WeaponConfig* getCurrentWeapon() const;
    const WeaponState& getWeaponState() const { return m_WeaponState; }
    glm::vec2 getCurrentSpread() const;
    glm::vec2 getCurrentRecoil() const { return m_WeaponState.currentRecoil; }

    // Audio integration
    void setAudioSystem(AudioSystem* audioSystem) { m_AudioSystem = audioSystem; }
    
    // Debug info
    void printDebugInfo() const;

private:
    // Core systems
    void updateWeaponState(float deltaTime);
    void updateRecoil(float deltaTime);
    void updateSpread(float deltaTime);
    void updateADS(float deltaTime);
    void applyViewPunch();
    
    // Recoil system
    void addRecoil();
    void resetRecoil();
    glm::vec2 getRecoilPatternPoint(int shotIndex) const;
    void applyRecoilToCamera();
    
    // Accuracy system
    float calculateCurrentSpread() const;
    glm::vec3 applySpreadToDirection(const glm::vec3& baseDirection) const;
    
    // Animation system
    void updateWeaponSway(float deltaTime);
    void updateWeaponBob(float deltaTime);
    glm::vec3 calculateWeaponPosition() const;
    
    // State management
    void changeWeaponState(Weapons::WeaponState newState);
    void updateStateMachine(float deltaTime);
    
    // Input helpers
    void updateInputTiming(float deltaTime);

private:
    // External references
    FPSCamera* m_Camera;
    PlayerController* m_Player;
    // Optional audio system for sound effects
    AudioSystem* m_AudioSystem = nullptr;
    
    // Current weapon
    std::unique_ptr<Weapons::WeaponConfig> m_CurrentWeapon;
    WeaponState m_WeaponState;
    ShootingInput m_Input;
    
    // Weapon database
    std::unordered_map<std::string, std::unique_ptr<Weapons::WeaponConfig>> m_WeaponConfigs;
    
    // Timing
    float m_GameTime = 0.0f;
    
    // Sway & bob
    glm::vec3 m_BaseWeaponPosition{0.5f, -0.3f, 0.8f}; // Relative to camera
    glm::vec2 m_SwayAmount{0.0f};
    float m_BobTime = 0.0f;
    
    // View punch (screen shake from recoil)
    glm::vec2 m_ViewPunch{0.0f};
    glm::vec2 m_ViewPunchVelocity{0.0f};
    
    // Performance tracking
    mutable float m_DebugTimer = 0.0f;
    mutable int m_ShotsFiredThisSecond = 0;
    mutable float m_AverageSpread = 0.0f;
};

// Weapon factory for creating predefined weapons
class WeaponFactory {
public:
    static std::unique_ptr<Weapons::WeaponConfig> createAK47();
    static std::unique_ptr<Weapons::WeaponConfig> createM4A4();
    static std::unique_ptr<Weapons::WeaponConfig> createAWP();
    static std::unique_ptr<Weapons::WeaponConfig> createGlock();
    static std::unique_ptr<Weapons::WeaponConfig> createDeagle();
    
private:
    // Recoil pattern generators
    static std::vector<Weapons::RecoilPoint> generateAK47Pattern();
    static std::vector<Weapons::RecoilPoint> generateM4A4Pattern();
    static std::vector<Weapons::RecoilPoint> generateControlledPattern(float verticalStrength, float horizontalVariation, int patternLength);
};
