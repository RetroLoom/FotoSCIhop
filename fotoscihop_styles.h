#pragma once
#include "imgui_integration.h"

// ============================================================================
// FOTOSCIHOP STYLING SYSTEM
// ============================================================================
// Separate header for all styling and theming functionality

namespace FotoSCIhopStyles {
    
    // ========================================================================
    // COLOR DEFINITIONS (using simple float arrays to avoid constexpr issues)
    // ========================================================================
    
    // Text Colors
    extern const float TEXT_NORMAL[4];          // White
    extern const float TEXT_HEADER[4];          // Light Blue
    extern const float TEXT_DISABLED[4];        // Gray
    extern const float TEXT_WARNING[4];         // Orange
    extern const float TEXT_ERROR[4];           // Red
    extern const float TEXT_SUCCESS[4];         // Green
    extern const float TEXT_INFO[4];            // Cyan
    
    // Button Colors
    extern const float BUTTON_APPLY[4];         // Green
    extern const float BUTTON_CANCEL[4];        // Red
    extern const float BUTTON_CLOSE[4];         // Purple
    extern const float BUTTON_ADD[4];           // Blue
    extern const float BUTTON_REMOVE[4];        // Light Red
    extern const float BUTTON_NEUTRAL[4];       // Gray
    
    // Section Colors
    extern const float SECTION_FILE_INFO[4];    // Dark Blue
    extern const float SECTION_RESOLUTION[4];   // Dark Green
    extern const float SECTION_CELL_PROPS[4];   // Dark Brown
    extern const float SECTION_MANAGEMENT[4];   // Dark Purple
    
    // Status Colors
    extern const float STATUS_MODIFIED[4];      // Orange
    extern const float STATUS_SAVED[4];         // Green
    extern const float STATUS_UNCHANGED[4];     // Gray
    
    // ========================================================================
    // STYLE SETTINGS
    // ========================================================================
    
    struct StyleSettings {
        // Window and Frame Settings
        static const float WINDOW_ROUNDING;
        static const float FRAME_ROUNDING;
        static const float SCROLLBAR_ROUNDING;
        static const float GRAB_ROUNDING;
        
        // Padding and Spacing
        static const float WINDOW_PADDING_X;
        static const float WINDOW_PADDING_Y;
        static const float FRAME_PADDING_X;
        static const float FRAME_PADDING_Y;
        static const float ITEM_SPACING_X;
        static const float ITEM_SPACING_Y;
        static const float ITEM_INNER_SPACING_X;
        static const float ITEM_INNER_SPACING_Y;
        static const float INDENT_SPACING;
        
        // Border and Line Settings
        static const float WINDOW_BORDER_SIZE;
        static const float FRAME_BORDER_SIZE;
        static const float POPUP_BORDER_SIZE;
        
        // Alpha/Transparency Settings
        static const float ALPHA_DISABLED;
        static const float ALPHA_SUBTLE;
        static const float ALPHA_NORMAL;
    };
    
    // ========================================================================
    // THEME MANAGEMENT
    // ========================================================================
    
    enum class ThemeMode {
        PHOTOSHOP_DARK,     // Professional dark theme (default)
        PHOTOSHOP_LIGHT,    // Professional light theme  
        HIGH_CONTRAST,      // Accessibility theme
        RETRO_SCI,          // Nostalgic SCI theme
        CUSTOM              // User-defined theme
    };
    
    // Theme functions
    void SetTheme(ThemeMode theme);
    void RefreshTheme();
    ThemeMode GetCurrentTheme();
    
    // Individual theme applications
    void ApplyPhotoshopDarkTheme();
    void ApplyPhotoshopLightTheme();
    void ApplyHighContrastTheme();
    void ApplyRetroSCITheme();
    void ApplyCustomTheme();
    
    // ========================================================================
    // CONVENIENCE STYLING FUNCTIONS
    // ========================================================================
    
    // Colored text helpers
    void HeaderText(const char* text);
    void WarningText(const char* text);
    void ErrorText(const char* text);
    void SuccessText(const char* text);
    void InfoText(const char* text);
    void DisabledText(const char* text);
    
    // Button helpers
    bool ApplyButton(const char* label = "Apply");
    bool CancelButton(const char* label = "Cancel");
    bool CloseButton(const char* label = "Close");
    bool AddButton(const char* label);
    bool RemoveButton(const char* label);
    
    // Status indicators
    void ShowModifiedStatus();
    void ShowSavedStatus();
    void ShowUnchangedStatus();
    
    // Section helpers
    bool BeginFileInfoSection(const char* title = "File Information");
    bool BeginResolutionSection(const char* title = "Resolution Settings");
    bool BeginCellPropsSection(const char* title = "Cell Properties");
    bool BeginManagementSection(const char* title = "Element Management");
    void EndSection();
    
    // Theme selection UI
    void ShowThemeSelector();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void Initialize();
    void Shutdown();
    
    // ========================================================================
    // CUSTOMIZATION FUNCTIONS
    // ========================================================================
    
    // Easy color modification
    void SetTextColor(const char* colorName, float r, float g, float b, float a = 1.0f);
    void SetButtonColor(const char* colorName, float r, float g, float b, float a = 1.0f);
    
    // Easy style modification
    void SetWindowRounding(float rounding);
    void SetFrameRounding(float rounding);
    void SetSpacing(float x, float y);
    void SetPadding(float x, float y);
}