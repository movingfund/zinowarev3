#include "aimbot.hpp"
#include "imgui.h"
#include <cmath>
#include <algorithm>

Aimbot::Aimbot(std::shared_ptr<RobloxSDK> sdk) : m_sdk(std::move(sdk)) {}

std::optional<Vector2> Aimbot::FindTarget(const std::vector<PlayerData>& players,
                                          const Matrix4x4& vm, const Vector2& vp,
                                          const Vector3& cameraPos) {
    if (players.empty() || !m_cfg.enabled) return std::nullopt;

    int width  = static_cast<int>(vp.x);
    int height = static_cast<int>(vp.y);
    Vector2 screenCenter(width * 0.5f, height * 0.5f);

    float fovRad  = m_cfg.fov * 3.14159265f / 180.0f;
    float maxDist = std::tan(fovRad) * (vp.x * 0.5f);

    float   bestFov = maxDist;
    Vector2 bestScreen{};
    bool    found = false;

    for (const auto& p : players) {
        if (!p.isAlive || p.isLocal) continue;

        Vector3 aimPos = (m_cfg.aimPart == AimPart::Head) ? p.headPos : p.position;
        Vector2 screen;
        if (!vm.WorldToScreen(aimPos, screen, width, height)) continue;

        float dist = screenCenter.Dist(screen);
        if (dist < bestFov) {
            bestFov    = dist;
            bestScreen = screen;
            found      = true;
        }
    }
    return found ? std::optional(bestScreen) : std::nullopt;
}

void Aimbot::DoAimbot(const std::vector<PlayerData>& players,
                      const Matrix4x4& vm, const Vector2& vp,
                      const Vector3& cameraPos) {
    if (!m_cfg.enabled || m_cfg.silentAim) return;

    bool shouldAim = true;
    if (m_cfg.aimOnFire)
        shouldAim = (GetAsyncKeyState(m_cfg.aimKey) & 0x8000) != 0;

    if (!shouldAim) { m_hadTarget = false; return; }

    auto target = FindTarget(players, vm, vp, cameraPos);
    if (!target) { m_hadTarget = false; return; }

    Vector2 targetPos = *target;

    if (m_hadTarget && m_cfg.smooth > 0.5f) {
        float t = 1.0f / m_cfg.smooth;
        targetPos.x = m_lastAimPos.x + (targetPos.x - m_lastAimPos.x) * t;
        targetPos.y = m_lastAimPos.y + (targetPos.y - m_lastAimPos.y) * t;
    }

    m_lastAimPos = targetPos;
    m_hadTarget  = true;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    double dx = targetPos.x - screenW * 0.5;
    double dy = targetPos.y - screenH * 0.5;

    double normX = dx * 65535.0 / screenW;
    double normY = dy * 65535.0 / screenH;

    mouse_event(MOUSEEVENTF_MOVE, (DWORD)normX, (DWORD)normY, 0, 0);
}

void Aimbot::DoSilentAim(const std::vector<PlayerData>& players,
                         const Vector3& cameraPos) {
    if (!m_cfg.enabled || !m_cfg.silentAim) return;

    bool shouldSilent = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (!shouldSilent) return;

    Vector2 vp = m_sdk->ReadViewport();
    Vector3 camPos = m_sdk->ReadCameraPos();
    Matrix4x4 vm = m_sdk->ReadViewMatrix();

    auto target = FindTarget(players, vm, vp, camPos);
    if (!target) return;

    Vector3 aimTarget{};
    float   bestDist = FLT_MAX;
    for (const auto& p : players) {
        if (!p.isAlive || p.isLocal) continue;
        Vector3 aimPos = (m_cfg.aimPart == AimPart::Head) ? p.headPos : p.position;
        float d = (aimPos - camPos).Length();
        if (d < bestDist) {
            bestDist  = d;
            aimTarget = aimPos;
        }
    }
    if (bestDist > 10000.0f) return;

    RobloxCFrame cf = RobloxCFrame::LookAt(camPos, aimTarget);
    m_sdk->WriteCameraCFrame(cf);
}

void Aimbot::RenderFovCircle(const Vector2& vp) {
    if (!m_cfg.drawFovCircle || !m_cfg.enabled) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    ImVec2 center(vp.x * 0.5f, vp.y * 0.5f);

    float fovRad  = m_cfg.fov * 3.14159265f / 180.0f;
    float radius  = std::tan(fovRad) * (vp.x * 0.5f);

    dl->AddCircle(center, radius, m_cfg.fovCircleColor, 64, 1.5f);
}

void Aimbot::RenderMenu() {
    if (!ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::Checkbox("Enabled", &m_cfg.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Silent aim", &m_cfg.silentAim);
    ImGui::Separator();

    if (ImGui::BeginCombo("Aim part", m_cfg.aimPart == AimPart::Head ? "Head" : "Torso")) {
        if (ImGui::Selectable("Head", m_cfg.aimPart == AimPart::Head))
            m_cfg.aimPart = AimPart::Head;
        if (ImGui::Selectable("Torso", m_cfg.aimPart == AimPart::Torso))
            m_cfg.aimPart = AimPart::Torso;
        ImGui::EndCombo();
    }

    ImGui::SliderFloat("FOV (degrees)", &m_cfg.fov, 0.5f, 90.0f, "%.1f");
    ImGui::SliderFloat("Smoothness", &m_cfg.smooth, 1.0f, 30.0f, "%.0f");
    ImGui::Checkbox("Aim on fire only", &m_cfg.aimOnFire);
    ImGui::Checkbox("Draw FOV circle", &m_cfg.drawFovCircle);
    ImGui::ColorEdit4("Circle color", (float*)&m_cfg.fovCircleColor);
    ImGui::Text("Silent aim writes Camera CFrame on fire.");
}
