#include "roblox.hpp"
#include <algorithm>
#include <sstream>

RobloxSDK::RobloxSDK(std::shared_ptr<SyscallManager> mem)
    : m_mem(std::move(mem))
{
    m_base = m_mem->BaseAddress();
}

bool RobloxSDK::Init() {
    m_visualEngine = m_mem->Read<uintptr_t>(m_base + offsets::VisualEngine::Pointer);
    if (!m_visualEngine) {
        std::cerr << "[!] VisualEngine pointer is null\n";
        return false;
    }
    std::cout << "[+] VisualEngine @ 0x" << std::hex << m_visualEngine << std::dec << "\n";

    uintptr_t fdm = m_mem->Read<uintptr_t>(m_visualEngine + offsets::VisualEngine::FakeDataModel);
    if (!fdm) {
        std::cerr << "[!] FakeDataModel is null\n";
        return false;
    }
    m_dataModel = m_mem->Read<uintptr_t>(fdm + offsets::FakeDataModel::RealDataModel);
    if (!m_dataModel) {
        std::cerr << "[!] Real DataModel is null\n";
        return false;
    }
    std::cout << "[+] DataModel @ 0x" << std::hex << m_dataModel << std::dec << "\n";

    m_workspace = m_mem->Read<uintptr_t>(m_dataModel + offsets::DataModel::Workspace);
    if (!m_workspace) {
        std::cerr << "[!] Workspace is null\n";
        return false;
    }

    m_camera = m_mem->Read<uintptr_t>(m_workspace + offsets::Workspace::CurrentCamera);

    m_playersService = FindChildByName(m_dataModel, "Players");
    if (!m_playersService) {
        std::cerr << "[!] Players service not found\n";
        return false;
    }
    std::cout << "[+] Players service @ 0x" << std::hex << m_playersService << std::dec << "\n";

    m_localPlayer = m_mem->Read<uintptr_t>(m_playersService + offsets::Players::LocalPlayer);
    if (m_localPlayer)
        std::cout << "[+] LocalPlayer @ 0x" << std::hex << m_localPlayer << std::dec << "\n";
    else
        std::cerr << "[!] LocalPlayer is null (not in-game?)\n";

    return true;
}

std::string RobloxSDK::ReadInstanceName(uintptr_t inst) {
    char raw[48]{};
    if (!m_mem->ReadRaw(inst + 0x8, raw, 48))
        return "?";

    size_t size = *reinterpret_cast<size_t*>(raw + 16);
    if (size < 16) {
        raw[size] = '\0';
        return std::string(raw);
    } else {
        uintptr_t strPtr = *reinterpret_cast<uintptr_t*>(raw);
        return m_mem->ReadString(strPtr, std::min<size_t>(size + 1, 256));
    }
}

std::string RobloxSDK::ReadClassName(uintptr_t inst) {
    uintptr_t cd = m_mem->Read<uintptr_t>(inst + offsets::Instance::ClassDescriptor);
    if (!cd) return "?";
    uintptr_t namePtr = m_mem->Read<uintptr_t>(cd + offsets::ClassDescriptor::ClassName);
    if (!namePtr) return "?";
    return m_mem->ReadString(namePtr, 128);
}

Entity RobloxSDK::ReadEntityTree(uintptr_t inst, int depth) {
    Entity ent;
    if (!inst) return ent;

    ent.address = inst;
    ent.name = ReadInstanceName(inst);
    ent.className = ReadClassName(inst);

    if (depth > 8) return ent;

    uintptr_t begin = m_mem->Read<uintptr_t>(inst + offsets::Instance::ChildrenStart);
    uintptr_t end   = m_mem->Read<uintptr_t>(inst + offsets::Instance::ChildrenStart +
                                             offsets::Instance::ChildrenEnd);
    if (!begin || !end || end <= begin) return ent;

    size_t count = (end - begin) / sizeof(uintptr_t);
    count = std::min<size_t>(count, 1024);

    for (size_t i = 0; i < count; ++i) {
        uintptr_t childAddr = m_mem->Read<uintptr_t>(begin + i * sizeof(uintptr_t));
        if (childAddr) {
            Entity child = ReadEntityTree(childAddr, depth + 1);
            ent.children.push_back(std::move(child));
        }
    }
    return ent;
}

uintptr_t RobloxSDK::FindChildByName(uintptr_t parent, const std::string& name) {
    if (!parent) return 0;

    uintptr_t begin = m_mem->Read<uintptr_t>(parent + offsets::Instance::ChildrenStart);
    uintptr_t end   = m_mem->Read<uintptr_t>(parent + offsets::Instance::ChildrenStart +
                                             offsets::Instance::ChildrenEnd);
    if (!begin || !end || end <= begin) return 0;

    size_t count = (end - begin) / sizeof(uintptr_t);
    count = std::min<size_t>(count, 2048);

    for (size_t i = 0; i < count; ++i) {
        uintptr_t child = m_mem->Read<uintptr_t>(begin + i * sizeof(uintptr_t));
        if (!child) continue;
        std::string childName = ReadInstanceName(child);
        if (childName == name)
            return child;
    }
    return 0;
}

