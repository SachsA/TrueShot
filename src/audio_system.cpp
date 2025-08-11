#include "audio_system.h"
#include "fps_camera.h"
#include "player_controller.h"
#include "weapon_system.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <random>

// Note: In a real implementation, you'd use OpenAL for actual audio
// This is a simulation that prints debug info instead of playing real audio

AudioSystem::AudioSystem() {
    // Initialize footstep settings
    m_FootstepSettings = Audio::FootstepSettings{};
    
    // Default reverb
    m_Listener.currentReverb = {
        "Default",
        0.3f,   // roomSize
        0.6f,   // damping
        0.2f,   // wetLevel
        1.0f,   // dryLevel
        1.2f    // decayTime
    };
    
    // Initialize category volumes
    for (int i = 0; i < 8; ++i) {
        m_Listener.categoryVolumes[i] = 1.0f;
    }
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize() {
    if (m_Initialized) return true;
    
    std::cout << "Initializing AudioSystem..." << std::endl;
    
    // In a real implementation, initialize OpenAL here
    initializeOpenAL();
    
    // Set up event mappings
    setupEventMappings();
    
    // Load default sound banks
    loadDefaultSounds();
    
    // Start audio thread
    m_ThreadRunning = true;
    m_AudioThread = std::thread(&AudioSystem::audioThreadFunc, this);
    
    m_Initialized = true;
    std::cout << "AudioSystem initialized successfully!" << std::endl;
    return true;
}

void AudioSystem::shutdown() {
    if (!m_Initialized) return;
    
    std::cout << "Shutting down AudioSystem..." << std::endl;
    
    // Stop audio thread
    m_ThreadRunning = false;
    if (m_AudioThread.joinable()) {
        m_AudioThread.join();
    }
    
    // Stop all sources
    for (auto& pair : m_ActiveSources) {
        stopSound(pair.first);
    }
    m_ActiveSources.clear();
    
    // Cleanup OpenAL
    shutdownOpenAL();
    
    // Clear data
    m_AudioClips.clear();
    m_SoundBanks.clear();
    
    m_Initialized = false;
    std::cout << "AudioSystem shut down." << std::endl;
}

void AudioSystem::update(float deltaTime) {
    if (!m_Initialized) return;
    
    // Update metrics timer
    m_MetricsTimer += deltaTime;
    m_FootstepTimer += deltaTime;
    
    // Process audio queue
    processAudioQueue();
    
    // Update source positions and effects
    updateSourcesPosition();
    updateEnvironmentalEffects();
    
    // Cleanup finished sources
    cleanupFinishedSources();
    
    // Update metrics every second
    if (m_MetricsTimer >= 1.0f) {
        updateMetrics();
        m_MetricsTimer = 0.0f;
    }
    
    // Reset frame counters
    m_Metrics.soundsPlayedThisFrame = 0;
}

void AudioSystem::setListenerFromCamera(FPSCamera* camera, PlayerController* player) {
    m_Camera = camera;
    m_Player = player;
    
    if (camera && player) {
        glm::vec3 position = player->getPosition();
        glm::vec3 velocity = player->getVelocity();
        glm::vec3 forward = camera->getForward();
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        
        updateListener(position, velocity, forward, up);
    }
}

void AudioSystem::updateListener(const glm::vec3& position, const glm::vec3& velocity, 
                                const glm::vec3& forward, const glm::vec3& up) {
    m_Listener.position = position;
    m_Listener.velocity = velocity;
    m_Listener.forward = forward;
    m_Listener.up = up;
    
    // Check for audio zone changes
    updateCurrentAudioZone();
    
    // In real implementation, update OpenAL listener
    // alListener3f(AL_POSITION, position.x, position.y, position.z);
    // alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    // etc.
}

int AudioSystem::playSound(const std::string& soundName, const glm::vec3& position, 
                          float volume, float pitch, bool looping) {
    
    // Find the audio clip
    auto clipIt = m_AudioClips.find(soundName);
    if (clipIt == m_AudioClips.end()) {
        // Try sound banks
        std::string bankSound = selectSoundFromBank(soundName);
        if (!bankSound.empty()) {
            clipIt = m_AudioClips.find(bankSound);
        }
        
        if (clipIt == m_AudioClips.end()) {
            std::cout << "Warning: Sound not found: " << soundName << std::endl;
            return -1;
        }
    }
    
    // Create audio source
    int sourceId = createAudioSource();
    if (sourceId == -1) {
        m_Metrics.droppedSounds++;
        return -1;
    }
    
    AudioSource* source = getAudioSource(sourceId);
    if (!source) return -1;
    
    // Configure source
    source->position = position;
    source->volume = volume;
    source->pitch = pitch;
    source->looping = looping;
    source->isPlaying = true;
    source->is3D = true;
    
    // Add slight pitch variation for realism
    if (soundName.find("footstep") != std::string::npos || 
        soundName.find("impact") != std::string::npos) {
        addPitchVariation(*source, 0.15f);
    }
    
    // Calculate 3D audio properties
    float distance = calculateDistance(position, m_Listener.position);
    float calculatedVolume = calculateVolume(*source);
    
    // In real implementation, start OpenAL source
    // alSourcePlay(source->alSourceId);
    
    // Debug output
    if (m_DebugVisualization) {
        std::cout << "🔊 Playing: " << soundName 
                  << " | Pos: (" << position.x << ", " << position.y << ", " << position.z << ")"
                  << " | Dist: " << distance << "m"
                  << " | Vol: " << calculatedVolume << std::endl;
    }
    
    m_Metrics.soundsPlayedThisFrame++;
    m_Metrics.totalSourcesCreated++;
    
    return sourceId;
}

int AudioSystem::playSound2D(const std::string& soundName, float volume, float pitch) {
    int sourceId = createAudioSource();
    if (sourceId == -1) return -1;
    
    AudioSource* source = getAudioSource(sourceId);
    if (!source) return -1;
    
    source->volume = volume;
    source->pitch = pitch;
    source->is3D = false;
    source->isPlaying = true;
    source->category = Audio::AudioCategory::UI;
    
    if (m_DebugVisualization) {
        std::cout << "🔊 Playing 2D: " << soundName << " | Vol: " << volume << std::endl;
    }
    
    return sourceId;
}

int AudioSystem::playSoundEvent(Audio::AudioEvent event, const glm::vec3& position, float volume) {
    auto eventIt = m_EventToSound.find(event);
    if (eventIt == m_EventToSound.end()) {
        std::cout << "Warning: No sound mapped for event: " << (int)event << std::endl;
        return -1;
    }
    
    return playSound(eventIt->second, position, volume);
}

void AudioSystem::onWeaponFire(const std::string& weaponName, const glm::vec3& position) {
    // Play weapon fire sound
    std::string fireSound = weaponName + "_fire";
    int sourceId = playSound(fireSound, position, 1.0f);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::WEAPONS;
            source->priority = Audio::Priority::HIGH;
            source->settings3D.minDistance = 5.0f;
            source->settings3D.maxDistance = 150.0f;
        }
    }
    
    // Play brass casing drop sound (delayed)
    scheduleDelayedSound(weaponName + "_brass", position, 0.2f, 0.3f);
    
    if (m_DebugVisualization) {
        std::cout << "🔫 " << weaponName << " fired at (" 
                  << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    }
}

