#pragma once
#include <windows.h>
#include "imgui.h"

// ============================================================================
// MINIMAL IMGUI INTEGRATION FOR FOTOSCIHOP
// ============================================================================
// Core dialog system and Win32/OpenGL integration only
// Use ImGui:: directly for all UI functions

// Simple callback type for dialog rendering functions
typedef void (*ImGuiDialogCallback)(void);

namespace ImGuiDialogs {
    
    // =========================================================================
    // DIALOG MANAGEMENT
    // =========================================================================
    
    enum DialogType {
        DIALOG_PROPERTIES = 0,
        DIALOG_ABOUT = 1,
        DIALOG_CLUT_GENERATOR = 2,
        DIALOG_REALMPAL = 3,
        DIALOG_PREFERENCES,
        DIALOG_PALETTE_MANAGER,
        DIALOG_COUNT
    };
    
    // Core dialog system
    void RegisterDialog(DialogType type, const char* title, ImGuiDialogCallback callback);
    void RegisterDialogWithInput(DialogType type, const char* title, ImGuiDialogCallback callback, bool allowMainWindowInput);
    void ShowDialog(DialogType type);
    void HideDialog(DialogType type);
    bool IsDialogOpen(DialogType type);
    bool IsAnyDialogOpen();
    bool CurrentDialogAllowsMainWindowInput();
    
    // Focus management functions
    bool HasDialogFocus();
    void RestoreDialogFocus();
    HWND GetDialogWindow();
    
    // =========================================================================
    // CORE ENGINE FUNCTIONS
    // =========================================================================
    
    // Initialize/cleanup the ImGui system
    bool Initialize(HWND parent);
    void Shutdown();
    void Hide(); // Hides all dialogs
    
    // Call from your main message loop and timer
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render();
    
    // =========================================================================
    // WINDOW MANAGEMENT
    // =========================================================================
    
    // Window management for dialog sizing
    void SetNextWindowSize(float width, float height);
    void SetNextWindowPos(float x, float y);
    void SetNextWindowFocus();
    
    // Dialog frame management - simplified wrapper around ImGui::Begin/End
    // that sets up full-window dialogs
    bool BeginDialog(const char* title, bool* open);
    void EndDialog();
    
    // =========================================================================
    // BASIC THEME SYSTEM
    // =========================================================================
    
    enum Theme {
        THEME_DARK = 0,
        THEME_LIGHT = 1,
        THEME_CLASSIC = 2,
        THEME_PHOTOSHOP = 3,
        THEME_HIGH_CONTRAST = 4
    };
    
    void ApplyTheme(Theme theme);
    
    // =========================================================================
    // USAGE NOTES:
    // =========================================================================
    // This integration provides only the essential dialog window management.
    // For all UI elements, use ImGui:: functions directly:
    //
    // ImGui::Text("Hello World");
    // if (ImGui::Button("Click Me")) { /* action */ }
    // ImGui::InputText("Label", buffer, sizeof(buffer));
    // ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);
    //
    // For advanced styling, use FotoSCIhopStyles:: functions or direct ImGui styling.
    // =========================================================================
}