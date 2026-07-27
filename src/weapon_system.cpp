#include "audio_system.h"
#include "fps_camera.h"
#include "game_world.h"
#include "hud.h"
#include "player_controller.h"
#include "weapon_system.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

WeaponSystem::WeaponSystem(FPSCamera* camera, PlayerController* player)
    : m_Camera(camera), m_Player(player) {
    // Initialize weapon database
    m_WeaponConfigs["ak47"]   = WeaponFactory::createAK47();
    m_WeaponConfigs["m4a4"]   = WeaponFactory::createM4A4();
    m_WeaponConfigs["awp"]    = WeaponFactory::createAWP();
    m_WeaponConfigs["glock"]  = WeaponFactory::createGlock();
    m_WeaponConfigs["deagle"] = WeaponFactory::createDeagle();

    // Start with AK47
    equipWeapon("ak47");

    std::cout << "WeaponSystem initialized with " << m_WeaponConfigs.size() << " weapons"
              << "\n";
}

WeaponSystem::~WeaponSystem() = default;

void WeaponSystem::update(float deltaTime) {
    m_GameTime += deltaTime;

    updateInputTiming(deltaTime);
    updateWeaponState(deltaTime);
    updateRecoil(deltaTime);
    updateADS(deltaTime);
    updateWeaponSway(deltaTime);
    updateWeaponBob(deltaTime);

    // View punch (screen shake) — driven by real frame time.
    {
        const float spring           = 15.0f;
        const float damping          = 8.0f;
        const glm::vec2 springForce  = -m_ViewPunch * spring;
        const glm::vec2 dampingForce = -m_ViewPunchVelocity * damping;
        m_ViewPunchVelocity += (springForce + dampingForce) * deltaTime;
        m_ViewPunch += m_ViewPunchVelocity * deltaTime;
    }

    // Reset per-frame input
    m_Input.reset();
}

void WeaponSystem::processInput(GLFWwindow* window, float deltaTime) {
    // Store previous states
    bool wasPrimaryFire = m_Input.primaryFire;
    bool wasReload      = m_Input.reload;

    // Primary fire (Mouse1)
    m_Input.primaryFire     = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    m_Input.primaryPressed  = m_Input.primaryFire && !wasPrimaryFire;
    m_Input.primaryReleased = !m_Input.primaryFire && wasPrimaryFire;

    // Secondary fire / ADS (Mouse2)
    bool currentSecondary = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (currentSecondary && !m_Input.secondaryFire) {
        startADS();
    } else if (!currentSecondary && m_Input.secondaryFire) {
        stopADS();
    }
    m_Input.secondaryFire = currentSecondary;

    // Reload (R key)
    m_Input.reload        = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    m_Input.reloadPressed = m_Input.reload && !wasReload;

    // Weapon switching (1-5 keys)
    for (int i = 1; i <= 5; ++i) {
        if (glfwGetKey(window, GLFW_KEY_0 + i) == GLFW_PRESS) {
            switchToWeapon(i);
            break;
        }
    }

    // Inspect (F key)
    m_Input.inspect = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;

    // Handle fire input
    if (m_Input.primaryPressed || (m_Input.primaryFire && canFire())) {
        fire();
    }

    // Handle reload input
    if (m_Input.reloadPressed && !isReloading()) {
        startReload();
    }
}

bool WeaponSystem::equipWeapon(const std::string& weaponName) {
    auto it = m_WeaponConfigs.find(weaponName);
    if (it == m_WeaponConfigs.end()) {
        std::cerr << "Weapon not found: " << weaponName << "\n";
        return false;
    }

    m_CurrentWeapon = std::make_unique<Weapons::WeaponConfig>(*it->second);

    // Reset weapon state
    m_WeaponState                = WeaponState{};
    m_WeaponState.currentAmmo    = m_CurrentWeapon->stats.magazineSize;
    m_WeaponState.reserveAmmo    = m_CurrentWeapon->stats.reserveAmmo;
    m_WeaponState.chamberedRound = true;

    changeWeaponState(Weapons::WeaponState::DRAWING);

    if (m_CurrentWeapon && m_AudioSystem) {
        m_AudioSystem->onWeaponDraw(m_CurrentWeapon->name, m_Player->getPosition());
    }

    std::cout << "Equipped: " << m_CurrentWeapon->name << " (" << m_WeaponState.currentAmmo << "/"
              << m_WeaponState.reserveAmmo << ")" << "\n";

    return true;
}