void AudioSystem::onWeaponDraw(const std::string& weaponName, const glm::vec3& position) {
    // Play weapon fire draw
    std::string drawSound = weaponName + "_draw";
    int sourceId = playSound(drawSound, position, 1.0f);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::WEAPONS;
            source->priority = Audio::Priority::HIGH;
            source->settings3D.minDistance = 5.0f;
            source->settings3D.maxDistance = 150.0f;
        }
    }
    
    if (m_DebugVisualization) {
        std::cout << "🔫 " << weaponName << " drew at (" 
                  << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    }
}

void AudioSystem::onWeaponReload(const std::string& weaponName, const glm::vec3& position, 
                                const std::string& reloadPhase) {
    std::string reloadSound = weaponName + "_reload_" + reloadPhase;
    int sourceId = playSound(reloadSound, position, 0.8f);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::WEAPONS;
            source->settings3D.minDistance = 2.0f;
            source->settings3D.maxDistance = 20.0f;
        }
    }
    
    if (m_DebugVisualization) {
        std::cout << "🔄 " << weaponName << " reload (" << reloadPhase << ")" << std::endl;
    }
}

void AudioSystem::onBulletImpact(const glm::vec3& position, Audio::SurfaceMaterial material) {
    std::string impactSound = AudioUtils::getSurfaceSoundName(material, "impact");
    int sourceId = playSound(impactSound, position, 0.7f);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::SFX;
            source->settings3D.minDistance = 1.0f;
            source->settings3D.maxDistance = 80.0f;
            addPitchVariation(*source, 0.2f);
        }
    }
    
    // Play ricochet sound occasionally
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    if (material == Audio::SurfaceMaterial::METAL && dis(gen) < 0.3f) {
        scheduleDelayedSound("ricochet", position, 0.1f, 0.5f);
    }
}

