#include "triggerbot.hpp"
#include "imgui.h"

Triggerbot::Triggerbot(std::shared_ptr<RobloxSDK> sdk)
    : m_sdk(std::move(sdk))
    , m_lastFireTime(std::chrono::steady_clock::now())
{
}

void Triggerbot::Update(const std::vector<PlayerData>& players,
                        const Matrix4x4& vm, const Vector2& vp) {
    if (!m_cfg.enabled) return;

    bool keyDown = (GetAsyncKeyState(m_cfg.triggerKey) & 0x8000) != 0;
    if (m_cfg.onKey && !keyDown) return;

    int width  = static_cast<int>(vp.x);
    int height = static_cast<int>(vp.y);
    Vector2 screenCenter(width * 0.5f, height * 0.5f);

    bool overEnemy = false;
    for (const auto& p : players) {
        if (!p.isAlive || p.isLocal) continue;

        Vector2 screen;
        if (!vm.WorldToScreen(p.position, screen, width, height)) continue;

        float dist = screenCenter.Dist(screen);
        if (dist < m_cfg.hitboxSize) {
            overEnemy = true;
            break;
        }
    }

    if (overEnemy) {
        if (!m_wasOverEnemy) {
            m_lastFireTime = std::chrono::steady_clock::now();
            m_wasOverEnemy = true;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFireTime).count();
        if (elapsed >= m_cfg.delayMs) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            m_lastFireTime = now;
        }
    } else {
        m_wasOverEnemy = false;
    }
}

void Triggerbot::RenderMenu() {
    if (!ImGui::CollapsingHeader("Triggerbot", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::Checkbox("Enabled", &m_cfg.enabled);
    ImGui::SliderInt("Delay (ms)", &m_cfg.delayMs, 0, 500);
    ImGui::SliderFloat("Hitbox size (px)", &m_cfg.hitboxSize, 1.0f, 50.0f);
    ImGui::Checkbox("On key only", &m_cfg.onKey);
    if (m_cfg.onKey)
        ImGui::Text("Trigger key: LMB");
}
