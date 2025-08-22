/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

void RenderAboutDialog() {
    using namespace ImGuiDialogs;
    
    bool open = true;
    if (!BeginDialog("About FotoSCIhop", &open)) {
        EndDialog();
        return;
    }
    
    // If user clicked the X button or pressed Escape, hide this dialog
    if (!open) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_ABOUT);
        EndDialog();
        return;
    }

    // Calculate responsive widths
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // APPLICATION INFO SECTION
    // =========================================================================
    
    // Center the main title
    const char* appTitle = "FotoSCIhop";
    float titleWidth = ImGui::CalcItemWidth() * 0.6f; // Estimate title width
    
    // Center alignment helper
    float windowWidth = ImGui::GetWindowSize().x;
    float center = (windowWidth - titleWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    
    // Large title with styling
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
    FotoSCIhopStyles::HeaderText(appTitle);
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // Subtitle - center alignment
    float subtitleWidth = availableWidth * 0.8f;
    center = (windowWidth - subtitleWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    ImGui::Text("Sierra SCI1.1/SCI32 Games Image Editor");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // =========================================================================
    // VERSION AND BUILD INFO
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Version Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Version: 2.0 (ImGui Edition)");
        ImGui::Text("Build Date: " __DATE__ " " __TIME__);
        ImGui::Text("Platform: Windows");
        
        #ifdef _WIN64
        ImGui::Text("Architecture: x64");
        #else
        ImGui::Text("Architecture: x86");
        #endif
        
        #ifdef _DEBUG
        FotoSCIhopStyles::WarningText("Build Type: Debug");
        #else
        FotoSCIhopStyles::SuccessText("Build Type: Release");
        #endif
    }
    
    // =========================================================================
    // COPYRIGHT AND AUTHORS
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Copyright & Credits", ImGuiTreeNodeFlags_DefaultOpen)) {
        FotoSCIhopStyles::HeaderText("Original Authors:");
        ImGui::Text("- Enrico Rolfi 'Endroz' (2004-2021)");
        ImGui::Text("- Daniel Arnold 'Dhel' (2022-2025)");
        
        ImGui::Spacing();
        FotoSCIhopStyles::HeaderText("ImGui Migration:");
        ImGui::Text("- Enhanced with modern ImGui interface");
        
        ImGui::Spacing();
        FotoSCIhopStyles::HeaderText("Copyright:");
        ImGui::Text("Copyright (C) Enrico Rolfi 'Endroz', 2004-2021");
        ImGui::Text("Copyright (C) Daniel Arnold 'Dhel', 2022-2025");
        
        ImGui::Spacing();
        FotoSCIhopStyles::InfoText("Part of the TraduSCI package");
    }
    
    // =========================================================================
    // DESCRIPTION
    // =========================================================================
    
    if (ImGui::CollapsingHeader("About This Tool")) {
        ImGui::TextWrapped("FotoSCIhop is a specialized tool for modifying .P56 and .V56 image files from Sierra SCI games. "
                          "It supports both SCI1.1 and SCI32 formats, allowing game modders and translators to edit "
                          "graphics, animations, and color palettes used in classic adventure games.");
        
        ImGui::Spacing();
        
        FotoSCIhopStyles::HeaderText("Supported File Types:");
        ImGui::BulletText(".P56 files - Picture resources (SCI1.1 and SCI32)");
        ImGui::BulletText(".V56 files - View/Animation resources");
        
        ImGui::Spacing();
        
        FotoSCIhopStyles::HeaderText("Key Features:");
        ImGui::BulletText("Import/Export BMP images");
        ImGui::BulletText("Edit color palettes");
        ImGui::BulletText("Modify animation loops and cells");
        ImGui::BulletText("Adjust link points and hot spots");
        ImGui::BulletText("Priority bar visualization");
        ImGui::BulletText("Reference image overlay support");
    }
    
    // =========================================================================
    // SYSTEM INFO (Optional)
    // =========================================================================
    
    if (ImGui::CollapsingHeader("System Information")) {
        char systemInfo[256];
        
        // Get Windows version info
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
        #pragma warning(push)
        #pragma warning(disable: 4996) // Disable deprecation warning for GetVersionEx
        if (GetVersionEx(&osvi)) {
            sprintf(systemInfo, "OS: Windows %d.%d (Build %d)", 
                   osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            ImGui::Text("%s", systemInfo);
        }
        #pragma warning(pop)
        
        // Memory info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            sprintf(systemInfo, "Total RAM: %.1f GB", (float)memInfo.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f));
            ImGui::Text("%s", systemInfo);
            
            sprintf(systemInfo, "Available RAM: %.1f GB", (float)memInfo.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f));
            ImGui::Text("%s", systemInfo);
        }
        
        // Current working directory
        char currentDir[MAX_PATH];
        if (GetCurrentDirectory(MAX_PATH, currentDir)) {
            ImGui::Text("Working Directory:");
            FotoSCIhopStyles::DisabledText(currentDir);
        }
    }
    
    // =========================================================================
    // THIRD PARTY ACKNOWLEDGMENTS
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Third Party Libraries")) {
        FotoSCIhopStyles::HeaderText("This application uses:");
        
        ImGui::BulletText("Dear ImGui - Immediate Mode GUI");
        FotoSCIhopStyles::DisabledText("   https://github.com/ocornut/imgui");
        
        ImGui::BulletText("OpenGL - Graphics rendering");
        ImGui::BulletText("Windows GDI+ - Image processing");
        
        ImGui::Spacing();
        FotoSCIhopStyles::InfoText("Special thanks to the Sierra game preservation community!");
    }
    
    // =========================================================================
    // MAIN BUTTONS
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Center the close button
    float buttonWidth = 120.0f;
    center = (windowWidth - buttonWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    
    if (FotoSCIhopStyles::CloseButton("Close")) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_ABOUT);
    }
    
    EndDialog();
}