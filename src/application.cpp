// glad must come before GLFW (and any header pulling system OpenGL headers).
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include "application.h"
#include "audio_system.h"
#include "fps_camera.h"
#include "game_world.h"
#include "hud.h"
#include "physics_types.h"
#include "player_controller.h"
#include "renderer.h"
#include "weapon_system.h"

#include "Network/NetCommon.h"
#include "net/network_client.h"
#include "net/remote_player.h"
#include <algorithm>
#include <iostream>

Application::Application() = default;
Application::~Application() {
    if (m_Net) m_Net->shutdown();
    if (m_Hud) m_Hud->shutdown();
    if (m_Audio) m_Audio->shutdown();
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    if (m_Initialised) {
        glfwTerminate();
    }
}

void Application::mouseCallbackThunk(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (self) self->onMouseMove(x, y);
}

void Application::resizeCallbackThunk(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (self) self->onResize(width, height);
}

bool Application::init(int width, int height, const char* title) {
    AppConfig cfg;
    cfg.width  = width;
    cfg.height = height;
    cfg.title  = title ? title : "TrueShot";
    return init(cfg);
}

bool Application::init(const AppConfig& config) {
    m_Width  = config.width;
    m_Height = config.height;

    if (!glfwInit()) {
        std::cerr << "[App] Failed to init GLFW\n";
        return false;
    }
    m_Initialised = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_Window = glfwCreateWindow(m_Width, m_Height, config.title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        std::cerr << "[App] Failed to create GLFW window\n";
        return false;
    }
    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetCursorPosCallback(m_Window, &Application::mouseCallbackThunk);
    glfwSetFramebufferSizeCallback(m_Window, &Application::resizeCallbackThunk);
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[App] Failed to init GLAD\n";
        return false;
    }

    // Build the simulation graph.
    m_Camera   = std::make_unique<FPSCamera>(glm::vec3(0.0f, Physics::PLAYER_HEIGHT, 3.0f));
    m_Player   = std::make_unique<PlayerController>(m_Camera.get());
    m_Weapons  = std::make_unique<WeaponSystem>(m_Camera.get(), m_Player.get());
    m_Audio    = std::make_unique<AudioSystem>();
    m_World    = std::make_unique<GameWorld>();
    m_Renderer = std::make_unique<Renderer>();

    if (!m_Audio->initialize()) {
        std::cerr << "[App] Audio init failed (continuing without sound)\n";
    }

    m_Weapons->setAudioSystem(m_Audio.get());
    m_Weapons->setGameWorld(m_World.get());
    m_Player->setAudioSystem(m_Audio.get());

    int fbW = m_Width, fbH = m_Height;
    glfwGetFramebufferSize(m_Window, &fbW, &fbH);
    if (!m_Renderer->initialize(fbW, fbH)) {
        std::cerr << "[App] Renderer init failed\n";
        return false;
    }

    // HUD goes last — it relies on a live GL context and window.
    m_Hud = std::make_unique<Hud>();
    if (!m_Hud->initialize(m_Window)) {
        std::cerr << "[App] HUD init failed (continuing without HUD)\n";
        m_Hud.reset();
    }

    // Let the weapon system trigger hit-marker feedback through the HUD.
    if (m_Hud) m_Weapons->setHud(m_Hud.get());

    // ----- Optional networking -----
    if (config.mode == AppConfig::Mode::Client) {
        m_Net = std::make_unique<NetworkClient>();
        if (!m_Net->initialize()) {
            std::cerr << "[App] NetworkClient init failed — falling back to offline\n";
            m_Net.reset();
        } else if (!m_Net->connectTo(config.serverHost, config.serverPort)) {
            std::cerr << "[App] connectTo() failed — falling back to offline\n";
            m_Net.reset();
        } else {
            std::cout << "[App] Network mode = Client → " << config.serverHost << ':'
                      << config.serverPort << '\n';
            m_Remotes = std::make_unique<Net::RemotePlayerRegistry>();
        }
    }

    printControls();
    return true;
}

