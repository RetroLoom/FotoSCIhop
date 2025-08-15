#include "stdafx.h"
#include "fotoscihop_styles.h"
#include "imgui_integration.h"

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
        using namespace ImGuiDialogs;
        
        // Clear any existing style stack
        if (g_styleStackDepth > 0) {
            PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        ApplyTheme(THEME_PHOTOSHOP);
        
        // Apply our custom settings
        PushStyleVar2(IMGUI_STYLE_VAR_WINDOW_PADDING, StyleSettings::WINDOW_PADDING_X, StyleSettings::WINDOW_PADDING_Y);
        PushStyleVar2(IMGUI_STYLE_VAR_FRAME_PADDING, StyleSettings::FRAME_PADDING_X, StyleSettings::FRAME_PADDING_Y);
        PushStyleVar2(IMGUI_STYLE_VAR_ITEM_SPACING, StyleSettings::ITEM_SPACING_X, StyleSettings::ITEM_SPACING_Y);
        PushStyleVar2(IMGUI_STYLE_VAR_ITEM_INNER_SPACING, StyleSettings::ITEM_INNER_SPACING_X, StyleSettings::ITEM_INNER_SPACING_Y);
        PushStyleVar(IMGUI_STYLE_VAR_INDENT_SPACING, StyleSettings::INDENT_SPACING);
        
        SetWindowRounding(StyleSettings::WINDOW_ROUNDING);
        SetFrameRounding(StyleSettings::FRAME_ROUNDING);
        SetScrollbarRounding(StyleSettings::SCROLLBAR_ROUNDING);
        SetGrabRounding(StyleSettings::GRAB_ROUNDING);
        
        g_styleStackDepth = 5;  // Track that we pushed 5 style variables
    }
    
    void ApplyPhotoshopLightTheme() {
        using namespace ImGuiDialogs;
        
        if (g_styleStackDepth > 0) {
            PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        ApplyTheme(THEME_LIGHT);
        
        // Light theme customizations
        PushStyleColor(IMGUI_COL_WINDOW_BG, 0.94f, 0.94f, 0.94f, 1.00f);
        PushStyleColor(IMGUI_COL_CHILD_BG, 0.90f, 0.90f, 0.90f, 1.00f);
        PushStyleColor(IMGUI_COL_FRAME_BG, 0.85f, 0.85f, 0.85f, 1.00f);
        
        PushStyleVar2(IMGUI_STYLE_VAR_WINDOW_PADDING, StyleSettings::WINDOW_PADDING_X, StyleSettings::WINDOW_PADDING_Y);
        PushStyleVar2(IMGUI_STYLE_VAR_FRAME_PADDING, StyleSettings::FRAME_PADDING_X, StyleSettings::FRAME_PADDING_Y);
        PushStyleVar2(IMGUI_STYLE_VAR_ITEM_SPACING, StyleSettings::ITEM_SPACING_X, StyleSettings::ITEM_SPACING_Y);
        
        SetWindowRounding(StyleSettings::WINDOW_ROUNDING);
        SetFrameRounding(StyleSettings::FRAME_ROUNDING);
        
        g_styleStackDepth = 3;
    }
    
    void ApplyHighContrastTheme() {
        using namespace ImGuiDialogs;
        
        if (g_styleStackDepth > 0) {
            PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        ApplyTheme(THEME_HIGH_CONTRAST);
        
        // High contrast uses sharp edges
        SetWindowRounding(0.0f);
        SetFrameRounding(0.0f);
        SetScrollbarRounding(0.0f);
        SetGrabRounding(0.0f);
    }
    
    void ApplyRetroSCITheme() {
        using namespace ImGuiDialogs;
        
        if (g_styleStackDepth > 0) {
            PopStyleVar(g_styleStackDepth);
            g_styleStackDepth = 0;
        }
        
        ApplyTheme(THEME_DARK);
        
        // Retro SCI colors (dark blue theme)
        PushStyleColor(IMGUI_COL_WINDOW_BG, 0.0f, 0.0f, 0.2f, 1.00f);
        PushStyleColor(IMGUI_COL_CHILD_BG, 0.05f, 0.05f, 0.25f, 1.00f);
        PushStyleColor(IMGUI_COL_FRAME_BG, 0.1f, 0.1f, 0.3f, 1.00f);
        PushStyleColor(IMGUI_COL_HEADER, 0.2f, 0.2f, 0.5f, 1.00f);
        PushStyleColor(IMGUI_COL_BUTTON, 0.15f, 0.15f, 0.4f, 1.00f);
        
        SetWindowRounding(2.0f);
        SetFrameRounding(1.0f);
        SetScrollbarRounding(1.0f);
        SetGrabRounding(1.0f);
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
        ImGuiDialogs::TextColored(TEXT_HEADER[0], TEXT_HEADER[1], TEXT_HEADER[2], TEXT_HEADER[3], text);
    }
    
    void WarningText(const char* text) {
        ImGuiDialogs::TextColored(TEXT_WARNING[0], TEXT_WARNING[1], TEXT_WARNING[2], TEXT_WARNING[3], text);
    }
    
    void ErrorText(const char* text) {
        ImGuiDialogs::TextColored(TEXT_ERROR[0], TEXT_ERROR[1], TEXT_ERROR[2], TEXT_ERROR[3], text);
    }
    
    void SuccessText(const char* text) {
        ImGuiDialogs::TextColored(TEXT_SUCCESS[0], TEXT_SUCCESS[1], TEXT_SUCCESS[2], TEXT_SUCCESS[3], text);
    }
    
    void InfoText(const char* text) {
        ImGuiDialogs::TextColored(TEXT_INFO[0], TEXT_INFO[1], TEXT_INFO[2], TEXT_INFO[3], text);
    }
    
    void DisabledText(const char* text) {
        ImGuiDialogs::TextColored(TEXT_DISABLED[0], TEXT_DISABLED[1], TEXT_DISABLED[2], TEXT_DISABLED[3], text);
    }
    
    // Button helpers
    bool ApplyButton(const char* label) {
        return ImGuiDialogs::ButtonColored(label, BUTTON_APPLY[0], BUTTON_APPLY[1], BUTTON_APPLY[2], BUTTON_APPLY[3]);
    }
    
    bool CancelButton(const char* label) {
        return ImGuiDialogs::ButtonColored(label, BUTTON_CANCEL[0], BUTTON_CANCEL[1], BUTTON_CANCEL[2], BUTTON_CANCEL[3]);
    }
    
    bool CloseButton(const char* label) {
        return ImGuiDialogs::ButtonColored(label, BUTTON_CLOSE[0], BUTTON_CLOSE[1], BUTTON_CLOSE[2], BUTTON_CLOSE[3]);
    }
    
    bool AddButton(const char* label) {
        return ImGuiDialogs::ButtonColored(label, BUTTON_ADD[0], BUTTON_ADD[1], BUTTON_ADD[2], BUTTON_ADD[3]);
    }
    
    bool RemoveButton(const char* label) {
        return ImGuiDialogs::ButtonColored(label, BUTTON_REMOVE[0], BUTTON_REMOVE[1], BUTTON_REMOVE[2], BUTTON_REMOVE[3]);
    }
    
    // Status indicators
    void ShowModifiedStatus() {
        ImGuiDialogs::TextColored(STATUS_MODIFIED[0], STATUS_MODIFIED[1], STATUS_MODIFIED[2], STATUS_MODIFIED[3], "* Modified *");
    }
    
    void ShowSavedStatus() {
        ImGuiDialogs::TextColored(STATUS_SAVED[0], STATUS_SAVED[1], STATUS_SAVED[2], STATUS_SAVED[3], "Saved");
    }
    
    void ShowUnchangedStatus() {
        ImGuiDialogs::TextColored(STATUS_UNCHANGED[0], STATUS_UNCHANGED[1], STATUS_UNCHANGED[2], STATUS_UNCHANGED[3], "No changes");
    }
    
    // Section helpers
    bool BeginFileInfoSection(const char* title) {
        ImGuiDialogs::PushStyleColor(IMGUI_COL_CHILD_BG, SECTION_FILE_INFO[0], SECTION_FILE_INFO[1], SECTION_FILE_INFO[2], SECTION_FILE_INFO[3]);
        bool result = ImGuiDialogs::CollapsingHeader(title, true);
        if (!result) ImGuiDialogs::PopStyleColor();
        return result;
    }
    
    bool BeginResolutionSection(const char* title) {
        ImGuiDialogs::PushStyleColor(IMGUI_COL_CHILD_BG, SECTION_RESOLUTION[0], SECTION_RESOLUTION[1], SECTION_RESOLUTION[2], SECTION_RESOLUTION[3]);
        bool result = ImGuiDialogs::CollapsingHeader(title, true);
        if (!result) ImGuiDialogs::PopStyleColor();
        return result;
    }
    
    bool BeginCellPropsSection(const char* title) {
        ImGuiDialogs::PushStyleColor(IMGUI_COL_CHILD_BG, SECTION_CELL_PROPS[0], SECTION_CELL_PROPS[1], SECTION_CELL_PROPS[2], SECTION_CELL_PROPS[3]);
        bool result = ImGuiDialogs::CollapsingHeader(title);
        if (!result) ImGuiDialogs::PopStyleColor();
        return result;
    }
    
    bool BeginManagementSection(const char* title) {
        ImGuiDialogs::PushStyleColor(IMGUI_COL_CHILD_BG, SECTION_MANAGEMENT[0], SECTION_MANAGEMENT[1], SECTION_MANAGEMENT[2], SECTION_MANAGEMENT[3]);
        bool result = ImGuiDialogs::CollapsingHeader(title);
        if (!result) ImGuiDialogs::PopStyleColor();
        return result;
    }
    
    void EndSection() {
        ImGuiDialogs::PopStyleColor();
    }
    
    // Theme selection UI
    void ShowThemeSelector() {
        using namespace ImGuiDialogs;
        
        if (CollapsingHeader("Theme Settings")) {
            const char* themeNames[] = {
                "Photoshop Dark",
                "Photoshop Light", 
                "High Contrast",
                "Retro SCI",
                "Custom"
            };
            
            int currentThemeIndex = (int)g_currentTheme;
            if (Combo("Theme", &currentThemeIndex, themeNames, 5)) {
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
            
            if (Button("Apply Theme")) {
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
            ImGuiDialogs::PopStyleVar(g_styleStackDepth);
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
        ImGuiDialogs::SetWindowRounding(rounding);
    }
    
    void SetFrameRounding(float rounding) {
        ImGuiDialogs::SetFrameRounding(rounding);
    }
    
    void SetSpacing(float x, float y) {
        // This would need to be implemented to change spacing at runtime
    }
    
    void SetPadding(float x, float y) {
        // This would need to be implemented to change padding at runtime
    }
}