void WeaponSystem::switchToWeapon(int slot) {
    // Simple weapon switching by slot
    switch (slot) {
    case 1:
        equipWeapon("glock");
        break;
    case 2:
        equipWeapon("deagle");
        break;
    case 3:
        equipWeapon("ak47");
        break;
    case 4:
        equipWeapon("m4a4");
        break;
    case 5:
        equipWeapon("awp");
        break;
    default:
        break;
    }
}

bool WeaponSystem::canFire() const {
    if (!m_CurrentWeapon) return false;
    if (m_WeaponState.state != Weapons::WeaponState::IDLE) return false;
    if (m_WeaponState.currentAmmo <= 0) return false;

    // Check fire rate
    float timeSinceLastShot = m_GameTime - m_WeaponState.lastFireTime;
    float fireInterval      = 60.0f / m_CurrentWeapon->stats.fireRate;

    if (timeSinceLastShot < fireInterval) return false;

    // Check fire mode
    if (m_CurrentWeapon->stats.fireMode == Weapons::FireMode::SEMI_AUTO) {
        return m_Input.primaryPressed; // Only fire on press, not hold
    }

    return true;
}

void WeaponSystem::fire() {
    if (!canFire()) return;

    // Update timing
    m_WeaponState.lastFireTime = m_GameTime;
    m_WeaponState.lastShotTime = m_GameTime;
    m_WeaponState.shotsFired++;

    // Consume ammo
    m_WeaponState.currentAmmo--;
    if (m_WeaponState.currentAmmo == 0) {
        m_WeaponState.chamberedRound = false;
    }

    // Track shots fired for accuracy stats.
    if (m_World) m_World->registerShot();

    // Calculate shot origin (eye) and direction with spread.
    const glm::vec3 origin    = m_Camera ? m_Camera->getPosition() : glm::vec3(0.0f);
    const glm::vec3 forward   = m_Camera ? m_Camera->getForward() : glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 direction = applySpreadToDirection(forward);

    HitResult hit             = performRaycast(origin, direction);

    // Resolve damage against the real world.
    if (hit.hit && m_World) {
        // performRaycast() now uses the world directly, so the index it
        // resolved is encoded in hit.targetId.
        if (hit.targetId >= 0) {
            m_World->registerHit();
            const bool killed = m_World->damageTarget(hit.targetId, hit.damage);
            if (killed) {
                std::cout << "[Score +" << m_World->targets()[hit.targetId].scoreValue
                          << "] Target down — total score: " << m_World->score() << '\n';
            }

            // On-screen feedback (white tick / yellow head / red kill).
            if (m_Hud) m_Hud->onHit(hit.isHeadshot, killed);
        }
    }

    // Audio feedback
    if (m_AudioSystem) {
        m_AudioSystem->onWeaponFire(m_CurrentWeapon->name, m_Player->getPosition());
        if (hit.hit) {
            m_AudioSystem->onBulletImpact(hit.hitPoint, Audio::SurfaceMaterial::CONCRETE);
        }
    }

    // Recoil
    addRecoil();

    // View punch (random kick); scaled in update loop with real dt.
    // We use a thread-local Mersenne Twister — rand() has known low
    // quality on some libc implementations and clang-tidy (cert-msc50-cpp)
    // rightly flags it.
    static thread_local std::mt19937 punchRng(std::random_device{}());
    std::uniform_real_distribution<float> unit01(0.0f, 1.0f);
    const float punchStrength = m_CurrentWeapon->stats.recoilMagnitude * 0.5f;
    m_ViewPunchVelocity.y += punchStrength * (0.8f + 0.4f * unit01(punchRng));
    m_ViewPunchVelocity.x += punchStrength * 0.3f * (unit01(punchRng) - 0.5f);

    // Debug
    std::cout << "FIRE! " << m_CurrentWeapon->name << " | Ammo: " << m_WeaponState.currentAmmo
              << " | Spread: " << calculateCurrentSpread() << "°";
    if (hit.hit) {
        std::cout << " | HIT at " << hit.distance << "m for " << hit.damage << " dmg";
        if (hit.isHeadshot) std::cout << " (HEADSHOT!)";
    }
    std::cout << '\n';

    // Auto-reload when empty
    if (m_WeaponState.currentAmmo == 0 && m_WeaponState.reserveAmmo > 0) {
        startReload();
    }
}

