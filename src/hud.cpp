// glad must come before any other GL/window header.
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include "game_world.h"
#include "hud.h"
#include "net/net_metrics.h"
#include "player_controller.h"
#include "weapon_system.h"
#include "weapon_types.h"
// vcpkg installs the ImGui GLFW/OpenGL3 backends directly into the include
// root (not under "backends/"), so include them by their flat name.
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {

// Resolve the GLSL version string for the ImGui OpenGL3 backend.
// We target OpenGL 3.3 Core, which maps to "#version 330 core".
constexpr const char* kImGuiGlslVersion = "#version 330 core";

// Returns the colour for an ammo bar based on how full the magazine is.
ImU32 ammoColor(float fraction) {
    if (fraction > 0.5f) return IM_COL32(120, 230, 120, 220); // green
    if (fraction > 0.2f) return IM_COL32(230, 200, 90, 220);  // amber
    return IM_COL32(230, 90, 90, 220);                        // red
}

} // namespace

Hud::Hud() = default;
Hud::~Hud() {
    shutdown();
}

bool Hud::initialize(GLFWwindow* window) {
    if (m_Initialised) return true;
    if (!window) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // don't write imgui.ini next to the binary

    ImGui::StyleColorsDark();
    ImGuiStyle& style      = ImGui::GetStyle();
    style.WindowRounding   = 6.0f;
    style.FrameRounding    = 4.0f;
    style.WindowBorderSize = 0.0f;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "[HUD] ImGui_ImplGlfw_InitForOpenGL failed\n";
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(kImGuiGlslVersion)) {
        std::cerr << "[HUD] ImGui_ImplOpenGL3_Init failed\n";
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_Initialised = true;
    return true;
}

void Hud::shutdown() {
    if (!m_Initialised) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_Initialised = false;
}

