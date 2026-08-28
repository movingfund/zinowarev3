#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <windows.h>
#include "driver/syscall.hpp"
#include "sdk/roblox.hpp"
#include "sdk/math.hpp"
#include "features/esp.hpp"
#include "features/aimbot.hpp"
#include "features/triggerbot.hpp"
#include "overlay/overlay.hpp"
#include "imgui.h"

static std::unique_ptr<Overlay> g_overlay;
static bool                     g_running = true;

void CleanupAndExit(int) {
    std::cout << "\n[!] Shutting down...\n";
    g_running = false;
    if (g_overlay) g_overlay->Destroy();
    exit(0);
}

void RenderMainMenu(ESP& esp, Aimbot& aimbot, Triggerbot& triggerbot) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("HackerAI - Roblox External", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Button("Unload (END)"))
        g_running = false;

    esp.RenderMenu();
    aimbot.RenderMenu();
    triggerbot.RenderMenu();

    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}

int main() {
    SetConsoleTitleA("HackerAI Roblox External");
    std::cout << "========================================\n";
    std::cout << "  HackerAI - Roblox External v1.0\n";
    std::cout << "  Console mode - errors printed below\n";
    std::cout << "========================================\n\n";

    signal(SIGINT, CleanupAndExit);
    signal(SIGTERM, CleanupAndExit);

    auto mem = std::make_shared<SyscallManager>();
    if (!mem->AttachToProcess("RobloxPlayerBeta.exe", "RobloxPlayer.exe")) {
        std::cerr << "[!] Could not attach. Make sure Roblox is running and relaunch as admin.\n";
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    auto rbx = std::make_shared<RobloxSDK>(mem);
    if (!rbx->Init()) {
        std::cerr << "[!] SDK initialisation failed. Check offsets or Roblox version.\n";
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    g_overlay = std::make_unique<Overlay>();
    if (!g_overlay->Create("HackerAI Overlay")) {
        std::cerr << "[!] Overlay creation failed.\n";
        return 1;
    }

    ESP       esp(rbx);
    Aimbot    aimbot(rbx);
    Triggerbot trigger(rbx);

    std::cout << "[+] Main loop running. Press END to unload.\n\n";

    while (g_running && g_overlay->IsRunning()) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            std::cout << "[+] END pressed, unloading.\n";
            break;
        }

        Vector2     vp  = rbx->ReadViewport();
        Matrix4x4   vm  = rbx->ReadViewMatrix();
        Vector3     cam = rbx->ReadCameraPos();
        auto players = rbx->GetPlayers();

        aimbot.DoAimbot(players, vm, vp, cam);
        aimbot.DoSilentAim(players, cam);
        trigger.Update(players, vm, vp);

        if (!g_overlay->BeginFrame())
            break;

        RenderMainMenu(esp, aimbot, trigger);
        esp.Render(players, vm, vp);
        aimbot.RenderFovCircle(vp);

        g_overlay->EndFrame();

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    g_overlay->Destroy();
    mem->Close();
    std::cout << "[+] Clean exit.\n";
    return 0;
}