HitResult WeaponSystem::performRaycast(const glm::vec3& origin, const glm::vec3& direction) const {
    HitResult result;
    if (!m_CurrentWeapon) return result;

    const float maxRange = m_CurrentWeapon->stats.maxRange;

    if (!m_World) return result;

    float dist = 0.0f;
    glm::vec3 hp{0.0f};
    const int idx = m_World->raycastTargets(origin, direction, maxRange, dist, hp);
    if (idx < 0) return result;

    // Determine hit location from where the ray hit the AABB:
    // top third = head, bottom third = legs, middle = chest.
    const Target& t  = m_World->targets()[idx];
    const float relY = (hp.y - t.position.y) / (t.halfExtents.y * t.scale);
    if (relY > 0.55f)
        result.hitLocation = HitResult::HEAD;
    else if (relY < -0.45f)
        result.hitLocation = HitResult::LEG_LEFT;
    else
        result.hitLocation = HitResult::CHEST;

    result.hit        = true;
    result.hitPoint   = hp;
    result.distance   = dist;
    result.targetId   = idx;
    result.isHeadshot = (result.hitLocation == HitResult::HEAD);
    result.damage     = calculateDamage(result);
    return result;
}

float WeaponSystem::calculateDamage(const HitResult& hit) const {
    if (!m_CurrentWeapon || !hit.hit) return 0.0f;

    float baseDamage = m_CurrentWeapon->stats.baseDamage;

    // Distance falloff
    float distanceFactor = 1.0f;
    if (hit.distance > m_CurrentWeapon->stats.optimalRange) {
        float falloffRange = m_CurrentWeapon->stats.maxRange - m_CurrentWeapon->stats.optimalRange;
        float falloffDistance = hit.distance - m_CurrentWeapon->stats.optimalRange;
        distanceFactor        = 1.0f - (falloffDistance / falloffRange) *
                                    (1.0f - m_CurrentWeapon->stats.minDamagePercent);
        distanceFactor = std::max(m_CurrentWeapon->stats.minDamagePercent, distanceFactor);
    }

    // Hit location multiplier
    float locationMultiplier = 1.0f;
    switch (hit.hitLocation) {
    case HitResult::HEAD:
        locationMultiplier = m_CurrentWeapon->stats.headshotMultiplier;
        break;
    case HitResult::CHEST:
        locationMultiplier = m_CurrentWeapon->stats.chestMultiplier;
        break;
    case HitResult::ARM_LEFT:
    case HitResult::ARM_RIGHT:
    case HitResult::LEG_LEFT:
    case HitResult::LEG_RIGHT:
        locationMultiplier = m_CurrentWeapon->stats.limbMultiplier;
        break;
    default:
        locationMultiplier = 1.0f;
        break;
    }

    return baseDamage * distanceFactor * locationMultiplier;
}

void WeaponSystem::addRecoil() {
    if (!m_CurrentWeapon) return;

    // Get recoil pattern point
    glm::vec2 patternRecoil = getRecoilPatternPoint(m_WeaponState.shotsFired - 1);

    // Add randomness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, m_CurrentWeapon->stats.recoilRandomness);

    patternRecoil.x += dist(gen);
    patternRecoil.y += dist(gen);

    // Scale by weapon recoil magnitude
    patternRecoil *= m_CurrentWeapon->stats.recoilMagnitude;

    // Add to current recoil
    m_WeaponState.targetRecoil += patternRecoil;

    // Apply to camera immediately for responsive feel
    applyRecoilToCamera();
}

glm::vec2 WeaponSystem::getRecoilPatternPoint(int shotIndex) const {
    if (!m_CurrentWeapon || m_CurrentWeapon->recoilPattern.empty()) {
        return glm::vec2(0.0f, 1.0f); // Default upward recoil
    }

    // Clamp to pattern length
    int patternIndex = std::min(shotIndex, (int)m_CurrentWeapon->recoilPattern.size() - 1);
    return m_CurrentWeapon->recoilPattern[patternIndex].offset;
}

void WeaponSystem::applyRecoilToCamera() {
    // Apply recoil as camera rotation (view punch)
    float punchX = m_WeaponState.targetRecoil.x * 0.1f;
    float punchY = -m_WeaponState.targetRecoil.y * 0.1f; // Negative because recoil goes up

    if (m_Camera) {
        m_Camera->processMouseMovement(punchX, punchY);
    }
}

