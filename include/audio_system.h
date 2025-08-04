#pragma once

#include "audio_types.h"
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>

class FPSCamera;
class PlayerController;
class WeaponSystem;

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // Initialization
    bool initialize();
    void shutdown();
    
    // Main update loop (call every frame)
    void update(float deltaTime);
    
    // Listener management (player's ears)
    void updateListener(const glm::vec3& position, const glm::vec3& velocity, 
                       const glm::vec3& forward, const glm::vec3& up);
    void setListenerFromCamera(FPSCamera* camera, PlayerController* player);
    
    // Sound playback
    int playSound(const std::string& soundName, const glm::vec3& position = glm::vec3(0.0f), 
                  float volume = 1.0f, float pitch = 1.0f, bool looping = false);
    int playSound2D(const std::string& soundName, float volume = 1.0f, float pitch = 1.0f);
    int playSoundEvent(Audio::AudioEvent event, const glm::vec3& position = glm::vec3(0.0f), 
                       float volume = 1.0f);
    
    // Source control
    void stopSound(int sourceId);
    void pauseSound(int sourceId);
    void resumeSound(int sourceId);
    void setSoundVolume(int sourceId, float volume);
    void setSoundPitch(int sourceId, float pitch);
    void fadeOut(int sourceId, float fadeTime = 1.0f);
    void fadeIn(int sourceId, float fadeTime = 1.0f);
    
    // Audio loading
    bool loadSound(const std::string& name, const std::string& filePath);
    bool loadSoundBank(const std::string& bankName, const std::vector<std::string>& filePaths);
    void unloadSound(const std::string& name);
    void unloadAll();
    
    // Weapon audio integration
    void onWeaponFire(const std::string& weaponName, const glm::vec3& position);
    void onWeaponReload(const std::string& weaponName, const glm::vec3& position, 
                        const std::string& reloadPhase = "start");
    void onWeaponDraw(const std::string& weaponName, const glm::vec3& position);
    void onBulletImpact(const glm::vec3& position, Audio::SurfaceMaterial material);
    
    // Movement audio
    void onFootstep(const glm::vec3& position, Audio::SurfaceMaterial surface, 
                    float movementSpeed, bool isLocalPlayer = false);
    void onJump(const glm::vec3& position, bool isLocalPlayer = false);
    void onLand(const glm::vec3& position, float impactForce, bool isLocalPlayer = false);
    
    // Environmental audio
    void setAudioZone(const AudioZone& zone);
    void clearAudioZone();
    void setGlobalReverb(const Audio::ReverbSettings& reverb);
    
    // Volume controls
    void setMasterVolume(float volume);
    void setCategoryVolume(Audio::AudioCategory category, float volume);
    float getMasterVolume() const { return m_Listener.masterVolume; }
    float getCategoryVolume(Audio::AudioCategory category) const;
    
    // 3D audio settings
    void setDopplerFactor(float factor);
    void setSpeedOfSound(float speed);
    void setDistanceModel(const std::string& model = "linear");
    
    // Occlusion/obstruction (for walls blocking sound)
    void updateOcclusion(int sourceId, float occlusionLevel, float obstructionLevel);
    bool performRaycastForOcclusion(const glm::vec3& from, const glm::vec3& to) const;
    
    // Debug and metrics
    const AudioMetrics& getMetrics() const { return m_Metrics; }
    void printDebugInfo() const;
    void toggleDebugVisualization() { m_DebugVisualization = !m_DebugVisualization; }
    
    // Advanced features
    void setLowPassFilter(int sourceId, float frequency);  // Muffle distant sounds
    void setHighPassFilter(int sourceId, float frequency); // Remove low frequencies
    void setEchoEffect(int sourceId, float delay, float feedback);
    
