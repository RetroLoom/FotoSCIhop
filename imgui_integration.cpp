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
    
    // Simple state - one window, multiple dialogs
    struct EngineState {
        HWND parent;
        HWND hwnd;
        HDC hdc;
        HGLRC hglrc;
        bool initialized;
        
        // Dialog visibility flags
        bool showProperties;
        
        // Callback to your dialog function
        ImGuiDialogCallback propertiesCallback;
        
        // Constructor
        EngineState() : parent(NULL), hwnd(NULL), hdc(NULL), hglrc(NULL), 
                       initialized(false), showProperties(false),
                       propertiesCallback(NULL) {}
    };
    
    static EngineState g_engine;
    
    // Window procedure for ImGui window
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
            
        switch (msg) {
        case WM_SIZE:
            if (g_engine.hglrc && wParam != SIZE_MINIMIZED) {
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
        
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
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
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(g_engine.hglrc);
            g_engine.hglrc = NULL;
        }
        if (g_engine.hdc && g_engine.hwnd) {
            ReleaseDC(g_engine.hwnd, g_engine.hdc);
            g_engine.hdc = NULL;
        }
    }
    
    bool Initialize(HWND parent) {
        g_engine.parent = parent;
        
        // Create window class
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_OWNDC; // Important for OpenGL
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"ImGuiDialogs";
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }
        
        // Create single window (hidden initially)
        g_engine.hwnd = CreateWindowW(
            wc.lpszClassName, 
            L"Dialog", 
            WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU, 
            100, 100, 500, 600, 
            parent, 
            NULL, 
            wc.hInstance, 
            NULL
        );
        
        if (!g_engine.hwnd) {
            return false;
        }
            
        // Initialize OpenGL
        if (!CreateOpenGLContext(g_engine.hwnd)) {
            CleanupOpenGL();
            DestroyWindow(g_engine.hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(g_engine.hwnd)) {
            return false;
        }
        
        if (!ImGui_ImplOpenGL3_Init("#version 130")) {
            return false;
        }
        
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
        UnregisterClassW(L"ImGuiDialogs", GetModuleHandle(NULL));
        
        g_engine = EngineState(); // Reset state
    }
    
    void SetDialogCallbacks(ImGuiDialogCallback propertiesCallback) {
        g_engine.propertiesCallback = propertiesCallback;
    }
    
    void ShowProperties() {
        if (!g_engine.initialized) return;
        g_engine.showProperties = true;
        if (g_engine.hwnd) {
            SetWindowTextW(g_engine.hwnd, L"Properties");
            ShowWindow(g_engine.hwnd, SW_SHOW);
            SetForegroundWindow(g_engine.hwnd);
        }
    }
    
    void ShowLinkPoints() {
        // Link Points are now part of Properties dialog
        ShowProperties();
    }
    
    void Hide() {
        g_engine.showProperties = false;
        if (g_engine.hwnd)
            ShowWindow(g_engine.hwnd, SW_HIDE);
    }
    
    void HideProperties() {
        Hide();
    }
    
    void HideLinkPoints() {
        Hide();
    }
    
    bool IsAnyDialogOpen() {
        return g_engine.showProperties;
    }
    
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Only handle messages for the ImGui window
        if (g_engine.hwnd && hwnd == g_engine.hwnd) {
            return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        }
        return false;
    }
    
    void Render() {
        if (!g_engine.initialized) return;
        if (!g_engine.showProperties) return;
        if (!IsWindowVisible(g_engine.hwnd)) return;
            
        // Make OpenGL context current
        if (!wglMakeCurrent(g_engine.hdc, g_engine.hglrc)) return;
            
        // Get window size for viewport
        RECT rect;
        GetClientRect(g_engine.hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        if (width <= 0 || height <= 0) return; // Avoid invalid viewport
        
        glViewport(0, 0, width, height);
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Call the dialog callback
        if (g_engine.showProperties && g_engine.propertiesCallback) {
            g_engine.propertiesCallback();
        }
        
        // Hide window if no dialogs are open after callbacks
        if (!g_engine.showProperties) {
            Hide();
        }
        
        // Rendering
        ImGui::Render();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data) {
            ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        }
        
        SwapBuffers(g_engine.hdc);
    }
    
    // Utility functions for your dialog callbacks
    bool BeginDialog(const char* title, bool* open) {
        // Create a borderless window that fills the entire Win32 window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                ImGuiWindowFlags_NoBringToFrontOnFocus;
                                
        return ImGui::Begin(title, open, flags);
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
    
    void TextFormatted(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }
    
    void Separator() {
        ImGui::Separator();
    }
    
    void SameLine() {
        ImGui::SameLine();
    }
    
    bool CollapsingHeader(const char* label, bool defaultOpen) {
        ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::CollapsingHeader(label, flags);
    }
    
    bool CollapsingHeader(const char* label) {
        return ImGui::CollapsingHeader(label);
    }
    
    void PushStyleVar(int var, float value) {
        if (var == 0) { // IMGUI_STYLE_VAR_ALPHA
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, value);
        }
    }
    
    void PopStyleVar() {
        ImGui::PopStyleVar();
    }
}