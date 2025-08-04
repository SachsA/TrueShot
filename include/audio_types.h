#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Audio {
    // Audio categories for mixing and volume control
    enum class AudioCategory {
        MASTER,
        SFX,            // Sound effects
        WEAPONS,        // Gunshots, reloads, etc.
        FOOTSTEPS,      // Player movement sounds
        ENVIRONMENT,    // Ambient, wind, etc.
        UI,             // Menu sounds, notifications
        VOICE,          // Voice chat, callouts
        MUSIC           // Background music
    };

    // Audio priority levels
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3    // Always play (important gameplay sounds)
    };

    // 3D audio settings
    struct Audio3DSettings {
        float minDistance = 1.0f;       // Distance where volume starts to drop
        float maxDistance = 100.0f;     // Distance where sound becomes inaudible
        float rolloffFactor = 1.0f;     // How quickly volume drops with distance
        bool enableDoppler = false;     // Doppler effect for moving sources
        float dopplerFactor = 1.0f;     // Doppler intensity
        bool enableOcclusion = true;    // Sound blocked by walls
        float occlusionFactor = 0.7f;   // How much occlusion reduces volume
    };

    // Sound material types for footsteps and impacts
    enum class SurfaceMaterial {
        CONCRETE,
        METAL,
        WOOD,
        GRAVEL,
        GRASS,
        WATER,
        SAND,
        TILE,
        CARPET,
        SNOW
    };

    // Footstep timing and intensity
    struct FootstepSettings {
        float walkInterval = 0.6f;      // Time between walk footsteps
        float runInterval = 0.35f;      // Time between run footsteps
        float crouchInterval = 0.8f;    // Time between crouch footsteps
        float jumpLandVolume = 1.2f;    // Volume multiplier for jump landing
        float minSpeedForSound = 10.0f; // Minimum movement speed to play footsteps
        bool enableOwnFootsteps = false; // Hear your own footsteps (usually off)
    };

    // Audio reverb zones (for different environments)
    struct ReverbSettings {
        std::string name;
        float roomSize = 0.5f;          // 0.0 = small room, 1.0 = large hall
        float damping = 0.5f;           // Sound absorption (0.0 = very echoey, 1.0 = dead)
        float wetLevel = 0.3f;          // Reverb volume
        float dryLevel = 1.0f;          // Direct sound volume
        float decayTime = 1.0f;         // How long reverb lasts
    };

    // Audio event types for gameplay sounds
    enum class AudioEvent {
        // Weapon sounds
        WEAPON_FIRE,
        WEAPON_RELOAD_START,
        WEAPON_RELOAD_INSERT,
        WEAPON_RELOAD_END,
        WEAPON_DRAW,
        WEAPON_HOLSTER,
        WEAPON_INSPECT,
        WEAPON_DRY_FIRE,        // Empty gun click
        
        // Impact sounds
        BULLET_IMPACT_CONCRETE,
        BULLET_IMPACT_METAL,
        BULLET_IMPACT_WOOD,
        BULLET_IMPACT_FLESH,
        
        // Movement sounds
        FOOTSTEP,
        JUMP,
        LAND,
        CROUCH_START,
        CROUCH_END,
        
        // UI sounds
        UI_HOVER,
        UI_SELECT,
        UI_ERROR,
        UI_NOTIFICATION,
        
        // Game events
        ROUND_START,
        ROUND_END,
        BOMB_PLANT,
        BOMB_DEFUSE,
        ENEMY_SPOTTED,
        
        // Environmental
        AMBIENT_OUTDOOR,
        AMBIENT_INDOOR,
        WIND,
        WATER_SPLASH
    };
}

// Audio source for 3D positioned sounds
struct AudioSource {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};       // For Doppler effect
    
    float volume = 1.0f;            // Base volume (0.0 - 1.0)
    float pitch = 1.0f;             // Pitch multiplier (0.5 = half speed, 2.0 = double speed)
    bool looping = false;           // Should the sound loop?
    bool is3D = true;               // 3D positioned or 2D UI sound
    
    Audio::AudioCategory category = Audio::AudioCategory::SFX;
    Audio::Priority priority = Audio::Priority::NORMAL;
    Audio::Audio3DSettings settings3D;
    
    // Runtime state
    bool isPlaying = false;
    bool isPaused = false;
    float currentTime = 0.0f;       // Current playback position
    float fadeTarget = 1.0f;        // For fade in/out effects
    float fadeSpeed = 2.0f;         // Fade speed (units per second)
    
    // Occlusion/obstruction
    float occlusionLevel = 0.0f;    // 0.0 = clear, 1.0 = fully blocked
    float obstructionLevel = 0.0f;  // Partial blocking (through walls)
    
    // Unique identifier
    int sourceId = -1;
};

// Audio listener (player's ears)
struct AudioListener {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};   // Looking direction
    glm::vec3 up{0.0f, 1.0f, 0.0f};         // Up vector
    
    // Environmental settings
    Audio::ReverbSettings currentReverb;
    Audio::SurfaceMaterial currentSurface = Audio::SurfaceMaterial::CONCRETE;
    
    // Master volume controls
    float masterVolume = 1.0f;
    std::vector<float> categoryVolumes{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}; // Per-category volumes
};

// Audio clip data (loaded sound file)
struct AudioClip {
    std::string filePath;
    std::vector<float> audioData;   // Raw audio samples
    int sampleRate = 44100;         // Samples per second
    int channels = 1;               // 1 = mono, 2 = stereo
    int bitDepth = 16;              // Bits per sample
    float duration = 0.0f;          // Length in seconds
    
    // Loaded state
    bool isLoaded = false;
    unsigned int bufferId = 0;      // OpenAL buffer ID
};

// Sound bank for organized audio management
struct SoundBank {
    std::string name;
    std::vector<std::shared_ptr<AudioClip>> clips;
    
    // Random selection for variation
    int lastPlayedIndex = -1;       // Avoid repeating same sound
    bool randomizePitch = false;    // Add slight pitch variation
    float pitchVariation = 0.1f;    // Amount of pitch randomization
};

// Audio zone for environmental audio
struct AudioZone {
    glm::vec3 center{0.0f};
    glm::vec3 size{10.0f};          // Box dimensions
    Audio::ReverbSettings reverb;
    Audio::SurfaceMaterial defaultSurface;
    
    // Background audio
    std::string ambientSound;
    float ambientVolume = 0.3f;
    
    bool contains(const glm::vec3& point) const {
        glm::vec3 halfSize = size * 0.5f;
        glm::vec3 minBounds = center - halfSize;
        glm::vec3 maxBounds = center + halfSize;
        
        return point.x >= minBounds.x && point.x <= maxBounds.x &&
               point.y >= minBounds.y && point.y <= maxBounds.y &&
               point.z >= minBounds.z && point.z <= maxBounds.z;
    }
};

// Audio performance metrics
struct AudioMetrics {
    int activeSources = 0;
    int totalSourcesCreated = 0;
    float cpuUsage = 0.0f;          // Percentage
    float memoryUsage = 0.0f;       // MB
    int soundsPlayedThisFrame = 0;
    float averageLatency = 0.0f;    // ms
    
    // Quality metrics
    int droppedSounds = 0;          // Sounds that couldn't play due to limits
    int occludedSounds = 0;         // Sounds currently occluded
    float compressionRatio = 0.0f;  // Audio compression efficiency
};
