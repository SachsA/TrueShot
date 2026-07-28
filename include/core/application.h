#pragma once

#include "net/net_metrics.h"

#include <cstdint>
#include <memory>
#include <string>

struct GLFWwindow;

class FPSCamera;
class PlayerController;
class WeaponSystem;
class AudioSystem;
class Renderer;
class GameWorld;
class Hud;
class NetworkClient;
namespace Net {
class RemotePlayerRegistry;
class ClientPrediction;
} // namespace Net

// Configuration consumed by Application::init(). Lets a caller (main.cpp)
// pick the launch mode without baking it into the class. Phase 1.4 only
// implements `Offline` (the existing solo practice) and `Client` (connect
// to a remote server). Listen-server lands in Phase 1.5.
struct AppConfig {
    enum class Mode {
        Offline = 0, // Single-player practice range (default).
        Client,      // Join a remote server at `serverHost:serverPort`.
    };

    int width              = 1280;
    int height             = 720;
    std::string title      = "TrueShot - Tactical FPS";
    Mode mode              = Mode::Offline;
    std::string serverHost = "127.0.0.1";
    uint16_t serverPort    = 7777;
};

// Top-level owner of every subsystem. Replaces the previous global
// state in main.cpp. Lifetime: construct, init(), run(), then destruct.
class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // Create the GLFW window, GL context, audio, world, weapons, and
    // (optionally) the network client.
    // Returns false on any unrecoverable init failure.
    bool init(const AppConfig& config);

    // Convenience overload kept for backwards compat — runs offline.
    bool init(int width, int height, const char* title);

    // Run the main loop until the user closes the window.
    int run();

    // Mouse callback dispatcher (set on the GLFWwindow).
    static void mouseCallbackThunk(GLFWwindow* w, double x, double y);
    static void resizeCallbackThunk(GLFWwindow* w, int width, int height);

private:
    void processInput(float deltaTime);
    void onMouseMove(double xpos, double ypos);
    void onResize(int width, int height);
    void printControls() const;
    void printDebugInfo();

    GLFWwindow* m_Window = nullptr;

    std::unique_ptr<FPSCamera> m_Camera;
    std::unique_ptr<PlayerController> m_Player;
    std::unique_ptr<WeaponSystem> m_Weapons;
    std::unique_ptr<AudioSystem> m_Audio;
    std::unique_ptr<GameWorld> m_World;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<Hud> m_Hud;
    // All three null in offline mode; populated when AppConfig::Mode == Client.
    std::unique_ptr<NetworkClient> m_Net;
    std::unique_ptr<Net::RemotePlayerRegistry> m_Remotes;
    std::unique_ptr<Net::ClientPrediction> m_Prediction;

    // Network metrics aggregator — driven from the main loop, displayed
    // by the HUD net panel (F2). Cheap default-constructible POD.
    Net::NetMetrics m_NetMetrics{};
    Net::NetMetricsSampler m_NetSampler;

    // F2 edge-trigger for net panel toggle.
    bool m_F2Prev = false;

    // Mouse state
    bool m_FirstMouse  = true;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;

    // Debug / timing
    float m_DebugTimer    = 0.0f;
    float m_DebugInterval = 2.0f;

    // Edge-trigger storage for F1 (toggle HUD)
    bool m_F1Prev = false;

    // Cached settings
    int m_Width        = 1280;
    int m_Height       = 720;
    bool m_Initialised = false;
};