private:
    // Core audio management
    void initializeOpenAL();
    void shutdownOpenAL();
    void cleanupFinishedSources();
    void updateSourcesPosition();
    void updateEnvironmentalEffects();
    
    // Sound loading helpers
    std::shared_ptr<AudioClip> loadAudioFile(const std::string& filePath);
    bool loadWAVFile(const std::string& filePath, AudioClip& clip);
    bool loadOGGFile(const std::string& filePath, AudioClip& clip);
    
    // 3D audio calculations
    float calculateDistance(const glm::vec3& sourcePos, const glm::vec3& listenerPos) const;
    float calculateVolume(const AudioSource& source) const;
    float calculatePanning(const glm::vec3& sourcePos) const;
    void applyDopplerEffect(AudioSource& source);
    void applyOcclusion(AudioSource& source);
    
    // Source management
    int createAudioSource();
    void releaseAudioSource(int sourceId);
    AudioSource* getAudioSource(int sourceId);
    void limitActiveSources(); // Enforce maximum source limits
    
    // Threading for audio streaming
    void audioThreadFunc();
    void processAudioQueue();
    
    // Sound banks and variation
    std::string selectSoundFromBank(const std::string& bankName);
    void addPitchVariation(AudioSource& source, float variation = 0.1f);
    void scheduleDelayedSound(const std::string& soundName, const glm::vec3& position, 
                             float delay, float volume);
    
    // Setup helpers
    void setupEventMappings();
    void loadDefaultSounds();
    void registerDummySound(const std::string& name, float duration);
    void updateMetrics();
    void updateCurrentAudioZone();

private:
    // OpenAL context
    void* m_ALDevice = nullptr;     // ALCdevice*
    void* m_ALContext = nullptr;    // ALCcontext*
    
    // Audio data
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> m_AudioClips;
    std::unordered_map<std::string, SoundBank> m_SoundBanks;
    std::unordered_map<int, std::unique_ptr<AudioSource>> m_ActiveSources;
    
    // Event mapping
    std::unordered_map<Audio::AudioEvent, std::string> m_EventToSound;
    
    // Listener and environment
    AudioListener m_Listener;
    std::vector<AudioZone> m_AudioZones;
    AudioZone* m_CurrentZone = nullptr;
    
    // Threading
    std::thread m_AudioThread;
    std::mutex m_AudioMutex;
    std::queue<std::function<void()>> m_AudioQueue;
    bool m_ThreadRunning = false;
    
    // Source management
    int m_NextSourceId = 1;
    static const int MAX_SOURCES = 64;      // Limit concurrent sounds
    static const int MAX_PRIORITY_SOURCES = 16; // Reserved for critical sounds
    
    // Performance tracking
    AudioMetrics m_Metrics;
    float m_MetricsTimer = 0.0f;
    
    // Settings
    bool m_Initialized = false;
    bool m_DebugVisualization = false;
    float m_MasterVolume = 1.0f;
    
    // Footstep system
    Audio::FootstepSettings m_FootstepSettings;
    float m_LastFootstepTime = 0.0f;
    float m_FootstepTimer = 0.0f;
    
    // External references (optional)
    FPSCamera* m_Camera = nullptr;
    PlayerController* m_Player = nullptr;
    WeaponSystem* m_Weapons = nullptr;
};

// Audio utility functions
namespace AudioUtils {
    // Convert between different units
    float decibelToLinear(float db);
    float linearToDecibel(float linear);
    
    // Distance calculations
    float calculateRolloff(float distance, float minDist, float maxDist, float rolloffFactor);
    
    // Audio file format detection
    std::string getAudioFormat(const std::string& filePath);
    
    // Generate procedural audio (for simple effects)
    std::vector<float> generateWhiteNoise(float duration, int sampleRate);
    std::vector<float> generateSineWave(float frequency, float duration, int sampleRate);
    
    // Audio processing
    void applyLowPassFilter(std::vector<float>& samples, float cutoffFreq, int sampleRate);
    void applyHighPassFilter(std::vector<float>& samples, float cutoffFreq, int sampleRate);
    void normalizeAudio(std::vector<float>& samples);
    
    // Surface material to sound mapping
    std::string getSurfaceSoundName(Audio::SurfaceMaterial material, const std::string& action);
}
