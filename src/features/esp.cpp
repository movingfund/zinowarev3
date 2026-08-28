#include "esp.hpp"
#include "imgui.h"
#include <string>

ESP::ESP(std::shared_ptr<RobloxSDK> sdk) : m_sdk(std::move(sdk)) {}

void ESP::Render(const std::vector<PlayerData>& players, const Matrix4x4& vm, const Vector2& vp) {
    if (!m_cfg.enabled) return;

    int width  = static_cast<int>(vp.x);
    int height = static_cast<int>(vp.y);

    for (const auto& p : players) {
        if (!p.isAlive || p.isLocal) continue;

        Vector2 screenPos, screenHead;
        if (!vm.WorldToScreen(p.position, screenPos, width, height)) continue;
        if (!vm.WorldToScreen(p.headPos, screenHead, width, height)) continue;

        ImColor color = m_cfg.enemyColor;
        if (p.teamColor != 0) color = m_cfg.teamColor;

        float boxHeight = screenPos.y - screenHead.y;
        float boxWidth  = boxHeight * 0.4f;
        if (boxWidth < 10.0f) boxWidth = 10.0f;

        Vector2 boxTop    = { screenHead.x - boxWidth * 0.5f, screenHead.y };
        Vector2 boxBottom = { screenPos.x  + boxWidth * 0.5f, screenPos.y  };

        if (m_cfg.drawBox)
            DrawBox(boxTop, boxBottom, color);
        if (m_cfg.drawName)
            DrawName(p.displayName.empty() ? p.name : p.displayName, boxTop);
        if (m_cfg.drawHealth)
            DrawHealthBar(p.health, p.maxHealth, boxTop, boxBottom);
        if (m_cfg.drawSnapline)
            DrawSnapline(screenPos, color);
    }
}

void ESP::DrawBox(const Vector2& top, const Vector2& bottom, ImColor color) {
    auto* dl = ImGui::GetBackgroundDrawList();
    float l = top.x, r = bottom.x, t = top.y, b = bottom.y;
    dl->AddRect(ImVec2(l, t), ImVec2(r, b), color, 0.0f, 0, m_cfg.lineWidth);
}

void ESP::DrawName(const std::string& name, const Vector2& top) {
    auto* dl = ImGui::GetBackgroundDrawList();
    ImVec2 textPos(top.x, top.y - 14.0f);
    if (textPos.y < 0) textPos.y = top.y + 2.0f;
    dl->AddText(nullptr, 0.0f, textPos, ImColor(255, 255, 255, 255), name.c_str());
}

void ESP::DrawHealthBar(float health, float maxHealth, const Vector2& top, const Vector2& bottom) {
    auto* dl = ImGui::GetBackgroundDrawList();
    float barWidth  = 3.0f;
    float barHeight = bottom.y - top.y;
    if (barHeight < 1.0f) return;

    float ratio = std::clamp(health / maxHealth, 0.0f, 1.0f);
    ImVec2 barTopLeft(top.x - 6.0f, top.y);
    ImVec2 barBotRight(top.x - 6.0f + barWidth, bottom.y);

    dl->AddRectFilled(barTopLeft, barBotRight, ImColor(30, 30, 30, 200));
    float fillH = barHeight * ratio;
    dl->AddRectFilled(
        ImVec2(barTopLeft.x, barBotRight.y - fillH),
        barBotRight,
        ImColor(static_cast<int>((1 - ratio) * 255), static_cast<int>(ratio * 255), 0, 255));
    dl->AddRect(barTopLeft, barBotRight, ImColor(200, 200, 200, 200));
}

void ESP::DrawSnapline(const Vector2& footPos, ImColor color) {
    auto* dl = ImGui::GetBackgroundDrawList();
    auto vpSize = ImGui::GetIO().DisplaySize;
    ImVec2 center(vpSize.x * 0.5f, vpSize.y);
    dl->AddLine(center, ImVec2(footPos.x, footPos.y), color, m_cfg.lineWidth);
}

void ESP::RenderMenu() {
    if (!ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::Checkbox("Enabled", &m_cfg.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Visible only", &m_cfg.visibleOnly);
    ImGui::Checkbox("Box", &m_cfg.drawBox);
    ImGui::SameLine();
    ImGui::Checkbox("Name", &m_cfg.drawName);
    ImGui::SameLine();
    ImGui::Checkbox("Health", &m_cfg.drawHealth);
    ImGui::SameLine();
    ImGui::Checkbox("Snapline", &m_cfg.drawSnapline);
    ImGui::ColorEdit4("Enemy color", (float*)&m_cfg.enemyColor);
    ImGui::ColorEdit4("Team color",  (float*)&m_cfg.teamColor);
    ImGui::SliderFloat("Line width", &m_cfg.lineWidth, 0.5f, 3.0f);
}
