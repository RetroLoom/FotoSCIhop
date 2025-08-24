/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Simplified CLUT Generator Dialog - Real-time Updates Only
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"
#include "ClutGenerator.h"

void RenderClutGeneratorDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static std::string generatedCode = "";
    static bool showCode = false;
    static bool shouldClose = false;
    static std::string statusMessage = "";
    static bool showStatus = false;
    
    // Enhanced state for better editing
    static int selectedRemapIndex = -1;  // Which remap is currently selected
    static int hoveredRemapIndex = -1;   // Which remap is being hovered
    static bool editingMode = false;     // Are we editing a selected remap?
    static std::string editModeStatus = "";
    static bool editingFromColor = true; // true = editing FROM, false = editing TO
    
    bool open = true;
    SetNextWindowSize(1200, 800);
    
    if (!BeginDialog("CLUT Generator - Real-time Preview", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button - always restore original palette and disable magic wand
    if (!open || shouldClose) {
        shouldClose = false;
        selectedRemapIndex = -1;
        editingMode = false;
        
        // Clean shutdown - restore original palette and disable magic wand
        if (g_clutGenerator) {
            g_clutGenerator->Shutdown(); // This automatically restores palette and disables magic wand
        }
        
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
        EndDialog();
        return;
    }
    
    // Initialize CLUT generator if needed
    if (!g_clutGenerator) {
        g_clutGenerator = new ClutGenerator();
    }
    
    if (!g_clutGenerator->IsActive()) {
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            g_clutGenerator->Initialize(currentPalette);
            g_clutGenerator->AnalyzeImageColorUsage(); // Analyze colors on startup
        } else {
            ErrorText("No palette loaded! Please open a .v56 or .p56 file first.");
            ImGui::Spacing();
            if (FotoSCIhopStyles::CloseButton("Close")) {
                ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
            }
            EndDialog();
            return;
        }
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // HEADER SECTION - Simplified
    // =========================================================================
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    
    if (ImGui::BeginChild("HeaderSection", ImVec2(0, 70), true)) {
        
        // Row 1: Main controls
        ImGui::BeginGroup();
        {
            HeaderText("Real-time CLUT Preview Active");
            ImGui::SameLine();
            SuccessText("* Magic Wand Enabled");
            
            ImGui::SameLine(); 
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Analyze Image")) {
                g_clutGenerator->AnalyzeImageColorUsage();
                statusMessage = "Image color analysis complete";
                showStatus = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            char usageText[64];
            sprintf(usageText, "(%d colors found)", (int)usedColors.size());
            InfoText(usageText);
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.72f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.48f, 0.16f, 1.0f));
            if (ImGui::Button("Revert All")) {
                g_clutGenerator->RevertToOriginal();
                selectedRemapIndex = -1;
                editingMode = false;
                statusMessage = "Reverted to original palette";
                showStatus = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
            if (ImGui::Button("Clear All")) {
                g_clutGenerator->ClearAllRemaps();
                selectedRemapIndex = -1;
                editingMode = false;
                statusMessage = "Cleared all remaps";
                showStatus = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            if (FotoSCIhopStyles::CloseButton("Close")) {
                shouldClose = true;
            }
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        
        // Row 2: Status and mode indicators
        if (editingMode && selectedRemapIndex >= 0) {
            WarningText(">> EDIT MODE:");
            ImGui::SameLine();
            SuccessText(editModeStatus.c_str());
            ImGui::SameLine();
            
            // Toggle between editing FROM and TO
            const char* editModeText = editingFromColor ? "[Editing FROM]" : "[Editing TO]";
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(editingFromColor ? 1.0f : 0.3f, editingFromColor ? 0.3f : 1.0f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(editingFromColor ? 1.0f : 0.5f, editingFromColor ? 0.5f : 1.0f, 0.5f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(editingFromColor ? 0.8f : 0.1f, editingFromColor ? 0.1f : 0.8f, 0.1f, 0.8f));
            if (ImGui::Button(editModeText)) {
                editingFromColor = !editingFromColor;
                editModeStatus = editingFromColor ? "Now editing FROM color" : "Now editing TO color";
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.36f, 0.36f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.24f, 0.24f, 0.8f));
            if (ImGui::Button("Exit Edit Mode")) {
                editingMode = false;
                selectedRemapIndex = -1;
                editModeStatus = "";
            }
            ImGui::PopStyleColor(3);
        } else {
            WarningText("* Magic Wand Active: Left-click = FROM, Right-click = TO");
            ImGui::SameLine();
            InfoText("- Click remap entries to select and edit them");
        }
        
    }
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // =========================================================================
    // MAIN WORKSPACE - 3-column layout
    // =========================================================================
    
    if (ImGui::BeginChild("MainWorkspace", ImVec2(0, -240))) {
        
        // =====================================================================
        // LEFT COLUMN - Color Selection
        // =====================================================================
        if (ImGui::BeginChild("LeftColumn", ImVec2(availableWidth * 0.32f, 0), true)) {
            
            HeaderText("Color Selection");
            ImGui::Separator();
            ImGui::Spacing();
            
            // Current selection display
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.2f, 0.9f));
            if (ImGui::BeginChild("CurrentSelection", ImVec2(0, 200), true)) {
                
                int fromColor = g_clutGenerator->GetSelectedFromColor();
                int toColor = g_clutGenerator->GetSelectedToColor();
                
                // FROM color display
                ImGui::Text("FROM Color:");
                PalEntry fromEntry;
                if (g_clutGenerator->GetOriginalPaletteEntry(fromColor, fromEntry)) {
                    char fromLabel[64];
                    sprintf(fromLabel, "  %d  ", fromColor);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                    ImGui::Button(fromLabel);
                    ImGui::PopStyleColor(3);
                    
                    ImGui::SameLine();
                    char rgbText[32];
                    sprintf(rgbText, "RGB(%d, %d, %d)", fromEntry.red, fromEntry.green, fromEntry.blue);
                    ImGui::Text("%s", rgbText);
                }
                
                ImGui::Spacing();
                
                // TO color display  
                ImGui::Text("TO Color:");
                PalEntry toEntry;
                if (g_clutGenerator->GetOriginalPaletteEntry(toColor, toEntry)) {
                    char toLabel[64];
                    sprintf(toLabel, "  %d  ", toColor);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                    ImGui::Button(toLabel);
                    ImGui::PopStyleColor(3);
                    
                    ImGui::SameLine();
                    char rgbText[32];
                    sprintf(rgbText, "RGB(%d, %d, %d)", toEntry.red, toEntry.green, toEntry.blue);
                    ImGui::Text("%s", rgbText);
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Edit mode status and navigation
                if (editingMode && selectedRemapIndex >= 0) {
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    if (selectedRemapIndex < remaps.size()) {
                        char editingText[64];
                        sprintf(editingText, "Editing Remap #%d", selectedRemapIndex + 1);
                        WarningText(editingText);
                        InfoText("Click palette colors to modify");
                        ImGui::Spacing();
                        
                        // Navigation buttons
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.72f, 0.96f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.48f, 0.64f, 0.8f));
                        if (ImGui::Button("<< Previous Color")) {
                            if (editingFromColor) {
                                int newFrom = (fromColor > 0) ? fromColor - 1 : 255;
                                g_clutGenerator->SetSelectedFromColor(newFrom);
                            } else {
                                int newTo = (toColor > 0) ? toColor - 1 : 255;
                                g_clutGenerator->SetSelectedToColor(newTo);
                            }
                        }
                        ImGui::PopStyleColor(3);
                        
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.72f, 0.96f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.48f, 0.64f, 0.8f));
                        if (ImGui::Button("Next Color >>")) {
                            if (editingFromColor) {
                                int newFrom = (fromColor < 255) ? fromColor + 1 : 0;
                                g_clutGenerator->SetSelectedFromColor(newFrom);
                            } else {
                                int newTo = (toColor < 255) ? toColor + 1 : 0;
                                g_clutGenerator->SetSelectedToColor(newTo);
                            }
                        }
                        ImGui::PopStyleColor(3);
                    }
                }
                
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            // Action buttons
            if (!editingMode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 1.0f));
                if (ImGui::Button("Add New Remap", ImVec2(-1, 0))) {
                    int fromCol = g_clutGenerator->GetSelectedFromColor();
                    int toCol = g_clutGenerator->GetSelectedToColor();
                    if (fromCol != toCol) {
                        g_clutGenerator->AddRemap(fromCol, toCol);
                        // Auto-select the new remap
                        const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                        selectedRemapIndex = remaps.size() - 1;
                        statusMessage = "Added new remap";
                        showStatus = true;
                    }
                }
                ImGui::PopStyleColor(3);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
                if (ImGui::Button("Remove FROM Color", ImVec2(-1, 0))) {
                    int fromCol = g_clutGenerator->GetSelectedFromColor();
                    g_clutGenerator->RemoveRemap(fromCol);
                    selectedRemapIndex = -1;
                    statusMessage = "Removed remap";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
            } else {
                // Edit mode buttons
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 1.0f));
                if (ImGui::Button("Apply Changes", ImVec2(-1, 0))) {
                    // Apply the current FROM/TO to the selected remap
                    if (selectedRemapIndex >= 0) {
                        const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                        if (selectedRemapIndex < remaps.size()) {
                            g_clutGenerator->RemoveRemap(remaps[selectedRemapIndex].fromColor);
                            g_clutGenerator->AddRemap(g_clutGenerator->GetSelectedFromColor(), 
                                                    g_clutGenerator->GetSelectedToColor());
                            statusMessage = "Applied changes to remap";
                            showStatus = true;
                        }
                    }
                    editingMode = false;
                    selectedRemapIndex = -1;
                }
                ImGui::PopStyleColor(3);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.36f, 0.36f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.24f, 0.24f, 0.8f));
                if (ImGui::Button("Cancel Edit", ImVec2(-1, 0))) {
                    editingMode = false;
                    selectedRemapIndex = -1;
                    editModeStatus = "";
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Usage instructions
            HeaderText("Quick Guide:");
            if (editingMode) {
                InfoText(">> EDIT MODE ACTIVE");
                InfoText("- Click palette colors to modify");
                InfoText("- Use arrows to navigate colors");
                InfoText("- Toggle FROM/TO in header");
                InfoText("- Apply or Cancel when done");
            } else {
                InfoText("- Left-click image: FROM color");
                InfoText("- Right-click image: TO color");
                InfoText("- Click remap entries to edit");
                InfoText("- Magic wand always active");
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // =====================================================================
        // MIDDLE COLUMN - Full Palette Grid
        // =====================================================================
        if (ImGui::BeginChild("MiddleColumn", ImVec2(availableWidth * 0.36f, 0), true)) {
            
            HeaderText("Palette Grid");
            ImGui::Separator();
            
            // Legend
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            if (usedColors.size() > 0) {
                ImGui::Text("Legend:");
                
                // Draw legend items with custom border examples
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                
                ImVec2 startPos = ImGui::GetCursorScreenPos();
                
                // Used in image (thin blue border)
                ImVec2 usedMin = startPos;
                ImVec2 usedMax = ImVec2(startPos.x + 12, startPos.y + 12);
                drawList->AddRectFilled(usedMin, usedMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(usedMin, usedMax, IM_COL32(100, 150, 255, 255), 0.0f, 0, 1.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Used");
                
                ImGui::SameLine();
                ImVec2 currentPos = ImGui::GetCursorScreenPos();
                
                // FROM color (thick red border)
                ImVec2 fromMin = currentPos;
                ImVec2 fromMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(fromMin, fromMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(fromMin, fromMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("FROM");
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // TO color (thick green border)
                ImVec2 toMin = currentPos;
                ImVec2 toMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(toMin, toMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(toMin, toMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("TO");
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // Remapped (medium yellow border)
                ImVec2 remapMin = currentPos;
                ImVec2 remapMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(remapMin, remapMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(remapMin, remapMax, IM_COL32(255, 255, 80, 255), 0.0f, 0, 2.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Remap");
                
                ImGui::NewLine();
                ImGui::Separator();
            }
            
            ImGui::Spacing();
            
            if (g_clutGenerator && g_clutGenerator->IsActive()) {
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.8f));
                if (ImGui::BeginChild("PaletteGrid", ImVec2(0, 0), true)) {
                    
                    const int COLORS_PER_ROW = 16;
                    const float BUTTON_SIZE = 18.0f;
                    const float SPACING_VAL = 1.0f;
                    
                    // Get current selected remap for highlighting
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    int highlightFromColor = -1;
                    int highlightToColor = -1;
                    
                    if (selectedRemapIndex >= 0 && selectedRemapIndex < remaps.size()) {
                        highlightFromColor = remaps[selectedRemapIndex].fromColor;
                        highlightToColor = remaps[selectedRemapIndex].toColor;
                    } else if (hoveredRemapIndex >= 0 && hoveredRemapIndex < remaps.size()) {
                        highlightFromColor = remaps[hoveredRemapIndex].fromColor;
                        highlightToColor = remaps[hoveredRemapIndex].toColor;
                    }
                    
                    for (int row = 0; row < 16; row++) {
                        for (int col = 0; col < 16; col++) {
                            int colorIndex = row * COLORS_PER_ROW + col;
                            
                            PalEntry originalEntry;
                            if (g_clutGenerator->GetOriginalPaletteEntry(colorIndex, originalEntry)) {
                                
                                char buttonId[16];
                                sprintf(buttonId, "##%d", colorIndex);
                                
                                // Enhanced state checking
                                int fromColor = g_clutGenerator->GetSelectedFromColor();
                                int toColor = g_clutGenerator->GetSelectedToColor();
                                bool isFromColor = (colorIndex == fromColor);
                                bool isToColor = (colorIndex == toColor);
                                bool hasRemap = g_clutGenerator->HasRemap(colorIndex);
                                bool isUsedInImage = g_clutGenerator->IsColorUsedInImage(colorIndex);
                                
                                // Check if this color is part of selected/hovered remap
                                bool isSelectedRemapFrom = (colorIndex == highlightFromColor);
                                bool isSelectedRemapTo = (colorIndex == highlightToColor);
                                
                                float r = originalEntry.red / 255.0f;
                                float g = originalEntry.green / 255.0f;
                                float b = originalEntry.blue / 255.0f;
                                
                                // Always use original color for button background
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.0f));
                                
                                // Store button position for border drawing
                                ImVec2 buttonPos = ImGui::GetCursorScreenPos();
                                
                                if (ImGui::Button(buttonId, ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
                                    // Handle clicks
                                }
                                
                                ImGui::PopStyleColor(3);
                                
                                // Draw colored borders for different states
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                ImVec2 buttonMin = buttonPos;
                                ImVec2 buttonMax = ImVec2(buttonPos.x + BUTTON_SIZE, buttonPos.y + BUTTON_SIZE);
                                
                                // Priority system for border colors (highest priority wins)
                                if (isSelectedRemapFrom) {
                                    // Thick magenta border for selected remap FROM
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isSelectedRemapTo) {
                                    // Thick cyan border for selected remap TO
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isFromColor) {
                                    // Thick red border for current FROM
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                                } else if (isToColor) {
                                    // Thick green border for current TO
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                                } else if (hasRemap) {
                                    // Medium yellow border for remapped colors
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 80, 200), 0.0f, 0, 2.0f);
                                } else if (isUsedInImage) {
                                    // Thin blue border for used in image
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(100, 150, 255, 150), 0.0f, 0, 1.0f);
                                }
                                
                                // Click handling for edit mode
                                if (ImGui::IsItemClicked(0)) { // Left click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM color updated";
                                        } else {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO color updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedFromColor(colorIndex);
                                    }
                                }
                                if (ImGui::IsItemClicked(1)) { // Right click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO color updated";
                                        } else {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM color updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedToColor(colorIndex);
                                    }
                                }
                                
                                // Enhanced tooltip
                                if (ImGui::IsItemHovered()) {
                                    char tooltipText[512];
                                    std::string roleText = "";
                                    
                                    if (isSelectedRemapFrom) roleText += " [SELECTED FROM]";
                                    if (isSelectedRemapTo) roleText += " [SELECTED TO]";
                                    if (isFromColor) roleText += " [CURRENT FROM]";
                                    if (isToColor) roleText += " [CURRENT TO]";
                                    if (hasRemap) roleText += " [REMAPPED]";
                                    if (isUsedInImage) roleText += " [USED IN IMAGE]";
                                    
                                    sprintf(tooltipText, 
                                        "Color %d: RGB(%d, %d, %d)%s\n"
                                        "Left = FROM, Right = TO%s",
                                        colorIndex, originalEntry.red, originalEntry.green, originalEntry.blue, 
                                        roleText.c_str(),
                                        editingMode ? " (EDIT MODE)" : ""
                                    );
                                    ImGui::SetTooltip("%s", tooltipText);
                                }
                                
                                if (col < 15) {
                                    ImGui::SameLine(0, SPACING_VAL);
                                }
                            }
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // =====================================================================
        // RIGHT COLUMN - Remap Management
        // =====================================================================
        if (ImGui::BeginChild("RightColumn", ImVec2(0, 0), true)) {
            
            HeaderText("Active Remaps");
            ImGui::Separator();
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            
            // Status summary
            char statusText[128];
            int activeRemaps = 0;
            for (size_t i = 0; i < remaps.size(); i++) {
                if (remaps[i].active) activeRemaps++;
            }
            
            sprintf(statusText, "%d Active (max 12)", activeRemaps);
            if (activeRemaps > 12) {
                WarningText(statusText);
            } else if (activeRemaps > 0) {
                SuccessText(statusText);
            } else {
                DisabledText(statusText);
            }
            
            if (selectedRemapIndex >= 0) {
                ImGui::SameLine();
                char selectedText[64];
                sprintf(selectedText, "(#%d selected)", selectedRemapIndex + 1);
                WarningText(selectedText);
            }
            
            ImGui::Spacing();
            
            if (remaps.empty()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.1f, 0.3f));
                if (ImGui::BeginChild("EmptyState", ImVec2(0, 120), true)) {
                    ImGui::Spacing();
                    InfoText("No remaps yet");
                    ImGui::Spacing();
                    InfoText("Click colors in image");
                    InfoText("then 'Add New Remap'");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            } else {
                // Remap table with selection
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.08f, 0.8f));
                if (ImGui::BeginChild("RemapTable", ImVec2(0, 0), true)) {
                    
                    hoveredRemapIndex = -1; // Reset hover state
                    
                    for (int i = 0; i < static_cast<int>(remaps.size()); i++) {
                        const ColorRemapEntry& remap = remaps[i];
                        
                        // Selection highlighting
                        bool isSelected = (i == selectedRemapIndex);
                        if (isSelected) {
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.2f, 0.1f, 0.7f));
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                        }
                        
                        char remapChildId[32];
                        sprintf(remapChildId, "RemapEntry_%d", i);
                        
                        if (ImGui::BeginChild(remapChildId, ImVec2(0, 45), true)) {
                            
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
                            ImGui::BeginGroup();
                            
                            // Color swatches and info
                            PalEntry fromEntry, toEntry;
                            if (g_clutGenerator->GetOriginalPaletteEntry(remap.fromColor, fromEntry) &&
                                g_clutGenerator->GetOriginalPaletteEntry(remap.toColor, toEntry)) {
                                
                                // FROM color
                                char fromId[32];
                                sprintf(fromId, "##from%d", i);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                                if (ImGui::Button(fromId, ImVec2(25, 25))) {
                                    // Enter edit mode for FROM color if already selected
                                    if (selectedRemapIndex == i && !editingMode) {
                                        editingMode = true;
                                        editingFromColor = true;
                                        g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                        g_clutGenerator->SetSelectedToColor(remap.toColor);
                                        editModeStatus = "Editing FROM color - click palette to modify";
                                    }
                                }
                                ImGui::PopStyleColor(3);
                                
                                ImGui::SameLine();
                                char fromText[16];
                                sprintf(fromText, "%d", remap.fromColor);
                                ImGui::Text("%s", fromText);
                                
                                ImGui::SameLine();
                                ImGui::Text("->");
                                
                                ImGui::SameLine();
                                // TO color
                                char toId[32];
                                sprintf(toId, "##to%d", i);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                                if (ImGui::Button(toId, ImVec2(25, 25))) {
                                    // Enter edit mode for TO color if already selected
                                    if (selectedRemapIndex == i && !editingMode) {
                                        editingMode = true;
                                        editingFromColor = false;
                                        g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                        g_clutGenerator->SetSelectedToColor(remap.toColor);
                                        editModeStatus = "Editing TO color - click palette to modify";
                                    }
                                }
                                ImGui::PopStyleColor(3);
                                
                                ImGui::SameLine();
                                char toText[16];
                                sprintf(toText, "%d", remap.toColor);
                                ImGui::Text("%s", toText);
                            }
                            
                            ImGui::SameLine();
                            
                            // Toggle button
                            char toggleId[32];
                            sprintf(toggleId, "%s##T%d", remap.active ? "ON" : "OFF", i);
                            if (remap.active) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 0.7f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 0.7f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 0.7f));
                                if (ImGui::Button(toggleId, ImVec2(30, 25))) {
                                    g_clutGenerator->ToggleRemapActive(i);
                                }
                                ImGui::PopStyleColor(3);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
                                if (ImGui::Button(toggleId, ImVec2(30, 25))) {
                                    g_clutGenerator->ToggleRemapActive(i);
                                }
                                ImGui::PopStyleColor(3);
                            }
                            
                            ImGui::SameLine();
                            
                            // Delete button
                            char deleteId[32];
                            sprintf(deleteId, "X##%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.24f, 0.24f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.16f, 0.16f, 0.7f));
                            if (ImGui::Button(deleteId, ImVec2(25, 25))) {
                                g_clutGenerator->ClearRemap(i);
                                if (selectedRemapIndex == i) {
                                    selectedRemapIndex = -1;
                                    editingMode = false;
                                } else if (selectedRemapIndex > i) {
                                    selectedRemapIndex--;
                                }
                                statusMessage = "Deleted remap";
                                showStatus = true;
                            }
                            ImGui::PopStyleColor(3);
                            
                            ImGui::EndGroup();
                            ImGui::PopStyleVar();
                            
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        
                        // Entry click detection
                        if (ImGui::IsItemClicked()) {
                            selectedRemapIndex = i;
                            g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                            g_clutGenerator->SetSelectedToColor(remap.toColor);
                            editingMode = false; // Exit edit mode when selecting a different remap
                        }
                        
                        // Track hover for palette highlighting
                        if (ImGui::IsItemHovered()) {
                            hoveredRemapIndex = i;
                            ImGui::SetTooltip("Click to select - Click FROM/TO buttons to edit");
                        }
                        
                        if (i < static_cast<int>(remaps.size()) - 1) {
                            ImGui::Spacing();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();

    // =========================================================================
    // BOTTOM SECTION - Import and Export
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    static int selectedTab = 0;
    
    if (ImGui::Button("Import from COLORTBL.SC")) selectedTab = 0;
    ImGui::SameLine();
    if (ImGui::Button("Generate SCI Code")) selectedTab = 1;
    
    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.9f));
    
    if (selectedTab == 0) {
        // IMPORT TAB
        if (ImGui::BeginChild("ImportTab", ImVec2(0, 0), true)) {
            
            HeaderText("Import Existing Remaps");
            InfoText("Paste a line from COLORTBL.SC to import existing color remaps");
            
            ImGui::Spacing();
            
            static char importBuffer[1024] = "";
            
            ImGui::Text("COLORTBL.SC Line:");
            ImGui::PushItemWidth(availableWidth - 150);
            ImGui::InputText("##import_text", importBuffer, sizeof(importBuffer));
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.24f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.16f, 0.64f, 1.0f));
            if (ImGui::Button("Import")) {
                if (strlen(importBuffer) > 0) {
                    g_clutGenerator->ClearAllRemaps();
                    selectedRemapIndex = -1;
                    editingMode = false;
                    std::string importLine(importBuffer);
                    bool success = g_clutGenerator->ImportFromSCITableEntry(importLine);
                    
                    if (success) {
                        const std::vector<ColorRemapEntry>& newRemaps = g_clutGenerator->GetCurrentRemaps();
                        char statusMsg[128];
                        sprintf(statusMsg, "Successfully imported %d remaps!", (int)newRemaps.size());
                        statusMessage = statusMsg;
                        importBuffer[0] = '\0';
                        showStatus = true;
                    } else {
                        statusMessage = "Failed to parse remap data. Check format.";
                        showStatus = true;
                    }
                } else {
                    statusMessage = "Please paste a COLORTBL.SC line first";
                    showStatus = true;
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                importBuffer[0] = '\0';
            }
            
            ImGui::Spacing();
            DisabledText("Example: 99 0 100 38 101 0 -1 -1 -1 -1 ... ; black wolf");
            
        }
        ImGui::EndChild();
        
    } else {
        // GENERATE CODE TAB
        if (ImGui::BeginChild("GenerateTab", ImVec2(0, 0), true)) {
            
            HeaderText("Generate COLORTBL.SC Code");
            InfoText("Export your remaps as Sierra SCI-compatible table entries");
            
            ImGui::Spacing();
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            int activeRemaps = 0;
            for (size_t i = 0; i < remaps.size(); i++) {
                if (remaps[i].active) activeRemaps++;
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Generate Code")) {
                if (activeRemaps > 0) {
                    generatedCode = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop CLUT Generator");
                    showCode = true;
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.48f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.32f, 1.0f));
            if (ImGui::Button("Copy to Clipboard")) {
                if (activeRemaps > 0) {
                    std::string sciTable = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop CLUT Generator");
                    
                    if (OpenClipboard(hWnd)) {
                        EmptyClipboard();
                        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, (SIZE_T)(sciTable.length() + 1));
                        if (hClipboardData) {
                            char* pchData = (char*)GlobalLock(hClipboardData);
                            if (pchData) {
                                strcpy(pchData, sciTable.c_str());
                                GlobalUnlock(hClipboardData);
                                SetClipboardData(CF_TEXT, hClipboardData);
                                generatedCode = sciTable;
                                showCode = true;
                            }
                        }
                        CloseClipboard();
                    }
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            
            if (showCode && !generatedCode.empty()) {
                HeaderText("Generated Code:");
                
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
                
                static char codeBuffer[1024];
                size_t len = generatedCode.length();
                if (len >= sizeof(codeBuffer)) len = sizeof(codeBuffer) - 1;
                memcpy(codeBuffer, generatedCode.c_str(), len);
                codeBuffer[len] = '\0';
                
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##generated_code", codeBuffer, sizeof(codeBuffer));
                ImGui::PopItemWidth();
                
                ImGui::PopStyleColor(2);
                
                ImGui::Spacing();
                InfoText("Copy this line into your COLORTBL.SC file's lRemapTable array");
                
            } else {
                InfoText("Create some remaps first, then generate the code!");
            }
            
        }
        ImGui::EndChild();
    }
    
    ImGui::PopStyleColor();
    
    // Status message
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (statusMessage.find("Success") != std::string::npos || statusMessage.find("complete") != std::string::npos) {
            SuccessText(statusMessage.c_str());
        } else if (statusMessage.find("Failed") != std::string::npos || statusMessage.find("ERROR") != std::string::npos) {
            ErrorText(statusMessage.c_str());
        } else {
            InfoText(statusMessage.c_str());
        }
        
        // Auto-hide status after a few seconds
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