void AudioSystem::onFootstep(const glm::vec3& position, Audio::SurfaceMaterial surface, 
                            float movementSpeed, bool isLocalPlayer) {
    
    // Don't play own footsteps unless enabled
    if (isLocalPlayer && !m_FootstepSettings.enableOwnFootsteps) {
        return;
    }
    
    // Check minimum speed
    if (movementSpeed < m_FootstepSettings.minSpeedForSound) {
        return;
    }
    
    // Calculate footstep interval based on movement speed
    float interval = m_FootstepSettings.walkInterval;
    if (movementSpeed > 200.0f) {
        interval = m_FootstepSettings.runInterval;
    } else if (movementSpeed < 100.0f) {
        interval = m_FootstepSettings.crouchInterval;
    }
    
    // Check timing
    static float lastFootstepTime = 0.0f;
    if (m_FootstepTimer - lastFootstepTime < interval) {
        return;
    }
    lastFootstepTime = m_FootstepTimer;
    
    // Play footstep sound
    std::string footstepSound = AudioUtils::getSurfaceSoundName(surface, "footstep");
    int sourceId = playSound(footstepSound, position, 0.6f);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::FOOTSTEPS;
            source->settings3D.minDistance = 1.0f;
            source->settings3D.maxDistance = 30.0f;
            
            // Adjust volume based on movement speed
            float speedVolume = std::min(1.5f, movementSpeed / 150.0f);
            source->volume *= speedVolume;
            
            addPitchVariation(*source, 0.1f);
        }
    }
    
    if (m_DebugVisualization && !isLocalPlayer) {
        std::cout << "👟 Footstep on " << (int)surface << " | Speed: " << movementSpeed << std::endl;
    }
}

void AudioSystem::onJump(const glm::vec3& position, bool isLocalPlayer) {
    if (isLocalPlayer && !m_FootstepSettings.enableOwnFootsteps) return;
    
    int sourceId = playSound("jump", position, 0.4f);
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::FOOTSTEPS;
            source->settings3D.maxDistance = 25.0f;
        }
    }
}

void AudioSystem::onLand(const glm::vec3& position, float impactForce, bool isLocalPlayer) {
    if (isLocalPlayer && !m_FootstepSettings.enableOwnFootsteps) return;
    
    float volume = std::min(1.0f, impactForce * m_FootstepSettings.jumpLandVolume);
    int sourceId = playSound("land", position, volume);
    
    if (sourceId != -1) {
        AudioSource* source = getAudioSource(sourceId);
        if (source) {
            source->category = Audio::AudioCategory::FOOTSTEPS;
            source->settings3D.maxDistance = 35.0f;
            
            // Heavy landings have lower pitch
            if (impactForce > 0.8f) {
                source->pitch *= 0.9f;
            }
        }
    }
}

void AudioSystem::stopSound(int sourceId) {
    auto it = m_ActiveSources.find(sourceId);
    if (it != m_ActiveSources.end()) {
        it->second->isPlaying = false;
        // In real implementation: alSourceStop(it->second->alSourceId);
        
        if (m_DebugVisualization) {
            std::cout << "⏹️ Stopped source " << sourceId << std::endl;
        }
    }
}

void AudioSystem::fadeOut(int sourceId, float fadeTime) {
    AudioSource* source = getAudioSource(sourceId);
    if (source) {
        source->fadeTarget = 0.0f;
        source->fadeSpeed = 1.0f / fadeTime;
    }
}

void AudioSystem::setMasterVolume(float volume) {
    m_Listener.masterVolume = std::clamp(volume, 0.0f, 1.0f);
    // In real implementation: alListenerf(AL_GAIN, m_Listener.masterVolume);
}

void AudioSystem::setCategoryVolume(Audio::AudioCategory category, float volume) {
    int index = static_cast<int>(category);
    if (index >= 0 && index < m_Listener.categoryVolumes.size()) {
        m_Listener.categoryVolumes[index] = std::clamp(volume, 0.0f, 1.0f);
    }
}

