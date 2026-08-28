#pragma once
#include <vector>
#include "sdk/roblox.hpp"
#include "overlay/overlay.hpp"

struct EspConfig {
    bool enabled       = true;
    bool drawBox       = true;
    bool drawName      = true;
    bool drawHealth    = true;
    bool drawSnapline  = true;
    bool visibleOnly   = false;
    ImColor boxColor   = ImColor(255, 255, 255, 255);
    ImColor enemyColor = ImColor(255, 60, 60, 255);
    ImColor teamColor  = ImColor(60, 255, 60, 255);
    float   lineWidth  = 1.5f;
};

class ESP {
public:
    explicit ESP(std::shared_ptr<RobloxSDK> sdk);
    void Render(const std::vector<PlayerData>& players, const Matrix4x4& vm, const Vector2& vp);
    void RenderMenu();
    EspConfig& Config() { return m_cfg; }

private:
    std::shared_ptr<RobloxSDK> m_sdk;
    EspConfig m_cfg;

    void DrawBox(const Vector2& top, const Vector2& bottom, ImColor color);
    void DrawName(const std::string& name, const Vector2& top);
    void DrawHealthBar(float health, float maxHealth, const Vector2& top, const Vector2& bottom);
    void DrawSnapline(const Vector2& footPos, ImColor color);
};
