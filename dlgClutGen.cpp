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
    SetNextWindowSize(800, 650);  // Reduced from 1000x700
    
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
    // COMPACT HEADER SECTION - Multi-row layout
    // =========================================================================
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    
    if (ImGui::BeginChild("HeaderSection", ImVec2(0, 105), true)) {
        
        // Row 1: Status and current selection
        ImGui::BeginGroup();
        {
            SuccessText("Magic Wand Active:");
            ImGui::SameLine();
            InfoText("Left-click = FROM, Right-click = TO");
            
            // Current FROM/TO selection - more compact
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(15, 0));
            ImGui::SameLine();
            
            int fromColor = g_clutGenerator->GetSelectedFromColor();
            int toColor = g_clutGenerator->GetSelectedToColor();
            PalEntry fromEntry, toEntry;
            
            if (g_clutGenerator->GetOriginalPaletteEntry(fromColor, fromEntry)) {
                char fromLabel[24];
                sprintf(fromLabel, "F:%d", fromColor);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                if (ImGui::Button(fromLabel, ImVec2(50, 18))) {
                    // Click FROM button to clear it
                    g_clutGenerator->SetSelectedFromColor(0);
                    statusMessage = "Cleared FROM selection";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
                
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("FROM Color %d: RGB(%d, %d, %d)\nClick to clear FROM selection", fromColor, fromEntry.red, fromEntry.green, fromEntry.blue);
                }
            }
            
            ImGui::SameLine();
            ImGui::Text(">");
            ImGui::SameLine();
            
            if (g_clutGenerator->GetOriginalPaletteEntry(toColor, toEntry)) {
                char toLabel[24];
                sprintf(toLabel, "T:%d", toColor);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                if (ImGui::Button(toLabel, ImVec2(50, 18))) {
                    // Click TO button to clear it
                    g_clutGenerator->SetSelectedToColor(0);
                    statusMessage = "Cleared TO selection";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
                
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("TO Color %d: RGB(%d, %d, %d)\nClick to clear TO selection", toColor, toEntry.red, toEntry.green, toEntry.blue);
                }
            }
            
            // Auto-add remap when both colors are selected and different
            if (fromColor != toColor && fromColor != 0 && toColor != 0) {
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(10, 0));
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.36f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.24f, 1.0f));
                if (ImGui::Button("Add Remap", ImVec2(80, 18))) {
                    g_clutGenerator->AddRemap(fromColor, toColor);
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    selectedRemapIndex = remaps.size() - 1;
                    statusMessage = "Added remap";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
                
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Add this FROM->TO mapping to the remap list");
                }
            }
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        
        // Row 2: Analysis and main controls
        ImGui::BeginGroup();
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Analyze Image", ImVec2(100, 20))) {
                g_clutGenerator->AnalyzeImageColorUsage();
                statusMessage = "Image analysis complete";
                showStatus = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            char usageText[64];
            sprintf(usageText, "(%d colors)", (int)usedColors.size());
            InfoText(usageText);
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.72f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.48f, 0.16f, 1.0f));
            if (ImGui::Button("Revert All", ImVec2(80, 20))) {
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
            if (ImGui::Button("Clear All", ImVec2(80, 20))) {
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
        
        // Row 3: Edit mode status (only when active)
        if (editingMode && selectedRemapIndex >= 0) {
            WarningText(">> EDIT MODE:");
            ImGui::SameLine();
            SuccessText(editModeStatus.c_str());
            ImGui::SameLine();
            
            // Toggle between editing FROM and TO
            const char* editModeText = editingFromColor ? "[Edit FROM]" : "[Edit TO]";
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(editingFromColor ? 1.0f : 0.3f, editingFromColor ? 0.3f : 1.0f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(editingFromColor ? 1.0f : 0.5f, editingFromColor ? 0.5f : 1.0f, 0.5f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(editingFromColor ? 0.8f : 0.1f, editingFromColor ? 0.1f : 0.8f, 0.1f, 0.8f));
            if (ImGui::Button(editModeText, ImVec2(80, 18))) {
                editingFromColor = !editingFromColor;
                editModeStatus = editingFromColor ? "Edit FROM" : "Edit TO";
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::Button("Apply", ImVec2(50, 18))) {
                if (selectedRemapIndex >= 0) {
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    if (selectedRemapIndex < remaps.size()) {
                        g_clutGenerator->RemoveRemap(remaps[selectedRemapIndex].fromColor);
                        g_clutGenerator->AddRemap(g_clutGenerator->GetSelectedFromColor(), 
                                                g_clutGenerator->GetSelectedToColor());
                        statusMessage = "Applied changes";
                        showStatus = true;
                    }
                }
                editingMode = false;
                selectedRemapIndex = -1;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(50, 18))) {
                editingMode = false;
                selectedRemapIndex = -1;
                editModeStatus = "";
            }
        } else {
            InfoText("Click remap entries to edit them - Double-click palette to clear selections");
        }
        
    }
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // =========================================================================
    // MAIN WORKSPACE - 2-column layout
    // =========================================================================
    
    if (ImGui::BeginChild("MainWorkspace", ImVec2(0, -160))) {  // Reduced bottom space
        
        // =====================================================================
        // LEFT COLUMN - Palette Grid
        // =====================================================================
        if (ImGui::BeginChild("LeftColumn", ImVec2(availableWidth * 0.58f, 0), true)) {
            
            HeaderText("Palette Grid");
            
            // Very compact legend - single line
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            if (usedColors.size() > 0) {
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(20, 0));
                ImGui::SameLine();
                
                // Draw very compact legend
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                
                // Used
                drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(100, 150, 255, 255), 0.0f, 0, 1.0f);
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                ImGui::Text("Used");
                
                ImGui::SameLine();
                pos = ImGui::GetCursorScreenPos();
                drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(255, 80, 80, 255), 0.0f, 0, 2.0f);
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                ImGui::Text("FROM");
                
                ImGui::SameLine();
                pos = ImGui::GetCursorScreenPos();
                drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(80, 255, 80, 255), 0.0f, 0, 2.0f);
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                ImGui::Text("TO");
                
                ImGui::SameLine();
                pos = ImGui::GetCursorScreenPos();
                drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(255, 255, 80, 255), 0.0f, 0, 2.0f);
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                ImGui::Text("Remap");
            }
            
            ImGui::Separator();
            ImGui::Spacing();
            
            // Palette Grid
            if (g_clutGenerator && g_clutGenerator->IsActive()) {
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.8f));
                if (ImGui::BeginChild("PaletteGrid", ImVec2(0, 0), true)) {
                    
                    const int COLORS_PER_ROW = 16;
                    const float BUTTON_SIZE = 18.0f;
                    const float SPACING_VAL = 1.0f;
                    
                    // Track if we clicked on empty space to clear selections
                    bool clickedEmpty = false;
                    
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
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isSelectedRemapTo) {
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isFromColor) {
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                                } else if (isToColor) {
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                                } else if (hasRemap) {
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 80, 200), 0.0f, 0, 2.0f);
                                } else if (isUsedInImage) {
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(100, 150, 255, 150), 0.0f, 0, 1.0f);
                                }
                                
                                // Click handling
                                if (ImGui::IsItemClicked(0)) { // Left click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM updated";
                                        } else {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedFromColor(colorIndex);
                                    }
                                }
                                if (ImGui::IsItemClicked(1)) { // Right click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO updated";
                                        } else {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedToColor(colorIndex);
                                    }
                                }
                                
                                // Double-click to clear selections
                                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                    g_clutGenerator->SetSelectedFromColor(0);
                                    g_clutGenerator->SetSelectedToColor(0);
                                    statusMessage = "Cleared selections";
                                    showStatus = true;
                                }
                                
                                // Enhanced tooltip with all the detailed info
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
                                        "Color Index: %d\n"
                                        "RGB: (%d, %d, %d)\n"
                                        "Hex: #%02X%02X%02X%s\n"
                                        "Left-click = FROM, Right-click = TO\n"
                                        "Double-click to clear selections%s",
                                        colorIndex, 
                                        originalEntry.red, originalEntry.green, originalEntry.blue,
                                        originalEntry.red, originalEntry.green, originalEntry.blue,
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
                    InfoText("1. Click colors in image/palette");
                    InfoText("2. Use 'Add Remap' button");
                    InfoText("3. Toggle ON/OFF as needed");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            } else {
                // Remap table with selection - more compact
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.08f, 0.8f));
                if (ImGui::BeginChild("RemapTable", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar)) {
                    
                    hoveredRemapIndex = -1; // Reset hover state
                    
                    for (int i = 0; i < static_cast<int>(remaps.size()); i++) {
                        const ColorRemapEntry& remap = remaps[i];
                        
                        // Selection highlighting with background color
                        bool isSelected = (i == selectedRemapIndex);
                        if (isSelected) {
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 24);
                            ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                                                                    IM_COL32(76, 51, 25, 180)); // Selection highlight
                        }
                        
                        ImGui::BeginGroup();
                        
                        // Color swatches and info - more compact
                        PalEntry fromEntry, toEntry;
                        if (g_clutGenerator->GetOriginalPaletteEntry(remap.fromColor, fromEntry) &&
                            g_clutGenerator->GetOriginalPaletteEntry(remap.toColor, toEntry)) {
                            
                            // FROM color - smaller
                            char fromId[32];
                            sprintf(fromId, "##from%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                            if (ImGui::Button(fromId, ImVec2(20, 20))) {
                                if (selectedRemapIndex == i && !editingMode) {
                                    editingMode = true;
                                    editingFromColor = true;
                                    g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                    g_clutGenerator->SetSelectedToColor(remap.toColor);
                                    editModeStatus = "Edit FROM";
                                }
                            }
                            ImGui::PopStyleColor(3);
                            
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("FROM %d: RGB(%d, %d, %d)", 
                                                remap.fromColor, fromEntry.red, fromEntry.green, fromEntry.blue);
                            }
                            
                            ImGui::SameLine();
                            char fromText[16];
                            sprintf(fromText, "%d", remap.fromColor);
                            ImGui::Text("%s", fromText);
                            
                            ImGui::SameLine();
                            ImGui::Text(">");
                            
                            ImGui::SameLine();
                            // TO color - smaller
                            char toId[32];
                            sprintf(toId, "##to%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                            if (ImGui::Button(toId, ImVec2(20, 20))) {
                                if (selectedRemapIndex == i && !editingMode) {
                                    editingMode = true;
                                    editingFromColor = false;
                                    g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                    g_clutGenerator->SetSelectedToColor(remap.toColor);
                                    editModeStatus = "Edit TO";
                                }
                            }
                            ImGui::PopStyleColor(3);
                            
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("TO %d: RGB(%d, %d, %d)", 
                                                remap.toColor, toEntry.red, toEntry.green, toEntry.blue);
                            }
                            
                            ImGui::SameLine();
                            char toText[16];
                            sprintf(toText, "%d", remap.toColor);
                            ImGui::Text("%s", toText);
                        }
                        
                        ImGui::SameLine();
                        
                        // Toggle button - smaller
                        char toggleId[32];
                        sprintf(toggleId, "%s##T%d", remap.active ? "ON" : "OFF", i);
                        if (remap.active) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 0.7f));
                            if (ImGui::Button(toggleId, ImVec2(25, 20))) {
                                g_clutGenerator->ToggleRemapActive(i);
                            }
                            ImGui::PopStyleColor(3);
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
                            if (ImGui::Button(toggleId, ImVec2(25, 20))) {
                                g_clutGenerator->ToggleRemapActive(i);
                            }
                            ImGui::PopStyleColor(3);
                        }
                        
                        ImGui::SameLine();
                        
                        // Delete button - smaller
                        char deleteId[32];
                        sprintf(deleteId, "X##%d", i);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.7f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.24f, 0.24f, 0.7f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.16f, 0.16f, 0.7f));
                        if (ImGui::Button(deleteId, ImVec2(20, 20))) {
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
                        
                        // Entry click detection - check if the group was clicked
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
    // BOTTOM SECTION - Import and Export (More Compact)
    // =========================================================================
    
    ImGui::Separator();
    
    static int selectedTab = 0;
    
    if (ImGui::Button("Import")) selectedTab = 0;
    ImGui::SameLine();
    if (ImGui::Button("Export")) selectedTab = 1;
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.9f));
    
    if (selectedTab == 0) {
        // IMPORT TAB - Very compact
        if (ImGui::BeginChild("ImportTab", ImVec2(0, 0), true)) {
            
            static char importBuffer[1024] = "";
            ImGui::PushItemWidth(availableWidth * 0.75f);
            ImGui::InputTextWithHint("##import_text", "Paste COLORTBL.SC line here...", importBuffer, sizeof(importBuffer));
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
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
                        sprintf(statusMsg, "Imported %d remaps", (int)newRemaps.size());
                        statusMessage = statusMsg;
                        importBuffer[0] = '\0';
                        showStatus = true;
                    } else {
                        statusMessage = "Failed to parse - check format";
                        showStatus = true;
                    }
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                importBuffer[0] = '\0';
            }
            
        }
        ImGui::EndChild();
        
    } else {
        // EXPORT TAB - Very compact
        if (ImGui::BeginChild("ExportTab", ImVec2(0, 0), true)) {
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            int activeRemaps = 0;
            for (size_t i = 0; i < remaps.size(); i++) {
                if (remaps[i].active) activeRemaps++;
            }
            
            if (ImGui::Button("Generate")) {
                if (activeRemaps > 0) {
                    generatedCode = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop");
                    showCode = true;
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Copy to Clipboard")) {
                if (activeRemaps > 0) {
                    std::string sciTable = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop");
                    
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
                                statusMessage = "Copied to clipboard";
                                showStatus = true;
                            }
                        }
                        CloseClipboard();
                    }
                }
            }
            
            if (showCode && !generatedCode.empty()) {
                ImGui::SameLine();
                ImGui::Text("Ready!");
                
                ImGui::Spacing();
                static char codeBuffer[1024];
                size_t len = generatedCode.length();
                if (len >= sizeof(codeBuffer)) len = sizeof(codeBuffer) - 1;
                memcpy(codeBuffer, generatedCode.c_str(), len);
                codeBuffer[len] = '\0';
                
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##generated_code", codeBuffer, sizeof(codeBuffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
            }
            
        }
        ImGui::EndChild();
    }
    
    ImGui::PopStyleColor();
    
    // Compact status message
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        if (statusMessage.find("Success") != std::string::npos || statusMessage.find("complete") != std::string::npos || statusMessage.find("Imported") != std::string::npos) {
            SuccessText(statusMessage.c_str());
        } else if (statusMessage.find("Failed") != std::string::npos) {
            ErrorText(statusMessage.c_str());
        } else {
            InfoText(statusMessage.c_str());
        }
        
        // Auto-hide status
        static int statusCounter = 0;
        statusCounter++;
        if (statusCounter > 180) {
            showStatus = false;
            statusMessage = "";
            statusCounter = 0;
        }
    }
    
    EndDialog();
}