float AudioSystem::getCategoryVolume(Audio::AudioCategory category) const {
    int index = static_cast<int>(category);
    if (index >= 0 && index < m_Listener.categoryVolumes.size()) {
        return m_Listener.categoryVolumes[index];
    }
    return 1.0f;
}

bool AudioSystem::loadSound(const std::string& name, const std::string& filePath) {
    auto clip = loadAudioFile(filePath);
    if (clip && clip->isLoaded) {
        m_AudioClips[name] = clip;
        std::cout << "Loaded audio: " << name << " (" << clip->duration << "s)" << std::endl;
        return true;
    }
    
    std::cout << "Failed to load audio: " << filePath << std::endl;
    return false;
}

void AudioSystem::initializeOpenAL() {
    // In a real implementation, initialize OpenAL here
    // m_ALDevice = alcOpenDevice(nullptr);
    // m_ALContext = alcCreateContext(m_ALDevice, nullptr);
    // alcMakeContextCurrent(m_ALContext);
    
    std::cout << "OpenAL initialized (simulated)" << std::endl;
}

void AudioSystem::shutdownOpenAL() {
    // In a real implementation, cleanup OpenAL here
    // alcMakeContextCurrent(nullptr);
    // alcDestroyContext(m_ALContext);
    // alcCloseDevice(m_ALDevice);
    
    std::cout << "OpenAL shut down (simulated)" << std::endl;
}

void AudioSystem::setupEventMappings() {
    // Map audio events to sound names
    m_EventToSound[Audio::AudioEvent::WEAPON_FIRE] = "weapon_fire";
    m_EventToSound[Audio::AudioEvent::WEAPON_RELOAD_START] = "weapon_reload_start";
    m_EventToSound[Audio::AudioEvent::WEAPON_DRY_FIRE] = "weapon_dry_fire";
    m_EventToSound[Audio::AudioEvent::FOOTSTEP] = "footstep";
    m_EventToSound[Audio::AudioEvent::JUMP] = "jump";
    m_EventToSound[Audio::AudioEvent::LAND] = "land";
    m_EventToSound[Audio::AudioEvent::BULLET_IMPACT_CONCRETE] = "impact_concrete";
    m_EventToSound[Audio::AudioEvent::BULLET_IMPACT_METAL] = "impact_metal";
    m_EventToSound[Audio::AudioEvent::BULLET_IMPACT_WOOD] = "impact_wood";
    m_EventToSound[Audio::AudioEvent::UI_SELECT] = "ui_select";
    m_EventToSound[Audio::AudioEvent::UI_HOVER] = "ui_hover";
}

void AudioSystem::loadDefaultSounds() {
    // In a real implementation, load actual audio files
    // For now, just register the sound names
    
    // Weapon sounds
    registerDummySound("ak47_fire", 0.15f);
    registerDummySound("ak47_reload_start", 0.3f);
    registerDummySound("ak47_reload_end", 0.2f);
    registerDummySound("ak47_brass", 0.8f);
    
    registerDummySound("m4a4_fire", 0.12f);
    registerDummySound("m4a4_reload_start", 0.25f);
    registerDummySound("m4a4_reload_end", 0.18f);
    
    registerDummySound("awp_fire", 0.35f);
    registerDummySound("awp_reload_start", 0.4f);
    
    registerDummySound("glock_fire", 0.08f);
    registerDummySound("deagle_fire", 0.2f);
    
    // Impact sounds
    registerDummySound("impact_concrete", 0.1f);
    registerDummySound("impact_metal", 0.12f);
    registerDummySound("impact_wood", 0.09f);
    registerDummySound("ricochet", 0.3f);
    
    // Movement sounds
    registerDummySound("footstep_concrete", 0.05f);
    registerDummySound("footstep_metal", 0.06f);
    registerDummySound("footstep_wood", 0.04f);
    registerDummySound("jump", 0.1f);
    registerDummySound("land", 0.15f);
    
    // UI sounds
    registerDummySound("ui_select", 0.05f);
    registerDummySound("ui_hover", 0.03f);
    
    std::cout << "Loaded " << m_AudioClips.size() << " default sounds" << std::endl;
}