void Hud::beginFrame() {
    if (!m_Initialised) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Hud::render(const GameWorld& world, const WeaponSystem& weapons,
                 const PlayerController& player, float deltaTime,
                 const Net::NetMetrics* netMetrics) {
    if (!m_Initialised) return;
    if (m_Visible) {
        drawStatsPanel(world, weapons, player, deltaTime);
    }
    if (m_NetPanelVisible && netMetrics) {
        drawNetPanel(*netMetrics);
    }
    drawHitMarker(deltaTime);
}

void Hud::onHit(bool headshot, bool killed) {
    // Restart the marker animation. A "kill" overrides any earlier flag,
    // a "headshot" overrides a body shot, and a body shot only sticks
    // if nothing better is already showing.
    constexpr float kMarkerDuration = 0.35f;
    m_HitMarkerTimer                = kMarkerDuration;

    if (killed) {
        m_LastHitWasKill = true;
        m_LastHitWasHead = headshot;
    } else if (headshot) {
        m_LastHitWasHead = true;
        m_LastHitWasKill = false;
    } else {
        // Don't downgrade an existing head/kill flag mid-animation.
        if (!m_LastHitWasHead && !m_LastHitWasKill) {
            // body shot — no special flag
        }
    }
}

void Hud::drawHitMarker(float deltaTime) {
    if (m_HitMarkerTimer <= 0.0f) return;

    m_HitMarkerTimer -= deltaTime;
    if (m_HitMarkerTimer <= 0.0f) {
        m_HitMarkerTimer = 0.0f;
        m_LastHitWasHead = false;
        m_LastHitWasKill = false;
        return;
    }

    // Fade from full opacity at the start to zero by the end.
    constexpr float kMarkerDuration = 0.35f;
    const float t                   = std::clamp(m_HitMarkerTimer / kMarkerDuration, 0.0f, 1.0f);
    const float alpha               = t; // linear fade

    // Pick colour: red kill > yellow head > white body.
    int cr = 255, cg = 255, cb = 255;
    if (m_LastHitWasKill) {
        cr = 230;
        cg = 60;
        cb = 60;
    } else if (m_LastHitWasHead) {
        cr = 245;
        cg = 210;
        cb = 70;
    }
    const ImU32 colour = IM_COL32(cr, cg, cb, int(alpha * 230.0f));

    // Marker grows briefly on the first frame, then settles — a small
    // "punch" feel without needing a dedicated animation curve.
    const float growth      = 1.0f + (1.0f - t) * 0.25f;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 centre(vp->WorkPos.x + vp->WorkSize.x * 0.5f,
                        vp->WorkPos.y + vp->WorkSize.y * 0.5f);

    // Four diagonal ticks around the centre. A short gap leaves the
    // crosshair visible underneath.
    const float gap    = 6.0f;
    const float length = 10.0f * growth;
    const float thick  = m_LastHitWasKill ? 3.0f : 2.0f;

    ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->WorkSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##hitmarker", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Top-left, top-right, bottom-left, bottom-right ticks.
    dl->AddLine(ImVec2(centre.x - gap - length, centre.y - gap - length),
                ImVec2(centre.x - gap, centre.y - gap), colour, thick);
    dl->AddLine(ImVec2(centre.x + gap + length, centre.y - gap - length),
                ImVec2(centre.x + gap, centre.y - gap), colour, thick);
    dl->AddLine(ImVec2(centre.x - gap - length, centre.y + gap + length),
                ImVec2(centre.x - gap, centre.y + gap), colour, thick);
    dl->AddLine(ImVec2(centre.x + gap + length, centre.y + gap + length),
                ImVec2(centre.x + gap, centre.y + gap), colour, thick);

    ImGui::End();
}

void Hud::endFrame() {
    if (!m_Initialised) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Hud::drawStatsPanel(const GameWorld& world, const WeaponSystem& weapons,
                         const PlayerController& player, float deltaTime) {
    // ---- FPS (exponential moving average) -----------------------------
    if (deltaTime > 0.0f) {
        const float instantFps = 1.0f / deltaTime;
        // 0.1 weight on the new sample = ~10-frame smoothing window.
        m_SmoothedFps =
            (m_SmoothedFps <= 0.0f) ? instantFps : (m_SmoothedFps * 0.9f + instantFps * 0.1f);
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();

    // -------------------------------------------------------------------
    // Top-left: scoreboard (score / accuracy / kills).
    // -------------------------------------------------------------------
    {
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 16.0f, vp->WorkPos.y + 16.0f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##scoreboard", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoInputs);

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "SCORE  %d", world.score());
        ImGui::Separator();
        ImGui::Text("Kills     %d", world.kills());
        ImGui::Text("Hits      %d / %d", world.hits(), world.shots());
        ImGui::Text("Accuracy  %.1f%%", world.accuracy() * 100.0f);
        ImGui::Text("FPS       %.0f", m_SmoothedFps);
        ImGui::End();
    }

    // -------------------------------------------------------------------
    // Bottom-right: weapon + ammo, with a coloured progress bar.
    // -------------------------------------------------------------------
    if (const auto* w = weapons.getCurrentWeapon()) {
        const WeaponState& ws = weapons.getWeaponState();
        const int magSize     = std::max(1, w->stats.magazineSize);
        const float fraction  = std::clamp(float(ws.currentAmmo) / float(magSize), 0.0f, 1.0f);

        const ImVec2 panelSize(260.0f, 96.0f);
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelSize.x - 16.0f,
                                       vp->WorkPos.y + vp->WorkSize.y - panelSize.y - 16.0f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##weaponpanel", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

        ImGui::TextColored(ImVec4(0.85f, 0.95f, 1.0f, 1.0f), "%s", w->name.c_str());

        char ammoText[64];
        std::snprintf(ammoText, sizeof(ammoText), "%d / %d", ws.currentAmmo, ws.reserveAmmo);

        // Push a colour onto the progress bar that reflects ammo level.
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImColor(ammoColor(fraction)).Value);
        ImGui::ProgressBar(fraction, ImVec2(-1.0f, 14.0f), ammoText);
        ImGui::PopStyleColor();

        if (weapons.isReloading()) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "RELOADING...");
        } else if (weapons.isAiming()) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "ADS");
        } else {
            ImGui::Text("Spread  %.2f deg", weapons.getCurrentSpread().x);
        }

        ImGui::End();
    }

    // -------------------------------------------------------------------
    // Bottom-left: movement readout (speed / bhop combo / on-ground).
    // -------------------------------------------------------------------
    {
        const MovementState& mv = player.getMovementState();

        const ImVec2 panelSize(220.0f, 96.0f);
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + 16.0f, vp->WorkPos.y + vp->WorkSize.y - panelSize.y - 16.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##movement", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

        ImGui::Text("Speed   %4d u/s", int(mv.speed));
        ImGui::Text("Bhop    x%d", mv.consecutiveHops);
        ImGui::Text("Ground  %s", mv.onGround ? "yes" : "no");
        if (player.isCrouching()) {
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "CROUCH");
        }
        ImGui::End();
    }
}