void WeaponSystem::updateRecoil(float deltaTime) {
    // Smooth recoil interpolation
    float recoverySpeed         = m_CurrentWeapon ? m_CurrentWeapon->stats.recoilRecovery : 8.0f;

    m_WeaponState.currentRecoil = glm::mix(m_WeaponState.currentRecoil, m_WeaponState.targetRecoil,
                                           deltaTime * 15.0f // Fast snap to target
    );

    // Recoil recovery when not shooting
    float timeSinceLastShot = m_GameTime - m_WeaponState.lastShotTime;
    if (timeSinceLastShot > 0.1f) { // Start recovery 100ms after last shot
        m_WeaponState.targetRecoil =
            glm::mix(m_WeaponState.targetRecoil, glm::vec2(0.0f), deltaTime * recoverySpeed);

        // Reset shot counter if recoil is nearly recovered
        if (glm::length(m_WeaponState.targetRecoil) < 0.1f) {
            m_WeaponState.shotsFired = 0;
        }
    }
}

float WeaponSystem::calculateCurrentSpread() const {
    if (!m_CurrentWeapon) return 0.0f;

    float spread = m_CurrentWeapon->stats.baseSpread;

    // Movement penalty
    if (m_Player) {
        float playerSpeed = m_Player->getSpeed();
        if (playerSpeed > 1.0f) {
            spread += m_CurrentWeapon->stats.movingSpread * (playerSpeed / 250.0f);
        }

        // Jumping penalty
        if (!m_Player->isOnGround()) {
            spread += m_CurrentWeapon->stats.jumpingSpread;
        }

        // Crouching bonus (crouchingSpread is a *negative* value in weapon configs).
        if (m_Player->isCrouching()) {
            spread += m_CurrentWeapon->stats.crouchingSpread;
        }
    }

    // ADS reduction
    if (m_WeaponState.isAiming) {
        spread *= (1.0f - m_CurrentWeapon->stats.adsSpreadReduction * m_WeaponState.adsProgress);
    }

    // Recoil increases spread
    float recoilSpread = glm::length(m_WeaponState.currentRecoil) * 0.01f;
    spread += recoilSpread;

    return std::max(0.0f, spread);
}

glm::vec3 WeaponSystem::applySpreadToDirection(const glm::vec3& baseDirection) const {
    float spreadAngle = calculateCurrentSpread();
    if (spreadAngle <= 0.0f) return baseDirection;

    // Convert spread from degrees to radians
    float spreadRad = glm::radians(spreadAngle);

    // Generate random offset within spread cone
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265359f);
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);

    const float angle  = angleDist(gen);
    const float radius = std::sqrt(radiusDist(gen)) * spreadRad; // sqrt for uniform distribution

    // Build a basis perpendicular to the shot direction.
    glm::vec3 up          = (std::fabs(baseDirection.y) < 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                                : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(baseDirection, up));
    up                    = glm::normalize(glm::cross(right, baseDirection));

    // Apply spread within a cone. Cast to float because std::cos/std::sin
    // return double for float inputs depending on overload resolution.
    const float c                = std::cos(angle) * radius;
    const float s                = std::sin(angle) * radius;
    const glm::vec3 spreadOffset = right * c + up * s;
    return glm::normalize(baseDirection + spreadOffset);
}

void WeaponSystem::startReload() {
    if (!m_CurrentWeapon || m_WeaponState.reserveAmmo <= 0) return;
    if (m_WeaponState.currentAmmo >= m_CurrentWeapon->stats.magazineSize) return;

    changeWeaponState(Weapons::WeaponState::RELOADING);
    m_WeaponState.reloadStartTime = m_GameTime;

    // Audio feedback
    if (m_AudioSystem) {
        m_AudioSystem->onWeaponReload(m_CurrentWeapon->name, m_Player->getPosition(), "start");
    }

    // Determine reload time
    float reloadTime = m_WeaponState.chamberedRound ? m_CurrentWeapon->stats.tacticalReloadTime
                                                    : m_CurrentWeapon->stats.reloadTime;

    m_WeaponState.stateTimer = reloadTime;

    std::cout << "Reloading " << m_CurrentWeapon->name << " (" << reloadTime << "s)" << "\n";
}

void WeaponSystem::cancelReload() {
    if (m_WeaponState.state == Weapons::WeaponState::RELOADING) {
        changeWeaponState(Weapons::WeaponState::IDLE);
        std::cout << "Reload cancelled" << "\n";
    }
}