void AudioSystem::registerDummySound(const std::string& name, float duration) {
    auto clip = std::make_shared<AudioClip>();
    clip->filePath = "dummy/" + name + ".wav";
    clip->duration = duration;
    clip->isLoaded = true;
    clip->sampleRate = 44100;
    clip->channels = 1;
    clip->bitDepth = 16;
    
    m_AudioClips[name] = clip;
}

std::shared_ptr<AudioClip> AudioSystem::loadAudioFile(const std::string& filePath) {
    auto clip = std::make_shared<AudioClip>();
    clip->filePath = filePath;
    
    // In a real implementation, load the actual audio file
    // For now, create dummy data
    clip->duration = 1.0f;
    clip->sampleRate = 44100;
    clip->channels = 1;
    clip->bitDepth = 16;
    clip->isLoaded = true;
    
    return clip;
}

int AudioSystem::createAudioSource() {
    // Check source limits
    if (m_ActiveSources.size() >= MAX_SOURCES) {
        limitActiveSources();
        if (m_ActiveSources.size() >= MAX_SOURCES) {
            return -1; // Still full after cleanup
        }
    }
    
    int sourceId = m_NextSourceId++;
    auto source = std::make_unique<AudioSource>();
    source->sourceId = sourceId;
    
    // In real implementation: alGenSources(1, &source->alSourceId);
    
    m_ActiveSources[sourceId] = std::move(source);
    m_Metrics.activeSources = m_ActiveSources.size();
    
    return sourceId;
}

AudioSource* AudioSystem::getAudioSource(int sourceId) {
    auto it = m_ActiveSources.find(sourceId);
    return (it != m_ActiveSources.end()) ? it->second.get() : nullptr;
}

void AudioSystem::cleanupFinishedSources() {
    std::vector<int> toRemove;
    
    for (auto& pair : m_ActiveSources) {
        AudioSource* source = pair.second.get();
        
        // Update fade effects
        if (source->fadeTarget != source->volume) {
            float fadeStep = source->fadeSpeed * (1.0f / 60.0f); // Assume 60fps
            if (source->fadeTarget < source->volume) {
                source->volume = std::max(source->fadeTarget, source->volume - fadeStep);
            } else {
                source->volume = std::min(source->fadeTarget, source->volume + fadeStep);
            }
            
            // Stop source if faded out
            if (source->fadeTarget == 0.0f && source->volume <= 0.01f) {
                source->isPlaying = false;
            }
        }
        
        // Check if source is finished
        if (!source->isPlaying && !source->looping) {
            toRemove.push_back(pair.first);
        }
        
        // Update playback time (simulated)
        if (source->isPlaying) {
            source->currentTime += 1.0f / 60.0f; // Assume 60fps
            
            // Check if non-looping sound finished
            auto clipIt = m_AudioClips.find("dummy"); // Would need actual clip lookup
            if (clipIt != m_AudioClips.end() && !source->looping) {
                if (source->currentTime >= clipIt->second->duration) {
                    source->isPlaying = false;
                }
            }
        }
    }
    
    // Remove finished sources
    for (int id : toRemove) {
        releaseAudioSource(id);
    }
}

void AudioSystem::releaseAudioSource(int sourceId) {
    auto it = m_ActiveSources.find(sourceId);
    if (it != m_ActiveSources.end()) {
        // In real implementation: alDeleteSources(1, &it->second->alSourceId);
        m_ActiveSources.erase(it);
        m_Metrics.activeSources = m_ActiveSources.size();
    }
}

void AudioSystem::limitActiveSources() {
    // Remove lowest priority sources if we hit the limit
    std::vector<std::pair<int, Audio::Priority>> sources;
    
    for (auto& pair : m_ActiveSources) {
        sources.push_back({pair.first, pair.second->priority});
    }
    
    // Sort by priority (lowest first)
    std::sort(sources.begin(), sources.end(), 
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });
    
    // Remove lowest priority sources
    int toRemove = m_ActiveSources.size() - MAX_SOURCES + 5; // Remove 5 to give some headroom
    for (int i = 0; i < toRemove && i < sources.size(); ++i) {
        stopSound(sources[i].first);
    }
}

float AudioSystem::calculateDistance(const glm::vec3& sourcePos, const glm::vec3& listenerPos) const {
    return glm::length(sourcePos - listenerPos);
}

