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
    
    // Individual dialog window state
    struct DialogWindow {
        HWND hwnd;
        HDC hdc;
        HGLRC hglrc;
        bool visible;
        ImGuiDialogCallback callback;
        const char* title;
        int width;
        int height;
        
        // Constructor
        DialogWindow() : hwnd(NULL), hdc(NULL), hglrc(NULL), visible(false), 
                        callback(NULL), title(""), width(400), height(300) {}
    };
    
    // Function declarations for DialogWindow operations
    bool CreateDialogWindow(DialogWindow* dialog, HWND parent, const char* windowTitle, int w, int h);
    void DestroyDialogWindow(DialogWindow* dialog);
    bool CreateOpenGLContext(DialogWindow* dialog);
    void CleanupOpenGL(DialogWindow* dialog);
    void ShowDialogWindow(DialogWindow* dialog);
    void HideDialogWindow(DialogWindow* dialog);
    void RenderDialogWindow(DialogWindow* dialog);
    
    // Global state
    struct GlobalState {
        HWND parent;
        bool initialized;
        
        // Individual dialog windows
        DialogWindow properties;
        DialogWindow linkPoints;
        
        // Constructor
        GlobalState() : parent(NULL), initialized(false) {}
    };
    
    static GlobalState g_state;
    
    // Window procedure for ImGui dialogs
    LRESULT WINAPI DialogWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
            
        switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                // Find which dialog this belongs to and update its viewport
                DialogWindow* dialog = NULL;
                if (hWnd == g_state.properties.hwnd) dialog = &g_state.properties;
                else if (hWnd == g_state.linkPoints.hwnd) dialog = &g_state.linkPoints;
                
                if (dialog && dialog->hglrc) {
                    wglMakeCurrent(dialog->hdc, dialog->hglrc);
                    glViewport(0, 0, (GLsizei)LOWORD(lParam), (GLsizei)HIWORD(lParam));
                }
            }
            return 0;
            
        case WM_CLOSE:
            // Hide instead of destroying
            ShowWindow(hWnd, SW_HIDE);
            if (hWnd == g_state.properties.hwnd) g_state.properties.visible = false;
            else if (hWnd == g_state.linkPoints.hwnd) g_state.linkPoints.visible = false;
            return 0;
            
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    bool CreateDialogWindow(DialogWindow* dialog, HWND parent, const char* windowTitle, int w, int h) {
        dialog->width = w;
        dialog->height = h;
        dialog->title = windowTitle;
        
        // Create borderless window that looks like a dialog
        dialog->hwnd = CreateWindowW(
            L"ImGuiDialog", 
            L"Dialog", 
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, // Borderless with caption for moving
            CW_USEDEFAULT, CW_USEDEFAULT, 
            dialog->width, dialog->height, 
            parent, 
            NULL, 
            GetModuleHandle(NULL), 
            NULL
        );
        
        if (!dialog->hwnd) return false;
        
        // Set the window title
        SetWindowTextA(dialog->hwnd, windowTitle);
        
        return CreateOpenGLContext(dialog);
    }
    
    void DestroyDialogWindow(DialogWindow* dialog) {
        CleanupOpenGL(dialog);
        if (dialog->hwnd) {
            DestroyWindow(dialog->hwnd);
            dialog->hwnd = NULL;
        }
    }
    
    bool CreateOpenGLContext(DialogWindow* dialog) {
        dialog->hdc = GetDC(dialog->hwnd);
        if (!dialog->hdc) return false;
        
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 16;
        pfd.iLayerType = PFD_MAIN_PLANE;
        
        int pixelFormat = ChoosePixelFormat(dialog->hdc, &pfd);
        if (!pixelFormat) return false;
        
        if (!SetPixelFormat(dialog->hdc, pixelFormat, &pfd)) return false;
        
        dialog->hglrc = wglCreateContext(dialog->hdc);
        if (!dialog->hglrc) return false;
        
        if (!wglMakeCurrent(dialog->hdc, dialog->hglrc)) return false;
        
        return true;
    }
    
    void CleanupOpenGL(DialogWindow* dialog) {
        if (dialog->hglrc) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dialog->hglrc);
            dialog->hglrc = NULL;
        }
        if (dialog->hdc && dialog->hwnd) {
            ReleaseDC(dialog->hwnd, dialog->hdc);
            dialog->hdc = NULL;
        }
    }
    
    void ShowDialogWindow(DialogWindow* dialog) {
        if (!dialog->hwnd) return;
        
        dialog->visible = true;
        ShowWindow(dialog->hwnd, SW_SHOW);
        SetForegroundWindow(dialog->hwnd);
        
        // Center on parent
        if (g_state.parent) {
            RECT parentRect, windowRect;
            GetWindowRect(g_state.parent, &parentRect);
            GetWindowRect(dialog->hwnd, &windowRect);
            
            int centerX = parentRect.left + (parentRect.right - parentRect.left - dialog->width) / 2;
            int centerY = parentRect.top + (parentRect.bottom - parentRect.top - dialog->height) / 2;
            
            SetWindowPos(dialog->hwnd, HWND_TOP, centerX, centerY, 0, 0, SWP_NOSIZE);
        }
    }
    
    void HideDialogWindow(DialogWindow* dialog) {
        dialog->visible = false;
        if (dialog->hwnd) ShowWindow(dialog->hwnd, SW_HIDE);
    }
    
    void RenderDialogWindow(DialogWindow* dialog) {
        if (!dialog->visible || !dialog->hwnd || !dialog->callback || !dialog->hglrc) return;
        
        // Make this window's context current
        if (!wglMakeCurrent(dialog->hdc, dialog->hglrc)) return;
        
        // Get window size for viewport
        RECT rect;
        GetClientRect(dialog->hwnd, &rect);
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        
        if (windowWidth <= 0 || windowHeight <= 0) return;
        
        glViewport(0, 0, windowWidth, windowHeight);
        
        // Start ImGui frame for this window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Call the dialog callback
        dialog->callback();
        
        // Check if dialog was closed by callback
        if (!dialog->visible) {
            HideDialogWindow(dialog);
            ImGui::EndFrame();
            return;
        }
        
        // Render
        ImGui::Render();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data) {
            ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        }
        
        SwapBuffers(dialog->hdc);
    }
    
    bool Initialize(HWND parent) {
        g_state.parent = parent;
        
        // Register window class for dialog windows
        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_OWNDC; // Important for OpenGL
        wc.lpfnWndProc = DialogWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"ImGuiDialog";
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }
        
        // Setup Dear ImGui context (shared across all dialogs)
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Create individual dialog windows
        if (!CreateDialogWindow(&g_state.properties, parent, "Properties", 500, 600)) {
            return false;
        }
        
        if (!CreateDialogWindow(&g_state.linkPoints, parent, "Link Points", 600, 500)) {
            return false;
        }
        
        // Initialize ImGui backends for each window
        // We'll switch contexts as needed during rendering
        wglMakeCurrent(g_state.properties.hdc, g_state.properties.hglrc);
        if (!ImGui_ImplWin32_Init(g_state.properties.hwnd)) {
            return false;
        }
        
        if (!ImGui_ImplOpenGL3_Init("#version 130")) {
            return false;
        }
        
        g_state.initialized = true;
        return true;
    }
    
    void Shutdown() {
        if (!g_state.initialized) return;
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        DestroyDialogWindow(&g_state.properties);
        DestroyDialogWindow(&g_state.linkPoints);
        
        UnregisterClassW(L"ImGuiDialog", GetModuleHandle(NULL));
        
        // Reset state
        g_state = GlobalState();
    }
    
    void SetDialogCallbacks(ImGuiDialogCallback propertiesCallback, ImGuiDialogCallback linkPointsCallback) {
        g_state.properties.callback = propertiesCallback;
        g_state.linkPoints.callback = linkPointsCallback;
    }
    
    void ShowProperties() {
        if (!g_state.initialized) return;
        ShowDialogWindow(&g_state.properties);
    }
    
    void ShowLinkPoints() {
        if (!g_state.initialized) return;
        ShowDialogWindow(&g_state.linkPoints);
    }
    
    void Hide() {
        HideDialogWindow(&g_state.properties);
        HideDialogWindow(&g_state.linkPoints);
    }
    
    void HideProperties() {
        HideDialogWindow(&g_state.properties);
    }
    
    void HideLinkPoints() {
        HideDialogWindow(&g_state.linkPoints);
    }
    
    bool IsAnyDialogOpen() {
        return g_state.properties.visible || g_state.linkPoints.visible;
    }
    
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Check if message is for one of our dialog windows
        if ((g_state.properties.hwnd && hwnd == g_state.properties.hwnd) ||
            (g_state.linkPoints.hwnd && hwnd == g_state.linkPoints.hwnd)) {
            return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        }
        return false;
    }
    
    void Render() {
        if (!g_state.initialized) return;
        
        // Render each dialog in its own window
        RenderDialogWindow(&g_state.properties);
        RenderDialogWindow(&g_state.linkPoints);
    }
    
    // Utility functions for your dialog callbacks - these create borderless ImGui windows
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
}