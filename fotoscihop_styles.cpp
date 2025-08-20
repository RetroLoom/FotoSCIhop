#include "stdafx.h"
#include "fotoscihop_styles.h"

namespace FotoSCIhopStyles {
    
    // ========================================================================
    // COLOR DEFINITIONS (using float arrays to avoid constexpr issues)
    // ========================================================================
    
    // Text Colors
    const float TEXT_NORMAL[4]      = {1.0f, 1.0f, 1.0f, 1.0f};    // White
    const float TEXT_HEADER[4]      = {0.8f, 0.9f, 1.0f, 1.0f};    // Light Blue
    const float TEXT_DISABLED[4]    = {0.5f, 0.5f, 0.5f, 1.0f};    // Gray
    const float TEXT_WARNING[4]     = {1.0f, 0.8f, 0.3f, 1.0f};    // Orange
    const float TEXT_ERROR[4]       = {1.0f, 0.3f, 0.3f, 1.0f};    // Red
    const float TEXT_SUCCESS[4]     = {0.3f, 1.0f, 0.3f, 1.0f};    // Green
    const float TEXT_INFO[4]        = {0.3f, 0.8f, 1.0f, 1.0f};    // Cyan
    
    // Button Colors
    const float BUTTON_APPLY[4]     = {0.2f, 0.7f, 0.2f, 1.0f};    // Green
    const float BUTTON_CANCEL[4]    = {0.8f, 0.3f, 0.3f, 1.0f};    // Red
    const float BUTTON_CLOSE[4]     = {0.6f, 0.6f, 0.8f, 1.0f};    // Purple
    const float BUTTON_ADD[4]       = {0.3f, 0.6f, 0.9f, 1.0f};    // Blue
    const float BUTTON_REMOVE[4]    = {0.9f, 0.4f, 0.4f, 1.0f};    // Light Red
    const float BUTTON_NEUTRAL[4]   = {0.5f, 0.5f, 0.5f, 1.0f};    // Gray
    
    // Section Colors
    const float SECTION_FILE_INFO[4]    = {0.2f, 0.3f, 0.4f, 0.8f};    // Dark Blue
    const float SECTION_RESOLUTION[4]   = {0.3f, 0.4f, 0.2f, 0.8f};    // Dark Green
    const float SECTION_CELL_PROPS[4]   = {0.4f, 0.3f, 0.2f, 0.8f};    // Dark Brown
    const float SECTION_MANAGEMENT[4]   = {0.3f, 0.2f, 0.4f, 0.8f};    // Dark Purple
    
    // Status Colors
    const float STATUS_MODIFIED[4]      = {1.0f, 0.8f, 0.3f, 1.0f};    // Orange
    const float STATUS_SAVED[4]         = {0.3f, 1.0f, 0.3f, 1.0f};    // Green
    const float STATUS_UNCHANGED[4]     = {0.7f, 0.7f, 0.7f, 1.0f};    // Gray
    
    // ========================================================================
    // STYLE SETTINGS
    // ========================================================================
    
    // Window and Frame Settings
    const float StyleSettings::WINDOW_ROUNDING = 6.0f;
    const float StyleSettings::FRAME_ROUNDING = 4.0f;
    const float StyleSettings::SCROLLBAR_ROUNDING = 3.0f;
    const float StyleSettings::GRAB_ROUNDING = 3.0f;
    
    // Padding and Spacing
    const float StyleSettings::WINDOW_PADDING_X = 10.0f;
    const float StyleSettings::WINDOW_PADDING_Y = 10.0f;
    const float StyleSettings::FRAME_PADDING_X = 6.0f;
    const float StyleSettings::FRAME_PADDING_Y = 4.0f;
    const float StyleSettings::ITEM_SPACING_X = 8.0f;
    const float StyleSettings::ITEM_SPACING_Y = 6.0f;
    const float StyleSettings::ITEM_INNER_SPACING_X = 4.0f;
    const float StyleSettings::ITEM_INNER_SPACING_Y = 4.0f;
    const float StyleSettings::INDENT_SPACING = 20.0f;
    