float AudioSystem::calculateVolume(const AudioSource& source) const {
    if (!source.is3D) {
        return source.volume * getCategoryVolume(source.category) * m_Listener.masterVolume;
    }
    
    float distance = calculateDistance(source.position, m_Listener.position);
    
    // Apply distance attenuation
    float attenuation = AudioUtils::calculateRolloff(
        distance, 
        source.settings3D.minDistance, 
        source.settings3D.maxDistance, 
        source.settings3D.rolloffFactor
    );
    
    // Apply occlusion/obstruction
    attenuation *= (1.0f - source.occlusionLevel * source.settings3D.occlusionFactor);
    attenuation *= (1.0f - source.obstructionLevel * 0.5f);
    
    return source.volume * attenuation * getCategoryVolume(source.category) * m_Listener.masterVolume;
}

void AudioSystem::updateSourcesPosition() {
    for (auto& pair : m_ActiveSources) {
        AudioSource* source = pair.second.get();
        if (!source->is3D || !source->isPlaying) continue;
        
        // Update 3D position in OpenAL (in real implementation)
        // alSource3f(source->alSourceId, AL_POSITION, 
        //           source->position.x, source->position.y, source->position.z);
        
        // Apply Doppler effect if enabled
        if (source->settings3D.enableDoppler) {
            applyDopplerEffect(*source);
        }
        
        // Update occlusion
        if (source->settings3D.enableOcclusion) {
            applyOcclusion(*source);
        }
    }
}

void AudioSystem::applyDopplerEffect(AudioSource& source) {
    // Calculate relative velocity
    glm::vec3 relativeVel = source.velocity - m_Listener.velocity;
    glm::vec3 toListener = glm::normalize(m_Listener.position - source.position);
    
    float speedOfSound = 343.0f; // m/s
    float relativeSpeed = glm::dot(relativeVel, toListener);
    
    // Doppler shift calculation
    float dopplerShift = (speedOfSound + relativeSpeed) / speedOfSound;
    dopplerShift = std::clamp(dopplerShift, 0.5f, 2.0f); // Reasonable limits
    
    // Apply to pitch
    float finalPitch = source.pitch * dopplerShift * source.settings3D.dopplerFactor;
    
    // In real implementation: alSourcef(source.alSourceId, AL_PITCH, finalPitch);
}

void AudioSystem::applyOcclusion(AudioSource& source) {
    // Perform simple raycast from source to listener
    bool isOccluded = performRaycastForOcclusion(source.position, m_Listener.position);
    
    if (isOccluded) {
        source.occlusionLevel = std::min(1.0f, source.occlusionLevel + 3.0f * (1.0f/60.0f));
        m_Metrics.occludedSounds++;
    } else {
        source.occlusionLevel = std::max(0.0f, source.occlusionLevel - 2.0f * (1.0f/60.0f));
    }
}

bool AudioSystem::performRaycastForOcclusion(const glm::vec3& from, const glm::vec3& to) const {
    // Simple occlusion check - in real game, use physics raycast
    // For now, simulate some basic occlusion
    
    float distance = glm::length(to - from);
    
    // Simulate walls at regular intervals
    if ((int)(from.x / 10.0f) != (int)(to.x / 10.0f) || 
        (int)(from.z / 10.0f) != (int)(to.z / 10.0f)) {
        return distance > 20.0f; // Far sounds get occluded through "walls"
    }
    
    return false;
}

void AudioSystem::updateCurrentAudioZone() {
    AudioZone* newZone = nullptr;
    
    // Find which zone the listener is in
    for (auto& zone : m_AudioZones) {
        if (zone.contains(m_Listener.position)) {
            newZone = &zone;
            break;
        }
    }
    
    // Zone changed
    if (newZone != m_CurrentZone) {
        m_CurrentZone = newZone;
        
        if (m_CurrentZone) {
            m_Listener.currentReverb = m_CurrentZone->reverb;
            m_Listener.currentSurface = m_CurrentZone->defaultSurface;
            
            if (m_DebugVisualization) {
                std::cout << "🏠 Entered audio zone: " << m_CurrentZone->reverb.name << std::endl;
            }
        }
    }
}

void AudioSystem::updateEnvironmentalEffects() {
    // Update reverb settings based on current zone
    // In real implementation, apply to OpenAL EFX
}