void Application::printControls() const {
    std::cout << "==============================\n"
                 "  TRUESHOT - Tactical FPS\n"
                 "==============================\n"
                 "MOVEMENT\n"
                 "  WASD       Move (strafe-jump for speed!)\n"
                 "  SPACE      Jump / Bhop\n"
                 "  CTRL / C   Crouch (slower, lower spread)\n"
                 "  Mouse      Look around\n"
                 "WEAPONS\n"
                 "  Mouse1     Fire\n"
                 "  Mouse2     Aim Down Sights (ADS)\n"
                 "  R          Reload\n"
                 "  1..5       Glock / Deagle / AK / M4 / AWP\n"
                 "AUDIO\n"
                 "  + / -      Master volume\n"
                 "  M          Toggle audio debug\n"
                 "MISC\n"
                 "  F1         Toggle HUD\n"
                 "  ESC        Quit\n"
                 "==============================\n\n";
}

void Application::onMouseMove(double xpos, double ypos) {
    if (m_FirstMouse) {
        m_LastMouseX = float(xpos);
        m_LastMouseY = float(ypos);
        m_FirstMouse = false;
        return;
    }
    const float xoffset = float(xpos) - m_LastMouseX;
    const float yoffset = m_LastMouseY - float(ypos); // y inverted

    m_LastMouseX        = float(xpos);
    m_LastMouseY        = float(ypos);

    if (m_Player) m_Player->processMouseInput(xoffset, yoffset);
}

void Application::onResize(int width, int height) {
    if (m_Renderer) m_Renderer->onResize(width, height);
    m_Width  = width;
    m_Height = height;
}

void Application::processInput(float deltaTime) {
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_Window, true);
    }

    if (m_Player) m_Player->processInput(m_Window, deltaTime);
    if (m_Weapons) m_Weapons->processInput(m_Window, deltaTime);

    // HUD toggle (F1, edge-triggered).
    const bool f1Now = glfwGetKey(m_Window, GLFW_KEY_F1) == GLFW_PRESS;
    if (m_Hud && f1Now && !m_F1Prev) {
        m_Hud->toggleVisible();
    }
    m_F1Prev = f1Now;

    // Audio volume + debug toggles (edge-triggered).
    if (!m_Audio) return;
    static bool plusPrev = false, minusPrev = false, mPrev = false;

    const bool plusNow = glfwGetKey(m_Window, GLFW_KEY_KP_ADD) == GLFW_PRESS ||
                         glfwGetKey(m_Window, GLFW_KEY_EQUAL) == GLFW_PRESS;
    const bool minusNow = glfwGetKey(m_Window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
                          glfwGetKey(m_Window, GLFW_KEY_MINUS) == GLFW_PRESS;
    const bool mNow = glfwGetKey(m_Window, GLFW_KEY_M) == GLFW_PRESS;

    if (plusNow && !plusPrev) {
        const float v = std::min(1.0f, m_Audio->getMasterVolume() + 0.1f);
        m_Audio->setMasterVolume(v);
        std::cout << "Master volume: " << int(v * 100) << "%\n";
    }
    if (minusNow && !minusPrev) {
        const float v = std::max(0.0f, m_Audio->getMasterVolume() - 0.1f);
        m_Audio->setMasterVolume(v);
        std::cout << "Master volume: " << int(v * 100) << "%\n";
    }
    if (mNow && !mPrev) {
        m_Audio->toggleDebugVisualization();
    }
    plusPrev  = plusNow;
    minusPrev = minusNow;
    mPrev     = mNow;
}

