#pragma once

struct GLFWwindow;

class GameWorld;
class WeaponSystem;
class PlayerController;

// In-game HUD built on Dear ImGui. Renders score, ammo, accuracy, HP
// and FPS as a non-interactive overlay above the 3D scene.
//
// Lifecycle is straightforward:
//   Hud hud;
//   hud.initialize(window);
//   while (running) {
//       hud.beginFrame();
//       // 3D rendering ...
//       hud.render(world, weapons, player, deltaTime);
//       hud.endFrame();
//       glfwSwapBuffers(window);
//   }
//   hud.shutdown();
class Hud {
public:
    Hud();
    ~Hud();

    Hud(const Hud&)            = delete;
    Hud& operator=(const Hud&) = delete;

    // Set up ImGui + GLFW/OpenGL3 backends. Returns false on failure.
    bool initialize(GLFWwindow* window);

    // Tear down ImGui. Safe to call even if initialize() was never called.
    void shutdown();

    // Start a new ImGui frame. Call before any other HUD code each frame.
    void beginFrame();

    // Build the overlay widgets for this frame. Reads stats from the
    // arguments — does not mutate them.
    void render(const GameWorld& world, const WeaponSystem& weapons, const PlayerController& player,
                float deltaTime);

    // Flush ImGui draw data to the current GL context. Call after the
    // 3D scene has been rendered but before swapping buffers.
    void endFrame();

    void setVisible(bool visible) { m_Visible = visible; }
    bool isVisible() const { return m_Visible; }
    void toggleVisible() { m_Visible = !m_Visible; }

    // Notify the HUD that the player just landed a hit.
    // Triggers the on-screen hit marker (coloured by severity):
    //   - white   = body shot
    //   - yellow  = headshot
    //   - red     = killing blow (overrides the above)
    void onHit(bool headshot, bool killed);

private:
    void drawStatsPanel(const GameWorld& world, const WeaponSystem& weapons,
                        const PlayerController& player, float deltaTime);

    void drawHitMarker(float deltaTime);

    bool m_Initialised = false;
    bool m_Visible     = true;

    // Smoothed FPS (low-pass) so the number doesn't flicker each frame.
    float m_SmoothedFps = 0.0f;

    // Hit marker state (counts down once a hit is registered).
    float m_HitMarkerTimer = 0.0f; // seconds remaining of fade-out
    bool m_LastHitWasHead  = false;
    bool m_LastHitWasKill  = false;
};