bool WeaponSystem::isReloading() const {
    return m_WeaponState.state == Weapons::WeaponState::RELOADING;
}

void WeaponSystem::startADS() {
    if (!m_CurrentWeapon || m_WeaponState.state != Weapons::WeaponState::IDLE) return;

    m_WeaponState.isAiming = true;
    std::cout << "ADS started" << "\n";
}

void WeaponSystem::stopADS() {
    m_WeaponState.isAiming = false;
    std::cout << "ADS stopped" << "\n";
}

bool WeaponSystem::isAiming() const {
    return m_WeaponState.isAiming;
}

void WeaponSystem::updateWeaponState(float deltaTime) {
    updateStateMachine(deltaTime);
}

void WeaponSystem::updateStateMachine(float deltaTime) {
    if (m_WeaponState.stateTimer > 0.0f) {
        m_WeaponState.stateTimer -= deltaTime;

        if (m_WeaponState.stateTimer <= 0.0f) {
            // State completed
            switch (m_WeaponState.state) {
            case Weapons::WeaponState::RELOADING:
                // Complete reload - call the method below
                {
                    int ammoNeeded =
                        m_CurrentWeapon->stats.magazineSize - m_WeaponState.currentAmmo;
                    int ammoToAdd = std::min(ammoNeeded, m_WeaponState.reserveAmmo);

                    m_WeaponState.currentAmmo += ammoToAdd;
                    m_WeaponState.reserveAmmo -= ammoToAdd;
                    m_WeaponState.chamberedRound = true;

                    std::cout << "Reload complete! " << m_WeaponState.currentAmmo << "/"
                              << m_WeaponState.reserveAmmo << "\n";
                }
                changeWeaponState(Weapons::WeaponState::IDLE);
                break;

            case Weapons::WeaponState::DRAWING:
            default:
                // Both fall back to IDLE — drawing has no dedicated
                // side-effect yet (Phase 4 will add draw animation).
                changeWeaponState(Weapons::WeaponState::IDLE);
                break;
            }
        }
    }
}

void WeaponSystem::updateADS(float deltaTime) {
    if (!m_CurrentWeapon) return;

    float adsSpeed = 1.0f / m_CurrentWeapon->stats.adsTime;

    if (m_WeaponState.isAiming) {
        m_WeaponState.adsProgress =
            std::min(1.0f, m_WeaponState.adsProgress + deltaTime * adsSpeed);
    } else {
        m_WeaponState.adsProgress =
            std::max(0.0f, m_WeaponState.adsProgress - deltaTime * adsSpeed);
    }
}

void WeaponSystem::updateWeaponSway(float deltaTime) {
    // Simple weapon sway based on mouse movement
    // In a real implementation, you'd get mouse delta from input system
    m_SwayAmount *= 0.95f; // Decay sway
}

void WeaponSystem::updateWeaponBob(float deltaTime) {
    // Weapon bobbing based on player movement
    if (m_Player && m_Player->getSpeed() > 1.0f) {
        m_BobTime += deltaTime * (m_Player->getSpeed() / 100.0f);
    }
}

void WeaponSystem::applyViewPunch() {
    // Deprecated: integration is now done inline in update() with the
    // real frame deltaTime. Kept as a no-op to preserve the public API.
}

void WeaponSystem::changeWeaponState(Weapons::WeaponState newState) {
    if (m_WeaponState.state == newState) return;

    m_WeaponState.state      = newState;
    m_WeaponState.stateTimer = 0.0f;

    // State-specific initialization
    switch (newState) {
    case Weapons::WeaponState::DRAWING:
        m_WeaponState.stateTimer = 0.5f; // Generic draw time
        break;
    default:
        break;
    }
}

void WeaponSystem::updateInputTiming(float deltaTime) {
    // Input timing is handled in processInput
}

const Weapons::WeaponConfig* WeaponSystem::getCurrentWeapon() const {
    return m_CurrentWeapon.get();
}

glm::vec2 WeaponSystem::getCurrentSpread() const {
    float spread = calculateCurrentSpread();
    return glm::vec2(spread, spread);
}

