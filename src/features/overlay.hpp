#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <string>
#include <functional>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Overlay {
public:
    Overlay();
    ~Overlay();

    bool Create(const std::string& title = "Overlay", int width = 1920, int height = 1080);
    bool BeginFrame();
    void EndFrame();
    void Destroy();

    bool IsRunning() const { return m_running; }
    HWND  WindowHandle() const { return m_hwnd; }
    ImVec2 GetSize() const { return ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)); }

    void SetRenderCallback(std::function<void()> callback) { m_renderCallback = std::move(callback); }

private:
    HWND        m_hwnd      = nullptr;
    WNDCLASSEXW m_wc        = {};
    bool        m_running   = false;
    int         m_width     = 1920;
    int         m_height    = 1080;

    ID3D11Device*           m_d3dDevice      = nullptr;
    ID3D11DeviceContext*    m_d3dContext      = nullptr;
    IDXGISwapChain*         m_swapChain       = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    std::function<void()> m_renderCallback;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool CreateSwapChain();
    bool CreateRenderTarget();
    void CleanupRenderTarget();
    void CleanupDevice();
};
