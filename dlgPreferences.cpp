/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

extern bool g_pendingThemeChange;
extern FotoSCIhopStyles::ThemeMode g_pendingTheme;

void SavePreferencesToINI() {
    char buffer[32];
    
    // Save main settings
    sprintf(buffer, "%d", gAppResX);
    WritePrivateProfileStringA("main", "resX", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gAppResY);
    WritePrivateProfileStringA("main", "resY", buffer, gConfigIni);
    
    sprintf(buffer, "%d", zScale);
    WritePrivateProfileStringA("main", "zScale", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gPosCells);
    WritePrivateProfileStringA("main", "posCells", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gBaseMagnify);
    WritePrivateProfileStringA("main", "magScale", buffer, gConfigIni);
    
    // Save theme setting
    sprintf(buffer, "%d", (int)FotoSCIhopStyles::GetCurrentTheme());
    WritePrivateProfileStringA("main", "theme", buffer, gConfigIni);
}

void LoadPreferencesFromINI() {
    // This essentially duplicates LoadConfig() but for the static variables
    // We'll load into the static variables in the dialog
}

void RenderPreferencesDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static bool configInitialized = false;
    static std::string statusMessage = "";
    static bool showStatus = false;
    static bool hasUnsavedChanges = false;
    
    // Static copies of all settings for editing
    static int tempAppResX = 700;
    static int tempAppResY = 500;
    static int tempZScale = 100;
    static int tempPosCells = 0;
    static int tempBaseMagnify = 100;
    static int tempTheme = 0;
    
    // Original values for comparison and reset
    static int originalAppResX = 700;
    static int originalAppResY = 500;
    static int originalZScale = 100;
    static int originalPosCells = 0;
    static int originalBaseMagnify = 100;
    static int originalTheme = 0;
    
    bool open = true;
    SetNextWindowSize(600, 700);
    
    if (!BeginDialog("Preferences", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open) {
        // Check for unsaved changes
        if (hasUnsavedChanges) {
            // Could add a confirmation dialog here, but for simplicity we'll just close
        }
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PREFERENCES);
        EndDialog();
        return;
    }
    
    // Initialize values on first run
    if (!configInitialized) {
        tempAppResX = originalAppResX = gAppResX;
        tempAppResY = originalAppResY = gAppResY;
        tempZScale = originalZScale = zScale;
        tempPosCells = originalPosCells = gPosCells;
        tempBaseMagnify = originalBaseMagnify = gBaseMagnify;
        tempTheme = originalTheme = (int)FotoSCIhopStyles::GetCurrentTheme();
        configInitialized = true;
        hasUnsavedChanges = false;
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // SCROLLABLE CONTENT AREA
    // =========================================================================
    
    // Reserve space for buttons at bottom
    float reservedHeight = ImGui::GetFrameHeightWithSpacing() * 2 + ImGui::GetStyle().WindowPadding.y;
    float contentHeight = ImGui::GetContentRegionAvail().y - reservedHeight;
    
    if (ImGui::BeginChild("PreferencesContent", ImVec2(0, contentHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        
        // =====================================================================
        // GENERAL SETTINGS SECTION
        // =====================================================================
        
        if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            
            HeaderText("Application Window:");
            ImGui::Spacing();
            
            // Window size settings
            ImGui::Text("Default Window Size:");
            ImGui::PushItemWidth(120);
            
            int oldResX = tempAppResX;
            if (ImGui::InputInt("Width##resX", &tempAppResX)) {
                tempAppResX = max(400, min(2560, tempAppResX)); // Reasonable bounds
                if (tempAppResX != oldResX) hasUnsavedChanges = true;
            }
            
            int oldResY = tempAppResY;
            if (ImGui::InputInt("Height##resY", &tempAppResY)) {
                tempAppResY = max(300, min(1440, tempAppResY)); // Reasonable bounds  
                if (tempAppResY != oldResY) hasUnsavedChanges = true;
            }
            
            ImGui::PopItemWidth();
            ImGui::Spacing();
            
            // Magnification settings
            HeaderText("Display Settings:");
            ImGui::Spacing();
            
            ImGui::Text("Default Magnification:");
            ImGui::PushItemWidth(120);
            int oldMagnify = tempBaseMagnify;
            if (ImGui::InputInt("Percent##magnify", &tempBaseMagnify)) {
                tempBaseMagnify = max(25, min(1600, tempBaseMagnify)); // Match zoom limits
                if (tempBaseMagnify != oldMagnify) hasUnsavedChanges = true;
            }
            
            ImGui::Text("Priority Scale:");
            int oldZScale = tempZScale;
            if (ImGui::InputInt("Scale##zscale", &tempZScale)) {
                tempZScale = max(1, min(500, tempZScale)); // Reasonable bounds
                if (tempZScale != oldZScale) hasUnsavedChanges = true;
            }
            
            ImGui::PopItemWidth();
            ImGui::Spacing();
            
            // Cell positioning
            HeaderText("Cell Display:");
            ImGui::Spacing();
            
            ImGui::Text("Cell Positioning Mode:");
            const char* posCellItems[] = { "Default", "Centered", "Custom" };
            int oldPosCells = tempPosCells;
            if (ImGui::Combo("##poscells", &tempPosCells, posCellItems, 3)) {
                if (tempPosCells != oldPosCells) hasUnsavedChanges = true;
            }
            
            ImGui::Spacing();
            
            // Show current values vs defaults
            if (ImGui::TreeNode("Current Values")) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                ImGui::Text("Window: %dx%d", tempAppResX, tempAppResY);
                ImGui::Text("Magnification: %d%%", tempBaseMagnify);
                ImGui::Text("Priority Scale: %d", tempZScale);
                ImGui::Text("Cell Mode: %s", posCellItems[tempPosCells]);
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }
        }
        
        ImGui::Spacing();
        
        // =====================================================================
        // THEME SETTINGS SECTION  
        // =====================================================================
        
        if (ImGui::CollapsingHeader("Theme Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            
            HeaderText("Appearance:");
            ImGui::Spacing();
            
            ImGui::Text("Interface Theme:");
            const char* themeNames[] = {
                "Photoshop Dark",
                "Photoshop Light", 
                "High Contrast",
                "Retro SCI",
                "Custom"
            };

            int oldTheme = tempTheme;
            if (ImGui::Combo("##theme_selector", &tempTheme, themeNames, 5))
            {
                if (tempTheme != oldTheme)
                {
                    hasUnsavedChanges = true;
                    // Don't apply immediately - defer until after frame
                    g_pendingThemeChange = true;
                    g_pendingTheme = (FotoSCIhopStyles::ThemeMode)tempTheme;
                }
            }

            ImGui::Spacing();
            
            // Theme description
            switch ((FotoSCIhopStyles::ThemeMode)tempTheme) {
            case FotoSCIhopStyles::ThemeMode::PHOTOSHOP_DARK:
                InfoText("Professional dark theme, easy on the eyes");
                break;
            case FotoSCIhopStyles::ThemeMode::PHOTOSHOP_LIGHT:
                InfoText("Professional light theme for bright environments");
                break;
            case FotoSCIhopStyles::ThemeMode::HIGH_CONTRAST:
                InfoText("High contrast theme for accessibility");
                break;
            case FotoSCIhopStyles::ThemeMode::RETRO_SCI:
                InfoText("Nostalgic theme reminiscent of classic SCI Studio");
                break;
            case FotoSCIhopStyles::ThemeMode::CUSTOM:
                InfoText("User-defined custom theme");
                break;
            }
            
            ImGui::Spacing();
            
            // Theme preview section
            if (ImGui::TreeNode("Theme Preview")) {
                ImGui::Text("Sample text elements:");
                ImGui::Spacing();
                
                HeaderText("Header Text");
                ImGui::Text("Normal text");
                SuccessText("Success message");
                WarningText("Warning message");
                ErrorText("Error message");
                InfoText("Information text");
                DisabledText("Disabled text");
                
                ImGui::Spacing();
                ImGui::Text("Sample buttons:");
                
                ImGui::BeginGroup();
                if (ApplyButton("Apply")) { /* demo only */ }
                ImGui::SameLine();
                if (CancelButton("Cancel")) { /* demo only */ }
                ImGui::SameLine();
                if (CloseButton("Close")) { /* demo only */ }
                ImGui::EndGroup();
                
                ImGui::TreePop();
            }
        }
        
        ImGui::Spacing();
        
        // =====================================================================
        // ADVANCED SETTINGS SECTION
        // =====================================================================
        
        if (ImGui::CollapsingHeader("Advanced Settings")) {
            
            HeaderText("Reset Options:");
            ImGui::Spacing();
            
            ImGui::TextWrapped("These options will reset settings to their default values.");
            ImGui::Spacing();
            
            if (ImGui::Button("Reset Window Settings", ImVec2(availableWidth * 0.48f, 0))) {
                tempAppResX = 700;
                tempAppResY = 500;
                tempBaseMagnify = 100;
                hasUnsavedChanges = true;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Reset All Settings", ImVec2(availableWidth * 0.48f, 0))) {
                tempAppResX = 700;
                tempAppResY = 500;
                tempZScale = 100;
                tempPosCells = 0;
                tempBaseMagnify = 100;
                tempTheme = 0;
                FotoSCIhopStyles::SetTheme(FotoSCIhopStyles::ThemeMode::PHOTOSHOP_DARK);
                FotoSCIhopStyles::RefreshTheme();
                hasUnsavedChanges = true;
            }
            
            ImGui::Spacing();
            
            // Configuration file info
            if (ImGui::TreeNode("Configuration File")) {
                ImGui::Text("Config location:");
                DisabledText(gConfigIni);
                ImGui::Spacing();
                
                if (ImGui::Button("Open Config Folder")) {
                    ShellExecute(NULL, "explore", gAppPath, NULL, NULL, SW_SHOW);
                }
                
                ImGui::TreePop();
            }
        }
        
    }
    ImGui::EndChild();
    
    // =========================================================================
    // BOTTOM BUTTONS
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Status indicator
    if (hasUnsavedChanges) {
        WarningText("* You have unsaved changes");
    } else {
        DisabledText("No unsaved changes");
    }
    
    ImGui::Spacing();
    
    // Button layout
    float buttonWidth = availableWidth * 0.22f;
    
    // Apply button
    bool canApply = hasUnsavedChanges;
    if (canApply) {
        if (ApplyButton("Apply")) {
            // Apply all settings
            gAppResX = tempAppResX;
            gAppResY = tempAppResY;
            zScale = tempZScale;
            gPosCells = tempPosCells;
            gBaseMagnify = tempBaseMagnify;
            MagnifyFactor = gBaseMagnify;

            // Apply theme change if needed
            if (tempTheme != originalTheme)
            {
                g_pendingThemeChange = true;
                g_pendingTheme = (FotoSCIhopStyles::ThemeMode)tempTheme;
            }

            // Save to INI file
            SavePreferencesToINI();

            // Update originals
            originalAppResX = tempAppResX;
            originalAppResY = tempAppResY;
            originalZScale = tempZScale;
            originalPosCells = tempPosCells;
            originalBaseMagnify = tempBaseMagnify;
            originalTheme = tempTheme;
            
            hasUnsavedChanges = false;
            statusMessage = "Preferences saved successfully!";
            showStatus = true;
            
            // Force window update if size changed
            if (hWnd) {
                UpdateScrollBars();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::Button("Apply", ImVec2(buttonWidth, 0));
        ImGui::PopStyleVar();
    }
    
    ImGui::SameLine();
    
    // Reset button
    if (hasUnsavedChanges) {
        if (CancelButton("Reset")) {
            // Reset to original values
            tempAppResX = originalAppResX;
            tempAppResY = originalAppResY;
            tempZScale = originalZScale;
            tempPosCells = originalPosCells;
            tempBaseMagnify = originalBaseMagnify;
            tempTheme = originalTheme;

            // Reset theme (deferred)
            g_pendingThemeChange = true;
            g_pendingTheme = (FotoSCIhopStyles::ThemeMode)tempTheme;

            hasUnsavedChanges = false;
            statusMessage = "Settings reset to last saved values";
            showStatus = true;
        }
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::Button("Reset", ImVec2(buttonWidth, 0));
        ImGui::PopStyleVar();
    }
    
    ImGui::SameLine();
    
    // OK button (Apply + Close)
    if (hasUnsavedChanges) {
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
            // Apply and close
            gAppResX = tempAppResX;
            gAppResY = tempAppResY;
            zScale = tempZScale;
            gPosCells = tempPosCells;
            gBaseMagnify = tempBaseMagnify;
            MagnifyFactor = gBaseMagnify;

            // Apply theme change if needed
            if (tempTheme != originalTheme)
            {
                g_pendingThemeChange = true;
                g_pendingTheme = (FotoSCIhopStyles::ThemeMode)tempTheme;
            }

            SavePreferencesToINI();

            if (hWnd)
            {
                UpdateScrollBars();
                InvalidateRect(hWnd, NULL, TRUE);
            }

            ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PREFERENCES);
        }
    } else {
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
            ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PREFERENCES);
        }
    }
    
    ImGui::SameLine();
    
    // Close button  
    if (CloseButton("Cancel")) {
        if (hasUnsavedChanges && tempTheme != originalTheme)
        {
            // Reset theme to original if it was changed (deferred)
            g_pendingThemeChange = true;
            g_pendingTheme = (FotoSCIhopStyles::ThemeMode)originalTheme;
        }
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PREFERENCES);
    }
    
    // Status message
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (statusMessage.find("success") != std::string::npos) {
            SuccessText(statusMessage.c_str());
        } else {
            InfoText(statusMessage.c_str());
        }
        
        // Auto-hide status after a few seconds (this is basic - you might want a timer)
        static int statusCounter = 0;
        statusCounter++;
        if (statusCounter > 180) { // ~3 seconds at 60fps
            showStatus = false;
            statusMessage = "";
            statusCounter = 0;
        }
    }
    
    EndDialog();
}