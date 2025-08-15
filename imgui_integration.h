#pragma once
#include <windows.h>

// ImGui constants we need
#define IMGUI_STYLE_VAR_ALPHA 0

// Simple callback type - your dialog rendering functions in FotoSCIhop.cpp
typedef void (*ImGuiDialogCallback)(void);

namespace ImGuiDialogs {
    
    // Initialize/cleanup the ImGui system
    bool Initialize(HWND parent);
    void Shutdown();
    
    // Set the callback functions (called from FotoSCIhop.cpp initialization)
    void SetDialogCallbacks(ImGuiDialogCallback propertiesCallback);
    
    // Show/hide the combined properties dialog
    void ShowProperties();
    void ShowLinkPoints(); // Now just calls ShowProperties()
    void Hide(); // Hides all dialogs
    
    // Hide specific dialogs
    void HideProperties();
    void HideLinkPoints();
    
    // Check if any dialog is open
    bool IsAnyDialogOpen();
    
    // Call from your main message loop and timer
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render();
    
    // Utility functions for dialog content (call these from your callback functions)
    bool BeginDialog(const char* title, bool* open);
    void EndDialog();
    bool Button(const char* label);
    bool InputInt(const char* label, int* value);
    bool Checkbox(const char* label, bool* value);
    void Text(const char* text);
    void Separator();
    void SameLine();
    bool CollapsingHeader(const char* label, bool defaultOpen = false);
    void PushStyleVar(int var, float value);
    void PopStyleVar();
}