void WeaponSystem::printDebugInfo() const {
    m_DebugTimer += 1.0f / 60.0f; // Assume 60fps

    if (m_DebugTimer >= 2.0f && m_CurrentWeapon) {
        std::cout << "=== WEAPON DEBUG ===" << "\n";
        std::cout << "Weapon: " << m_CurrentWeapon->name << "\n";
        std::cout << "Ammo: " << m_WeaponState.currentAmmo << "/" << m_WeaponState.reserveAmmo
                  << "\n";
        std::cout << "State: " << (int)m_WeaponState.state << "\n";
        std::cout << "Spread: " << calculateCurrentSpread() << "°" << "\n";
        std::cout << "Recoil: (" << m_WeaponState.currentRecoil.x << ", "
                  << m_WeaponState.currentRecoil.y << ")" << "\n";
        std::cout << "ADS: " << (m_WeaponState.adsProgress * 100.0f) << "%" << "\n";
        std::cout << "Shots Fired: " << m_WeaponState.shotsFired << "\n";
        std::cout << "===================" << "\n";

        m_DebugTimer = 0.0f;
    }
}

// WeaponFactory implementations
std::unique_ptr<Weapons::WeaponConfig> WeaponFactory::createAK47() {
    auto weapon  = std::make_unique<Weapons::WeaponConfig>();

    weapon->name = "AK-47";
    weapon->type = Weapons::WeaponType::RIFLE;

    // AK-47 stats (high damage, high recoil)
    weapon->stats.baseDamage              = 36.0f;
    weapon->stats.headshotMultiplier      = 4.0f;
    weapon->stats.optimalRange            = 25.0f;
    weapon->stats.maxRange                = 80.0f;
    weapon->stats.minDamagePercent        = 0.25f;

    weapon->stats.baseSpread              = 0.15f;
    weapon->stats.movingSpread            = 0.4f;
    weapon->stats.jumpingSpread           = 1.2f;
    weapon->stats.crouchingSpread         = -0.08f;

    weapon->stats.recoilMagnitude         = 1.2f;
    weapon->stats.recoilRecovery          = 6.0f;
    weapon->stats.recoilRandomness        = 0.15f;

    weapon->stats.fireRate                = 600.0f;
    weapon->stats.fireMode                = Weapons::FireMode::FULL_AUTO;

    weapon->stats.magazineSize            = 30;
    weapon->stats.reserveAmmo             = 90;
    weapon->stats.reloadTime              = 2.5f;
    weapon->stats.tacticalReloadTime      = 2.0f;

    weapon->stats.movementSpeedMultiplier = 0.87f;
    weapon->stats.adsTime                 = 0.35f;
    weapon->stats.adsSpreadReduction      = 0.75f;

    weapon->recoilPattern                 = generateAK47Pattern();

    return weapon;
}

std::unique_ptr<Weapons::WeaponConfig> WeaponFactory::createM4A4() {
    auto weapon  = std::make_unique<Weapons::WeaponConfig>();

    weapon->name = "M4A4";
    weapon->type = Weapons::WeaponType::RIFLE;

    // M4A4 stats (balanced, controllable)
    weapon->stats.baseDamage              = 33.0f;
    weapon->stats.headshotMultiplier      = 4.0f;
    weapon->stats.optimalRange            = 30.0f;
    weapon->stats.maxRange                = 85.0f;
    weapon->stats.minDamagePercent        = 0.3f;

    weapon->stats.baseSpread              = 0.12f;
    weapon->stats.movingSpread            = 0.35f;
    weapon->stats.jumpingSpread           = 1.0f;
    weapon->stats.crouchingSpread         = -0.06f;

    weapon->stats.recoilMagnitude         = 1.0f;
    weapon->stats.recoilRecovery          = 7.0f;
    weapon->stats.recoilRandomness        = 0.1f;

    weapon->stats.fireRate                = 666.0f;
    weapon->stats.fireMode                = Weapons::FireMode::FULL_AUTO;

    weapon->stats.magazineSize            = 30;
    weapon->stats.reserveAmmo             = 90;
    weapon->stats.reloadTime              = 3.1f;
    weapon->stats.tacticalReloadTime      = 2.3f;

    weapon->stats.movementSpeedMultiplier = 0.9f;
    weapon->stats.adsTime                 = 0.3f;
    weapon->stats.adsSpreadReduction      = 0.8f;

    weapon->recoilPattern                 = generateM4A4Pattern();

    return weapon;
}

