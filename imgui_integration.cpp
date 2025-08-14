#include "stdafx.h"
#include "imgui_integration.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

// OpenGL includes (built into Windows)
#include <GL/gl.h>

// Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImGuiDialogs {
    
    // Internal state - much simpler with OpenGL!
    struct EngineState {
        HWND parent = nullptr;
        HWND hwnd = nullptr;
        HDC hdc = nullptr;
        HGLRC hglrc = nullptr;
        bool initialized = false;
        
        // Dialog visibility flags
        bool showProperties = false;
        bool showLinkPoints = false;
        
        // Callbacks to your dialog functions
        ImGuiDialogCallback propertiesCallback = nullptr;
        ImGuiDialogCallback linkPointsCallback = nullptr;
    };
    
    static EngineState g_engine;
    
    // Window procedure for ImGui window
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
            
        switch (msg) {
        case WM_SIZE:
            if (g_engine.hglrc) {
                glViewport(0, 0, (GLsizei)LOWORD(lParam), (GLsizei)HIWORD(lParam));
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_CLOSE:
            Hide();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    bool CreateOpenGLContext(HWND hWnd) {
        g_engine.hdc = GetDC(hWnd);
        if (!g_engine.hdc) return false;
        
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 16;
        pfd.iLayerType = PFD_MAIN_PLANE;
        
        int pixelFormat = ChoosePixelFormat(g_engine.hdc, &pfd);
        if (!pixelFormat) return false;
        
        if (!SetPixelFormat(g_engine.hdc, pixelFormat, &pfd)) return false;
        
        g_engine.hglrc = wglCreateContext(g_engine.hdc);
        if (!g_engine.hglrc) return false;
        
        if (!wglMakeCurrent(g_engine.hdc, g_engine.hglrc)) return false;
        
        return true;
    }
    
    void CleanupOpenGL() {
        if (g_engine.hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g_engine.hglrc);
            g_engine.hglrc = nullptr;
        }
        if (g_engine.hdc && g_engine.hwnd) {
            ReleaseDC(g_engine.hwnd, g_engine.hdc);
            g_engine.hdc = nullptr;
        }
    }
    
    bool Initialize(HWND parent) {
        g_engine.parent = parent;
        
        // Create window class
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_OWNDC; // Important for OpenGL
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"ImGuiDialogs";
        RegisterClassExW(&wc);
        
        // Create window
        g_engine.hwnd = CreateWindowW(wc.lpszClassName, L"Tools", 
                                     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                     100, 100, 500, 600, parent, nullptr, wc.hInstance, nullptr);
        
        if (!g_engine.hwnd)
            return false;
            
        // Initialize OpenGL
        if (!CreateOpenGLContext(g_engine.hwnd)) {
            CleanupOpenGL();
            DestroyWindow(g_engine.hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }
        
        // Show the window (hidden initially)
        ShowWindow(g_engine.hwnd, SW_HIDE);
        UpdateWindow(g_engine.hwnd);
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(g_engine.hwnd);
        ImGui_ImplOpenGL3_Init("#version 130"); // OpenGL 3.0+ GLSL 130
        
        g_engine.initialized = true;
        return true;
    }
    
    void Shutdown() {
        if (!g_engine.initialized) return;
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        CleanupOpenGL();
        DestroyWindow(g_engine.hwnd);
        UnregisterClassW(L"ImGuiDialogs", GetModuleHandle(nullptr));
        
        g_engine = {}; // Reset state
    }
    
    void SetDialogCallbacks(ImGuiDialogCallback propertiesCallback, ImGuiDialogCallback linkPointsCallback) {
        g_engine.propertiesCallback = propertiesCallback;
        g_engine.linkPointsCallback = linkPointsCallback;
    }
    
    void ShowProperties() {
        if (!g_engine.initialized) return;
        g_engine.showProperties = true;
        ShowWindow(g_engine.hwnd, SW_SHOW);
        SetForegroundWindow(g_engine.hwnd);
    }
    
    void ShowLinkPoints() {
        if (!g_engine.initialized) return;
        g_engine.showLinkPoints = true;
        ShowWindow(g_engine.hwnd, SW_SHOW);
        SetForegroundWindow(g_engine.hwnd);
    }
    
    void Hide() {
        g_engine.showProperties = false;
        g_engine.showLinkPoints = false;
        if (g_engine.hwnd)
            ShowWindow(g_engine.hwnd, SW_HIDE);
    }
    
    bool IsAnyDialogOpen() {
        return g_engine.showProperties || g_engine.showLinkPoints;
    }
    
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (g_engine.hwnd && IsAnyDialogOpen()) {
            return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        }
        return false;
    }
    
    void Render() {
        if (!g_engine.initialized || (!g_engine.showProperties && !g_engine.showLinkPoints))
            return;
            
        // Make OpenGL context current
        if (!wglMakeCurrent(g_engine.hdc, g_engine.hglrc))
            return;
            
        // Get window size for viewport
        RECT rect;
        GetClientRect(g_engine.hwnd, &rect);
        glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Call your dialog callbacks
        if (g_engine.showProperties && g_engine.propertiesCallback) {
            g_engine.propertiesCallback();
        }
        
        if (g_engine.showLinkPoints && g_engine.linkPointsCallback) {
            g_engine.linkPointsCallback();
        }
        
        // Hide window if no dialogs are open
        if (!g_engine.showProperties && !g_engine.showLinkPoints) {
            Hide();
        }
        
        // Rendering
        ImGui::Render();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        SwapBuffers(g_engine.hdc);
    }
    
    // Utility functions for your dialog callbacks
    bool BeginDialog(const char* title, bool* open) {
        return ImGui::Begin(title, open);
    }
    
    void EndDialog() {
        ImGui::End();
    }
    
    bool Button(const char* label) {
        return ImGui::Button(label);
    }
    
    bool InputInt(const char* label, int* value) {
        return ImGui::InputInt(label, value);
    }
    
    bool Checkbox(const char* label, bool* value) {
        return ImGui::Checkbox(label, value);
    }
    
    void Text(const char* text) {
        ImGui::Text("%s", text);
    }
    
    void Separator() {
        ImGui::Separator();
    }
    
    void SameLine() {
        ImGui::SameLine();
    }
}