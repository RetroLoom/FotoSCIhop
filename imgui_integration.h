#pragma once
#include <windows.h>

// Simple callback type - your dialog rendering functions in FotoSCIhop.cpp
typedef void (*ImGuiDialogCallback)(void);

namespace ImGuiDialogs {
    
    // Initialize/cleanup the ImGui system
    bool Initialize(HWND parent);
    void Shutdown();
    
    // Set the callback functions (called from FotoSCIhop.cpp initialization)
    void SetDialogCallbacks(ImGuiDialogCallback propertiesCallback, 
                           ImGuiDialogCallback linkPointsCallback);
    
    // Show/hide dialogs
    void ShowProperties();
    void ShowLinkPoints();
    void Hide();
    
    // Check if any dialog is open
    bool IsAnyDialogOpen();
    
    // Call from your main message loop
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
}