void Hud::drawNetPanel(const Net::NetMetrics& m) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 panelSize(260.0f, 200.0f);

    // Top-right corner, below the standard HUD's ammo/scoreboard zones.
    ImGui::SetNextWindowPos(
        ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelSize.x - 16.0f, vp->WorkPos.y + 16.0f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.65f);
    ImGui::Begin("##netpanel", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

    // ---- Header + connection state -----------------------------------
    const char* stateName = "?";
    ImVec4 stateColour(1.0f, 1.0f, 1.0f, 1.0f);
    switch (m.state) {
    case 0:
        stateName   = "DISCONNECTED";
        stateColour = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        break;
    case 1:
        stateName   = "CONNECTING";
        stateColour = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
        break;
    case 2:
        stateName   = "CONNECTED";
        stateColour = ImVec4(0.4f, 0.95f, 0.5f, 1.0f);
        break;
    case 3:
        stateName   = "FAILED";
        stateColour = ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
        break;
    default:
        break;
    }
    ImGui::TextColored(ImVec4(0.85f, 0.95f, 1.0f, 1.0f), "NETWORK");
    ImGui::SameLine();
    ImGui::TextColored(stateColour, "%s", stateName);
    ImGui::Separator();

    // ---- Latency + tick -----------------------------------------------
    // Colour RTT to give a glanceable health indicator.
    ImVec4 rttColour(0.4f, 0.95f, 0.5f, 1.0f);
    if (m.rttMs > 100) rttColour = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
    if (m.rttMs > 200) rttColour = ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
    ImGui::TextColored(rttColour, "RTT       %u ms", m.rttMs);

    ImGui::Text("Tick      L%u / S%u", m.localTick, m.serverTick);
    ImGui::Text("Player    id=%u", m.localId);

    // ---- Bandwidth ---------------------------------------------------
    ImGui::Text("Up        %.1f KB/s  (%.0f pkt)", m.bytesSentPerSec / 1024.0f,
                static_cast<float>(m.packetsSent));
    ImGui::Text("Down      %.1f KB/s  (%.0f pkt)", m.bytesRecvPerSec / 1024.0f,
                static_cast<float>(m.packetsRecv));
    ImGui::Text("Snaps     %.1f /s", m.snapshotsPerSec);

    // ---- Prediction health ------------------------------------------
    ImVec4 corrColour(0.5f, 0.95f, 0.5f, 1.0f);
    if (m.lastCorrectionMeters > 0.05f) corrColour = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
    if (m.lastCorrectionMeters > 0.50f) corrColour = ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
    ImGui::TextColored(corrColour, "LastCorr  %.3f m", m.lastCorrectionMeters);
    ImGui::Text("Pending   %u in flight", m.pendingInputs);
    ImGui::Text("Remotes   %u", m.remotePlayerCount);

    ImGui::End();
}