std::unique_ptr<Weapons::WeaponConfig> WeaponFactory::createAWP() {
    auto weapon  = std::make_unique<Weapons::WeaponConfig>();

    weapon->name = "AWP";
    weapon->type = Weapons::WeaponType::SNIPER;

    // AWP stats (one-shot potential, slow)
    weapon->stats.baseDamage              = 115.0f;
    weapon->stats.headshotMultiplier      = 2.5f; // Always kills anyway
    weapon->stats.optimalRange            = 60.0f;
    weapon->stats.maxRange                = 150.0f;
    weapon->stats.minDamagePercent        = 0.8f;

    weapon->stats.baseSpread              = 0.05f;
    weapon->stats.movingSpread            = 0.8f;
    weapon->stats.jumpingSpread           = 2.0f;
    weapon->stats.crouchingSpread         = -0.02f;

    weapon->stats.recoilMagnitude         = 2.0f;
    weapon->stats.recoilRecovery          = 4.0f;
    weapon->stats.recoilRandomness        = 0.05f;

    weapon->stats.fireRate                = 41.0f; // Very slow
    weapon->stats.fireMode                = Weapons::FireMode::BOLT_ACTION;

    weapon->stats.magazineSize            = 10;
    weapon->stats.reserveAmmo             = 30;
    weapon->stats.reloadTime              = 3.7f;
    weapon->stats.tacticalReloadTime      = 2.9f;

    weapon->stats.movementSpeedMultiplier = 0.76f; // Very slow movement
    weapon->stats.adsTime                 = 0.45f;
    weapon->stats.adsSpreadReduction      = 0.95f;
    weapon->stats.adsFOVMultiplier        = 0.2f; // High zoom

    // Simple recoil pattern - mostly vertical
    weapon->recoilPattern = {{glm::vec2(0.0f, 8.0f), 0.0f, 8.0f}};

    return weapon;
}

std::unique_ptr<Weapons::WeaponConfig> WeaponFactory::createGlock() {
    auto weapon  = std::make_unique<Weapons::WeaponConfig>();

    weapon->name = "Glock-18";
    weapon->type = Weapons::WeaponType::PISTOL;

    // Glock stats (low damage, high mobility)
    weapon->stats.baseDamage              = 28.0f;
    weapon->stats.headshotMultiplier      = 4.0f;
    weapon->stats.optimalRange            = 15.0f;
    weapon->stats.maxRange                = 50.0f;
    weapon->stats.minDamagePercent        = 0.4f;

    weapon->stats.baseSpread              = 0.2f;
    weapon->stats.movingSpread            = 0.25f;
    weapon->stats.jumpingSpread           = 0.8f;
    weapon->stats.crouchingSpread         = -0.05f;

    weapon->stats.recoilMagnitude         = 0.8f;
    weapon->stats.recoilRecovery          = 10.0f;
    weapon->stats.recoilRandomness        = 0.2f;

    weapon->stats.fireRate                = 400.0f;
    weapon->stats.fireMode                = Weapons::FireMode::SEMI_AUTO;

    weapon->stats.magazineSize            = 20;
    weapon->stats.reserveAmmo             = 120;
    weapon->stats.reloadTime              = 2.2f;
    weapon->stats.tacticalReloadTime      = 1.8f;

    weapon->stats.movementSpeedMultiplier = 1.0f; // No movement penalty
    weapon->stats.adsTime                 = 0.2f;
    weapon->stats.adsSpreadReduction      = 0.6f;

    weapon->recoilPattern                 = generateControlledPattern(0.5f, 0.3f, 10);

    return weapon;
}

std::unique_ptr<Weapons::WeaponConfig> WeaponFactory::createDeagle() {
    auto weapon  = std::make_unique<Weapons::WeaponConfig>();

    weapon->name = "Desert Eagle";
    weapon->type = Weapons::WeaponType::PISTOL;

    // Deagle stats (high damage, high recoil)
    weapon->stats.baseDamage              = 53.0f;
    weapon->stats.headshotMultiplier      = 4.0f;
    weapon->stats.optimalRange            = 20.0f;
    weapon->stats.maxRange                = 70.0f;
    weapon->stats.minDamagePercent        = 0.3f;

    weapon->stats.baseSpread              = 0.3f;
    weapon->stats.movingSpread            = 0.5f;
    weapon->stats.jumpingSpread           = 1.5f;
    weapon->stats.crouchingSpread         = -0.1f;

    weapon->stats.recoilMagnitude         = 1.8f;
    weapon->stats.recoilRecovery          = 5.0f;
    weapon->stats.recoilRandomness        = 0.3f;

    weapon->stats.fireRate                = 267.0f; // Slow
    weapon->stats.fireMode                = Weapons::FireMode::SEMI_AUTO;

    weapon->stats.magazineSize            = 7;
    weapon->stats.reserveAmmo             = 35;
    weapon->stats.reloadTime              = 2.2f;
    weapon->stats.tacticalReloadTime      = 1.8f;

    weapon->stats.movementSpeedMultiplier = 0.95f;
    weapon->stats.adsTime                 = 0.25f;
    weapon->stats.adsSpreadReduction      = 0.7f;

    weapon->recoilPattern                 = generateControlledPattern(1.5f, 0.8f, 7);

    return weapon;
}

