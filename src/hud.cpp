// glad must come before any other GL/window header.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "hud.h"

#include "game_world.h"
#include "weapon_system.h"
#include "weapon_types.h"
#include "player_controller.h"

#include <imgui.h>
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
    if (fraction > 0.5f) return IM_COL32(120, 230, 120, 220);  // green
    if (fraction > 0.2f) return IM_COL32(230, 200,  90, 220);  // amber
    return                IM_COL32(230,  90,  90, 220);        // red
}

} // namespace

Hud::Hud()  = default;
Hud::~Hud() { shutdown(); }

bool Hud::initialize(GLFWwindow* window) {
    if (m_Initialised) return true;
    if (!window) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = nullptr;  // don't write imgui.ini next to the binary

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
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

void Hud::render(const GameWorld&        world,
                 const WeaponSystem&     weapons,
                 const PlayerController& player,
                 float                   deltaTime) {
    if (!m_Initialised || !m_Visible) return;
    drawStatsPanel(world, weapons, player, deltaTime);
}

void Hud::endFrame() {
    if (!m_Initialised) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Hud::drawStatsPanel(const GameWorld&        world,
                         const WeaponSystem&     weapons,
                         const PlayerController& player,
                         float                   deltaTime) {
    // ---- FPS (exponential moving average) -----------------------------
    if (deltaTime > 0.0f) {
        const float instantFps = 1.0f / deltaTime;
        // 0.1 weight on the new sample = ~10-frame smoothing window.
        m_SmoothedFps = (m_SmoothedFps <= 0.0f)
            ? instantFps
            : (m_SmoothedFps * 0.9f + instantFps * 0.1f);
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();

    // -------------------------------------------------------------------
    // Top-left: scoreboard (score / accuracy / kills).
    // -------------------------------------------------------------------
    {
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 16.0f, vp->WorkPos.y + 16.0f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##scoreboard",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoInputs);

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f),
                           "SCORE  %d", world.score());
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
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelSize.x - 16.0f,
                   vp->WorkPos.y + vp->WorkSize.y - panelSize.y - 16.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##weaponpanel",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoInputs);

        ImGui::TextColored(ImVec4(0.85f, 0.95f, 1.0f, 1.0f), "%s", w->name.c_str());

        char ammoText[64];
        std::snprintf(ammoText, sizeof(ammoText),
                      "%d / %d", ws.currentAmmo, ws.reserveAmmo);

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

        const ImVec2 panelSize(220.0f, 72.0f);
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos.x + 16.0f,
                   vp->WorkPos.y + vp->WorkSize.y - panelSize.y - 16.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##movement",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoInputs);

        ImGui::Text("Speed   %4d u/s", int(mv.speed));
        ImGui::Text("Bhop    x%d", mv.consecutiveHops);
        ImGui::Text("Ground  %s", mv.onGround ? "yes" : "no");
        ImGui::End();
    }
}
