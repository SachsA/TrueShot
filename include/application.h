#pragma once

#include <memory>

struct GLFWwindow;

class FPSCamera;
class PlayerController;
class WeaponSystem;
class AudioSystem;
class Renderer;
class GameWorld;
class Hud;

// Top-level owner of every subsystem. Replaces the previous global
// state in main.cpp. Lifetime: construct, init(), run(), then destruct.
class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // Create the GLFW window, GL context, audio, world, weapons.
    // Returns false on any unrecoverable init failure.
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