void Application::printDebugInfo() {
    if (m_DebugTimer < m_DebugInterval) return;
    m_DebugTimer = 0.0f;

    if (!m_Player || !m_World) return;

    const auto& mv = m_Player->getMovementState();

    std::cout << "\n=== TRUESHOT DEBUG ===\n";
    std::cout << "MOVEMENT  speed=" << int(mv.speed) << "  max=" << int(mv.maxSpeed)
              << "  bhop=" << mv.consecutiveHops << "  ground=" << (mv.onGround ? "Y" : "N")
              << '\n';

    if (m_Weapons && m_Weapons->getCurrentWeapon()) {
        const auto* w  = m_Weapons->getCurrentWeapon();
        const auto& ws = m_Weapons->getWeaponState();
        std::cout << "WEAPON    " << w->name << "  " << ws.currentAmmo << "/" << ws.reserveAmmo
                  << "  spread=" << m_Weapons->getCurrentSpread().x << "°\n";
    }

    std::cout << "SCORE     " << m_World->score() << "  hits=" << m_World->hits() << "/"
              << m_World->shots() << "  acc=" << int(m_World->accuracy() * 100.0f) << '%'
              << "  kills=" << m_World->kills() << '\n';
    std::cout << "======================\n\n";
}

int Application::run() {
    if (!m_Window) return 1;

    float lastFrame    = float(glfwGetTime());
    uint32_t localTick = 0;
    uint32_t inputSeq  = 0;
    double netAccum    = 0.0;

    while (!glfwWindowShouldClose(m_Window)) {
        const float currentFrame = float(glfwGetTime());
        const float deltaTime    = std::min(0.1f, currentFrame - lastFrame); // clamp big stalls
        lastFrame                = currentFrame;
        m_DebugTimer += deltaTime;

        processInput(deltaTime);

        if (m_Player) m_Player->update(deltaTime);
        if (m_Weapons) m_Weapons->update(deltaTime);
        if (m_World) m_World->update(deltaTime);
        if (m_Audio) {
            m_Audio->update(deltaTime);
            m_Audio->setListenerFromCamera(m_Camera.get(), m_Player.get());
        }

        // ----- Network step -----
        // We send one ClientInput per simulation tick (128 Hz). The frame
        // rate is independent — we use an accumulator over the network
        // tick interval so a 200 FPS client sends 128 packets/s, not 200.
        if (m_Net) {
            m_Net->tick();
            netAccum += static_cast<double>(deltaTime);
            const double netStep = static_cast<double>(Physics::FIXED_TIMESTEP);
            while (netAccum >= netStep) {
                netAccum -= netStep;
                ++localTick;
                ++inputSeq;
                if (m_Player && m_Camera) {
                    Net::InputState in;
                    in.tick         = localTick;
                    in.seq          = inputSeq;
                    in.moveForward  = 0; // wired up properly in Phase 1.7
                    in.moveRight    = 0;
                    in.yaw          = m_Camera->getYaw();
                    in.pitch        = m_Camera->getPitch();
                    in.buttons      = 0;
                    in.clientPingMs = static_cast<uint16_t>(m_Net->roundTripMs());
                    m_Net->sendInput(in);
                }
            }
            // Drain incoming snapshots into the RemotePlayer registry, which
            // builds a 100 ms interpolated view of every other player.
            Net::Snapshot snap;
            while (m_Net->popSnapshot(snap)) {
                if (m_Remotes) {
                    m_Remotes->ingestSnapshot(snap, static_cast<double>(currentFrame),
                                              m_Net->localId());
                }
            }
        }

        printDebugInfo();

        // ImGui requires its NewFrame() call BEFORE the rest of the frame
        // produces any draw commands that affect GL state we share.
        if (m_Hud) m_Hud->beginFrame();

        if (m_Renderer && m_Camera && m_World && m_Weapons && m_Player) {
            m_Renderer->render(*m_Camera, *m_World, *m_Weapons, *m_Player, currentFrame,
                               m_Remotes.get());
        }

        // HUD is drawn after the 3D scene so it composites on top.
        if (m_Hud && m_World && m_Weapons && m_Player) {
            m_Hud->render(*m_World, *m_Weapons, *m_Player, deltaTime);
            m_Hud->endFrame();
        }

        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
    return 0;
}