std::vector<Weapons::RecoilPoint> WeaponFactory::generateAK47Pattern() {
    // Simplified AK-47 recoil pattern (first 15 shots)
    return {
        {glm::vec2(0.0f, 2.0f), 0.0f, 8.0f},  // 1
        {glm::vec2(0.1f, 2.2f), 0.1f, 8.0f},  // 2
        {glm::vec2(-0.2f, 2.4f), 0.2f, 8.0f}, // 3
        {glm::vec2(-0.4f, 2.1f), 0.3f, 8.0f}, // 4
        {glm::vec2(-0.6f, 1.8f), 0.4f, 8.0f}, // 5
        {glm::vec2(-0.8f, 1.5f), 0.5f, 8.0f}, // 6
        {glm::vec2(-1.0f, 1.2f), 0.6f, 8.0f}, // 7
        {glm::vec2(-1.1f, 1.0f), 0.7f, 8.0f}, // 8
        {glm::vec2(-0.9f, 0.8f), 0.8f, 8.0f}, // 9
        {glm::vec2(-0.6f, 0.7f), 0.9f, 8.0f}, // 10
        {glm::vec2(-0.2f, 0.6f), 1.0f, 8.0f}, // 11
        {glm::vec2(0.3f, 0.6f), 1.1f, 8.0f},  // 12
        {glm::vec2(0.7f, 0.7f), 1.2f, 8.0f},  // 13
        {glm::vec2(1.0f, 0.8f), 1.3f, 8.0f},  // 14
        {glm::vec2(1.2f, 0.9f), 1.4f, 8.0f}   // 15
    };
}

std::vector<Weapons::RecoilPoint> WeaponFactory::generateM4A4Pattern() {
    // M4A4 pattern - more vertical, less horizontal movement
    return {{glm::vec2(0.0f, 1.8f), 0.0f, 8.0f},   {glm::vec2(0.05f, 1.9f), 0.1f, 8.0f},
            {glm::vec2(-0.1f, 2.0f), 0.2f, 8.0f},  {glm::vec2(-0.2f, 1.8f), 0.3f, 8.0f},
            {glm::vec2(-0.3f, 1.6f), 0.4f, 8.0f},  {glm::vec2(-0.4f, 1.4f), 0.5f, 8.0f},
            {glm::vec2(-0.45f, 1.2f), 0.6f, 8.0f}, {glm::vec2(-0.4f, 1.0f), 0.7f, 8.0f},
            {glm::vec2(-0.3f, 0.9f), 0.8f, 8.0f},  {glm::vec2(-0.1f, 0.8f), 0.9f, 8.0f},
            {glm::vec2(0.1f, 0.8f), 1.0f, 8.0f},   {glm::vec2(0.3f, 0.9f), 1.1f, 8.0f},
            {glm::vec2(0.4f, 1.0f), 1.2f, 8.0f},   {glm::vec2(0.45f, 1.1f), 1.3f, 8.0f},
            {glm::vec2(0.4f, 1.2f), 1.4f, 8.0f}};
}

std::vector<Weapons::RecoilPoint>
WeaponFactory::generateControlledPattern(float verticalStrength, float horizontalVariation,
                                         int patternLength) {
    std::vector<Weapons::RecoilPoint> pattern;
    pattern.reserve(patternLength);

    for (int i = 0; i < patternLength; ++i) {
        float progress = float(i) / float(patternLength - 1);

        // Vertical component (strongest at beginning)
        float vertical = verticalStrength * (1.0f - progress * 0.3f);

        // Horizontal component (alternating with decay)
        float horizontal =
            horizontalVariation * std::sin(progress * 6.28f) * (1.0f - progress * 0.5f);

        pattern.push_back({glm::vec2(horizontal, vertical), static_cast<float>(i) * 0.1f, 8.0f});
    }

    return pattern;
}