    // Border and Line Settings
    const float StyleSettings::WINDOW_BORDER_SIZE = 1.0f;
    const float StyleSettings::FRAME_BORDER_SIZE = 1.0f;
    const float StyleSettings::POPUP_BORDER_SIZE = 1.0f;
    
    // Alpha/Transparency Settings
    const float StyleSettings::ALPHA_DISABLED = 0.5f;
    const float StyleSettings::ALPHA_SUBTLE = 0.7f;
    const float StyleSettings::ALPHA_NORMAL = 1.0f;
    
    // ========================================================================
    // GLOBAL STATE
    // ========================================================================
    
    static ThemeMode g_currentTheme = ThemeMode::PHOTOSHOP_DARK;
    static bool g_stylesNeedRefresh = true;
    static int g_styleStackDepth = 0;  // Track how many styles we've pushed
    
    // ========================================================================
    // THEME IMPLEMENTATION
    // ========================================================================
    
    void ApplyPhotoshopDarkTheme() {
        // Clear any existing style stack
        if (g_styleStackDepth > 0) {
            ImGui::PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        // Apply base dark theme
        ImGui::StyleColorsDark();
        
        // Customize with Photoshop-like colors
        ImGuiStyle& style = ImGui::GetStyle();
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
        
        // Apply our custom settings
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(StyleSettings::WINDOW_PADDING_X, StyleSettings::WINDOW_PADDING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(StyleSettings::FRAME_PADDING_X, StyleSettings::FRAME_PADDING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(StyleSettings::ITEM_SPACING_X, StyleSettings::ITEM_SPACING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(StyleSettings::ITEM_INNER_SPACING_X, StyleSettings::ITEM_INNER_SPACING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, StyleSettings::INDENT_SPACING);
        
        style.WindowRounding = StyleSettings::WINDOW_ROUNDING;
        style.FrameRounding = StyleSettings::FRAME_ROUNDING;
        style.ScrollbarRounding = StyleSettings::SCROLLBAR_ROUNDING;
        style.GrabRounding = StyleSettings::GRAB_ROUNDING;
        
        g_styleStackDepth = 5;  // Track that we pushed 5 style variables
    }
    
    void ApplyPhotoshopLightTheme() {
        if (g_styleStackDepth > 0) {
            ImGui::PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        // Apply base light theme
        ImGui::StyleColorsLight();
        
        // Light theme customizations
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.94f, 0.94f, 0.94f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.85f, 0.85f, 0.85f, 1.00f));
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(StyleSettings::WINDOW_PADDING_X, StyleSettings::WINDOW_PADDING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(StyleSettings::FRAME_PADDING_X, StyleSettings::FRAME_PADDING_Y));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(StyleSettings::ITEM_SPACING_X, StyleSettings::ITEM_SPACING_Y));
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = StyleSettings::WINDOW_ROUNDING;
        style.FrameRounding = StyleSettings::FRAME_ROUNDING;
        
        g_styleStackDepth = 3;
    }
    
    void ApplyHighContrastTheme() {
        if (g_styleStackDepth > 0) {
            ImGui::PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        // Apply base dark theme
        ImGui::StyleColorsDark();
        
        // High contrast customizations
        ImGuiStyle& style = ImGui::GetStyle();
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
        
        // High contrast uses sharp edges
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.WindowBorderSize = 2.0f;
        style.FrameBorderSize = 1.0f;
    }
    
    void ApplyRetroSCITheme() {
        if (g_styleStackDepth > 0) {
            ImGui::PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        // Apply base dark theme
        ImGui::StyleColorsDark();
        
        // Retro SCI colors (dark blue theme)
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.2f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.25f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.3f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.5f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.4f, 1.00f));
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 2.0f;
        style.FrameRounding = 1.0f;
        style.ScrollbarRounding = 1.0f;
        style.GrabRounding = 1.0f;
    }
    
    void ApplyCustomTheme() {
        // Users can modify this function for their own custom theme
        ApplyPhotoshopDarkTheme();  // Default to dark theme
    }
    
    // ========================================================================
    // THEME MANAGEMENT
    // ========================================================================
    
    void SetTheme(ThemeMode theme) {
        g_currentTheme = theme;
        g_stylesNeedRefresh = true;
    }
    
    void RefreshTheme() {
        if (!g_stylesNeedRefresh) return;
        
        // Apply ImGui theme
        switch (g_currentTheme) {
        case ThemeMode::PHOTOSHOP_DARK:
            ApplyPhotoshopDarkTheme();
            break;
        case ThemeMode::PHOTOSHOP_LIGHT:
            ApplyPhotoshopLightTheme();
            break;
        case ThemeMode::HIGH_CONTRAST:
            ApplyHighContrastTheme();
            break;
        case ThemeMode::RETRO_SCI:
            ApplyRetroSCITheme();
            break;
        case ThemeMode::CUSTOM:
            ApplyCustomTheme();
            break;
        }
        
        // Force Win32 window redraw to update colors
        extern HWND hWnd;  // Reference the global hWnd from main application
        if (::hWnd) {  // Use :: to specify global scope
            InvalidateRect(::hWnd, NULL, TRUE);
        }
        
        g_stylesNeedRefresh = false;
    }
    
    ThemeMode GetCurrentTheme() {
        return g_currentTheme;
    }
    
    // ========================================================================
    // CONVENIENCE STYLING FUNCTIONS
    // ========================================================================
    
    // Colored text helpers
    void HeaderText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_HEADER[0], TEXT_HEADER[1], TEXT_HEADER[2], TEXT_HEADER[3]), "%s", text);
    }
    
    void WarningText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_WARNING[0], TEXT_WARNING[1], TEXT_WARNING[2], TEXT_WARNING[3]), "%s", text);
    }
    
    void ErrorText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_ERROR[0], TEXT_ERROR[1], TEXT_ERROR[2], TEXT_ERROR[3]), "%s", text);
    }
    
    void SuccessText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_SUCCESS[0], TEXT_SUCCESS[1], TEXT_SUCCESS[2], TEXT_SUCCESS[3]), "%s", text);
    }
    
    void InfoText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_INFO[0], TEXT_INFO[1], TEXT_INFO[2], TEXT_INFO[3]), "%s", text);
    }
    
    void DisabledText(const char* text) {
        ImGui::TextColored(ImVec4(TEXT_DISABLED[0], TEXT_DISABLED[1], TEXT_DISABLED[2], TEXT_DISABLED[3]), "%s", text);
    }
    
    // Button helpers - implement using ImGui's push/pop style colors
    bool ApplyButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(BUTTON_APPLY[0], BUTTON_APPLY[1], BUTTON_APPLY[2], BUTTON_APPLY[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BUTTON_APPLY[0] * 1.2f, BUTTON_APPLY[1] * 1.2f, BUTTON_APPLY[2] * 1.2f, BUTTON_APPLY[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BUTTON_APPLY[0] * 0.8f, BUTTON_APPLY[1] * 0.8f, BUTTON_APPLY[2] * 0.8f, BUTTON_APPLY[3]));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    bool CancelButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(BUTTON_CANCEL[0], BUTTON_CANCEL[1], BUTTON_CANCEL[2], BUTTON_CANCEL[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BUTTON_CANCEL[0] * 1.2f, BUTTON_CANCEL[1] * 1.2f, BUTTON_CANCEL[2] * 1.2f, BUTTON_CANCEL[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BUTTON_CANCEL[0] * 0.8f, BUTTON_CANCEL[1] * 0.8f, BUTTON_CANCEL[2] * 0.8f, BUTTON_CANCEL[3]));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    bool CloseButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(BUTTON_CLOSE[0], BUTTON_CLOSE[1], BUTTON_CLOSE[2], BUTTON_CLOSE[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BUTTON_CLOSE[0] * 1.2f, BUTTON_CLOSE[1] * 1.2f, BUTTON_CLOSE[2] * 1.2f, BUTTON_CLOSE[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BUTTON_CLOSE[0] * 0.8f, BUTTON_CLOSE[1] * 0.8f, BUTTON_CLOSE[2] * 0.8f, BUTTON_CLOSE[3]));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    bool AddButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(BUTTON_ADD[0], BUTTON_ADD[1], BUTTON_ADD[2], BUTTON_ADD[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BUTTON_ADD[0] * 1.2f, BUTTON_ADD[1] * 1.2f, BUTTON_ADD[2] * 1.2f, BUTTON_ADD[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BUTTON_ADD[0] * 0.8f, BUTTON_ADD[1] * 0.8f, BUTTON_ADD[2] * 0.8f, BUTTON_ADD[3]));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    bool RemoveButton(const char* label) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(BUTTON_REMOVE[0], BUTTON_REMOVE[1], BUTTON_REMOVE[2], BUTTON_REMOVE[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BUTTON_REMOVE[0] * 1.2f, BUTTON_REMOVE[1] * 1.2f, BUTTON_REMOVE[2] * 1.2f, BUTTON_REMOVE[3]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BUTTON_REMOVE[0] * 0.8f, BUTTON_REMOVE[1] * 0.8f, BUTTON_REMOVE[2] * 0.8f, BUTTON_REMOVE[3]));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    // Status indicators
    void ShowModifiedStatus() {
        ImGui::TextColored(ImVec4(STATUS_MODIFIED[0], STATUS_MODIFIED[1], STATUS_MODIFIED[2], STATUS_MODIFIED[3]), "* Modified *");
    }
    
    void ShowSavedStatus() {
        ImGui::TextColored(ImVec4(STATUS_SAVED[0], STATUS_SAVED[1], STATUS_SAVED[2], STATUS_SAVED[3]), "Saved");
    }
    
    void ShowUnchangedStatus() {
        ImGui::TextColored(ImVec4(STATUS_UNCHANGED[0], STATUS_UNCHANGED[1], STATUS_UNCHANGED[2], STATUS_UNCHANGED[3]), "No changes");
    }
    
    // Section helpers
    bool BeginFileInfoSection(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(SECTION_FILE_INFO[0], SECTION_FILE_INFO[1], SECTION_FILE_INFO[2], SECTION_FILE_INFO[3]));
        bool result = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
        if (!result) ImGui::PopStyleColor();
        return result;
    }
    
    bool BeginResolutionSection(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(SECTION_RESOLUTION[0], SECTION_RESOLUTION[1], SECTION_RESOLUTION[2], SECTION_RESOLUTION[3]));
        bool result = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
        if (!result) ImGui::PopStyleColor();
        return result;
    }
    
    bool BeginCellPropsSection(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(SECTION_CELL_PROPS[0], SECTION_CELL_PROPS[1], SECTION_CELL_PROPS[2], SECTION_CELL_PROPS[3]));
        bool result = ImGui::CollapsingHeader(title);
        if (!result) ImGui::PopStyleColor();
        return result;
    }
    
    bool BeginManagementSection(const char* title) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(SECTION_MANAGEMENT[0], SECTION_MANAGEMENT[1], SECTION_MANAGEMENT[2], SECTION_MANAGEMENT[3]));
        bool result = ImGui::CollapsingHeader(title);
        if (!result) ImGui::PopStyleColor();
        return result;
    }
    
    void EndSection() {
        ImGui::PopStyleColor();
    }
    
    // Theme selection UI
    void ShowThemeSelector() {
        if (ImGui::CollapsingHeader("Theme Settings")) {
            const char* themeNames[] = {
                "Photoshop Dark",
                "Photoshop Light", 
                "High Contrast",
                "Retro SCI",
                "Custom"
            };
            
            int currentThemeIndex = (int)g_currentTheme;
            if (ImGui::Combo("Theme", &currentThemeIndex, themeNames, 5)) {
                SetTheme((ThemeMode)currentThemeIndex);
            }
            
            // Show current theme info
            switch (g_currentTheme) {
            case ThemeMode::PHOTOSHOP_DARK:
                InfoText("Professional dark theme, easy on the eyes");
                break;
            case ThemeMode::PHOTOSHOP_LIGHT:
                InfoText("Professional light theme for bright environments");
                break;
            case ThemeMode::HIGH_CONTRAST:
                InfoText("High contrast theme for accessibility");
                break;
            case ThemeMode::RETRO_SCI:
                InfoText("Nostalgic theme reminiscent of classic SCI Studio");
                break;
            case ThemeMode::CUSTOM:
                InfoText("User-defined custom theme");
                break;
            }
            
            if (ImGui::Button("Apply Theme")) {
                RefreshTheme();
            }
        }
    }
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void Initialize() {
        SetTheme(ThemeMode::PHOTOSHOP_DARK);
        RefreshTheme();
    }
    
    void Shutdown() {
        // Clean up any style stack
        if (g_styleStackDepth > 0) {
            ImGui::PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
    }
    
    // ========================================================================
    // CUSTOMIZATION FUNCTIONS
    // ========================================================================
    
    void SetTextColor(const char* colorName, float r, float g, float b, float a) {
        // Users can extend this to modify colors at runtime
        // For now, this is a placeholder for future customization
    }
    
    void SetButtonColor(const char* colorName, float r, float g, float b, float a) {
        // Users can extend this to modify button colors at runtime
    }
    
    void SetWindowRounding(float rounding) {
        ImGui::GetStyle().WindowRounding = rounding;
    }
    
    void SetFrameRounding(float rounding) {
        ImGui::GetStyle().FrameRounding = rounding;
    }
    
    void SetSpacing(float x, float y) {
        ImGui::GetStyle().ItemSpacing = ImVec2(x, y);
    }
    
    void SetPadding(float x, float y) {
        ImGui::GetStyle().WindowPadding = ImVec2(x, y);
    }

    // ========================================================================
    // THEME COLOR DEFINITIONS
    // ========================================================================

    static UnifiedColors g_photoshopDarkColors = {
        RGB(45, 45, 48),        // background
        RGB(60, 60, 65),        // surface
        RGB(70, 70, 75),        // surfaceHover
        RGB(241, 241, 241),     // textPrimary
        RGB(170, 170, 170),     // textSecondary
        RGB(120, 120, 120),     // textDisabled
        RGB(0, 122, 204),       // accent
        RGB(28, 151, 234),      // accentHover
        RGB(85, 85, 85),        // border
        RGB(100, 100, 100),     // borderLight
        RGB(16, 185, 129),      // success
        RGB(245, 158, 11),      // warning
        RGB(239, 68, 68),       // error
        RGB(59, 130, 246),      // info
        RGB(34, 197, 94),       // buttonApply
        RGB(239, 68, 68),       // buttonCancel
        RGB(107, 114, 128)      // buttonNeutral
    };

    static UnifiedColors g_photoshopLightColors = {
        RGB(240, 240, 240),     // background
        RGB(250, 250, 250),     // surface
        RGB(255, 255, 255),     // surfaceHover
        RGB(17, 24, 39),        // textPrimary
        RGB(75, 85, 99),        // textSecondary
        RGB(156, 163, 175),     // textDisabled
        RGB(37, 99, 235),       // accent
        RGB(29, 78, 216),       // accentHover
        RGB(209, 213, 219),     // border
        RGB(156, 163, 175),     // borderLight
        RGB(34, 197, 94),       // success
        RGB(245, 158, 11),      // warning
        RGB(239, 68, 68),       // error
        RGB(59, 130, 246),      // info
        RGB(34, 197, 94),       // buttonApply
        RGB(239, 68, 68),       // buttonCancel
        RGB(107, 114, 128)      // buttonNeutral
    };

    static UnifiedColors g_highContrastColors = {
        RGB(0, 0, 0),           // background
        RGB(32, 32, 32),        // surface
        RGB(64, 64, 64),        // surfaceHover
        RGB(255, 255, 255),     // textPrimary
        RGB(192, 192, 192),     // textSecondary
        RGB(128, 128, 128),     // textDisabled
        RGB(0, 255, 255),       // accent
        RGB(64, 255, 255),      // accentHover
        RGB(255, 255, 255),     // border
        RGB(192, 192, 192),     // borderLight
        RGB(0, 255, 0),         // success
        RGB(255, 255, 0),       // warning
        RGB(255, 0, 0),         // error
        RGB(0, 255, 255),       // info
        RGB(0, 255, 0),         // buttonApply
        RGB(255, 0, 0),         // buttonCancel
        RGB(128, 128, 128)      // buttonNeutral
    };

    static UnifiedColors g_retroSCIColors = {
        RGB(0, 0, 51),          // background
        RGB(13, 13, 64),        // surface
        RGB(26, 26, 77),        // surfaceHover
        RGB(192, 192, 255),     // textPrimary
        RGB(128, 128, 192),     // textSecondary
        RGB(64, 64, 128),       // textDisabled
        RGB(51, 153, 255),      // accent
        RGB(102, 178, 255),     // accentHover
        RGB(51, 51, 102),       // border
        RGB(77, 77, 128),       // borderLight
        RGB(51, 255, 51),       // success
        RGB(255, 255, 51),      // warning
        RGB(255, 51, 51),       // error
        RGB(51, 255, 255),      // info
        RGB(51, 255, 51),       // buttonApply
        RGB(255, 51, 51),       // buttonCancel
        RGB(102, 102, 153)      // buttonNeutral
    };

    // ========================================================================
    // CORE FUNCTIONS
    // ========================================================================

    const UnifiedColors& GetCurrentColors() {
        switch (GetCurrentTheme()) {
        case ThemeMode::PHOTOSHOP_LIGHT:
            return g_photoshopLightColors;
        case ThemeMode::HIGH_CONTRAST:
            return g_highContrastColors;
        case ThemeMode::RETRO_SCI:
            return g_retroSCIColors;
        case ThemeMode::CUSTOM:
            return g_photoshopDarkColors;
        case ThemeMode::PHOTOSHOP_DARK:
        default:
            return g_photoshopDarkColors;
        }
    }

    // ========================================================================
    // DRAWING HELPER IMPLEMENTATIONS
    // ========================================================================

    void DrawRoundedRect(HDC hdc, RECT rect, COLORREF fillColor, COLORREF borderColor, int radius) {
        HBRUSH brush = CreateSolidBrush(fillColor);
        HPEN pen = borderColor ? CreatePen(PS_SOLID, 1, borderColor) : CreatePen(PS_SOLID, 1, GetCurrentColors().border);
        
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        
        if (radius > 0) {
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius * 2, radius * 2);
        } else {
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        }
        
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void DrawThemedButton(HDC hdc, RECT rect, const char* text, bool hovered, bool pressed, bool enabled) {
        const UnifiedColors& colors = GetCurrentColors();
        COLORREF bgColor, textColor;
        
        if (!enabled) {
            bgColor = colors.surface;
            textColor = colors.textDisabled;
        } else if (pressed) {
            bgColor = colors.accentHover;
            textColor = colors.textPrimary;
        } else if (hovered) {
            bgColor = colors.accent;
            textColor = colors.textPrimary;
        } else {
            bgColor = colors.surface;
            textColor = colors.textPrimary;
        }
        
        DrawRoundedRect(hdc, rect, bgColor, colors.border, 4);
        
        COLORREF oldTextColor = SetTextColor(hdc, textColor);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);
        
        DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        
        SetTextColor(hdc, oldTextColor);
        SetBkMode(hdc, oldBkMode);
    }

    void DrawThemedText(HDC hdc, const char* text, int x, int y, int width, int height, bool secondary) {
        const UnifiedColors& colors = GetCurrentColors();
        COLORREF textColor = secondary ? colors.textSecondary : colors.textPrimary;
        
        COLORREF oldColor = SetTextColor(hdc, textColor);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);
        
        RECT textRect = {x, y, x + width, y + height};
        DrawText(hdc, text, -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
        
        SetTextColor(hdc, oldColor);
        SetBkMode(hdc, oldBkMode);
    }

    void DrawThemedFrame(HDC hdc, RECT rect, bool highlighted) {
        const UnifiedColors& colors = GetCurrentColors();
        COLORREF borderColor = highlighted ? colors.accent : colors.border;
        DrawRoundedRect(hdc, rect, colors.surface, borderColor, 4);
    }

    void DrawStatusText(HDC hdc, const char* text, int x, int y, int width, int height, StatusType type) {
        const UnifiedColors& colors = GetCurrentColors();
        COLORREF textColor;
        
        switch (type) {
        case STATUS_SUCCESS:
            textColor = colors.success;
            break;
        case STATUS_WARNING:
            textColor = colors.warning;
            break;
        case STATUS_ERROR:
            textColor = colors.error;
            break;
        case STATUS_INFO:
            textColor = colors.info;
            break;
        case STATUS_NORMAL:
        default:
            textColor = colors.textPrimary;
            break;
        }
        
        COLORREF oldColor = SetTextColor(hdc, textColor);
        int oldBkMode = SetBkMode(hdc, TRANSPARENT);
        
        RECT textRect = {x, y, x + width, y + height};
        DrawText(hdc, text, -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
        
        SetTextColor(hdc, oldColor);
        SetBkMode(hdc, oldBkMode);
    }

    // ========================================================================
    // RESOURCE CREATION HELPERS
    // ========================================================================

    HBRUSH CreateThemeBrush(const char* colorName) {
        const UnifiedColors& colors = GetCurrentColors();
        
        if (strcmp(colorName, "background") == 0) return CreateSolidBrush(colors.background);
        if (strcmp(colorName, "surface") == 0) return CreateSolidBrush(colors.surface);
        if (strcmp(colorName, "accent") == 0) return CreateSolidBrush(colors.accent);
        if (strcmp(colorName, "success") == 0) return CreateSolidBrush(colors.success);
        if (strcmp(colorName, "warning") == 0) return CreateSolidBrush(colors.warning);
        if (strcmp(colorName, "error") == 0) return CreateSolidBrush(colors.error);
        
        return CreateSolidBrush(colors.surface);
    }

    HPEN CreateThemePen(const char* colorName, int width) {
        const UnifiedColors& colors = GetCurrentColors();
        
        if (strcmp(colorName, "border") == 0) return CreatePen(PS_SOLID, width, colors.border);
        if (strcmp(colorName, "accent") == 0) return CreatePen(PS_SOLID, width, colors.accent);
        if (strcmp(colorName, "success") == 0) return CreatePen(PS_SOLID, width, colors.success);
        if (strcmp(colorName, "warning") == 0) return CreatePen(PS_SOLID, width, colors.warning);
        if (strcmp(colorName, "error") == 0) return CreatePen(PS_SOLID, width, colors.error);
        
        return CreatePen(PS_SOLID, width, colors.border);
    }

    HFONT CreateThemeFont(int size, bool bold) {
        return CreateFont(
            size, 0, 0, 0, 
            bold ? FW_BOLD : FW_NORMAL, 
            FALSE, FALSE, FALSE, 
            DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, 
            CLEARTYPE_QUALITY, 
            VARIABLE_PITCH | FF_SWISS, 
            TEXT("Segoe UI")
        );
    }

    void SafeDeleteBrush(HBRUSH& brush) {
        if (brush && brush != GetStockObject(NULL_BRUSH)) {
            DeleteObject(brush);
            brush = NULL;
        }
    }

    void SafeDeletePen(HPEN& pen) {
        if (pen && pen != GetStockObject(NULL_PEN)) {
            DeleteObject(pen);
            pen = NULL;
        }
    }

    void SafeDeleteFont(HFONT& font) {
        if (font && font != GetStockObject(SYSTEM_FONT)) {
            DeleteObject(font);
            font = NULL;
        }
    }
}