void AudioSystem::updateMetrics() {
    m_Metrics.activeSources = m_ActiveSources.size();
    
    // Calculate average latency (simulated)
    m_Metrics.averageLatency = 10.0f + (m_ActiveSources.size() * 0.5f);
    
    // Memory usage estimate
    m_Metrics.memoryUsage = m_AudioClips.size() * 0.5f + m_ActiveSources.size() * 0.01f;
    
    // CPU usage estimate
    m_Metrics.cpuUsage = (m_ActiveSources.size() / float(MAX_SOURCES)) * 15.0f;
}

void AudioSystem::addPitchVariation(AudioSource& source, float variation) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-variation, variation);
    
    source.pitch += dis(gen);
    source.pitch = std::clamp(source.pitch, 0.5f, 2.0f);
}

std::string AudioSystem::selectSoundFromBank(const std::string& bankName) {
    auto bankIt = m_SoundBanks.find(bankName);
    if (bankIt == m_SoundBanks.end()) return "";
    
    SoundBank& bank = bankIt->second;
    if (bank.clips.empty()) return "";
    
    // Simple selection - avoid repeating last sound
    int index = (bank.lastPlayedIndex + 1) % bank.clips.size();
    bank.lastPlayedIndex = index;
    
    return bank.clips[index]->filePath;
}

void AudioSystem::scheduleDelayedSound(const std::string& soundName, const glm::vec3& position, 
                                     float delay, float volume) {
    // In a real implementation, use a timer system
    // For now, just play immediately with reduced volume
    playSound(soundName, position, volume * 0.7f);
}

void AudioSystem::audioThreadFunc() {
    while (m_ThreadRunning) {
        processAudioQueue();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }
}

void AudioSystem::processAudioQueue() {
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    
    while (!m_AudioQueue.empty()) {
        auto task = m_AudioQueue.front();
        m_AudioQueue.pop();
        task();
    }
}

void AudioSystem::printDebugInfo() const {
    std::cout << "\n=== AUDIO SYSTEM DEBUG ===" << std::endl;
    std::cout << "Active Sources: " << m_Metrics.activeSources << "/" << MAX_SOURCES << std::endl;
    std::cout << "Sounds This Frame: " << m_Metrics.soundsPlayedThisFrame << std::endl;
    std::cout << "Dropped Sounds: " << m_Metrics.droppedSounds << std::endl;
    std::cout << "Occluded Sounds: " << m_Metrics.occludedSounds << std::endl;
    std::cout << "CPU Usage: " << m_Metrics.cpuUsage << "%" << std::endl;
    std::cout << "Memory Usage: " << m_Metrics.memoryUsage << " MB" << std::endl;
    std::cout << "Average Latency: " << m_Metrics.averageLatency << " ms" << std::endl;
    
    std::cout << "\nListener Position: (" << m_Listener.position.x 
              << ", " << m_Listener.position.y << ", " << m_Listener.position.z << ")" << std::endl;
    std::cout << "Master Volume: " << (m_Listener.masterVolume * 100.0f) << "%" << std::endl;
    
    if (m_CurrentZone) {
        std::cout << "Current Zone: " << m_CurrentZone->reverb.name << std::endl;
    }
    
    std::cout << "========================\n" << std::endl;
}

// AudioUtils namespace implementations
namespace AudioUtils {
    float decibelToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    float linearToDecibel(float linear) {
        return 20.0f * std::log10(std::max(0.001f, linear));
    }
    
    float calculateRolloff(float distance, float minDist, float maxDist, float rolloffFactor) {
        if (distance <= minDist) return 1.0f;
        if (distance >= maxDist) return 0.0f;
        
        float normalizedDist = (distance - minDist) / (maxDist - minDist);
        return std::pow(1.0f - normalizedDist, rolloffFactor);
    }
    
    std::string getSurfaceSoundName(Audio::SurfaceMaterial material, const std::string& action) {
        std::string materialName;
        
        switch (material) {
            case Audio::SurfaceMaterial::CONCRETE: materialName = "concrete"; break;
            case Audio::SurfaceMaterial::METAL: materialName = "metal"; break;
            case Audio::SurfaceMaterial::WOOD: materialName = "wood"; break;
            case Audio::SurfaceMaterial::GRAVEL: materialName = "gravel"; break;
            case Audio::SurfaceMaterial::GRASS: materialName = "grass"; break;
            case Audio::SurfaceMaterial::WATER: materialName = "water"; break;
            default: materialName = "concrete"; break;
        }
        
        return action + "_" + materialName;
    }
}
