#include "stdafx.h"
#include "imgui_integration.h"
#include <windowsx.h>

// Ensure Win32 constants are available
#ifndef RDW_NOACTIVATE
#define RDW_NOACTIVATE 0x2000
#endif

// ImGui includes
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

// OpenGL includes (built into Windows)
#include <GL/gl.h>
#include <algorithm>

// Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// External dialog activity flag from FotoSCIhop.cpp
extern bool g_dialogActive;

namespace ImGuiDialogs {
        
    struct DialogInfo {
        char title[256];
        ImGuiDialogCallback callback;
        bool isOpen;
        
        // Per-dialog size settings
        int preferredWidth;
        int preferredHeight;
        int currentWidth;
        int currentHeight;
        bool hasCustomSize;
        
        DialogInfo() : callback(nullptr), isOpen(false), 
                      preferredWidth(500), preferredHeight(600),
                      currentWidth(500), currentHeight(600),
                      hasCustomSize(false) {
            title[0] = '\0';
        }
        
        DialogInfo(const char* t, ImGuiDialogCallback cb) : callback(cb), isOpen(false),
                                                           preferredWidth(500), preferredHeight(600),
                                                           currentWidth(500), currentHeight(600),
                                                           hasCustomSize(false) {
            if (t) {
                strncpy(title, t, sizeof(title) - 1);
                title[sizeof(title) - 1] = '\0';
            } else {
                title[0] = '\0';
            }
        }
        
        void SetPreferredSize(int width, int height) {
            preferredWidth = width;
            preferredHeight = height;
            if (!hasCustomSize) {
                currentWidth = width;
                currentHeight = height;
            }
        }
    };
    
    struct EngineState {
        HWND parent;
        HWND hwnd;
        HDC hdc;
        HGLRC hglrc;
        bool initialized;
        
        DialogInfo dialogs[DIALOG_COUNT];
        Theme currentTheme;
        
        // Current active dialog tracking
        int activeDialogType;
        bool windowVisible;
        bool isResizing;
        bool parentWasEnabled;  // Track parent's original enabled state
        
        EngineState() : parent(NULL), hwnd(NULL), hdc(NULL), hglrc(NULL), 
                       initialized(false), currentTheme(THEME_DARK),
                       activeDialogType(-1), windowVisible(false), isResizing(false),
                       parentWasEnabled(true) {
            
            // Set default sizes for each dialog type
            dialogs[DIALOG_PROPERTIES].SetPreferredSize(600, 500);
            dialogs[DIALOG_ABOUT].SetPreferredSize(550, 450);
            dialogs[DIALOG_CLUT_GENERATOR].SetPreferredSize(1000, 750);
            dialogs[DIALOG_REALMPAL].SetPreferredSize(800, 600);
            dialogs[DIALOG_PREFERENCES].SetPreferredSize(500, 400);
            dialogs[DIALOG_PALETTE_MANAGER].SetPreferredSize(700, 550);
        }
    };
    
    static EngineState g_engine;
    
    // =========================================================================
    // WIN32/OPENGL INTEGRATION
    // =========================================================================
    
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
            