std::vector<uintptr_t> RobloxSDK::FindChildrenByClass(uintptr_t parent, const std::string& className) {
    std::vector<uintptr_t> results;
    if (!parent) return results;

    uintptr_t begin = m_mem->Read<uintptr_t>(parent + offsets::Instance::ChildrenStart);
    uintptr_t end   = m_mem->Read<uintptr_t>(parent + offsets::Instance::ChildrenStart +
                                             offsets::Instance::ChildrenEnd);
    if (!begin || !end || end <= begin) return results;

    size_t count = (end - begin) / sizeof(uintptr_t);
    count = std::min<size_t>(count, 2048);

    for (size_t i = 0; i < count; ++i) {
        uintptr_t child = m_mem->Read<uintptr_t>(begin + i * sizeof(uintptr_t));
        if (!child) continue;
        std::string cls = ReadClassName(child);
        if (cls == className)
            results.push_back(child);
    }
    return results;
}

Matrix4x4 RobloxSDK::ReadViewMatrix() const {
    Matrix4x4 mat;
    m_mem->ReadRaw(m_visualEngine + offsets::VisualEngine::ViewMatrix, &mat, sizeof(Matrix4x4));
    return mat;
}

Vector3 RobloxSDK::ReadCameraPos() const {
    if (!m_camera) return {};
    Vector3 pos;
    m_mem->ReadRaw(m_camera + offsets::Camera::Position, &pos, sizeof(Vector3));
    return pos;
}

Vector2 RobloxSDK::ReadViewport() const {
    Vector2 vp;
    m_mem->ReadRaw(m_visualEngine + offsets::VisualEngine::Dimensions, &vp, sizeof(Vector2));
    return vp;
}

RobloxCFrame RobloxSDK::ReadCameraCFrame() const {
    RobloxCFrame cf;
    if (!m_camera) return cf;
    m_mem->ReadRaw(m_camera + offsets::Camera::CFrame, &cf, sizeof(RobloxCFrame));
    return cf;
}

bool RobloxSDK::WriteCameraCFrame(const RobloxCFrame& cf) {
    if (!m_camera) {
        std::cerr << "[!] WriteCameraCFrame: no camera\n";
        return false;
    }
    return m_mem->WriteRaw(m_camera + offsets::Camera::CFrame,
                           const_cast<RobloxCFrame*>(&cf), sizeof(RobloxCFrame));
}

Vector3 RobloxSDK::ReadPartPosition(uintptr_t partAddr) {
    if (!partAddr) return {};
    return m_mem->Read<Vector3>(partAddr + offsets::Primitive::Position);
}

Vector3 RobloxSDK::ReadPartSize(uintptr_t partAddr) {
    if (!partAddr) return {};
    return m_mem->Read<Vector3>(partAddr + offsets::Primitive::Size);
}

PlayerData RobloxSDK::GetPlayerData(uintptr_t playerAddr, bool isLocal) {
    PlayerData pd;
    pd.instanceAddr = playerAddr;
    pd.isLocal = isLocal;
    pd.name = ReadInstanceName(playerAddr);
    pd.displayName = m_mem->ReadString(
        m_mem->Read<uintptr_t>(playerAddr + offsets::Player::DisplayName), 64);

    pd.characterAddr = m_mem->Read<uintptr_t>(playerAddr + offsets::Player::Character);
    if (!pd.characterAddr) return pd;

    pd.rootPartAddr = m_mem->Read<uintptr_t>(pd.characterAddr + offsets::Model::PrimaryPart);
    if (!pd.rootPartAddr) return pd;

    pd.position = ReadPartPosition(pd.rootPartAddr);

    Vector3 size = ReadPartSize(pd.rootPartAddr);
    pd.headPos = pd.position;
    pd.headPos.y += size.y * 0.8f;

    uintptr_t primitive = m_mem->Read<uintptr_t>(pd.rootPartAddr + offsets::BasePart::Primitive);
    pd.velocity = m_mem->Read<Vector3>(primitive + offsets::Primitive::AssemblyLinearVelocity);

    auto humanoids = FindChildrenByClass(pd.characterAddr, "Humanoid");
    if (!humanoids.empty()) {
        pd.humanoidAddr = humanoids[0];
        pd.health    = m_mem->Read<float>(pd.humanoidAddr + offsets::Humanoid::Health);
        pd.maxHealth = m_mem->Read<float>(pd.humanoidAddr + offsets::Humanoid::MaxHealth);
    }

    pd.isAlive = pd.health > 0.0f;
    return pd;
}

std::vector<PlayerData> RobloxSDK::GetPlayers() {
    std::vector<PlayerData> result;
    if (!m_playersService) return result;

    uintptr_t begin = m_mem->Read<uintptr_t>(m_playersService + offsets::Instance::ChildrenStart);
    uintptr_t end   = m_mem->Read<uintptr_t>(m_playersService + offsets::Instance::ChildrenStart +
                                             offsets::Instance::ChildrenEnd);
    if (!begin || !end || end <= begin) return result;

    size_t count = (end - begin) / sizeof(uintptr_t);
    count = std::min<size_t>(count, 256);

    for (size_t i = 0; i < count; ++i) {
        uintptr_t child = m_mem->Read<uintptr_t>(begin + i * sizeof(uintptr_t));
        if (!child) continue;
        std::string cls = ReadClassName(child);
        if (cls == "Player") {
            result.push_back(GetPlayerData(child, child == m_localPlayer));
        }
    }
    return result;
}
