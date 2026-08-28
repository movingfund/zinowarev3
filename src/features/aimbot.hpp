#pragma once
#include "sdk/roblox.hpp"
#include <optional>
#include <chrono>

enum class AimPart { Head, Torso };

struct AimbotConfig {
    bool    enabled        = false;
    bool    silentAim      = false;
    float   fov            = 10.0f;
    float   smooth         = 8.0f;
    AimPart aimPart        = AimPart::Head;
    bool    aimOnFire      = true;
    int     aimKey         = VK_LBUTTON;
    bool    drawFovCircle  = true;
    ImColor fovCircleColor = ImColor(255, 255, 255, 60);
};

class Aimbot {
public:
    explicit Aimbot(std::shared_ptr<RobloxSDK> sdk);

    std::optional<Vector2> FindTarget(const std::vector<PlayerData>& players,
                                      const Matrix4x4& vm, const Vector2& vp,
                                      const Vector3& cameraPos);

    void DoAimbot(const std::vector<PlayerData>& players,
                  const Matrix4x4& vm, const Vector2& vp,
                  const Vector3& cameraPos);

    void DoSilentAim(const std::vector<PlayerData>& players,
                     const Vector3& cameraPos);

    void RenderFovCircle(const Vector2& vp);
    void RenderMenu();
    AimbotConfig& Config() { return m_cfg; }

private:
    std::shared_ptr<RobloxSDK> m_sdk;
    AimbotConfig m_cfg;
    Vector2 m_lastAimPos{};
    bool    m_hadTarget = false;
};