        switch (msg) {
        case WM_ACTIVATE:
            // Ensure dialog stays active when clicked
            if (LOWORD(wParam) != WA_INACTIVE && g_engine.parent) {
                // Make sure parent doesn't steal focus
                if (GetForegroundWindow() == g_engine.parent) {
                    SetForegroundWindow(hWnd);
                }
            }
            return 0;

        case WM_SIZE:
            if (g_engine.hglrc && wParam != SIZE_MINIMIZED && !g_engine.isResizing) {
                int newWidth = LOWORD(lParam);
                int newHeight = HIWORD(lParam);
                
                if (g_engine.activeDialogType >= 0) {
                    DialogInfo& activeDialog = g_engine.dialogs[g_engine.activeDialogType];
                    activeDialog.currentWidth = max(200, newWidth - 16);
                    activeDialog.currentHeight = max(150, newHeight - 39);
                    activeDialog.hasCustomSize = true;
                }
                
                glViewport(0, 0, newWidth, newHeight);
            }
            return 0;
            
        case WM_ENTERSIZEMOVE:
            g_engine.isResizing = true;
            return 0;
            
        case WM_EXITSIZEMOVE:
            g_engine.isResizing = false;
            return 0;
            
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
            
        case WM_CLOSE:
            Hide();
            return 0;
            
        case WM_DESTROY:
            if (g_engine.initialized) {
                Hide();
                return 0;
            }
            break;
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
    
    // =========================================================================
    // DIALOG MANAGEMENT
    // =========================================================================
        
    void RegisterDialog(DialogType type, const char* title, ImGuiDialogCallback callback) {
        if (type >= 0 && type < DIALOG_COUNT) {
            g_engine.dialogs[type] = DialogInfo(title, callback);
        }
    }

    void ShowDialog(DialogType type)
    {
        if (!g_engine.initialized || type < 0 || type >= DIALOG_COUNT)
            return;

        // Hide other dialogs first
        for (int i = 0; i < DIALOG_COUNT; i++)
        {
            if (i != type)
            {
                g_engine.dialogs[i].isOpen = false;
            }
        }

        g_engine.dialogs[type].isOpen = true;
        g_engine.activeDialogType = type;
        g_dialogActive = true;

        if (g_engine.hwnd)
        {
            DialogInfo &dialog = g_engine.dialogs[type];

            int winWidth = dialog.currentWidth + 16;
            int winHeight = dialog.currentHeight + 39;

            winWidth = max(300, min(1400, winWidth));
            winHeight = max(200, min(900, winHeight));

            RECT currentRect;
            bool hasCurrentPos = GetWindowRect(g_engine.hwnd, &currentRect);

            if (!g_engine.windowVisible || !hasCurrentPos)
            {
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                int x = (screenWidth - winWidth) / 2;
                int y = (screenHeight - winHeight) / 2;

                SetWindowPos(g_engine.hwnd, NULL, x, y, winWidth, winHeight, SWP_NOZORDER);
            }
            else
            {
                SetWindowPos(g_engine.hwnd, NULL,
                             currentRect.left, currentRect.top,
                             winWidth, winHeight,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }

            // Set proper window relationships and make modal
            SetWindowLongPtr(g_engine.hwnd, GWLP_HWNDPARENT, (LONG_PTR)g_engine.parent);
            
            // Store parent's current enabled state and disable it
            if (g_engine.parent) {
                g_engine.parentWasEnabled = IsWindowEnabled(g_engine.parent);
                EnableWindow(g_engine.parent, FALSE);
            }

            // Show the dialog window with proper Z-order
            SetWindowPos(g_engine.hwnd, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            
            // Activate the dialog
            SetForegroundWindow(g_engine.hwnd);
            SetFocus(g_engine.hwnd);
            
            g_engine.windowVisible = true;
        }
    }

    void HideDialog(DialogType type) {
        if (type >= 0 && type < DIALOG_COUNT) {
            g_engine.dialogs[type].isOpen = false;
        }
    }
    
    bool IsDialogOpen(DialogType type) {
        return (type >= 0 && type < DIALOG_COUNT) ? g_engine.dialogs[type].isOpen : false;
    }
    
    bool IsAnyDialogOpen() {
        for (int i = 0; i < DIALOG_COUNT; i++) {
            if (g_engine.dialogs[i].isOpen) return true;
        }
        return false;
    }

    bool HasDialogFocus() {
        if (!g_engine.hwnd || !IsWindowVisible(g_engine.hwnd)) return false;
        
        HWND foregroundWindow = GetForegroundWindow();
        return (foregroundWindow == g_engine.hwnd);
    }

    void RestoreDialogFocus() {
        if (g_engine.hwnd && IsWindowVisible(g_engine.hwnd)) {
            SetForegroundWindow(g_engine.hwnd);
            SetFocus(g_engine.hwnd);
        }
    }

    HWND GetDialogWindow() {
        return g_engine.hwnd;
    }
    
    // =========================================================================
    // CORE ENGINE FUNCTIONS
    // =========================================================================

    bool Initialize(HWND parent)
    {
        g_engine.parent = parent;

        WNDCLASSEXW wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"ImGuiDialogs";

        if (!RegisterClassExW(&wc))
        {
            return false;
        }

        int defaultWidth = 500;
        int defaultHeight = 600;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - defaultWidth) / 2;
        int y = (screenHeight - defaultHeight) / 2;

        g_engine.hwnd = CreateWindowW(
            wc.lpszClassName,
            L"Dialog",
            WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
            x, y, defaultWidth, defaultHeight,
            NULL, // Don't set parent here - we'll do it in ShowDialog
            NULL,
            wc.hInstance,
            NULL);

        if (!g_engine.hwnd)
        {
            return false;
        }

        if (!CreateOpenGLContext(g_engine.hwnd))
        {
            CleanupOpenGL();
            DestroyWindow(g_engine.hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        if (!ImGui_ImplWin32_Init(g_engine.hwnd))
        {
            return false;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 130"))
        {
            return false;
        }

        ApplyTheme(THEME_PHOTOSHOP);

        g_engine.initialized = true;
        return true;
    }

    void Shutdown() {
        if (!g_engine.initialized) return;
        
        // Clean up dialog state
        Hide();
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        CleanupOpenGL();
        if (g_engine.hwnd) {
            DestroyWindow(g_engine.hwnd);
            g_engine.hwnd = NULL;
        }
        UnregisterClassW(L"ImGuiDialogs", GetModuleHandle(NULL));
        
        g_engine = EngineState();
    }
    
    void Hide() {
        // Re-enable parent window if it was enabled before
        if (g_engine.parent && !g_engine.parentWasEnabled) {
            EnableWindow(g_engine.parent, g_engine.parentWasEnabled);
        } else if (g_engine.parent) {
            EnableWindow(g_engine.parent, TRUE);
            // Restore focus to parent
            SetForegroundWindow(g_engine.parent);
        }

        for (int i = 0; i < DIALOG_COUNT; i++) {
            g_engine.dialogs[i].isOpen = false;
        }
        
        g_engine.activeDialogType = -1;
        g_dialogActive = false;
               
        if (g_engine.hwnd && g_engine.windowVisible) {
            ShowWindow(g_engine.hwnd, SW_HIDE);
            g_engine.windowVisible = false;
        }
    }
           
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (g_engine.hwnd && hwnd == g_engine.hwnd) {
            return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        }
        return false;
    }
    
    void Render() {
        if (!g_engine.initialized) return;
        if (!IsAnyDialogOpen()) return;
        if (!IsWindowVisible(g_engine.hwnd)) return;

        // Ensure dialog stays on top of parent
        if (g_engine.hwnd && g_engine.parent && IsWindowVisible(g_engine.hwnd)) {
            HWND foreground = GetForegroundWindow();
            
            // If parent somehow got foreground focus, bring dialog back to front
            if (foreground == g_engine.parent) {
                SetWindowPos(g_engine.hwnd, HWND_TOP, 0, 0, 0, 0, 
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                SetForegroundWindow(g_engine.hwnd);
            }
            
            // Ensure dialog is always above parent in Z-order
            SetWindowPos(g_engine.hwnd, g_engine.parent, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
            
        if (!wglMakeCurrent(g_engine.hdc, g_engine.hglrc)) return;
            
        RECT rect;
        GetClientRect(g_engine.hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        if (width <= 0 || height <= 0) return;
        
        glViewport(0, 0, width, height);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        for (int i = 0; i < DIALOG_COUNT; i++) {
            if (g_engine.dialogs[i].isOpen && g_engine.dialogs[i].callback) {
                if (g_engine.hwnd) {
                    SetWindowTextA(g_engine.hwnd, g_engine.dialogs[i].title);
                }
                g_engine.dialogs[i].callback();
            }
        }
        
        if (!IsAnyDialogOpen()) {
            Hide();
        }
        
        ImGui::Render();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data) {
            ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        }
        
        SwapBuffers(g_engine.hdc);
    }
    
    // =========================================================================
    // DIALOG FRAME MANAGEMENT
    // =========================================================================
    
    bool BeginDialog(const char* title, bool* open) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                ImGuiWindowFlags_NoBringToFrontOnFocus;
                                
        return ImGui::Begin(title, open, flags);
    }
    
    void EndDialog() {
        ImGui::End();
    }
    
    // =========================================================================
    // WINDOW SIZE MANAGEMENT
    // =========================================================================
    
    void SetNextWindowPos(float x, float y) {
        if (g_engine.hwnd) {
            SetWindowPos(g_engine.hwnd, NULL, (int)x, (int)y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    void SetNextWindowSize(float width, float height)
    {
        if (g_engine.hwnd && width > 0 && height > 0 && g_engine.activeDialogType >= 0)
        {
            if (g_engine.isResizing)
                return;
            g_engine.isResizing = true;

            DialogInfo &activeDialog = g_engine.dialogs[g_engine.activeDialogType];

            int newWidth = (int)width;
            int newHeight = (int)height;

            if (abs(newWidth - activeDialog.currentWidth) < 5 &&
                abs(newHeight - activeDialog.currentHeight) < 5)
            {
                g_engine.isResizing = false;
                return;
            }

            activeDialog.currentWidth = newWidth;
            activeDialog.currentHeight = newHeight;
            activeDialog.hasCustomSize = true;

            int winWidth = newWidth + 16;
            int winHeight = newHeight + 39;

            winWidth = max(300, min(1400, winWidth));
            winHeight = max(200, min(900, winHeight));

            RECT currentRect;
            if (GetWindowRect(g_engine.hwnd, &currentRect))
            {
                SetWindowPos(g_engine.hwnd, NULL,
                             currentRect.left, currentRect.top,
                             winWidth, winHeight,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
            }

            g_engine.isResizing = false;
        }
    }

    void SetNextWindowFocus() {
        if (g_engine.hwnd) {
            SetForegroundWindow(g_engine.hwnd);
            SetFocus(g_engine.hwnd);
        }
    }
        
    // =========================================================================
    // THEME FUNCTIONS
    // =========================================================================
    
    void ApplyTheme(Theme theme) {
        g_engine.currentTheme = theme;
        ImGuiStyle& style = ImGui::GetStyle();
        
        switch (theme) {
        case THEME_DARK:
            ImGui::StyleColorsDark();
            break;
            
        case THEME_LIGHT:
            ImGui::StyleColorsLight();
            break;
            
        case THEME_CLASSIC:
            ImGui::StyleColorsClassic();
            break;
            
        case THEME_PHOTOSHOP: {
            ImGui::StyleColorsDark();
            
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            
            style.WindowRounding = 4.0f;
            style.FrameRounding = 2.0f;
            style.ScrollbarRounding = 2.0f;
            style.GrabRounding = 2.0f;
            style.WindowPadding = ImVec2(8, 8);
            style.FramePadding = ImVec2(4, 3);
            style.ItemSpacing = ImVec2(8, 4);
            style.ItemInnerSpacing = ImVec2(4, 4);
            break;
        }
        
        case THEME_HIGH_CONTRAST: {
            ImGui::StyleColorsDark();
            ImVec4* colors = style.Colors;
            
            colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            
            style.WindowBorderSize = 2.0f;
            style.FrameBorderSize = 1.0f;
            break;
        }
        }
    }
}