#include "stdafx.h"
#include "imgui_integration.h"
#include <windowsx.h>

// ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

// OpenGL includes (built into Windows)
#include <GL/gl.h>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <cfloat>
#include <climits>

// Forward declare message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
        
        // Set preferred size for this dialog type
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
        bool isResizing;  // Simpler resize flag
        
        // Constructor
        EngineState() : parent(NULL), hwnd(NULL), hdc(NULL), hglrc(NULL), 
                       initialized(false), currentTheme(THEME_DARK),
                       activeDialogType(-1), windowVisible(false), isResizing(false) {
            
            // Set default sizes for each dialog type
            dialogs[DIALOG_PROPERTIES].SetPreferredSize(600, 500);      // Properties: smaller
            dialogs[DIALOG_ABOUT].SetPreferredSize(550, 450);          // About: medium  
            dialogs[DIALOG_CLUT_GENERATOR].SetPreferredSize(1000, 750); // CLUT: larger
        }
    };
    
    static EngineState g_engine;
        
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

        if (g_engine.hwnd)
        {
            // Resize window to dialog's preferred size
            DialogInfo &dialog = g_engine.dialogs[type];

            // Calculate Win32 window size
            int winWidth = dialog.currentWidth + 16;   // Padding for borders
            int winHeight = dialog.currentHeight + 39; // Padding for title bar + borders

            // Clamp to reasonable sizes
            winWidth = max(300, min(1400, winWidth));
            winHeight = max(200, min(900, winHeight));

            // Get current position or center if first time
            RECT currentRect;
            bool hasCurrentPos = GetWindowRect(g_engine.hwnd, &currentRect);

            if (!g_engine.windowVisible || !hasCurrentPos)
            {
                // Center on screen for first show
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                int x = (screenWidth - winWidth) / 2;
                int y = (screenHeight - winHeight) / 2;

                SetWindowPos(g_engine.hwnd, NULL, x, y, winWidth, winHeight, SWP_NOZORDER);
            }
            else
            {
                // Keep current position, just resize
                SetWindowPos(g_engine.hwnd, NULL,
                             currentRect.left, currentRect.top,
                             winWidth, winHeight,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }

            ShowWindow(g_engine.hwnd, SW_SHOW);
            SetForegroundWindow(g_engine.hwnd);
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
    
    // =========================================================================
    // HELPER FUNCTIONS
    // =========================================================================
    
    ImVec4 ToImVec4(const ImGuiColor& color) {
        return ImVec4(color.r, color.g, color.b, color.a);
    }
    
    ImU32 ToImU32(const ImGuiColor& color) {
        return IM_COL32((int)(color.r * 255), (int)(color.g * 255), (int)(color.b * 255), (int)(color.a * 255));
    }
    
    ImGuiStyleVar ConvertStyleVar(int var) {
        switch (var) {
        case IMGUI_STYLE_VAR_ALPHA: return ImGuiStyleVar_Alpha;
        case IMGUI_STYLE_VAR_WINDOW_PADDING: return ImGuiStyleVar_WindowPadding;
        case IMGUI_STYLE_VAR_WINDOW_ROUNDING: return ImGuiStyleVar_WindowRounding;
        case IMGUI_STYLE_VAR_FRAME_PADDING: return ImGuiStyleVar_FramePadding;
        case IMGUI_STYLE_VAR_FRAME_ROUNDING: return ImGuiStyleVar_FrameRounding;
        case IMGUI_STYLE_VAR_ITEM_SPACING: return ImGuiStyleVar_ItemSpacing;
        case IMGUI_STYLE_VAR_ITEM_INNER_SPACING: return ImGuiStyleVar_ItemInnerSpacing;
        case IMGUI_STYLE_VAR_INDENT_SPACING: return ImGuiStyleVar_IndentSpacing;
        default: return ImGuiStyleVar_Alpha;
        }
    }
    
    ImGuiCol ConvertStyleColor(int colorId) {
        switch (colorId) {
        case IMGUI_COL_TEXT: return ImGuiCol_Text;
        case IMGUI_COL_TEXT_DISABLED: return ImGuiCol_TextDisabled;
        case IMGUI_COL_WINDOW_BG: return ImGuiCol_WindowBg;
        case IMGUI_COL_CHILD_BG: return ImGuiCol_ChildBg;
        case IMGUI_COL_POPUP_BG: return ImGuiCol_PopupBg;
        case IMGUI_COL_BORDER: return ImGuiCol_Border;
        case IMGUI_COL_FRAME_BG: return ImGuiCol_FrameBg;
        case IMGUI_COL_FRAME_BG_HOVERED: return ImGuiCol_FrameBgHovered;
        case IMGUI_COL_FRAME_BG_ACTIVE: return ImGuiCol_FrameBgActive;
        case IMGUI_COL_TITLE_BG: return ImGuiCol_TitleBg;
        case IMGUI_COL_TITLE_BG_ACTIVE: return ImGuiCol_TitleBgActive;
        case IMGUI_COL_BUTTON: return ImGuiCol_Button;
        case IMGUI_COL_BUTTON_HOVERED: return ImGuiCol_ButtonHovered;
        case IMGUI_COL_BUTTON_ACTIVE: return ImGuiCol_ButtonActive;
        case IMGUI_COL_HEADER: return ImGuiCol_Header;
        case IMGUI_COL_HEADER_HOVERED: return ImGuiCol_HeaderHovered;
        case IMGUI_COL_HEADER_ACTIVE: return ImGuiCol_HeaderActive;
        default: return ImGuiCol_Text;
        }
    }
    
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Handle ImGui messages first
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
            
        switch (msg) {
        case WM_SIZE:
            // Smoother resize handling
            if (g_engine.hglrc && wParam != SIZE_MINIMIZED && !g_engine.isResizing) {
                int newWidth = LOWORD(lParam);
                int newHeight = HIWORD(lParam);
                
                // Update active dialog's size when user manually resizes
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
            // User started resizing - set flag to prevent conflicts
            g_engine.isResizing = true;
            return 0;
            
        case WM_EXITSIZEMOVE:
            // User finished resizing - clear flag
            g_engine.isResizing = false;
            return 0;
            
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
            
        case WM_CLOSE:
            // Better close handling
            Hide();
            return 0;
            
        case WM_DESTROY:
            // Only allow destroy during shutdown
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

    bool Initialize(HWND parent)
    {
        g_engine.parent = parent;

        // Create window class
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

        // FIXED: Use default size for initial window creation
        int defaultWidth = 500;
        int defaultHeight = 600;

        // FIXED: Simplified positioning - center on screen by default
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - defaultWidth) / 2;
        int y = (screenHeight - defaultHeight) / 2;

        // FIXED: Correct CreateWindow call (not CreateWindowEx)
        g_engine.hwnd = CreateWindowW(
            wc.lpszClassName,
            L"Dialog",
            WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, // Remove maximize to avoid sizing conflicts
            x, y, defaultWidth, defaultHeight,
            NULL, // No parent to avoid complex interactions
            NULL,
            wc.hInstance,
            NULL);

        if (!g_engine.hwnd)
        {
            return false;
        }

        // Initialize OpenGL
        if (!CreateOpenGLContext(g_engine.hwnd))
        {
            CleanupOpenGL();
            DestroyWindow(g_engine.hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(g_engine.hwnd))
        {
            return false;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 130"))
        {
            return false;
        }

        // Apply default theme
        ApplyTheme(THEME_PHOTOSHOP);

        g_engine.initialized = true;
        return true;
    }

    void Shutdown() {
        if (!g_engine.initialized) return;
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        CleanupOpenGL();
        if (g_engine.hwnd) {
            DestroyWindow(g_engine.hwnd);
            g_engine.hwnd = NULL;
        }
        UnregisterClassW(L"ImGuiDialogs", GetModuleHandle(NULL));
        
        g_engine = EngineState(); // Reset state
    }
    
    void Hide() {
        for (int i = 0; i < DIALOG_COUNT; i++) {
            g_engine.dialogs[i].isOpen = false;
        }
               
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
        
        for (int i = 0; i < DIALOG_COUNT; i++) {
            if (g_engine.dialogs[i].isOpen && g_engine.dialogs[i].callback) {
                if (g_engine.hwnd) {
                    SetWindowTextA(g_engine.hwnd, g_engine.dialogs[i].title);
                }
                g_engine.dialogs[i].callback();
            }
        }
        
        // Hide window if no dialogs are open after callbacks
        if (!IsAnyDialogOpen()) {
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
    
    // =========================================================================
    // DIALOG MANAGEMENT
    // =========================================================================
    
    bool BeginDialog(const char* title, bool* open) {
        // FIXED: Create seamless fullscreen ImGui window that fills Win32 window exactly
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        // FIXED: Better window flags for seamless integration
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
        // FIXED: Move Win32 window instead of ImGui window for better control
        if (g_engine.hwnd) {
            SetWindowPos(g_engine.hwnd, NULL, (int)x, (int)y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    void SetNextWindowSize(float width, float height)
    {
        if (g_engine.hwnd && width > 0 && height > 0 && g_engine.activeDialogType >= 0)
        {

            // Prevent recursive resizing
            if (g_engine.isResizing)
                return;
            g_engine.isResizing = true;

            DialogInfo &activeDialog = g_engine.dialogs[g_engine.activeDialogType];

            // Only resize if size actually changed significantly (avoid micro-adjustments)
            int newWidth = (int)width;
            int newHeight = (int)height;

            if (abs(newWidth - activeDialog.currentWidth) < 5 &&
                abs(newHeight - activeDialog.currentHeight) < 5)
            {
                g_engine.isResizing = false;
                return; // Skip minor size changes
            }

            // Update dialog's current size
            activeDialog.currentWidth = newWidth;
            activeDialog.currentHeight = newHeight;
            activeDialog.hasCustomSize = true;

            // Calculate Win32 window size
            int winWidth = newWidth + 16;
            int winHeight = newHeight + 39;

            // Clamp to reasonable sizes
            winWidth = max(300, min(1400, winWidth));
            winHeight = max(200, min(900, winHeight));

            // Get current position
            RECT currentRect;
            if (GetWindowRect(g_engine.hwnd, &currentRect))
            {
                // Smooth resize without forcing
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
        
    // THEME FUNCTIONS
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
            // Custom Photoshop-like theme
            ImGui::StyleColorsDark();
            
            // Customize colors for a more professional look
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
            
            // Adjust style parameters
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
            
            // High contrast colors
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
    
    void PushStyleVar(int var, float value) {
        ImGui::PushStyleVar(ConvertStyleVar(var), value);
    }
    
    void PushStyleVar2(int var, float x, float y) {
        ImGui::PushStyleVar(ConvertStyleVar(var), ImVec2(x, y));
    }
    
    void PushStyleColor(int colorId, ImGuiColor color) {
        ImGui::PushStyleColor(ConvertStyleColor(colorId), ToImVec4(color));
    }
    
    void PushStyleColor(int colorId, float r, float g, float b, float a) {
        ImGui::PushStyleColor(ConvertStyleColor(colorId), ImVec4(r, g, b, a));
    }
    
    void PopStyleVar(int count) {
        ImGui::PopStyleVar(count);
    }
    
    void PopStyleColor(int count) {
        ImGui::PopStyleColor(count);
    }
    
    void SetWindowRounding(float rounding) {
        ImGui::GetStyle().WindowRounding = rounding;
    }
    
    void SetFrameRounding(float rounding) {
        ImGui::GetStyle().FrameRounding = rounding;
    }
    
    void SetScrollbarRounding(float rounding) {
        ImGui::GetStyle().ScrollbarRounding = rounding;
    }
    
    void SetGrabRounding(float rounding) {
        ImGui::GetStyle().GrabRounding = rounding;
    }
        
    bool BeginChild(const char* id, float width, float height, bool border) {
        return ImGui::BeginChild(id, ImVec2(width, height), border);
    }
    
    void EndChild() {
        ImGui::EndChild();
    }
    
    bool BeginGroup() {
        ImGui::BeginGroup();
        return true;
    }
    
    void EndGroup() {
        ImGui::EndGroup();
    }
    
    void Separator() {
        ImGui::Separator();
    }
    
    void SameLine(float offset_from_start_x, float spacing) {
        ImGui::SameLine(offset_from_start_x, spacing);
    }
    
    void NewLine() {
        ImGui::NewLine();
    }
    
    void Spacing() {
        ImGui::Spacing();
    }
    
    void Dummy(float width, float height) {
        ImGui::Dummy(ImVec2(width, height));
    }
    
    void Indent(float indent_w) {
        ImGui::Indent(indent_w);
    }
    
    void Unindent(float indent_w) {
        ImGui::Unindent(indent_w);
    }
    
    void AlignTextToFramePadding() {
        ImGui::AlignTextToFramePadding();
    }
    
    void CenterNextItem(float itemWidth) {
        float windowWidth = ImGui::GetWindowSize().x;
        float center = (windowWidth - itemWidth) * 0.5f;
        if (center > 0) {
            ImGui::SetCursorPosX(center);
        }
    }
    
    void RightAlignNextItem(float itemWidth) {
        float windowWidth = ImGui::GetWindowSize().x;
        float rightAlign = windowWidth - itemWidth - ImGui::GetStyle().WindowPadding.x;
        if (rightAlign > 0) {
            ImGui::SetCursorPosX(rightAlign);
        }
    }
    
    // TEXT FUNCTIONS
    void Text(const char* text) {
        ImGui::Text("%s", text);
    }
    
    void TextFormatted(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }
    
    void TextColored(ImGuiColor color, const char* text) {
        ImGui::TextColored(ToImVec4(color), "%s", text);
    }
    
    void TextColored(float r, float g, float b, float a, const char* text) {
        ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
    }
    
    void TextDisabled(const char* text) {
        ImGui::TextDisabled("%s", text);
    }
    
    void TextWrapped(const char* text) {
        ImGui::TextWrapped("%s", text);
    }
    
    void LabelText(const char* label, const char* text) {
        ImGui::LabelText(label, "%s", text);
    }
    
    void BulletText(const char* text) {
        ImGui::BulletText("%s", text);
    }
    
    bool CollapsingHeader(const char* label, bool defaultOpen) {
        ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::CollapsingHeader(label, flags);
    }
    
    bool TreeNode(const char* label) {
        return ImGui::TreeNode(label);
    }
    
    bool TreeNodeEx(const char* label, bool defaultOpen) {
        ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::TreeNodeEx(label, flags);
    }
    
    void TreePop() {
        ImGui::TreePop();
    }
    
    // BUTTON FUNCTIONS
    bool Button(const char* label) {
        return ImGui::Button(label);
    }
    
    bool Button(const char* label, float width, float height) {
        return ImGui::Button(label, ImVec2(width, height));
    }
    
    bool SmallButton(const char* label) {
        return ImGui::SmallButton(label);
    }
    
    bool InvisibleButton(const char* str_id, float width, float height) {
        return ImGui::InvisibleButton(str_id, ImVec2(width, height));
    }
    
    bool ArrowButton(const char* str_id, int dir) {
        ImGuiDir direction = (ImGuiDir)dir;
        return ImGui::ArrowButton(str_id, direction);
    }
    
    bool ImageButton(const char* str_id, void* texture_id, float width, float height) {
        return ImGui::ImageButton(str_id, texture_id, ImVec2(width, height));
    }
    
    bool ButtonColored(const char* label, ImGuiColor color) {
        ImGui::PushStyleColor(ImGuiCol_Button, ToImVec4(color));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor();
        return result;
    }
    
    bool ButtonColored(const char* label, float r, float g, float b, float a) {
        return ButtonColored(label, ImGuiColor(r, g, b, a));
    }
    
    // INPUT WIDGETS
    bool InputInt(const char* label, int* value, int step, int step_fast) {
        return ImGui::InputInt(label, value, step, step_fast);
    }
    
    bool InputFloat(const char* label, float* value, float step, float step_fast, int decimal_precision) {
        const char* format = (decimal_precision >= 0) ? "%.3f" : "%.3f";
        return ImGui::InputFloat(label, value, step, step_fast, format);
    }
    
    bool InputDouble(const char* label, double* value, double step, double step_fast, const char* format) {
        return ImGui::InputDouble(label, value, step, step_fast, format);
    }
    
    bool InputText(const char* label, char* buf, size_t buf_size) {
        return ImGui::InputText(label, buf, buf_size);
    }
    
    bool InputTextMultiline(const char* label, char* buf, size_t buf_size, float width, float height) {
        return ImGui::InputTextMultiline(label, buf, buf_size, ImVec2(width, height));
    }
    
    bool Checkbox(const char* label, bool* value) {
        return ImGui::Checkbox(label, value);
    }
    
    bool SliderInt(const char* label, int* value, int min_value, int max_value) {
        return ImGui::SliderInt(label, value, min_value, max_value);
    }
    
    bool SliderFloat(const char* label, float* value, float min_value, float max_value) {
        return ImGui::SliderFloat(label, value, min_value, max_value);
    }
    
    bool SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max) {
        return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max);
    }
    
    bool DragInt(const char* label, int* value, float v_speed, int v_min, int v_max) {
        return ImGui::DragInt(label, value, v_speed, v_min, v_max);
    }
    
    bool DragFloat(const char* label, float* value, float v_speed, float v_min, float v_max) {
        return ImGui::DragFloat(label, value, v_speed, v_min, v_max);
    }
    
    bool ColorEdit3(const char* label, float col[3]) {
        return ImGui::ColorEdit3(label, col);
    }
    
    bool ColorEdit4(const char* label, float col[4]) {
        return ImGui::ColorEdit4(label, col);
    }
    
    bool ColorPicker3(const char* label, float col[3]) {
        return ImGui::ColorPicker3(label, col);
    }
    
    bool ColorPicker4(const char* label, float col[4]) {
        return ImGui::ColorPicker4(label, col);
    }
    
    // SELECTION WIDGETS
    bool BeginCombo(const char* label, const char* preview_value) {
        return ImGui::BeginCombo(label, preview_value);
    }
    
    void EndCombo() {
        ImGui::EndCombo();
    }
    
    bool Combo(const char* label, int* current_item, const char* const items[], int items_count) {
        return ImGui::Combo(label, current_item, items, items_count);
    }
    
    bool BeginListBox(const char* label, float width, float height) {
        return ImGui::BeginListBox(label, ImVec2(width, height));
    }
    
    void EndListBox() {
        ImGui::EndListBox();
    }
    
    bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items) {
        return ImGui::ListBox(label, current_item, items, items_count, height_in_items);
    }
    
    bool Selectable(const char* label, bool selected) {
        return ImGui::Selectable(label, selected);
    }
    
    bool Selectable(const char* label, bool* p_selected) {
        return ImGui::Selectable(label, p_selected);
    }
    
    bool RadioButton(const char* label, bool active) {
        return ImGui::RadioButton(label, active);
    }
    
    bool RadioButton(const char* label, int* v, int v_button) {
        return ImGui::RadioButton(label, v, v_button);
    }
    
    // TOOLTIPS AND POPUPS
    void SetTooltip(const char* text) {
        ImGui::SetTooltip("%s", text);
    }
    
    void SetTooltipFormatted(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        ImGui::SetTooltipV(fmt, args);
        va_end(args);
    }
    
    bool BeginTooltip() {
        ImGui::BeginTooltip();
        return true;
    }
    
    void EndTooltip() {
        ImGui::EndTooltip();
    }
    
    bool BeginPopup(const char* str_id) {
        return ImGui::BeginPopup(str_id);
    }
    
    bool BeginPopupModal(const char* name, bool* p_open) {
        return ImGui::BeginPopupModal(name, p_open);
    }
    
    void EndPopup() {
        ImGui::EndPopup();
    }
    
    void OpenPopup(const char* str_id) {
        ImGui::OpenPopup(str_id);
    }
    
    void CloseCurrentPopup() {
        ImGui::CloseCurrentPopup();
    }
    
    // TABLES
    bool BeginTable(const char* str_id, int column_count) {
        return ImGui::BeginTable(str_id, column_count);
    }
    
    void EndTable() {
        ImGui::EndTable();
    }
    
    void TableNextRow() {
        ImGui::TableNextRow();
    }
    
    bool TableNextColumn() {
        return ImGui::TableNextColumn();
    }
    
    void TableSetupColumn(const char* label) {
        ImGui::TableSetupColumn(label);
    }
    
    void TableHeadersRow() {
        ImGui::TableHeadersRow();
    }
    
    // UTILITY FUNCTIONS
    void SetCursorPosX(float local_x) {
        ImGui::SetCursorPosX(local_x);
    }
    
    void SetCursorPosY(float local_y) {
        ImGui::SetCursorPosY(local_y);
    }
    
    void SetCursorPos(float local_x, float local_y) {
        ImGui::SetCursorPos(ImVec2(local_x, local_y));
    }
    
    float GetCursorPosX() {
        return ImGui::GetCursorPosX();
    }
    
    float GetCursorPosY() {
        return ImGui::GetCursorPosY();
    }
    
    float GetContentRegionAvailWidth() {
        return ImGui::GetContentRegionAvail().x;
    }
    
    float GetContentRegionAvailHeight() {
        return ImGui::GetContentRegionAvail().y;
    }
    
    float GetWindowWidth() {
        return ImGui::GetWindowSize().x;
    }
    
    float GetWindowHeight() {
        return ImGui::GetWindowSize().y;
    }
    
    bool IsItemHovered() {
        return ImGui::IsItemHovered();
    }
    
    bool IsItemActive() {
        return ImGui::IsItemActive();
    }
    
    bool IsItemClicked(int mouse_button) {
        return ImGui::IsItemClicked(mouse_button);
    }
    
    bool IsItemVisible() {
        return ImGui::IsItemVisible();
    }
    
    bool IsItemEdited() {
        return ImGui::IsItemEdited();
    }
    
    bool IsItemActivated() {
        return ImGui::IsItemActivated();
    }
    
    bool IsItemDeactivated() {
        return ImGui::IsItemDeactivated();
    }
    
    bool IsItemDeactivatedAfterEdit() {
        return ImGui::IsItemDeactivatedAfterEdit();
    }
    
    void SetItemDefaultFocus() {
        ImGui::SetItemDefaultFocus();
    }
    
    void SetKeyboardFocusHere(int offset) {
        ImGui::SetKeyboardFocusHere(offset);
    }

    void SetNextItemWidth(float item_width) {
        ImGui::SetNextItemWidth(item_width);
    }
    
    void PushItemWidth(float item_width) {
        ImGui::PushItemWidth(item_width);
    }
    
    void PopItemWidth() {
        ImGui::PopItemWidth();
    }
    
    float CalcItemWidth() {
        return ImGui::CalcItemWidth();
    }
    
    // DRAWING AND GRAPHICS
    void DrawLine(float x1, float y1, float x2, float y2, ImGuiColor color, float thickness) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ToImU32(color), thickness);
    }
    
    void DrawRect(float x, float y, float width, float height, ImGuiColor color, float rounding, float thickness) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), ToImU32(color), rounding, 0, thickness);
    }
    
    void DrawRectFilled(float x, float y, float width, float height, ImGuiColor color, float rounding) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), ToImU32(color), rounding);
    }
    
    void DrawCircle(float center_x, float center_y, float radius, ImGuiColor color, int num_segments, float thickness) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddCircle(ImVec2(center_x, center_y), radius, ToImU32(color), num_segments, thickness);
    }
    
    void DrawCircleFilled(float center_x, float center_y, float radius, ImGuiColor color, int num_segments) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddCircleFilled(ImVec2(center_x, center_y), radius, ToImU32(color), num_segments);
    }
    
    void DrawText(float x, float y, ImGuiColor color, const char* text) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddText(ImVec2(x, y), ToImU32(color), text);
    }
    
    // CONVENIENCE FUNCTIONS FOR YOUR APP
    bool PropertyInt(const char* label, int* value, int min_val, int max_val) {
        bool changed = ImGui::InputInt(label, value);
        if (changed) {
            *value = (std::max)(min_val, (std::min)(max_val, *value));
        }
        return changed;
    }
    
    bool PropertyFloat(const char* label, float* value, float min_val, float max_val) {
        bool changed = ImGui::InputFloat(label, value);
        if (changed) {
            *value = (std::max)(min_val, (std::min)(max_val, *value));
        }
        return changed;
    }
    
    bool PropertyBool(const char* label, bool* value) {
        return ImGui::Checkbox(label, value);
    }
    
    bool PropertyText(const char* label, char* buffer, size_t buffer_size) {
        return ImGui::InputText(label, buffer, buffer_size);
    }
    
    bool BeginPropertySection(const char* name, bool defaultOpen) {
        return CollapsingHeader(name, defaultOpen);
    }
    
    void EndPropertySection() {
        // Nothing needed for collapsing headers
    }
    
    void ShowStatus(const char* text, ImGuiColor color) {
        ImGui::TextColored(ToImVec4(color), "%s", text);
    }
    
    void ShowError(const char* text) {
        ShowStatus(text, ImGuiColor(1.0f, 0.3f, 0.3f, 1.0f));
    }
    
    void ShowWarning(const char* text) {
        ShowStatus(text, ImGuiColor(1.0f, 0.8f, 0.3f, 1.0f));
    }
    
    void ShowSuccess(const char* text) {
        ShowStatus(text, ImGuiColor(0.3f, 1.0f, 0.3f, 1.0f));
    }
    
    void HelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    
    void HelpTooltip(const char* desc) {
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    
    bool ConfirmationDialog(const char* title, const char* message, bool* open) {
        bool result = false;
        
        if (*open) {
            ImGui::OpenPopup(title);
        }
        
        if (ImGui::BeginPopupModal(title, open)) {
            ImGui::Text("%s", message);
            ImGui::Separator();
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                result = true;
                *open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                *open = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        return result;
    }
    
    void ProgressBar(float fraction, float width, float height) {
        ImGui::ProgressBar(fraction, ImVec2(width, height));
    }
    
    void ProgressBar(float fraction, const char* overlay, float width, float height) {
        ImGui::ProgressBar(fraction, ImVec2(width, height), overlay);
    }
}
