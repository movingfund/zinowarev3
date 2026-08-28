#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "driver/syscall.hpp"
#include "sdk/math.hpp"
#include "sdk/offsets.hpp"

struct PlayerData {
    uintptr_t instanceAddr  = 0;
    uintptr_t characterAddr = 0;
    uintptr_t humanoidAddr  = 0;
    uintptr_t rootPartAddr  = 0;

    std::string name;
    std::string displayName;
    Vector3     position{};
    Vector3     headPos{};
    Vector3     velocity{};
    float       health     = 0.0f;
    float       maxHealth  = 100.0f;
    bool        isLocal    = false;
    bool        isAlive    = false;
    int         teamColor  = 0;
};

struct Entity {
    uintptr_t address = 0;
    std::string name;
    std::string className;
    std::vector<Entity> children;

    bool IsValid() const { return address != 0; }
};

class RobloxSDK {
public:
    explicit RobloxSDK(std::shared_ptr<SyscallManager> mem);
    ~RobloxSDK() = default;

    bool Init();

    uintptr_t VisualEngine()    const { return m_visualEngine; }
    uintptr_t DataModel()       const { return m_dataModel; }
    uintptr_t Workspace()       const { return m_workspace; }
    uintptr_t CurrentCamera()   const { return m_camera; }
    uintptr_t LocalPlayer()     const { return m_localPlayer; }
    uintptr_t PlayersService()  const { return m_playersService; }

    Matrix4x4   ReadViewMatrix()   const;
    Vector3     ReadCameraPos()    const;
    Vector2     ReadViewport()     const;
    RobloxCFrame ReadCameraCFrame() const;
    bool        WriteCameraCFrame(const RobloxCFrame& cf);

    std::string ReadInstanceName(uintptr_t inst);
    std::string ReadClassName(uintptr_t inst);
    Entity      ReadEntityTree(uintptr_t inst, int depth = 0);
    uintptr_t   FindChildByName(uintptr_t parent, const std::string& name);
    std::vector<uintptr_t> FindChildrenByClass(uintptr_t parent, const std::string& className);

    std::vector<PlayerData> GetPlayers();
    PlayerData              GetPlayerData(uintptr_t playerAddr, bool isLocal = false);

    Vector3 ReadPartPosition(uintptr_t partAddr);
    Vector3 ReadPartSize(uintptr_t partAddr);

    std::shared_ptr<SyscallManager> Mem() const { return m_mem; }

private:
    std::shared_ptr<SyscallManager> m_mem;
    uintptr_t m_base = 0;

    uintptr_t m_visualEngine   = 0;
    uintptr_t m_dataModel      = 0;
    uintptr_t m_workspace      = 0;
    uintptr_t m_camera         = 0;
    uintptr_t m_localPlayer    = 0;
    uintptr_t m_playersService = 0;
};
