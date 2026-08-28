#include "overlay.hpp"
#include <stdexcept>
#include <iostream>

Overlay::Overlay() {}

Overlay::~Overlay() { Destroy(); }

bool Overlay::Create(const std::string& title, int width, int height) {
    m_width  = width;
    m_height = height;

    m_wc = {};
    m_wc.cbSize        = sizeof(WNDCLASSEXW);
    m_wc.style         = CS_HREDRAW | CS_VREDRAW;
    m_wc.lpfnWndProc   = WndProc;
    m_wc.hInstance     = GetModuleHandleW(nullptr);
    m_wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    m_wc.hbrBackground = (HBRUSH)CreateSolidBrush(0);
    m_wc.lpszClassName = L"OverlayWindowClass";

    if (!RegisterClassExW(&m_wc)) {
        std::cerr << "[!] RegisterClassExW failed\n";
        return false;
    }

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        m_wc.lpszClassName,
        std::wstring(title.begin(), title.end()).c_str(),
        WS_POPUP,
        0, 0, m_width, m_height,
        nullptr, nullptr, m_wc.hInstance, this
    );

    if (!m_hwnd) {
        std::cerr << "[!] CreateWindowExW failed\n";
        return false;
    }

    SetLayeredWindowAttributes(m_hwnd, 0, 0, LWA_ALPHA);

    if (!CreateSwapChain()) {
        std::cerr << "[!] CreateSwapChain failed\n";
        return false;
    }
    if (!CreateRenderTarget()) {
        std::cerr << "[!] CreateRenderTarget failed\n";
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(m_hwnd)) {
        std::cerr << "[!] ImGui_ImplWin32_Init failed\n";
        return false;
    }
    if (!ImGui_ImplDX11_Init(m_d3dDevice, m_d3dContext)) {
        std::cerr << "[!] ImGui_ImplDX11_Init failed\n";
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    m_running = true;

    std::cout << "[+] Overlay created (" << m_width << "x" << m_height << ")\n";
    return true;
}

bool Overlay::BeginFrame() {
    if (!m_running) return false;

    MSG msg;
    while (PeekMessageW(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            m_running = false;
            return false;
        }
    }

    if (!m_running) return false;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    return true;
}

void Overlay::EndFrame() {
    ImGui::Render();

    m_d3dContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

    float clearColor[4] = {0, 0, 0, 0};
    m_d3dContext->ClearRenderTargetView(m_renderTargetView, clearColor);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_swapChain->Present(1, 0);
}

void Overlay::Destroy() {
    m_running = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupRenderTarget();
    CleanupDevice();

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

bool Overlay::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferDesc.Width                   = m_width;
    scd.BufferDesc.Height                  = m_height;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count                   = 1;
    scd.SampleDesc.Quality                 = 0;
    scd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount                        = 2;
    scd.OutputWindow                       = m_hwnd;
    scd.Windowed                           = TRUE;
    scd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    scd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 1, D3D11_SDK_VERSION,
        &scd, &m_swapChain, &m_d3dDevice, &level, &m_d3dContext
    );
    if (FAILED(hr)) {
        std::cerr << "[!] D3D11CreateDeviceAndSwapChain failed: 0x" << std::hex << hr << std::dec << "\n";
        return false;
    }
    return true;
}

bool Overlay::CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr) || !backBuffer) {
        std::cerr << "[!] GetBuffer failed\n";
        return false;
    }

    hr = m_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    backBuffer->Release();
    if (FAILED(hr)) {
        std::cerr << "[!] CreateRenderTargetView failed\n";
        return false;
    }
    return true;
}

void Overlay::CleanupRenderTarget() {
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }
}

void Overlay::CleanupDevice() {
    if (m_swapChain)  { m_swapChain->Release();  m_swapChain  = nullptr; }
    if (m_d3dContext) { m_d3dContext->Release();  m_d3dContext = nullptr; }
    if (m_d3dDevice)  { m_d3dDevice->Release();   m_d3dDevice  = nullptr; }
}

LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}