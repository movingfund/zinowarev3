#pragma once
#include "sdk/roblox.hpp"
#include <chrono>

struct TriggerbotConfig {
    bool    enabled    = false;
    int     delayMs    = 50;
    float   hitboxSize = 5.0f;
    bool    onKey      = true;
    int     triggerKey = VK_LBUTTON;
};

class Triggerbot {
public:
    explicit Triggerbot(std::shared_ptr<RobloxSDK> sdk);

    void Update(const std::vector<PlayerData>& players,
                const Matrix4x4& vm, const Vector2& vp);

    void RenderMenu();
    TriggerbotConfig& Config() { return m_cfg; }

private:
    std::shared_ptr<RobloxSDK> m_sdk;
    TriggerbotConfig m_cfg;
    std::chrono::steady_clock::time_point m_lastFireTime;
    bool m_wasOverEnemy = false;
};
