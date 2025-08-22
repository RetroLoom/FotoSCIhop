/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"
#include "ClutGenerator.h"
#include "imgui_integration.h"
#include "imgui.h"
#include "fotoscihop_styles.h"
#include "librealmpal.h"
#include <set>

std::string g_realmpalInputFile = "";
std::string g_realmpalPaletteFile = "";
std::string g_realmpalExtraFile = "";
bool g_requestInputDialog = false;
bool g_requestPaletteDialog = false; 
bool g_requestExtraDialog = false;
extern bool g_pendingThemeChange;
extern FotoSCIhopStyles::ThemeMode g_pendingTheme;

// ==== ImGui Dialog Callbacks ====
void RenderPropertiesDialog() {
    using namespace ImGuiDialogs;
    
    bool open = true;
    if (!BeginDialog("Properties", &open)) {
        EndDialog();
        return;
    }
    
    // If user clicked the X button or pressed Escape, hide this dialog
    if (!open) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PROPERTIES);
        EndDialog();
        return;
    }

    // Calculate responsive widths
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = availableWidth * 0.22f; // 22% for each button

    // =========================================================================
    // SCROLLABLE CONTENT AREA
    // =========================================================================
    
    // Reserve space for the close button at the bottom
    float reservedHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y;
    float contentHeight = ImGui::GetContentRegionAvail().y - reservedHeight;
    
    // Create scrollable child window for all content
    // This allows the dialog content to scroll when sections are expanded beyond window height
    if (ImGui::BeginChild("PropertiesContent", ImVec2(0, contentHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        
        // Auto-scroll functionality - track which sections were just opened
        // When a section is newly opened, automatically scroll to make it visible
        static bool wasFileInfoOpen = false;
        static bool wasResolutionOpen = false;
        static bool wasLoopPropsOpen = false;
        static bool wasCellPropsOpen = false;
        static bool wasAddRemoveOpen = false;
        static bool wasLinkPointsOpen = false;
        static bool wasRefImageOpen = false;

        // =========================================================================
        // FILE INFO SECTION (just adding colors)
        // =========================================================================
            bool fileInfoOpen = ImGui::CollapsingHeader("File Information", ImGuiTreeNodeFlags_DefaultOpen);
        
        // Auto-scroll when section is newly opened
        if (fileInfoOpen && !wasFileInfoOpen) {
            ImGui::SetScrollHereY(0.0f); // Scroll so this section is at the top
        }
        wasFileInfoOpen = fileInfoOpen;
        
        if (fileInfoOpen) {
        char textBuffer[256];
        
        if (globalView) {
            ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "File Type: View File (.v56)");  // Light blue
            sprintf(textBuffer, "Current Loop: %d / %d", curLoopIndex + 1, globalView->Head.view32.loopCount);
            ImGui::Text("%s", textBuffer);
            
            if (curLoop && (*curLoop)) {
                if ((*curLoop)->Head.flags) {
                    sprintf(textBuffer, "Loop Type: Mirror of Loop %d", (*curLoop)->Head.altLoop + 1);
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", textBuffer);  // Orange
                } else {
                    sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, (*curLoop)->Head.numCels);
                    ImGui::Text("%s", textBuffer);
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loop Type: Normal");  // Green
                }
            }
        } else if (globalPicture) {
            const char* version = (globalPicture->format == _PIC_11) ? "SCI1.1 Picture" : "SCI32 Picture";
            sprintf(textBuffer, "File Type: %s (.p56)", version);
            ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", textBuffer);  // Light blue
            sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, globalPicture->CellsCount());
            ImGui::Text("%s", textBuffer);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No file loaded");  // Red
        }
    }
    
    // =========================================================================
    // RESOLUTION SECTION (keeping original logic, adding color to apply button)
    // =========================================================================
    if (ImGui::CollapsingHeader("Resolution Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // Get current data - same logic as before
        static int resX = 320, resY = 200;
        static bool needsResolutionRefresh = true;
        
        // Refresh data when needed
        if (needsResolutionRefresh) {
            if (globalView) {
                resX = globalView->Head.view32.resX;
                resY = globalView->Head.view32.resY;
            } else if (globalPicture) {
                switch (globalPicture->format) {
                case _PIC_11: {
                    PicHeader11 *bPic11 = (PicHeader11 *)&globalPicture->Head;
                    resX = bPic11->vanishX; 
                    resY = bPic11->viewAngle;
                    break;
                }
                case _PIC_32: {
                    PicHeader32 *bPic32 = (PicHeader32 *)&globalPicture->Head;
                    resX = bPic32->resX; 
                    resY = bPic32->resY;
                    break;
                }
                }
            }
            needsResolutionRefresh = false;
        }

        // Single column layout for resolution
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Width", &resX);
        ImGui::InputInt("Height", &resY);
        
        // Apply resolution button (now with green color)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
        if (ImGui::Button("Apply Resolution")) {
            if (globalView) {
                globalView->Head.view32.resX = resX;
                globalView->Head.view32.resY = resY;
            } else if (globalPicture) {
                switch (globalPicture->format) {
                case _PIC_11: {
                    PicHeader11 *bPic11 = (PicHeader11 *)&globalPicture->Head;
                    bPic11->vanishX = resX;
                    bPic11->viewAngle = resY;
                    break;
                }
                case _PIC_32: {
                    PicHeader32 *bPic32 = (PicHeader32 *)&globalPicture->Head;
                    bPic32->resX = resX;
                    bPic32->resY = resY;
                    break;
                }
                }
            }
            datasaved = false;
            needsResolutionRefresh = true;
            InvalidateRgn(hWnd, NULL, true);
        }
        ImGui::PopStyleColor(3);
    }
    
    // =========================================================================
    // LOOP PROPERTIES SECTION (View files only) - just adding colors
    // =========================================================================
    if (globalView && curLoop && (*curLoop)) {
        if (ImGui::CollapsingHeader("Loop Properties")) {
            
            static int loopMirror = 0, loopBase = 0;
            static int loopContinue = -1, loopStartCell = -1, loopEndCell = -1;
            static int loopRepeat = 255, loopStepSize = 3;
            static bool needsLoopRefresh = true;
            
            // Refresh loop data
            if (needsLoopRefresh) {
                int selLoop = curLoopIndex;
                loopMirror = globalView->loops[selLoop]->Head.flags;
                loopBase = globalView->loops[selLoop]->Head.altLoop;
                
                if (!loopMirror) {
                    loopContinue = globalView->loops[selLoop]->Head.contLoop;
                    loopStartCell = globalView->loops[selLoop]->Head.startCel;
                    loopEndCell = globalView->loops[selLoop]->Head.endCel;
                    loopRepeat = globalView->loops[selLoop]->Head.repeatCount;
                    loopStepSize = globalView->loops[selLoop]->Head.stepSize;
                } else {
                    loopContinue = -1; loopStartCell = -1; loopEndCell = -1;
                    loopRepeat = 255; loopStepSize = 3;
                }
                needsLoopRefresh = false;
            }

            // Single column layout for loop properties
            bool mirror = (loopMirror != 0);
            ImGui::Checkbox("Mirror Loop", &mirror);
            loopMirror = mirror ? 1 : 0;
            
            ImGui::InputInt("Base Loop", &loopBase);
            
            if (!loopMirror) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Animation Settings:");  // Light blue header
                ImGui::InputInt("Continue Loop", &loopContinue);
                ImGui::InputInt("Start Cell", &loopStartCell);
                ImGui::InputInt("End Cell", &loopEndCell);
                ImGui::InputInt("Repeat Count", &loopRepeat);
                ImGui::InputInt("Step Size", &loopStepSize);
            }
            
            // Apply loop properties button (now with green color)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
            if (ImGui::Button("Apply Loop Properties")) {
                int selLoop = curLoopIndex;
                globalView->loops[selLoop]->Head.flags = loopMirror;
                globalView->loops[selLoop]->Head.altLoop = loopBase;
                
                if (!loopMirror) {
                    globalView->loops[selLoop]->Head.contLoop = loopContinue;
                    globalView->loops[selLoop]->Head.startCel = loopStartCell;
                    globalView->loops[selLoop]->Head.endCel = loopEndCell;
                    globalView->loops[selLoop]->Head.repeatCount = loopRepeat;
                    globalView->loops[selLoop]->Head.stepSize = loopStepSize;
                } else {
                    globalView->loops[selLoop]->Head.contLoop = -1;
                    globalView->loops[selLoop]->Head.startCel = -1;
                    globalView->loops[selLoop]->Head.endCel = -1;
                    globalView->loops[selLoop]->Head.repeatCount = 255;
                    globalView->loops[selLoop]->Head.stepSize = 3;
                }
                
                ShowLoopCell(curLoopIndex, curCellIndex);
                datasaved = false;
                needsLoopRefresh = true;
            }
            ImGui::PopStyleColor(3);
        }
    }
    
    // =========================================================================
    // CELL PROPERTIES SECTION WITH AUTO-APPLY (keeping original logic)
    // =========================================================================
    if (curCell && (*curCell)) {
        if (ImGui::CollapsingHeader("Cell Properties")) {
            
            // Current values
            static int cellX = 0, cellY = 0, cellPriority = 0;
            // Original values for reset/cancel
            static int originalCellX = 0, originalCellY = 0, originalCellPriority = 0;
            // Previous values for change detection
            static int prevCellX = 0, prevCellY = 0, prevCellPriority = 0;
            static bool needsCellRefresh = true;
            static bool cellEditingStarted = false;
            static bool cellHasChanges = false;
            
            // Refresh cell data
            if (needsCellRefresh) {
                if (globalView) {
                    if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                        CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                        cellX = originalCellX = prevCellX = bCell->xHot;
                        cellY = originalCellY = prevCellY = bCell->yHot;
                        cellPriority = originalCellPriority = prevCellPriority = 0;
                    } else {
                        cellX = originalCellX = prevCellX = 0; 
                        cellY = originalCellY = prevCellY = 0;
                        cellPriority = originalCellPriority = prevCellPriority = 0;
                    }
                } else if (globalPicture) {
                    CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                    cellX = originalCellX = prevCellX = bCell->xpos; 
                    cellY = originalCellY = prevCellY = bCell->ypos; 
                    cellPriority = originalCellPriority = prevCellPriority = bCell->priority;
                }
                needsCellRefresh = false;
                cellEditingStarted = false;
                cellHasChanges = false;
            }

            // Single column layout for cell properties
            if (globalView) {
                if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Hot Spot:");  // Light blue header
                    
                    if (ImGui::InputInt("X Hot", &cellX)) cellEditingStarted = true;
                    if (ImGui::InputInt("Y Hot", &cellY)) cellEditingStarted = true;
                    
                } else {
                    ImGui::TextDisabled("Cell properties not available for mirror loops");
                }
            } else if (globalPicture) {
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Position:");  // Light blue header
                
                if (ImGui::InputInt("X Position", &cellX)) cellEditingStarted = true;
                if (ImGui::InputInt("Y Position", &cellY)) cellEditingStarted = true;
                if (ImGui::InputInt("Priority", &cellPriority)) cellEditingStarted = true;
            }
            
            // Auto-apply changes when values change
            if (cellEditingStarted && (cellX != prevCellX || cellY != prevCellY || cellPriority != prevCellPriority)) {
                
                // Apply changes immediately
                if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                    bCell->xHot = cellX;
                    bCell->yHot = cellY;
                    ShowLoopCell(curLoopIndex, curCellIndex);
                } else if (globalPicture) {
                    CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                    bCell->xpos = cellX;
                    bCell->ypos = cellY;
                    bCell->priority = cellPriority;
                    ShowCell(curCellIndex);
                }
                
                // Update tracking variables
                prevCellX = cellX;
                prevCellY = cellY;
                prevCellPriority = cellPriority;
                
                // Check if we have changes from original
                cellHasChanges = (cellX != originalCellX || cellY != originalCellY || cellPriority != originalCellPriority);
                
                if (cellHasChanges) {
                    datasaved = false;
                }
            }
            
            // Reset and Cancel buttons (only show if we have changes or are editing)
            if (cellEditingStarted) {
                ImGui::Separator();
                
                // Show changed indicator with colors
                if (cellHasChanges) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* Values have been modified *");  // Orange
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No changes");  // Gray
                }
                
                if (cellHasChanges) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.36f, 0.36f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
                    if (ImGui::Button("Reset to Original")) {  // Red button
                        cellX = originalCellX;
                        cellY = originalCellY;
                        cellPriority = originalCellPriority;
                        
                        // Apply the reset values
                        if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                            CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                            bCell->xHot = cellX;
                            bCell->yHot = cellY;
                            ShowLoopCell(curLoopIndex, curCellIndex);
                        } else if (globalPicture) {
                            CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                            bCell->xpos = cellX;
                            bCell->ypos = cellY;
                            bCell->priority = cellPriority;
                            ShowCell(curCellIndex);
                        }
                        
                        prevCellX = cellX;
                        prevCellY = cellY;
                        prevCellPriority = cellPriority;
                        cellHasChanges = false;
                        cellEditingStarted = false;
                    }
                    ImGui::PopStyleColor(3);
                }
                
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
                if (ImGui::Button("Done Editing")) {  // Green button
                    cellEditingStarted = false;
                    cellHasChanges = false;
                    // Keep current values as new originals
                    originalCellX = cellX;
                    originalCellY = cellY;
                    originalCellPriority = cellPriority;
                }
                ImGui::PopStyleColor(3);
            }
        }
    }
    
    // =========================================================================
    // ADD / REMOVE SECTION
    // =========================================================================
    if (FotoSCIhopStyles::BeginManagementSection("Add / Remove"))
    {

        // Determine what we're working with
        bool hasLoops = (globalView != nullptr);
        bool hasCells = (globalView != nullptr || globalPicture != nullptr);

        if (!hasCells)
        {
            FotoSCIhopStyles::ErrorText("No file loaded");
            FotoSCIhopStyles::InfoText("Load a .v56 or .p56 file to begin editing");
            FotoSCIhopStyles::EndSection();
            return;
        }

        // Display current file info
        char fileInfo[256];
        if (globalView)
        {
            sprintf(fileInfo, "View File: %d loops, current loop %d (%d cells)",
                    globalView->Head.view32.loopCount, curLoopIndex + 1,
                    (curLoop && (*curLoop)) ? (*curLoop)->Head.numCels : 0);
        }
        else if (globalPicture)
        {
            sprintf(fileInfo, "Picture File: %d cells, current cell %d",
                    globalPicture->CellsCount(), curCellIndex + 1);
        }
        FotoSCIhopStyles::InfoText(fileInfo);
        ImGui::Separator();

        // =====================================================================
        // QUICK OPERATIONS
        // =====================================================================
        if (ImGui::CollapsingHeader("Quick Operations", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Loop operations (V56 only)
            if (hasLoops)
            {
                FotoSCIhopStyles::HeaderText("Loop Operations:");

                ImGui::BeginGroup();
                if (ImGui::Button("Add Loop", ImVec2(buttonWidth, 0)))
                {
                    if (globalView->addLoop(curLoopIndex))
                    {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                        datasaved = false;
                        FotoSCIhopStyles::SuccessText("Loop added successfully");
                    }
                    else
                    {
                        FotoSCIhopStyles::ErrorText("Failed to add loop");
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Add a new loop after the current loop");
                }

                ImGui::SameLine();
                if (globalView->Head.view32.loopCount > 1)
                {
                    if (FotoSCIhopStyles::RemoveButton("Remove Loop"))
                    {
                        if (globalView->deleteLoop(curLoopIndex))
                        {
                            // Adjust current loop index if needed
                            if (curLoopIndex >= globalView->Head.view32.loopCount && globalView->Head.view32.loopCount > 0)
                            {
                                curLoopIndex = globalView->Head.view32.loopCount - 1;
                            }
                            ShowLoopCell(curLoopIndex, curCellIndex);
                            datasaved = false;
                            FotoSCIhopStyles::SuccessText("Loop removed successfully");
                        }
                        else
                        {
                            FotoSCIhopStyles::ErrorText("Failed to remove loop");
                        }
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Remove the current loop");
                    }
                }
                else
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Remove Loop", ImVec2(buttonWidth, 0));
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Cannot remove the last loop");
                    }
                }
                ImGui::EndGroup();

                ImGui::Separator();
            }

            // Cell operations (both P56 and V56)
            FotoSCIhopStyles::HeaderText("Cell Operations:");

            ImGui::BeginGroup();
            if (ImGui::Button("Add Cell", ImVec2(buttonWidth, 0)))
            {
                bool success = false;
                if (globalView)
                {
                    success = globalView->addCell(curLoopIndex, curCellIndex);
                    if (success)
                        ShowLoopCell(curLoopIndex, curCellIndex);
                }
                else if (globalPicture)
                {
                    success = globalPicture->addCell(curCellIndex);
                    if (success)
                        ShowCell(curCellIndex);
                }
                if (success)
                {
                    datasaved = false;
                    FotoSCIhopStyles::SuccessText("Cell added successfully");
                }
                else
                {
                    FotoSCIhopStyles::ErrorText("Failed to add cell");
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Add a new empty cell after the current cell");
            }

            ImGui::SameLine();
            bool canRemoveCell = false;
            if (globalView && curLoop && (*curLoop))
            {
                canRemoveCell = ((*curLoop)->Head.numCels > 1);
            }
            else if (globalPicture)
            {
                canRemoveCell = (globalPicture->CellsCount() > 1);
            }

            if (canRemoveCell)
            {
                if (FotoSCIhopStyles::RemoveButton("Remove Cell"))
                {
                    bool success = false;

                    if (globalView)
                    {
                        // Store current counts before deletion
                        int oldCellCount = (*curLoop)->Head.numCels;

                        success = globalView->deleteCell(curLoopIndex, curCellIndex);

                        if (success)
                        {
                            int newCellCount = (*curLoop)->Head.numCels;

                            // Handle index adjustment more safely
                            if (newCellCount > 0)
                            {
                                // If we deleted the last cell, move to the new last cell
                                if (curCellIndex >= newCellCount)
                                {
                                    curCellIndex = newCellCount - 1;
                                }
                                // Ensure index is still valid
                                if (curCellIndex < 0)
                                {
                                    curCellIndex = 0;
                                }

                                // Only show if we have a valid cell to show
                                ShowLoopCell(curLoopIndex, curCellIndex);
                            }
                            else
                            {
                                // No cells left - set invalid index and handle accordingly
                                curCellIndex = -1;
                                // Don't call ShowLoopCell - maybe show empty state instead
                                // ShowEmptyLoop(curLoopIndex); // If you have such a function
                            }
                        }
                    }
                    else if (globalPicture)
                    {
                        // Store current count before deletion
                        int oldCellCount = globalPicture->CellsCount();

                        success = globalPicture->deleteCell(curCellIndex);

                        if (success)
                        {
                            int newCellCount = globalPicture->CellsCount();

                            // Handle index adjustment more safely
                            if (newCellCount > 0)
                            {
                                // If we deleted the last cell, move to the new last cell
                                if (curCellIndex >= newCellCount)
                                {
                                    curCellIndex = newCellCount - 1;
                                }
                                // Ensure index is still valid
                                if (curCellIndex < 0)
                                {
                                    curCellIndex = 0;
                                }

                                // Only show if we have a valid cell to show
                                ShowCell(curCellIndex);
                            }
                            else
                            {
                                // No cells left - set invalid index
                                curCellIndex = -1;
                                // Don't call ShowCell - handle empty state
                                // This should never happen due to our "don't delete last cell" check
                            }
                        }
                    }

                    if (success)
                    {
                        datasaved = false;
                        FotoSCIhopStyles::SuccessText("Cell removed successfully");
                    }
                    else
                    {
                        FotoSCIhopStyles::ErrorText("Failed to remove cell");
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Remove the current cell");
                }
            }
            else
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::Button("Remove Cell", ImVec2(buttonWidth, 0));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Cannot remove the last cell");
                }
            }
            ImGui::EndGroup();
        }
        FotoSCIhopStyles::EndSection();
    }
    
    // =========================================================================
    // LINK POINTS SECTION WITH AUTO-APPLY
    // =========================================================================

    // Only show Link Points section for valid view files
    bool canShowLinkPoints = false;
    if (globalView && curCell && (*curCell) && curLoop && (*curLoop)) {
        if (!(*curLoop)->Head.flags) {  // Not a mirrored loop
            canShowLinkPoints = true;
        }
    }
    
    if (canShowLinkPoints) {
        if (ImGui::CollapsingHeader("Link Points")) {
            
            // Current link point data
            static int linkCount = 0;
            static int linkX[10] = {0};
            static int linkY[10] = {0};
            static int linkPri[10] = {0};
            static int linkType[10] = {0};
            
            // Original values for reset/cancel
            static int originalLinkCount = 0;
            static int originalLinkX[10] = {0};
            static int originalLinkY[10] = {0};
            static int originalLinkPri[10] = {0};
            static int originalLinkType[10] = {0};
            
            // Previous values for change detection
            static int prevLinkCount = 0;
            static int prevLinkX[10] = {0};
            static int prevLinkY[10] = {0};
            static int prevLinkPri[10] = {0};
            static int prevLinkType[10] = {0};
            
            static bool linkNeedsRefresh = true;
            static bool linkEditingStarted = false;
            static bool linkHasChanges = false;
            
            // Refresh link points data
            if (linkNeedsRefresh) {
                CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                linkCount = originalLinkCount = prevLinkCount = bCell->linkTableCount;
                
                // Clear all arrays first
                for (int i = 0; i < 10; i++) {
                    linkX[i] = originalLinkX[i] = prevLinkX[i] = 0;
                    linkY[i] = originalLinkY[i] = prevLinkY[i] = 0;
                    linkPri[i] = originalLinkPri[i] = prevLinkPri[i] = 0;
                    linkType[i] = originalLinkType[i] = prevLinkType[i] = 0;
                }
                
                // Fill in the actual link points
                for (int i = 0; i < linkCount && i < 10; i++) {
                    linkX[i] = originalLinkX[i] = prevLinkX[i] = (*curCell)->linkPoints[i].x;
                    linkY[i] = originalLinkY[i] = prevLinkY[i] = (*curCell)->linkPoints[i].y;
                    linkPri[i] = originalLinkPri[i] = prevLinkPri[i] = (*curCell)->linkPoints[i].priority;
                    linkType[i] = originalLinkType[i] = prevLinkType[i] = (*curCell)->linkPoints[i].positionType;
                }
                
                linkNeedsRefresh = false;
                linkEditingStarted = false;
                linkHasChanges = false;
            }
            
            // Link Count control
            int oldLinkCount = linkCount;
            if (ImGui::InputInt("Number of Link Points", &linkCount)) {
                linkEditingStarted = true;
            }
            if (linkCount < 0) linkCount = 0;
            if (linkCount > 10) linkCount = 10;
            
            if (linkCount > 0) {
                ImGui::Separator();
                ImGui::Text("Link Point Coordinates:");
                
                // Show link points in single column layout
                for (int i = 0; i < linkCount; i++) {
                    char headerLabel[32];
                    sprintf(headerLabel, "Link Point %d", i + 1);
                    
                    if (ImGui::CollapsingHeader(headerLabel)) {
                        char label[32];
                        
                        sprintf(label, "X##%d", i);
                        if (ImGui::InputInt(label, &linkX[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Y##%d", i);
                        if (ImGui::InputInt(label, &linkY[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Priority##%d", i);
                        if (ImGui::InputInt(label, &linkPri[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Type##%d", i);
                        if (ImGui::InputInt(label, &linkType[i])) linkEditingStarted = true;
                    }
                }
            }
            
            // Check for changes and auto-apply
            bool valuesChanged = (linkCount != prevLinkCount);
            if (!valuesChanged) {
                for (int i = 0; i < linkCount && i < 10; i++) {
                    if (linkX[i] != prevLinkX[i] || linkY[i] != prevLinkY[i] || 
                        linkPri[i] != prevLinkPri[i] || linkType[i] != prevLinkType[i]) {
                        valuesChanged = true;
                        break;
                    }
                }
            }
            
            if (linkEditingStarted && valuesChanged) {
                // Auto-apply changes
                if (globalView && curCell) {
                    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                    
                    bCell->linkTableCount = linkCount;
                    
                    for (int i = 0; i < bCell->linkTableCount && i < 10; i++) {
                        (*curCell)->linkPoints[i].x = linkX[i];
                        (*curCell)->linkPoints[i].y = linkY[i];
                        (*curCell)->linkPoints[i].priority = linkPri[i];
                        (*curCell)->linkPoints[i].positionType = linkType[i];
                    }
                    
                    ShowLoopCell(curLoopIndex, curCellIndex); // refresh screen
                    datasaved = false;
                }
                
                // Update previous values
                prevLinkCount = linkCount;
                for (int i = 0; i < 10; i++) {
                    prevLinkX[i] = linkX[i];
                    prevLinkY[i] = linkY[i];
                    prevLinkPri[i] = linkPri[i];
                    prevLinkType[i] = linkType[i];
                }
                
                // Check if we have changes from original
                linkHasChanges = (linkCount != originalLinkCount);
                if (!linkHasChanges) {
                    for (int i = 0; i < linkCount && i < 10; i++) {
                        if (linkX[i] != originalLinkX[i] || linkY[i] != originalLinkY[i] || 
                            linkPri[i] != originalLinkPri[i] || linkType[i] != originalLinkType[i]) {
                            linkHasChanges = true;
                            break;
                        }
                    }
                }
            }
            
            // Reset and Cancel buttons (only show if we have changes or are editing)
            if (linkEditingStarted) {
                ImGui::Separator();
                
                // Show changed indicator here to prevent shifting
                if (linkHasChanges) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
                    ImGui::Text("* Link points have been modified *");
                    ImGui::PopStyleVar();
                }
                
                if (linkHasChanges && ImGui::Button("Reset Link Points")) {
                    linkCount = originalLinkCount;
                    for (int i = 0; i < 10; i++) {
                        linkX[i] = originalLinkX[i];
                        linkY[i] = originalLinkY[i];
                        linkPri[i] = originalLinkPri[i];
                        linkType[i] = originalLinkType[i];
                    }
                    
                    // Apply the reset values
                    if (globalView && curCell) {
                        CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                        bCell->linkTableCount = linkCount;
                        
                        for (int i = 0; i < bCell->linkTableCount && i < 10; i++) {
                            (*curCell)->linkPoints[i].x = linkX[i];
                            (*curCell)->linkPoints[i].y = linkY[i];
                            (*curCell)->linkPoints[i].priority = linkPri[i];
                            (*curCell)->linkPoints[i].positionType = linkType[i];
                        }
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    
                    // Update tracking
                    prevLinkCount = linkCount;
                    for (int i = 0; i < 10; i++) {
                        prevLinkX[i] = linkX[i];
                        prevLinkY[i] = linkY[i];
                        prevLinkPri[i] = linkPri[i];
                        prevLinkType[i] = linkType[i];
                    }
                    linkHasChanges = false;
                    linkEditingStarted = false;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Done with Link Points")) {
                    linkEditingStarted = false;
                    linkHasChanges = false;
                    // Keep current values as new originals
                    originalLinkCount = linkCount;
                    for (int i = 0; i < 10; i++) {
                        originalLinkX[i] = linkX[i];
                        originalLinkY[i] = linkY[i];
                        originalLinkPri[i] = linkPri[i];
                        originalLinkType[i] = linkType[i];
                    }
                }
            }
        }
    } else {
        // Show grayed out section when link points aren't available
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
        if (ImGui::CollapsingHeader("Link Points (Not Available)")) {
            ImGui::Text("Link points are only available for:");
            ImGui::Text("- View files (.v56)");
            ImGui::Text("- Non-mirrored loops");
            ImGui::Text("- When a loop and cell are selected");
        }
        ImGui::PopStyleVar();
    }

    // =========================================================================
    // REFERENCE IMAGE SECTION WITH AUTO-APPLY
    // =========================================================================
    if (ImGui::CollapsingHeader("Reference Image")) {
        
        // Current reference image data
        static float refScaleX = 100.0f, refScaleY = 100.0f;
        static char refBitmapName[_MAX_PATH] = "";
        static int refXHot = 0, refYHot = 0;
        static int refLinkPoint = 0, refLinkPointX = 0, refLinkPointY = 0;
        static bool refPriority = false;
        
        // Original values for reset/cancel
        static float originalRefScaleX = 100.0f, originalRefScaleY = 100.0f;
        static char originalRefBitmapName[_MAX_PATH] = "";
        static int originalRefXHot = 0, originalRefYHot = 0;
        static int originalRefLinkPoint = 0, originalRefLinkPointX = 0, originalRefLinkPointY = 0;
        static bool originalRefPriority = false;
        
        // Previous values for change detection
        static float prevRefScaleX = 100.0f, prevRefScaleY = 100.0f;
        static char prevRefBitmapName[_MAX_PATH] = "";
        static int prevRefXHot = 0, prevRefYHot = 0;
        static int prevRefLinkPoint = 0, prevRefLinkPointX = 0, prevRefLinkPointY = 0;
        static bool prevRefPriority = false;
        
        static bool refNeedsRefresh = true;
        static bool refEditingStarted = false;
        static bool refHasChanges = false;
        
        // Refresh reference image data
        if (refNeedsRefresh) {
            refScaleX = originalRefScaleX = prevRefScaleX = gReferenceScaleX;
            refScaleY = originalRefScaleY = prevRefScaleY = gReferenceScaleY;
            strncpy(refBitmapName, gReferenceBM, _MAX_PATH - 1);
            strncpy(originalRefBitmapName, gReferenceBM, _MAX_PATH - 1);
            strncpy(prevRefBitmapName, gReferenceBM, _MAX_PATH - 1);
            refBitmapName[_MAX_PATH - 1] = '\0';
            originalRefBitmapName[_MAX_PATH - 1] = '\0';
            prevRefBitmapName[_MAX_PATH - 1] = '\0';
            
            refXHot = originalRefXHot = prevRefXHot = gReferenceXHot;
            refYHot = originalRefYHot = prevRefYHot = gReferenceYHot;
            refLinkPoint = originalRefLinkPoint = prevRefLinkPoint = gReferenceLinkPoint;
            refLinkPointX = originalRefLinkPointX = prevRefLinkPointX = gReferenceLinkPointX;
            refLinkPointY = originalRefLinkPointY = prevRefLinkPointY = gReferenceLinkPointY;
            refPriority = originalRefPriority = prevRefPriority = gReferencePriority;
            
            refNeedsRefresh = false;
            refEditingStarted = false;
            refHasChanges = false;
        }

        // Scale Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Scale Settings:");  // Light blue header
        
        // Limit scale input to 3 digits (like original dialog)
        ImGui::PushItemWidth(120);
        if (ImGui::InputFloat("Scale X (%)", &refScaleX, 0.0f, 0.0f, "%.1f")) refEditingStarted = true;
        if (refScaleX < 0) refScaleX = 0;
        if (refScaleX > 999) refScaleX = 999;
        
        if (ImGui::InputFloat("Scale Y (%)", &refScaleY, 0.0f, 0.0f, "%.1f")) refEditingStarted = true;
        if (refScaleY < 0) refScaleY = 0;
        if (refScaleY > 999) refScaleY = 999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Bitmap File
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Reference Bitmap:");  // Light blue header
        if (ImGui::InputText("Bitmap File", refBitmapName, _MAX_PATH)) refEditingStarted = true;
        
        ImGui::Separator();
        
        // Hot Spot Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Hot Spot:");  // Light blue header
        
        ImGui::PushItemWidth(120);
        // Limit hot spot values to 4 digits (like original dialog)
        if (ImGui::InputInt("X Hot", &refXHot)) refEditingStarted = true;
        if (refXHot < -9999) refXHot = -9999;
        if (refXHot > 9999) refXHot = 9999;
        
        if (ImGui::InputInt("Y Hot", &refYHot)) refEditingStarted = true;
        if (refYHot < -9999) refYHot = -9999;
        if (refYHot > 9999) refYHot = 9999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Link Point Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Link Point:");  // Light blue header
        
        ImGui::PushItemWidth(120);
        // Limit link point to 2 digits (like original dialog)
        if (ImGui::InputInt("Link Point Index", &refLinkPoint)) refEditingStarted = true;
        if (refLinkPoint < 0) refLinkPoint = 0;
        if (refLinkPoint > 99) refLinkPoint = 99;
        
        // Limit link point coordinates to 4 digits (like original dialog)
        if (ImGui::InputInt("Link Point X", &refLinkPointX)) refEditingStarted = true;
        if (refLinkPointX < -9999) refLinkPointX = -9999;
        if (refLinkPointX > 9999) refLinkPointX = 9999;
        
        if (ImGui::InputInt("Link Point Y", &refLinkPointY)) refEditingStarted = true;
        if (refLinkPointY < -9999) refLinkPointY = -9999;
        if (refLinkPointY > 9999) refLinkPointY = 9999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Priority Setting
        if (ImGui::Checkbox("Priority", &refPriority)) refEditingStarted = true;
        
        // Auto-apply changes when values change
        if (refEditingStarted && (
            refScaleX != prevRefScaleX || refScaleY != prevRefScaleY ||
            strcmp(refBitmapName, prevRefBitmapName) != 0 ||
            refXHot != prevRefXHot || refYHot != prevRefYHot ||
            refLinkPoint != prevRefLinkPoint || 
            refLinkPointX != prevRefLinkPointX || refLinkPointY != prevRefLinkPointY ||
            refPriority != prevRefPriority)) {
            
            // Apply changes immediately to global variables
            gReferenceScaleX = refScaleX;
            gReferenceScaleY = refScaleY;
            strncpy(gReferenceBM, refBitmapName, _MAX_PATH - 1);
            gReferenceBM[_MAX_PATH - 1] = '\0';
            gReferenceXHot = refXHot;
            gReferenceYHot = refYHot;
            gReferenceLinkPoint = refLinkPoint;
            gReferenceLinkPointX = refLinkPointX;
            gReferenceLinkPointY = refLinkPointY;
            gReferencePriority = refPriority;
            
            // Write to INI file (same as original dialog)
            char buffer[16];
            WritePrivateProfileStringA("reference", "referenceBM", (LPCSTR)gReferenceBM, gConfigIni);
            
            sprintf(buffer, "%d", gReferenceXHot);
            WritePrivateProfileStringA("reference", "referenceXHot", buffer, gConfigIni);
            
            sprintf(buffer, "%d", gReferenceYHot);
            WritePrivateProfileStringA("reference", "referenceYHot", buffer, gConfigIni);

            sprintf(buffer, "%f", gReferenceScaleX);
            WritePrivateProfileStringA("reference", "referenceScaleX", buffer, gConfigIni);

            sprintf(buffer, "%f", gReferenceScaleY);
            WritePrivateProfileStringA("reference", "referenceScaleY", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferenceLinkPoint);
            WritePrivateProfileStringA("reference", "referenceLinkPoint", buffer, gConfigIni);	

            sprintf(buffer, "%d", gReferenceLinkPointX);
            WritePrivateProfileStringA("reference", "referenceLinkPointX", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferenceLinkPointY);
            WritePrivateProfileStringA("reference", "referenceLinkPointY", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferencePriority);
            WritePrivateProfileStringA("reference", "referencePriority", buffer, gConfigIni);
            
            // Invalidate main window (same as original dialog)
            InvalidateRgn(hWnd, NULL, true);
            
            // Update tracking variables
            prevRefScaleX = refScaleX;
            prevRefScaleY = refScaleY;
            strncpy(prevRefBitmapName, refBitmapName, _MAX_PATH - 1);
            prevRefBitmapName[_MAX_PATH - 1] = '\0';
            prevRefXHot = refXHot;
            prevRefYHot = refYHot;
            prevRefLinkPoint = refLinkPoint;
            prevRefLinkPointX = refLinkPointX;
            prevRefLinkPointY = refLinkPointY;
            prevRefPriority = refPriority;
            
            // Check if we have changes from original
            refHasChanges = (refScaleX != originalRefScaleX || refScaleY != originalRefScaleY ||
                           strcmp(refBitmapName, originalRefBitmapName) != 0 ||
                           refXHot != originalRefXHot || refYHot != originalRefYHot ||
                           refLinkPoint != originalRefLinkPoint || 
                           refLinkPointX != originalRefLinkPointX || refLinkPointY != originalRefLinkPointY ||
                           refPriority != originalRefPriority);
            
            if (refHasChanges) {
                datasaved = false;
            }
        }
        
        // Reset and Cancel buttons (only show if we have changes or are editing)
        if (refEditingStarted) {
            ImGui::Separator();
            
            // Show changed indicator with colors
            if (refHasChanges) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* Reference image settings have been modified *");  // Orange
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No changes");  // Gray
            }
            
            if (refHasChanges) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.36f, 0.36f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
                if (ImGui::Button("Reset to Original")) {  // Red button
                    refScaleX = originalRefScaleX;
                    refScaleY = originalRefScaleY;
                    strncpy(refBitmapName, originalRefBitmapName, _MAX_PATH - 1);
                    refBitmapName[_MAX_PATH - 1] = '\0';
                    refXHot = originalRefXHot;
                    refYHot = originalRefYHot;
                    refLinkPoint = originalRefLinkPoint;
                    refLinkPointX = originalRefLinkPointX;
                    refLinkPointY = originalRefLinkPointY;
                    refPriority = originalRefPriority;
                    
                    // Apply the reset values
                    gReferenceScaleX = refScaleX;
                    gReferenceScaleY = refScaleY;
                    strncpy(gReferenceBM, refBitmapName, _MAX_PATH - 1);
                    gReferenceBM[_MAX_PATH - 1] = '\0';
                    gReferenceXHot = refXHot;
                    gReferenceYHot = refYHot;
                    gReferenceLinkPoint = refLinkPoint;
                    gReferenceLinkPointX = refLinkPointX;
                    gReferenceLinkPointY = refLinkPointY;
                    gReferencePriority = refPriority;
                    
                    // Write reset values to INI
                    char buffer[16];
                    WritePrivateProfileStringA("reference", "referenceBM", (LPCSTR)gReferenceBM, gConfigIni);
                    sprintf(buffer, "%d", gReferenceXHot);
                    WritePrivateProfileStringA("reference", "referenceXHot", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceYHot);
                    WritePrivateProfileStringA("reference", "referenceYHot", buffer, gConfigIni);
                    sprintf(buffer, "%f", gReferenceScaleX);
                    WritePrivateProfileStringA("reference", "referenceScaleX", buffer, gConfigIni);
                    sprintf(buffer, "%f", gReferenceScaleY);
                    WritePrivateProfileStringA("reference", "referenceScaleY", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceLinkPoint);
                    WritePrivateProfileStringA("reference", "referenceLinkPoint", buffer, gConfigIni);	
                    sprintf(buffer, "%d", gReferenceLinkPointX);
                    WritePrivateProfileStringA("reference", "referenceLinkPointX", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceLinkPointY);
                    WritePrivateProfileStringA("reference", "referenceLinkPointY", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferencePriority);
                    WritePrivateProfileStringA("reference", "referencePriority", buffer, gConfigIni);
                    
                    InvalidateRgn(hWnd, NULL, true);
                    
                    // Update tracking
                    prevRefScaleX = refScaleX;
                    prevRefScaleY = refScaleY;
                    strncpy(prevRefBitmapName, refBitmapName, _MAX_PATH - 1);
                    prevRefBitmapName[_MAX_PATH - 1] = '\0';
                    prevRefXHot = refXHot;
                    prevRefYHot = refYHot;
                    prevRefLinkPoint = refLinkPoint;
                    prevRefLinkPointX = refLinkPointX;
                    prevRefLinkPointY = refLinkPointY;
                    prevRefPriority = refPriority;
                    refHasChanges = false;
                    refEditingStarted = false;
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
            if (ImGui::Button("Done Editing")) {  // Green button
                refEditingStarted = false;
                refHasChanges = false;
                // Keep current values as new originals
                originalRefScaleX = refScaleX;
                originalRefScaleY = refScaleY;
                strncpy(originalRefBitmapName, refBitmapName, _MAX_PATH - 1);
                originalRefBitmapName[_MAX_PATH - 1] = '\0';
                originalRefXHot = refXHot;
                originalRefYHot = refYHot;
                originalRefLinkPoint = refLinkPoint;
                originalRefLinkPointX = refLinkPointX;
                originalRefLinkPointY = refLinkPointY;
                originalRefPriority = refPriority;
            }
            ImGui::PopStyleColor(3);
        }
    }
    
    } // End scrollable content area
    ImGui::EndChild();
    
    // =========================================================================
    // MAIN BUTTONS (Fixed at bottom, outside scroll area)
    // =========================================================================
    
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.72f, 0.96f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.48f, 0.64f, 1.0f));
    if (ImGui::Button("Close")) {  // Light purple button
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PROPERTIES);
    }
    ImGui::PopStyleColor(3);

    EndDialog();
}

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

bool SampleColorAtScreenPosition(int clientX, int clientY, int& colorIndex) {
    if (!g_clutGenerator || !g_clutGenerator->IsActive()) return false;
    
    // Use the same display origin calculation as the original display code
    int displayOriginX = UI_LEFT_MARGIN + picX + tableX;
    int displayOriginY = UI_TOP_MARGIN + picY;
    
    // Calculate relative position within the display area
    int relativeX = clientX - displayOriginX;
    int relativeY = clientY - displayOriginY;
    
    // Account for magnification factor (same as original display code)
    if (MagnifyFactor > 0) {
        relativeX = (relativeX * 100) / MagnifyFactor;
        relativeY = (relativeY * 100) / MagnifyFactor;
    }
    
    if (globalView && curCell && (*curCell)) {
        // For view files - sample from current view cell
        if (!(*curCell)->bmImage || !(*curCell)->bmInfo) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
            CelHeaderView* header = (CelHeaderView*)&(*curCell)->Head;
            
            // Adjust for hot spot offset (same as DisplayCurrentView function)
            int imageX = relativeX - header->xHot;
            int imageY = relativeY - header->yHot;
            
            int width = (*curCell)->bmInfo->bmiHeader.biWidth;
            int height = abs((*curCell)->bmInfo->bmiHeader.biHeight);
            
            // Check bounds
            if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                // Calculate pixel index (accounting for row padding)
                int rowWidth = ((width + 3) & ~3); // Round up to multiple of 4
                int pixelIndex = imageY * rowWidth + imageX;
                colorIndex = (*curCell)->bmImage[pixelIndex];
                return true;
            }
        }
    }
    else if (globalPicture && curCellIndex >= 0 && curCellIndex < globalPicture->CellsCount()) {
        // For picture files - we need to handle both single cell and all cells display
        if (curCellIndex == 0) {
            // When displaying all cells, we need to check each cell
            for (int i = 0; i < globalPicture->CellsCount(); i++) {
                Cell* cell = globalPicture->cells[i];
                if (!cell) continue;
                
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    // Check if click is within this cell's bounds
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        } else {
            // When displaying specific cell
            Cell* cell = globalPicture->cells[curCellIndex];
            if (cell) {
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void RenderClutGeneratorDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static std::string generatedCode = "";
    static bool showCode = false;
    static bool shouldClose = false;
    static bool magicWandWasEnabled = false;
    
    // Enhanced state for better editing - keeping original complexity but organizing better
    static int selectedRemapIndex = -1;  // Which remap is currently selected
    static int hoveredRemapIndex = -1;   // Which remap is being hovered
    static bool editingMode = false;     // Are we editing a selected remap?
    static std::string editModeStatus = "";
    static bool editingFromColor = true; // true = editing FROM, false = editing TO
    
    bool open = true;
    SetNextWindowSize(1200, 800); // Slightly larger for better spacing
    
    if (!BeginDialog("CLUT Generator - Live Preview", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open || shouldClose) {
        shouldClose = false;
        selectedRemapIndex = -1;
        editingMode = false;
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            g_clutGenerator->SetMagicWandEnabled(false);
            magicWandWasEnabled = false;
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
    
    // Auto-enable magic wand
    if (!magicWandWasEnabled && g_clutGenerator) {
        g_clutGenerator->SetMagicWandEnabled(true);
        g_clutGenerator->AnalyzeImageColorUsage();
        magicWandWasEnabled = true;
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // HEADER SECTION - Cleaner but still comprehensive
    // =========================================================================
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    
    if (ImGui::BeginChild("HeaderSection", ImVec2(0, 90), true)) {
        
        // Row 1: Main controls
        ImGui::BeginGroup();
        {
            bool remapActive = g_clutGenerator->IsPreviewEnabled();
            if (ImGui::Checkbox("Live Preview", &remapActive)) {
                g_clutGenerator->SetPreviewEnabled(remapActive);
            }
            
            ImGui::SameLine();
            if (remapActive) {
                SuccessText("* ACTIVE");
            } else {
                DisabledText("- OFF");
            }
            
            ImGui::SameLine(); 
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Analyze Image")) {
                g_clutGenerator->AnalyzeImageColorUsage();
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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
            if (ImGui::Button("Clear All")) {
                g_clutGenerator->ClearAllRemaps();
                g_clutGenerator->SetPreviewEnabled(false);
                selectedRemapIndex = -1;
                editingMode = false;
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
        
        // Row 2: Status and mode indicators - more organized
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
    // MAIN WORKSPACE - Keep 3-column layout but improve organization
    // =========================================================================
    
    if (ImGui::BeginChild("MainWorkspace", ImVec2(0, -240))) {
        
        // =====================================================================
        // LEFT COLUMN - Enhanced Color Selection (keep full functionality)
        // =====================================================================
        if (ImGui::BeginChild("LeftColumn", ImVec2(availableWidth * 0.32f, 0), true)) {
            
            HeaderText("Color Selection");
            ImGui::Separator();
            ImGui::Spacing();
            
            // Enhanced current selection display
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
            
            // Action buttons - keep original functionality
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
        // MIDDLE COLUMN - Full Palette Grid (keep all original features)
        // =====================================================================
        if (ImGui::BeginChild("MiddleColumn", ImVec2(availableWidth * 0.36f, 0), true)) {
            
            HeaderText("Palette Grid");
            ImGui::Separator();
            
            // Legend - updated for border system
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            if (usedColors.size() > 0) {
                ImGui::Text("Legend:");
                
                // Draw legend items with custom border examples
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                
                // Row 1
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
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // Selected remap (thick magenta border)
                ImVec2 selMin = currentPos;
                ImVec2 selMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(selMin, selMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(selMin, selMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Selected");
                
                // End the line and add separator
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
                                
                                // Draw colored borders for different states (after button is drawn)
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
                                
                                // Enhanced click handling for edit mode
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
                                    
                                    if (isSelectedRemapFrom) roleText += " [SELECTED FROM - Magenta Border]";
                                    if (isSelectedRemapTo) roleText += " [SELECTED TO - Cyan Border]";
                                    if (isFromColor) roleText += " [CURRENT FROM - Red Border]";
                                    if (isToColor) roleText += " [CURRENT TO - Green Border]";
                                    if (hasRemap) roleText += " [REMAPPED - Yellow Border]";
                                    if (isUsedInImage) roleText += " [USED IN IMAGE - Blue Border]";
                                    
                                    if (editingMode) {
                                        sprintf(tooltipText, 
                                            "Color %d: RGB(%d, %d, %d)%s\n"
                                            "Left = %s, Right = %s (EDIT MODE)",
                                            colorIndex, originalEntry.red, originalEntry.green, originalEntry.blue, 
                                            roleText.c_str(),
                                            editingFromColor ? "FROM" : "TO",
                                            editingFromColor ? "TO" : "FROM"
                                        );
                                    } else {
                                        sprintf(tooltipText, 
                                            "Color %d: RGB(%d, %d, %d)%s\n"
                                            "Left = FROM, Right = TO",
                                            colorIndex, originalEntry.red, originalEntry.green, originalEntry.blue, 
                                            roleText.c_str()
                                        );
                                    }
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
        // RIGHT COLUMN - Full Remap Management (keep all features)
        // =====================================================================
        if (ImGui::BeginChild("RightColumn", ImVec2(0, 0), true)) {
            
            HeaderText("Active Remaps");
            ImGui::Separator();
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            
            // Enhanced status summary
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
                // Enhanced remap table with selection
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.08f, 0.8f));
                if (ImGui::BeginChild("RemapTable", ImVec2(0, 0), true)) {
                    
                    hoveredRemapIndex = -1; // Reset hover state
                    
                    for (int i = 0; i < static_cast<int>(remaps.size()); i++) {
                        const ColorRemapEntry& remap = remaps[i];
                        
                        // Enhanced selection highlighting
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
                            }
                            ImGui::PopStyleColor(3);
                            
                            ImGui::EndGroup();
                            ImGui::PopStyleVar();
                            
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        
                        // ENTIRE ENTRY CLICK DETECTION
                        if (ImGui::IsItemClicked()) {
                            selectedRemapIndex = i;
                            g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                            g_clutGenerator->SetSelectedToColor(remap.toColor);
                            editingMode = false; // Exit edit mode when selecting a different remap
                        }
                        
                        // Track hover for palette highlighting
                        if (ImGui::IsItemHovered()) {
                            hoveredRemapIndex = i;
                            ImGui::SetTooltip("Click to select - Click FROM/TO buttons to edit - Selected remap highlights in palette");
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
    // BOTTOM SECTION - Import and Export (keep full functionality)
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
            static std::string importStatus = "";
            static bool showImportStatus = false;
            
            ImGui::Text("COLORTBL.SC Line:");
            ImGui::PushItemWidth(availableWidth - 150);
            if (ImGui::InputText("##import_text", importBuffer, sizeof(importBuffer))) {
                showImportStatus = false;
                importStatus = "";
            }
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
                        importStatus = statusMsg;
                        importBuffer[0] = '\0';
                    } else {
                        importStatus = "Failed to parse remap data. Check format.";
                    }
                    showImportStatus = true;
                } else {
                    importStatus = "Please paste a COLORTBL.SC line first";
                    showImportStatus = true;
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                importBuffer[0] = '\0';
                showImportStatus = false;
                importStatus = "";
            }
            
            ImGui::Spacing();
            
            if (showImportStatus && !importStatus.empty()) {
                if (importStatus.find("Success") != std::string::npos) {
                    SuccessText(importStatus.c_str());
                } else {
                    ErrorText(importStatus.c_str());
                }
            } else {
                DisabledText("Example: 99 0 100 38 101 0 -1 -1 -1 -1 ... ; black wolf");
            }
            
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
    
    EndDialog();
}

// Enhanced RenderRealmpalDialog with Image Preview and Live Palette Grid
// Add these static variables and helper functions before RenderRealmpalDialog()

// Preview system state
static HBITMAP g_previewBitmap = nullptr;
static BITMAPINFO* g_previewBmpInfo = nullptr;
static unsigned char* g_previewImageData = nullptr;
static int g_previewWidth = 0;
static int g_previewHeight = 0;
static bool g_previewValid = false;
static RGBQUAD g_previewPalette[256];
static bool g_palettePreviewValid = false;
static std::string g_lastPreviewError = "";
static DWORD g_lastPreviewTime = 0;

// Helper function to clean up preview resources
void CleanupPreviewResources() {
    // Only keep palette preview cleanup
    g_palettePreviewValid = false;
}

// Function to update palette preview based on current settings
bool UpdatePalettePreview(const RealmpalConfig& config) {
    // Clear previous palette
    memset(g_previewPalette, 0, sizeof(g_previewPalette));
    g_palettePreviewValid = false;
    
    if (g_realmpalInputFile.empty()) {
        return false;
    }
    
    // Create temporary BMP file for conversion
    char tempFile[MAX_PATH];
    GetTempPath(MAX_PATH, tempFile);
    sprintf(tempFile, "%s\\realmpal_palette_temp_%d.bmp", tempFile, GetTickCount());
    
    // Setup config for conversion
    RealmpalConfig paletteConfig = config;
    paletteConfig.input_file = g_realmpalInputFile.c_str();
    paletteConfig.output_file = tempFile;
    paletteConfig.palette_file = g_realmpalPaletteFile.empty() ? nullptr : g_realmpalPaletteFile.c_str();
    paletteConfig.extra_palette_file = g_realmpalExtraFile.empty() ? nullptr : g_realmpalExtraFile.c_str();
    
    // Perform conversion
    int result = realmpal_convert_image(&paletteConfig);
    
    if (result == REALMPAL_SUCCESS) {
        // Read palette from the generated BMP
        FILE* file = fopen(tempFile, "rb");
        if (file) {
            BITMAPFILEHEADER fileHeader;
            BITMAPINFOHEADER infoHeader;
            
            // Read headers
            if (fread(&fileHeader, sizeof(fileHeader), 1, file) == 1 &&
                fread(&infoHeader, sizeof(infoHeader), 1, file) == 1) {
                
                // Validate BMP format
                if (fileHeader.bfType == 'MB' && infoHeader.biBitCount == 8) {
                    // Read palette
                    if (fread(g_previewPalette, sizeof(RGBQUAD), 256, file) == 256) {
                        g_palettePreviewValid = true;
                    }
                }
            }
            fclose(file);
        }
        DeleteFile(tempFile);
    }
    
    return g_palettePreviewValid;
}

// Function to render palette preview grid with optional constraint editing
void RenderPalettePreview(const RealmpalConfig& config, bool visualEditMode = false, 
                         std::vector<bool>* selectedIndices = nullptr, 
                         bool* isDragging = nullptr, int* dragStart = nullptr) {
    using namespace FotoSCIhopStyles;
    
    HeaderText("Palette Preview");
    if (visualEditMode) {
        ImGui::SameLine();
        WarningText("(EDIT MODE)");
    }
    ImGui::Separator();
    ImGui::Spacing();
    
    // Palette controls
    ImGui::BeginGroup();
    {
        if (ImGui::Button("Update Palette", ImVec2(120, 0))) {
            UpdatePalettePreview(config);
        }
        
        ImGui::SameLine();
        char paletteStatus[64];
        if (g_palettePreviewValid) {
            sprintf(paletteStatus, "✓ 256 colors");
            SuccessText(paletteStatus);
        } else {
            sprintf(paletteStatus, "⚠ No palette");
            WarningText(paletteStatus);
        }
    }
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Palette grid display
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.9f));
    if (ImGui::BeginChild("PalettePreviewGrid", ImVec2(0, 0), true)) {
        
        if (g_palettePreviewValid) {
            // Show legend
            if (visualEditMode) {
                InfoText("Legend: Yellow = selected, Red = transparency, Click/drag to select");
            } else if (config.index_constraints.enabled && config.index_constraints.count > 0) {
                InfoText("Legend: Green border = allowed by constraints, Dimmed = blocked");
            }
            ImGui::Spacing();
            
            // Draw 16x16 palette grid
            const int COLORS_PER_ROW = 16;
            const float BUTTON_SIZE = 18.0f;
            const float SPACING_VAL = 2.0f;
            const float CORNER_ROUNDING = 3.0f;
            
            ImGuiIO& io = ImGui::GetIO();
            bool ctrlPressed = io.KeyCtrl;
            
            for (int row = 0; row < 16; row++) {
                for (int col = 0; col < 16; col++) {
                    int colorIndex = row * COLORS_PER_ROW + col;
                    
                    char buttonId[16];
                    sprintf(buttonId, "##pal%d", colorIndex);
                    
                    // Determine color state
                    bool isTransparencyIndex = (colorIndex == config.transparency_index);
                    bool isSelected = visualEditMode && selectedIndices && (*selectedIndices)[colorIndex];
                    bool isConstraintAllowed = !visualEditMode && realmpal_index_allowed_by_constraints(
                        colorIndex, &config.index_constraints, config.transparency_index);
                    
                    // Get color for display
                    RGBQUAD color;
                    if (g_palettePreviewValid) {
                        color = g_previewPalette[colorIndex];
                    } else {
                        color = {128, 128, 128, 255}; // Fallback gray color
                    }
                    
                    float r = color.rgbRed / 255.0f;
                    float g = color.rgbGreen / 255.0f;
                    float b = color.rgbBlue / 255.0f;
                    
                    // Apply mode-specific color modifications
                    if (visualEditMode) {
                        // Brighten selected colors
                        if (isSelected) {
                            r = fmin(r + 0.3f, 1.0f);
                            g = fmin(g + 0.3f, 1.0f);
                            b = fmin(b + 0.3f, 1.0f);
                        }
                    } else {
                        // Dim colors that aren't allowed by constraints
                        if (!isConstraintAllowed && config.index_constraints.count > 0) {
                            r *= 0.4f; g *= 0.4f; b *= 0.4f;
                        }
                    }
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
                        fmin(r * 1.3f, 1.0f), fmin(g * 1.3f, 1.0f), fmin(b * 1.3f, 1.0f), 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.7f, g * 0.7f, b * 0.7f, 1.0f));
                    
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, CORNER_ROUNDING);
                    
                    ImVec2 buttonPos = ImGui::GetCursorScreenPos();
                    bool buttonClicked = ImGui::Button(buttonId, ImVec2(BUTTON_SIZE, BUTTON_SIZE));
                    
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(3);
                    
                    // Handle selection in edit mode
                    if (visualEditMode && selectedIndices && isDragging && dragStart) {
                        if (buttonClicked) {
                            if (ctrlPressed) {
                                // Toggle individual selection
                                (*selectedIndices)[colorIndex] = !(*selectedIndices)[colorIndex];
                            } else {
                                // Start new selection
                                if (!(*isDragging)) {
                                    std::fill(selectedIndices->begin(), selectedIndices->end(), false);
                                    (*selectedIndices)[colorIndex] = true;
                                }
                            }
                        }
                        
                        // Handle drag selection
                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                            if (!(*isDragging)) {
                                *isDragging = true;
                                *dragStart = colorIndex;
                                if (!ctrlPressed) {
                                    std::fill(selectedIndices->begin(), selectedIndices->end(), false);
                                }
                            }
                            
                            // Select range from dragStart to current
                            int start = min(*dragStart, colorIndex);
                            int end = max(*dragStart, colorIndex);
                            for (int i = start; i <= end; i++) {
                                (*selectedIndices)[i] = true;
                            }
                        }
                        
                        if (ImGui::IsMouseReleased(0)) {
                            *isDragging = false;
                        }
                    }
                    
                    // Draw borders
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 buttonMin = buttonPos;
                    ImVec2 buttonMax = ImVec2(buttonPos.x + BUTTON_SIZE, buttonPos.y + BUTTON_SIZE);
                    
                    if (visualEditMode) {
                        if (isSelected) {
                            // Yellow border for selected
                            drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 0, 255), CORNER_ROUNDING, 0, 2.0f);
                        } else if (isTransparencyIndex) {
                            // Red border for transparency
                            drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 0, 0, 180), CORNER_ROUNDING, 0, 1.0f);
                        }
                    } else {
                        if (isTransparencyIndex) {
                            // Yellow border for transparency index
                            drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 0, 255), CORNER_ROUNDING, 0, 2.0f);
                        } else if (isConstraintAllowed && config.index_constraints.count > 0) {
                            // Green border for constraint-allowed indices
                            drawList->AddRect(buttonMin, buttonMax, IM_COL32(0, 255, 0, 180), CORNER_ROUNDING, 0, 1.5f);
                        }
                    }
                    
                    // Tooltip
                    if (ImGui::IsItemHovered()) {
                        char tooltip[256];
                        sprintf(tooltip, "Index %d\nRGB(%d, %d, %d)", 
                               colorIndex, color.rgbRed, color.rgbGreen, color.rgbBlue);
                        
                        if (visualEditMode) {
                            if (isSelected) strcat(tooltip, "\n[SELECTED]");
                            if (isTransparencyIndex) strcat(tooltip, "\n[TRANSPARENCY]");
                            strcat(tooltip, "\nClick = select, Ctrl+click = multi-select, Drag = range");
                        } else {
                            if (isTransparencyIndex) {
                                strcat(tooltip, "\n[TRANSPARENCY]");
                            } else if (config.index_constraints.count > 0) {
                                strcat(tooltip, isConstraintAllowed ? "\n[ALLOWED BY CONSTRAINTS]" : "\n[BLOCKED BY CONSTRAINTS]");
                            }
                        }
                        
                        ImGui::SetTooltip("%s", tooltip);
                    }
                    
                    if (col < 15) {
                        ImGui::SameLine(0, SPACING_VAL);
                    }
                }
            }
            
            // Show range information
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            char rangeInfo[256];
            sprintf(rangeInfo, "Base: %d-%d (%d colors)", 
                   config.index_offset,
                   config.index_offset + config.num_colors - 1,
                   config.num_colors);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
            ImGui::Text("%s", rangeInfo);
            
            if (config.extra_offset >= 0) {
                sprintf(rangeInfo, "Inject: %d-%d (%d colors)", 
                       config.extra_offset,
                       config.extra_offset + config.extra_colors - 1,
                       config.extra_colors);
                ImGui::Text("%s", rangeInfo);
            }
            
            if (config.transparency_index >= 0) {
                sprintf(rangeInfo, "Transparent: %d", config.transparency_index);
                ImGui::Text("%s", rangeInfo);
            }
            
            if (config.index_constraints.count > 0) {
                int totalConstrainedIndices = realmpal_count_constraint_indices(&config.index_constraints, config.transparency_index);
                sprintf(rangeInfo, "Constraint: %d indices allowed", totalConstrainedIndices);
                ImGui::Text("%s", rangeInfo);
            }
            
            ImGui::PopStyleColor();
            
        } else {
            // Show instructions
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            const char* instructions = "Click 'Update Palette' to\ngenerate palette preview\nbased on current settings";
            ImVec2 textSize = ImGui::CalcTextSize(instructions);
            ImGui::SetCursorPos(ImVec2(
                (contentSize.x - textSize.x) * 0.5f,
                (contentSize.y - textSize.y) * 0.5f
            ));
            DisabledText(instructions);
        }
        
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void RenderRealmpalDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static RealmpalConfig config;
    static bool configInitialized = false;
    static std::string conversionStatus = "";
    static bool showStatus = false;
    static bool isConverting = false;

    static bool visualEditMode = false;
    static std::vector<bool> selectedIndices(256, false);
    static std::vector<std::pair<int, int>> selectedRanges;
    static bool isDragging = false;
    static int dragStart = -1;
    static std::string statusMessage = "";
    static bool showStatusMessage = false;
    
    // Preset system
    static int selectedPreset = 0;
    static const char* presetNames[] = { 
        "Custom Configuration", 
        "Auto Mode", 
        "Hybrid Mode", 
        "Palette Mode", 
        "P56 Neutral Mode",
        "Icon Mode",
        "Full Screen Mode",
        "Sprite Mode"
    };
    
    // Initialize config on first run
    if (!configInitialized) {
        realmpal_config_init(&config);
        config.num_colors = 256;
        config.dither = REALMPAL_DITHER_FS_SERP;
        config.fs_strength = 0.8;
        config.transparency_index = 255;
        config.alpha_threshold = 128;
        config.ordered_matrix_size = 4;
        configInitialized = true;
    }
    
    bool open = true;
    SetNextWindowSize(1000, 750); // Increased height to prevent clipping
    
    if (!BeginDialog("Realmpal Import Tool", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open) {
        CleanupPreviewResources();
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_REALMPAL);
        EndDialog();
        return;
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // MAIN CONTENT AREA - Two Column Layout
    // =========================================================================
    
    if (ImGui::BeginChild("MainContent", ImVec2(0, -140))) { // Increased reserved space for bottom section
        
        // Left Column: Settings (50% width)
        if (ImGui::BeginChild("SettingsColumn", ImVec2(availableWidth * 0.50f, 0), true)) {
            
            // =====================================================================
            // PRESET SELECTION
            // =====================================================================
            HeaderText("Conversion Mode");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushItemWidth(-20); // Narrower input width
            if (ImGui::Combo("##preset_combo", &selectedPreset, presetNames, 8)) {
                // Auto-apply the selected preset
                switch (selectedPreset) {
                    case 1: // Auto Mode
                        config.mode = REALMPAL_MODE_AUTO;
                        config.num_colors = 256;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.quantizer = REALMPAL_QUANT_WU;
                        config.fs_strength = 0.8;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    case 2: // Hybrid Mode
                        config.mode = REALMPAL_MODE_AUTO;
                        config.num_colors = 108;
                        config.index_offset = 128;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.quantizer = REALMPAL_QUANT_WU;
                        config.fs_strength = 0.8;
                        config.extra_offset = 0;
                        config.extra_colors = 128;
                        break;
                        
                    case 3: // Palette Mode
                        config.mode = REALMPAL_MODE_PALETTE;
                        config.num_colors = 256;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.fs_strength = 0.8;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    case 4: // P56 Neutral Mode
                        config.mode = REALMPAL_MODE_PALETTE;
                        config.num_colors = 128;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.fs_strength = 0.8;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    case 5: // Icon Mode
                        config.mode = REALMPAL_MODE_AUTO;
                        config.num_colors = 64;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.quantizer = REALMPAL_QUANT_WU;
                        config.fs_strength = 1.0;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    case 6: // Full Screen Mode
                        config.mode = REALMPAL_MODE_AUTO;
                        config.num_colors = 256;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.quantizer = REALMPAL_QUANT_WU;
                        config.fs_strength = 0.6;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    case 7: // Sprite Mode
                        config.mode = REALMPAL_MODE_AUTO;
                        config.num_colors = 128;
                        config.index_offset = 0;
                        config.dither = REALMPAL_DITHER_FS_SERP;
                        config.quantizer = REALMPAL_QUANT_WU;
                        config.fs_strength = 0.9;
                        config.extra_offset = -1;
                        config.extra_colors = -1;
                        break;
                        
                    default: // Custom - don't change anything
                        break;
                }
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            
            // =====================================================================
            // FILE SELECTION
            // =====================================================================
            if (ImGui::CollapsingHeader("Input Files", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Input file
                ImGui::Text("Source Image:");
                static char inputDisplay[MAX_PATH] = "No file selected";
                if (!g_realmpalInputFile.empty()) {
                    const char* fileName = strrchr(g_realmpalInputFile.c_str(), '\\');
                    if (fileName) {
                        strncpy(inputDisplay, fileName + 1, MAX_PATH - 1);
                    } else {
                        strncpy(inputDisplay, g_realmpalInputFile.c_str(), MAX_PATH - 1);
                    }
                    inputDisplay[MAX_PATH - 1] = '\0';
                } else {
                    strcpy(inputDisplay, "No file selected");
                }
                
                ImGui::PushItemWidth(-20); // Narrower input width
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.2f, 0.8f));
                ImGui::InputText("##input_display", inputDisplay, MAX_PATH, ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();
                
                if (ImGui::Button("Browse...", ImVec2(-1, 0))) {
                    g_requestInputDialog = true;
                }
                
                ImGui::Spacing();
                
                // Base palette file
                ImGui::Text("Base Palette:");
                static char paletteDisplay[MAX_PATH] = "No palette selected";
                if (!g_realmpalPaletteFile.empty()) {
                    const char* fileName = strrchr(g_realmpalPaletteFile.c_str(), '\\');
                    if (fileName) {
                        strncpy(paletteDisplay, fileName + 1, MAX_PATH - 1);
                    } else {
                        strncpy(paletteDisplay, g_realmpalPaletteFile.c_str(), MAX_PATH - 1);
                    }
                    paletteDisplay[MAX_PATH - 1] = '\0';
                } else {
                    strcpy(paletteDisplay, "No palette selected");
                }
                
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.2f, 0.8f));
                ImGui::InputText("##palette_display", paletteDisplay, MAX_PATH, ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();
                
                if (ImGui::Button("Browse Palette##palette", ImVec2(-1, 0))) {
                    g_requestPaletteDialog = true;
                }
                
                ImGui::Spacing();
                
                // Extra palette file
                ImGui::Text("Extra Palette:");
                static char extraDisplay[MAX_PATH] = "No extra palette";
                if (!g_realmpalExtraFile.empty()) {
                    const char* fileName = strrchr(g_realmpalExtraFile.c_str(), '\\');
                    if (fileName) {
                        strncpy(extraDisplay, fileName + 1, MAX_PATH - 1);
                    } else {
                        strncpy(extraDisplay, g_realmpalExtraFile.c_str(), MAX_PATH - 1);
                    }
                    extraDisplay[MAX_PATH - 1] = '\0';
                } else {
                    strcpy(extraDisplay, "No extra palette");
                }
                
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.2f, 0.8f));
                ImGui::InputText("##extra_display", extraDisplay, MAX_PATH, ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();
                
                if (ImGui::Button("Browse Extra##extra", ImVec2(-1, 0))) {
                    g_requestExtraDialog = true;
                }
                
                ImGui::PopItemWidth();
            }
            
            ImGui::Spacing();
            
            // =====================================================================
            // QUANTIZATION SETTINGS
            // =====================================================================
            if (ImGui::CollapsingHeader("Quantization", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                ImGui::Text("Mode:");
                int modeIndex = (config.mode == REALMPAL_MODE_AUTO) ? 0 : 1;
                const char* modeItems[] = { "AUTO", "PALETTE" };
                ImGui::PushItemWidth(-20);
                if (ImGui::Combo("##mode", &modeIndex, modeItems, 2)) {
                    config.mode = (modeIndex == 0) ? REALMPAL_MODE_AUTO : REALMPAL_MODE_PALETTE;
                    selectedPreset = 0;
                }
                
                ImGui::Text("Colors:");
                int tempColors = config.num_colors;
                if (ImGui::InputInt("##colors", &tempColors)) {
                    config.num_colors = realmpal_clamp_int(tempColors, 1, 256);
                    selectedPreset = 0;
                }
                
                ImGui::Text("Start Index:");
                int tempOffset = config.index_offset;
                if (ImGui::InputInt("##start_index", &tempOffset)) {
                    config.index_offset = realmpal_clamp_int(tempOffset, 0, 255);
                    selectedPreset = 0;
                }
                
                if (config.mode == REALMPAL_MODE_AUTO) {
                    ImGui::Text("Algorithm:");
                    int quantIndex = (config.quantizer == REALMPAL_QUANT_WU) ? 0 : 1;
                    const char* quantItems[] = { "Wu", "Median" };
                    if (ImGui::Combo("##quantizer", &quantIndex, quantItems, 2)) {
                        config.quantizer = (quantIndex == 0) ? REALMPAL_QUANT_WU : REALMPAL_QUANT_MEDIAN;
                        selectedPreset = 0;
                    }
                }
                
                ImGui::PopItemWidth();
            }
            
            ImGui::Spacing();

            // =====================================================================
            // ENHANCED INDEX MAPPING CONSTRAINTS SECTION (VISUAL SELECTION)
            // =====================================================================
            if (ImGui::CollapsingHeader("Index Mapping Constraints")) {
                
                // Enable/disable checkbox
                bool tempEnforce = (config.index_constraints.enabled != 0);
                if (ImGui::Checkbox("Enable Multi-Range Index Constraints", &tempEnforce)) {
                    config.index_constraints.enabled = tempEnforce ? 1 : 0;
                    selectedPreset = 0;
                    statusMessage = tempEnforce ? "Constraints enabled" : "Constraints disabled";
                    showStatusMessage = true;
                    
                    // Clear visual selection when disabling
                    if (!tempEnforce) {
                        visualEditMode = false;
                        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                        selectedRanges.clear();
                    }
                }
                
                ImGui::Spacing();
                
                if (config.index_constraints.enabled) {
                    
                    // Mode toggle
                    HeaderText("Visual Constraint Editor:");
                    
                    if (ImGui::RadioButton("View Current Constraints", !visualEditMode)) {
                        visualEditMode = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Edit Constraints Visually", visualEditMode)) {
                        visualEditMode = true;
                        // Sync current constraints to selection when entering edit mode
                        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                        selectedRanges.clear();
                        for (int i = 0; i < config.index_constraints.count; i++) {
                            int start = config.index_constraints.ranges[i].start;
                            int end = config.index_constraints.ranges[i].end;
                            for (int idx = start; idx <= end && idx < 256; idx++) {
                                selectedIndices[idx] = true;
                            }
                        }
                        statusMessage = "Visual edit mode activated - select ranges on palette grid below";
                        showStatusMessage = true;
                    }
                    
                    ImGui::Spacing();
                    
                    if (visualEditMode) {
                        // VISUAL EDITING MODE
                        WarningText("EDIT MODE: Click and drag on the palette grid below to select constraint ranges");
                        
                        ImGui::Spacing();
                        
                        // Update selectedRanges from selectedIndices
                        selectedRanges.clear();
                        for (int i = 0; i < 256; i++) {
                            if (selectedIndices[i]) {
                                int start = i;
                                int end = i;
                                while (end + 1 < 256 && selectedIndices[end + 1]) {
                                    end++;
                                }
                                selectedRanges.push_back({start, end});
                                i = end;
                            }
                        }
                        
                        // Show selected ranges
                        if (!selectedRanges.empty()) {
                            char summaryText[128];
                            int totalIndices = 0;
                            for (const auto& range : selectedRanges) {
                                totalIndices += range.second - range.first + 1;
                            }
                            sprintf(summaryText, "Selected: %d ranges (%d total indices)", (int)selectedRanges.size(), totalIndices);
                            SuccessText(summaryText);
                            
                            ImGui::Spacing();
                            
                            // Range list with remove buttons
                            HeaderText("Selected Ranges:");
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.8f));
                            if (ImGui::BeginChild("SelectedRanges", ImVec2(0, 120), true)) {
                                for (int i = 0; i < selectedRanges.size(); i++) {
                                    int start = selectedRanges[i].first;
                                    int end = selectedRanges[i].second;
                                    
                                    char rangeText[64];
                                    if (start == end) {
                                        sprintf(rangeText, "Index %d", start);
                                    } else {
                                        sprintf(rangeText, "Range %d-%d (%d indices)", start, end, end - start + 1);
                                    }
                                    
                                    ImGui::BulletText("%s", rangeText);
                                    ImGui::SameLine();
                                    
                                    char removeId[32];
                                    sprintf(removeId, "Remove##%d", i);
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.7f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.4f, 0.4f, 0.8f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 0.9f));
                                    
                                    if (ImGui::Button(removeId, ImVec2(60, 0))) {
                                        // Remove this range from selection
                                        for (int idx = start; idx <= end; idx++) {
                                            selectedIndices[idx] = false;
                                        }
                                    }
                                    ImGui::PopStyleColor(3);
                                }
                            }
                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                        } else {
                            DisabledText("No ranges selected - click and drag on the palette grid below");
                        }
                        
                        ImGui::Spacing();
                        
                        // Action buttons
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
                        if (ImGui::Button("Apply Constraints", ImVec2(140, 0))) {
                            // Convert selection to constraints
                            config.index_constraints.count = 0;
                            for (const auto& range : selectedRanges) {
                                if (config.index_constraints.count < 32) {
                                    config.index_constraints.ranges[config.index_constraints.count].start = range.first;
                                    config.index_constraints.ranges[config.index_constraints.count].end = range.second;
                                    config.index_constraints.count++;
                                }
                            }
                            
                            statusMessage = "Constraints applied successfully!";
                            showStatusMessage = true;
                            visualEditMode = false;
                            selectedPreset = 0;
                        }
                        ImGui::PopStyleColor(3);
                        
                        ImGui::SameLine();
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
                        if (ImGui::Button("Clear Selection", ImVec2(120, 0))) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            selectedRanges.clear();
                            statusMessage = "Selection cleared";
                            showStatusMessage = true;
                        }
                        ImGui::PopStyleColor(3);
                        
                        ImGui::SameLine();
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.72f, 0.96f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.48f, 0.64f, 1.0f));
                        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
                            visualEditMode = false;
                            // Reset selection to current constraints
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            selectedRanges.clear();
                            for (int i = 0; i < config.index_constraints.count; i++) {
                                int start = config.index_constraints.ranges[i].start;
                                int end = config.index_constraints.ranges[i].end;
                                for (int idx = start; idx <= end && idx < 256; idx++) {
                                    selectedIndices[idx] = true;
                                }
                            }
                            statusMessage = "Edit cancelled";
                            showStatusMessage = true;
                        }
                        ImGui::PopStyleColor(3);
                        
                    } else {
                        // VIEW MODE - Show current constraints
                        InfoText("Current constraints are applied to palette mapping below");
                        
                        if (config.index_constraints.count > 0) {
                            char summaryText[256];
                            int totalIndices = realmpal_count_constraint_indices(&config.index_constraints, config.transparency_index);
                            sprintf(summaryText, "%d ranges/indices defined, %d total available for mapping", 
                                config.index_constraints.count, totalIndices);
                            SuccessText(summaryText);
                            
                            ImGui::Spacing();
                            
                            // Current constraints list
                            if (ImGui::TreeNode("View Current Constraints")) {
                                for (int i = 0; i < config.index_constraints.count; i++) {
                                    char rangeText[64];
                                    if (config.index_constraints.ranges[i].start == config.index_constraints.ranges[i].end) {
                                        sprintf(rangeText, "Index %d", config.index_constraints.ranges[i].start);
                                    } else {
                                        sprintf(rangeText, "Range %d-%d (%d indices)", 
                                            config.index_constraints.ranges[i].start,
                                            config.index_constraints.ranges[i].end,
                                            config.index_constraints.ranges[i].end - config.index_constraints.ranges[i].start + 1);
                                    }
                                    ImGui::BulletText("%s", rangeText);
                                }
                                
                                ImGui::Spacing();
                                
                                if (config.transparency_index >= 0) {
                                    char transparencyNote[128];
                                    sprintf(transparencyNote, "Note: Transparency index %d is excluded from mapping", config.transparency_index);
                                    InfoText(transparencyNote);
                                }
                                
                                ImGui::TreePop();
                            }
                        } else {
                            DisabledText("No constraints defined - all indices available for mapping");
                        }
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Quick preset buttons (available in both modes)
                    HeaderText("Quick Constraint Presets:");
                    
                    if (ImGui::Button("SCI Upper Half (128-254)", ImVec2(-1, 0))) {
                        // Apply preset
                        config.index_constraints.count = 1;
                        config.index_constraints.ranges[0].start = 128;
                        config.index_constraints.ranges[0].end = 254;
                        selectedPreset = 0;
                        statusMessage = "Applied SCI Upper Half preset";
                        showStatusMessage = true;
                        
                        // Update visual selection if in edit mode
                        if (visualEditMode) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            for (int i = 128; i <= 254; i++) {
                                selectedIndices[i] = true;
                            }
                        }
                    }
                    
                    if (ImGui::Button("SCI Lower Half (0-127)", ImVec2(-1, 0))) {
                        config.index_constraints.count = 1;
                        config.index_constraints.ranges[0].start = 0;
                        config.index_constraints.ranges[0].end = 127;
                        selectedPreset = 0;
                        statusMessage = "Applied SCI Lower Half preset";
                        showStatusMessage = true;
                        
                        if (visualEditMode) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            for (int i = 0; i <= 127; i++) {
                                selectedIndices[i] = true;
                            }
                        }
                    }
                    
                    if (ImGui::Button("SCI Safe Range (16-239)", ImVec2(-1, 0))) {
                        config.index_constraints.count = 1;
                        config.index_constraints.ranges[0].start = 16;
                        config.index_constraints.ranges[0].end = 239;
                        selectedPreset = 0;
                        statusMessage = "Applied SCI Safe Range preset";
                        showStatusMessage = true;
                        
                        if (visualEditMode) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            for (int i = 16; i <= 239; i++) {
                                selectedIndices[i] = true;
                            }
                        }
                    }
                    
                    if (ImGui::Button("SCI Hybrid (0-63, 128-191)", ImVec2(-1, 0))) {
                        config.index_constraints.count = 2;
                        config.index_constraints.ranges[0].start = 0;
                        config.index_constraints.ranges[0].end = 63;
                        config.index_constraints.ranges[1].start = 128;
                        config.index_constraints.ranges[1].end = 191;
                        selectedPreset = 0;
                        statusMessage = "Applied SCI Hybrid preset";
                        showStatusMessage = true;
                        
                        if (visualEditMode) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            for (int i = 0; i <= 63; i++) {
                                selectedIndices[i] = true;
                            }
                            for (int i = 128; i <= 191; i++) {
                                selectedIndices[i] = true;
                            }
                        }
                    }
                    
                    if (ImGui::Button("Clear All Constraints", ImVec2(-1, 0))) {
                        config.index_constraints.count = 0;
                        memset(&config.index_constraints.ranges, 0, sizeof(config.index_constraints.ranges));
                        selectedPreset = 0;
                        statusMessage = "All constraints cleared";
                        showStatusMessage = true;
                        
                        if (visualEditMode) {
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            selectedRanges.clear();
                        }
                    }
                    
                } else {
                    // Show disabled state
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Text("Multi-range constraints disabled");
                    ImGui::Spacing();
                    ImGui::TextWrapped("When disabled, all indices (0-255) are available for mapping except the transparency index.");
                    ImGui::Spacing();
                    ImGui::TextWrapped("Enable the checkbox above to use visual constraint editing with the palette grid.");
                    ImGui::PopStyleVar();
                }
                
                // Status message display
                if (showStatusMessage && !statusMessage.empty()) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    if (statusMessage.find("success") != std::string::npos || statusMessage.find("Applied") != std::string::npos) {
                        SuccessText(statusMessage.c_str());
                    } else if (statusMessage.find("cleared") != std::string::npos || statusMessage.find("cancelled") != std::string::npos) {
                        WarningText(statusMessage.c_str());
                    } else {
                        InfoText(statusMessage.c_str());
                    }
                    
                    // Auto-hide status after showing for a while
                    static int statusCounter = 0;
                    statusCounter++;
                    if (statusCounter > 120) { // ~2 seconds at 60fps
                        showStatusMessage = false;
                        statusMessage = "";
                        statusCounter = 0;
                    }
                }
            }
            
            // =====================================================================
            // TRANSPARENCY SETTINGS
            // =====================================================================
            if (ImGui::CollapsingHeader("Transparency")) {
                
                ImGui::Text("Index:");
                ImGui::PushItemWidth(-20);
                if (ImGui::InputInt("##trans_index", &config.transparency_index)) {
                    config.transparency_index = realmpal_clamp_int(config.transparency_index, -1, 255);
                    selectedPreset = 0;
                }
                
                if (config.transparency_index >= 0) {
                    ImGui::Text("Threshold:");
                    if (ImGui::InputInt("##alpha_thresh", &config.alpha_threshold)) {
                        config.alpha_threshold = realmpal_clamp_int(config.alpha_threshold, 0, 255);
                        selectedPreset = 0;
                    }
                    
                    ImGui::Spacing();
                    
                    // Alpha color settings
                    ImGui::Checkbox("Use Alpha Color", &config.use_alpha_color);
                    if (config.use_alpha_color) {
                        ImGui::Text("Alpha Color (RGB):");
                        int tempR = config.alpha_color.r;
                        int tempG = config.alpha_color.g;
                        int tempB = config.alpha_color.b;
                        
                        if (ImGui::InputInt("R##alpha_r", &tempR)) {
                            config.alpha_color.r = realmpal_clamp_int(tempR, 0, 255);
                            selectedPreset = 0;
                        }
                        if (ImGui::InputInt("G##alpha_g", &tempG)) {
                            config.alpha_color.g = realmpal_clamp_int(tempG, 0, 255);
                            selectedPreset = 0;
                        }
                        if (ImGui::InputInt("B##alpha_b", &tempB)) {
                            config.alpha_color.b = realmpal_clamp_int(tempB, 0, 255);
                            selectedPreset = 0;
                        }
                    }
                }
                
                ImGui::Spacing();
                
                // Quick preset buttons
                if (ImGui::Button("SCI (255)", ImVec2(-1, 0))) {
                    config.transparency_index = 255;
                    config.alpha_threshold = 128;
                    config.use_alpha_color = true;
                    config.alpha_color = {255, 0, 255};
                    selectedPreset = 0;
                }
                
                if (ImGui::Button("None", ImVec2(-1, 0))) {
                    config.transparency_index = -1;
                    config.use_alpha_color = false;
                    selectedPreset = 0;
                }
                
                ImGui::PopItemWidth();
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right Column: Palette Preview (50% width - narrower than before)
        if (ImGui::BeginChild("PaletteColumn", ImVec2(0, 0), true)) {
            RenderPalettePreview(config, visualEditMode, &selectedIndices, &isDragging, &dragStart);
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();
    
    // =========================================================================
    // BOTTOM ACTION SECTION
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Status and requirements check
    bool canImport = (curCell && (*curCell)) && !g_realmpalInputFile.empty();
    bool needsPalette = (config.mode == REALMPAL_MODE_PALETTE) || (selectedPreset == 2 || selectedPreset == 3 || selectedPreset == 4);
    bool paletteOk = !needsPalette || !g_realmpalPaletteFile.empty();
    bool needsExtra = (selectedPreset == 2) && (config.extra_offset >= 0);
    bool extraOk = !needsExtra || !g_realmpalExtraFile.empty();
    
    canImport = canImport && paletteOk && extraOk;
    
    // Status display
    ImGui::BeginGroup();
    {
        ImGui::Text("Status:");
        ImGui::SameLine();
        
        if (!g_realmpalInputFile.empty()) {
            SuccessText("Input Ready");
        } else {
            ErrorText("Need Input File");
        }
        
        ImGui::SameLine(); ImGui::Text(" | ");
        
        if (needsPalette) {
            if (!g_realmpalPaletteFile.empty()) {
                SuccessText("Palette Ready");
            } else {
                ErrorText("Need Palette File");
            }
        } else {
            DisabledText("Palette Optional");
        }
        
        ImGui::SameLine(); ImGui::Text(" | ");
        
        if (curCell && (*curCell)) {
            SuccessText("Target Cell Ready");
        } else {
            ErrorText("No Target Cell");
        }
    }
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Main action buttons
    ImGui::Columns(2, "ActionButtons", false);
    ImGui::SetColumnWidth(0, availableWidth * 0.50f);
    
    // Convert & Import button
    if (isConverting) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Text("Converting and importing...");
        ImGui::PopStyleColor();
    } else {
        if (canImport) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.7f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 0.6f));
        }
        
        if (ImGui::Button("CONVERT & IMPORT", ImVec2(-1, 50)) && canImport) {
            isConverting = true;
            
            // Create temporary BMP file
            char tempPath[MAX_PATH];
            GetTempPath(MAX_PATH, tempPath);
            char tempFile[MAX_PATH];
            sprintf(tempFile, "%s\\realmpal_temp_%d.bmp", tempPath, GetTickCount());
            
            // Setup config for conversion
            config.input_file = g_realmpalInputFile.c_str();
            config.output_file = tempFile;
            config.palette_file = g_realmpalPaletteFile.empty() ? nullptr : g_realmpalPaletteFile.c_str();
            config.extra_palette_file = g_realmpalExtraFile.empty() ? nullptr : g_realmpalExtraFile.c_str();
            
            // Perform conversion
            int result = realmpal_convert_image(&config);
            isConverting = false;
            
            if (result == REALMPAL_SUCCESS) {
                // Check if temp file exists
                if (GetFileAttributes(tempFile) == INVALID_FILE_ATTRIBUTES) {
                    conversionStatus = "ERROR: Temp file was not created";
                    showStatus = true;
                    DeleteFile(tempFile);
                    return;
                }
                
                // Import the converted BMP into current cell
                BOOL importResult = ImportBMPToCurrentCell(tempFile, TRUE);
                
                if (importResult) {
                    // Also import the palette from the converted BMP
                    Palette* globalPal = isPicture ? globalPicture->palSCI : globalView->palSCI;
                    if (globalPal && ImportPaletteFromBMP(tempFile, globalPal)) {
                        conversionStatus = "SUCCESS: Image and palette imported successfully!";
                    } else {
                        conversionStatus = "WARNING: Image imported, palette import failed";
                    }
                    
                    // CRITICAL: Refresh the current cell/loop to update display
                    if (isPicture) {
                        ShowCell(curCellIndex);
                    } else {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    
                    datasaved = false;
                    InvalidateRect(hWnd, NULL, TRUE);
                    
                } else {
                    conversionStatus = "ERROR: Conversion succeeded but import failed";
                }
                
                DeleteFile(tempFile);
            } else {
                DeleteFile(tempFile);
                const char* error_msg = realmpal_get_last_error();
                char errorBuf[512];
                sprintf(errorBuf, "ERROR: %s", error_msg ? error_msg : "Unknown conversion error");
                conversionStatus = errorBuf;
            }
            showStatus = true;
        }
        ImGui::PopStyleColor(3);
    }
    
    ImGui::NextColumn();
    
    // Close button - matching the styling and size of the Convert button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.72f, 0.96f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.48f, 0.64f, 1.0f));
    if (ImGui::Button("CLOSE", ImVec2(-1, 50))) {
        CleanupPreviewResources();
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_REALMPAL);
    }
    ImGui::PopStyleColor(3);
    
    ImGui::Columns(1);
    
    // Status message
    if (showStatus && !conversionStatus.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (conversionStatus.find("SUCCESS") != std::string::npos) {
            SuccessText(conversionStatus.c_str());
        } else if (conversionStatus.find("WARNING") != std::string::npos) {
            WarningText(conversionStatus.c_str());
        } else {
            ErrorText(conversionStatus.c_str());
        }
    }
    
    EndDialog();
}

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

void RenderPaletteManagerDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static bool configInitialized = false;
    static std::string statusMessage = "";
    static bool showStatus = false;
    static RGB8 workingPalette[256];
    static bool paletteModified = false;
    static std::string lastImportFile = "";
    static std::string lastExportFile = "";
    
    // Palette analysis results
    static RealmpalPaletteStats paletteStats;
    static bool statsValid = false;
    
    // Selection state for interactive palette grid
    static std::vector<bool> selectedIndices(256, false);
    static bool isDragging = false;
    static int dragStart = -1;
    static int selectionStart = -1;
    static int selectionEnd = -1;
    static bool updatePixels = true;  // Whether operations should update pixel indices
    
    // Initialize working palette on first run or when dialog is reopened
    if (!configInitialized) {
        // Copy current palette to working copy
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            for (int i = 0; i < 256; i++) {
                workingPalette[i].r = currentPalette->palData[i].red;
                workingPalette[i].g = currentPalette->palData[i].green;
                workingPalette[i].b = currentPalette->palData[i].blue;
            }
        } else {
            // Default grayscale palette if none available
            for (int i = 0; i < 256; i++) {
                workingPalette[i] = {(uint8_t)i, (uint8_t)i, (uint8_t)i};
            }
        }
        
        configInitialized = true;
        paletteModified = false;
        statsValid = false;
        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
        selectionStart = selectionEnd = -1;
    }
    
    bool open = true;
    SetNextWindowSize(1000, 700);
    
    if (!BeginDialog("Palette Manager", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open) {
        // Reset state for next time dialog is opened
        configInitialized = false;
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
        EndDialog();
        return;
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // HEADER SECTION
    // =========================================================================
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    if (ImGui::BeginChild("HeaderSection", ImVec2(0, 60), true)) {
        
        ImGui::BeginGroup();
        {
            HeaderText("Palette Index Manager");
            ImGui::SameLine();
            if (paletteModified) {
                WarningText("* Modified");
            } else {
                DisabledText("- No changes");
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            ImGui::Checkbox("Update Pixels", &updatePixels);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("When enabled, pixel indices will be updated to follow palette changes");
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            if (ApplyButton("Apply to Current")) {
                // Apply working palette back to the current file
                Palette* currentPalette = nullptr;
                if (globalView && globalView->palSCI) {
                    currentPalette = globalView->palSCI;
                } else if (globalPicture && globalPicture->palSCI) {
                    currentPalette = globalPicture->palSCI;
                }
                
                if (currentPalette) {
                    for (int i = 0; i < 256; i++) {
                        currentPalette->palData[i].red = workingPalette[i].r;
                        currentPalette->palData[i].green = workingPalette[i].g;
                        currentPalette->palData[i].blue = workingPalette[i].b;
                        // Keep existing remap value
                    }
                    
                    // Refresh display
                    if (isPicture) {
                        ShowCell(curCellIndex);
                    } else {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    
                    datasaved = false;
                    InvalidateRect(hWnd, NULL, TRUE);
                    
                    paletteModified = false;
                    statusMessage = "Palette applied successfully!";
                    showStatus = true;
                } else {
                    statusMessage = "ERROR: No target palette found";
                    showStatus = true;
                }
            }
            
            ImGui::SameLine();
            if (CancelButton("Reset")) {
                // Reset working palette to original
                Palette* currentPalette = nullptr;
                if (globalView && globalView->palSCI) {
                    currentPalette = globalView->palSCI;
                } else if (globalPicture && globalPicture->palSCI) {
                    currentPalette = globalPicture->palSCI;
                }
                
                if (currentPalette) {
                    for (int i = 0; i < 256; i++) {
                        workingPalette[i].r = currentPalette->palData[i].red;
                        workingPalette[i].g = currentPalette->palData[i].green;
                        workingPalette[i].b = currentPalette->palData[i].blue;
                    }
                    paletteModified = false;
                    statsValid = false;
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    selectionStart = selectionEnd = -1;
                    statusMessage = "Palette reset to original";
                    showStatus = true;
                } else {
                    statusMessage = "ERROR: No source palette found";
                    showStatus = true;
                }
            }
        }
        ImGui::EndGroup();
        
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // =========================================================================
    // MAIN CONTENT - Two Column Layout
    // =========================================================================
    
    if (ImGui::BeginChild("MainContent", ImVec2(0, -80))) { // Reserve space for bottom
        
        // Left Column: Tools (55% width)
        if (ImGui::BeginChild("ToolsColumn", ImVec2(availableWidth * 0.55f, 0), true)) {
            
            // =================================================================
            // SELECTION INFO SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Count selected indices
                int selectedCount = 0;
                int firstSelected = -1;
                int lastSelected = -1;
                
                for (int i = 0; i < 256; i++) {
                    if (selectedIndices[i]) {
                        selectedCount++;
                        if (firstSelected == -1) firstSelected = i;
                        lastSelected = i;
                    }
                }
                
                if (selectedCount == 0) {
                    DisabledText("No indices selected - click palette grid to select");
                } else if (selectedCount == 1) {
                    char selText[64];
                    sprintf(selText, "Selected: Index %d", firstSelected);
                    SuccessText(selText);
                } else {
                    char selText[128];
                    sprintf(selText, "Selected: %d indices (%d-%d)", selectedCount, firstSelected, lastSelected);
                    SuccessText(selText);
                }
                
                ImGui::Spacing();
                
                // Quick selection tools
                if (ImGui::Button("Select All", ImVec2(80, 0))) {
                    std::fill(selectedIndices.begin(), selectedIndices.end(), true);
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear", ImVec2(80, 0))) {
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    selectionStart = selectionEnd = -1;
                }
                ImGui::SameLine();
                if (ImGui::Button("Invert", ImVec2(80, 0))) {
                    for (int i = 0; i < 256; i++) {
                        selectedIndices[i] = !selectedIndices[i];
                    }
                }
                
                ImGui::Spacing();
                
                // Range selection
                static int rangeStart = 0;
                static int rangeEnd = 255;
                
                ImGui::Text("Select Range:");
                ImGui::PushItemWidth(70);
                if (ImGui::InputInt("From##range", &rangeStart)) {
                    rangeStart = realmpal_clamp_int(rangeStart, 0, 255);
                }
                ImGui::SameLine();
                if (ImGui::InputInt("To##range", &rangeEnd)) {
                    rangeEnd = realmpal_clamp_int(rangeEnd, rangeStart, 255);
                }
                ImGui::PopItemWidth();
                
                ImGui::SameLine();
                if (ImGui::Button("Select##range", ImVec2(60, 0))) {
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    for (int i = rangeStart; i <= rangeEnd; i++) {
                        selectedIndices[i] = true;
                    }
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // INDEX MANIPULATION SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Index Operations", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Get selection range for operations
                int firstSel = -1, lastSel = -1;
                for (int i = 0; i < 256; i++) {
                    if (selectedIndices[i]) {
                        if (firstSel == -1) firstSel = i;
                        lastSel = i;
                    }
                }
                
                bool hasSelection = (firstSel != -1);
                
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices in the palette grid to enable operations");
                    ImGui::PopStyleVar();
                } else {
                    
                    // Shift operations
                    HeaderText("Shift Selected Range:");
                    
                    static int shiftTo = 0;
                    ImGui::PushItemWidth(100);
                    ImGui::InputInt("Move to index", &shiftTo);
                    shiftTo = realmpal_clamp_int(shiftTo, 0, 255);
                    ImGui::PopItemWidth();
                    
                    if (ImGui::Button("Shift Range", ImVec2(-1, 0))) {
                        if (firstSel != -1 && lastSel != -1 && shiftTo != firstSel) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = updatePixels ? (curCell && (*curCell) && (*curCell)->bmImage) ? (*curCell)->bmImage : nullptr : nullptr;
                            ctx.width = updatePixels ? ((*curCell) && (*curCell)->bmInfo) ? (*curCell)->bmInfo->bmiHeader.biWidth : 0 : 0;
                            ctx.height = updatePixels ? ((*curCell) && (*curCell)->bmInfo) ? abs((*curCell)->bmInfo->bmiHeader.biHeight) : 0 : 0;
                            ctx.update_indices = updatePixels;
                            
                            if (realmpal_palette_shift_range(&ctx, firstSel, lastSel, shiftTo)) {
                                paletteModified = true;
                                statsValid = false;
                                
                                // Update selection to follow the shift
                                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                int rangeSize = lastSel - firstSel + 1;
                                for (int i = 0; i < rangeSize && (shiftTo + i) < 256; i++) {
                                    selectedIndices[shiftTo + i] = true;
                                }
                                
                                statusMessage = "Range shifted successfully";
                                showStatus = true;
                                
                                if (updatePixels) {
                                    // Refresh display if pixels were updated
                                    if (isPicture) {
                                        ShowCell(curCellIndex);
                                    } else {
                                        ShowLoopCell(curLoopIndex, curCellIndex);
                                    }
                                    InvalidateRect(hWnd, NULL, TRUE);
                                }
                            } else {
                                statusMessage = "ERROR: Failed to shift range";
                                showStatus = true;
                            }
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Copy operations
                    HeaderText("Copy Selected Range:");
                    
                    static int copyTo = 0;
                    ImGui::PushItemWidth(100);
                    ImGui::InputInt("Copy to index", &copyTo);
                    copyTo = realmpal_clamp_int(copyTo, 0, 255);
                    ImGui::PopItemWidth();
                    
                    if (ImGui::Button("Copy Range", ImVec2(-1, 0))) {
                        if (firstSel != -1 && lastSel != -1 && copyTo != firstSel) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = nullptr; // Copy doesn't need pixel updates
                            ctx.width = ctx.height = 0;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_copy_range(&ctx, firstSel, lastSel, copyTo)) {
                                paletteModified = true;
                                statsValid = false;
                                statusMessage = "Range copied successfully";
                                showStatus = true;
                            } else {
                                statusMessage = "ERROR: Failed to copy range";
                                showStatus = true;
                            }
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Reverse operation
                    HeaderText("Reverse Selected Range:");
                    
                    if (ImGui::Button("Reverse Order", ImVec2(-1, 0))) {
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.indices = updatePixels ? (curCell && (*curCell) && (*curCell)->bmImage) ? (*curCell)->bmImage : nullptr : nullptr;
                        ctx.width = updatePixels ? ((*curCell) && (*curCell)->bmInfo) ? (*curCell)->bmInfo->bmiHeader.biWidth : 0 : 0;
                        ctx.height = updatePixels ? ((*curCell) && (*curCell)->bmInfo) ? abs((*curCell)->bmInfo->bmiHeader.biHeight) : 0 : 0;
                        ctx.update_indices = updatePixels;
                        
                        if (realmpal_palette_reverse_range(&ctx, firstSel, lastSel)) {
                            paletteModified = true;
                            statsValid = false;
                            statusMessage = "Range reversed successfully";
                            showStatus = true;
                            
                            if (updatePixels) {
                                // Refresh display if pixels were updated
                                if (isPicture) {
                                    ShowCell(curCellIndex);
                                } else {
                                    ShowLoopCell(curLoopIndex, curCellIndex);
                                }
                                InvalidateRect(hWnd, NULL, TRUE);
                            }
                        } else {
                            statusMessage = "ERROR: Failed to reverse range";
                            showStatus = true;
                        }
                    }
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // COLOR ADJUSTMENT SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Color Adjustments")) {
                
                // Get selection range for operations
                int firstSel = -1, lastSel = -1;
                for (int i = 0; i < 256; i++) {
                    if (selectedIndices[i]) {
                        if (firstSel == -1) firstSel = i;
                        lastSel = i;
                    }
                }
                
                bool hasSelection = (firstSel != -1);
                
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices to adjust their colors");
                    ImGui::PopStyleVar();
                } else {
                    
                    static float brightness = 0.0f;
                    static float contrast = 0.0f;
                    static float hueShift = 0.0f;
                    static float satFactor = 1.0f;
                    
                    ImGui::PushItemWidth(150);
                    ImGui::SliderFloat("Brightness", &brightness, -1.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Contrast", &contrast, -1.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Hue Shift", &hueShift, -180.0f, 180.0f, "%.0f°");
                    ImGui::SliderFloat("Saturation", &satFactor, 0.0f, 2.0f, "%.2f");
                    ImGui::PopItemWidth();
                    
                    ImGui::Spacing();
                    
                    if (ImGui::Button("Apply Adjustments", ImVec2(-1, 0))) {
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.indices = nullptr; // Color adjustments don't need pixel updates
                        ctx.width = ctx.height = 0;
                        ctx.update_indices = false;
                        
                        bool applied = false;
                        
                        if (brightness != 0.0f || contrast != 0.0f) {
                            if (realmpal_palette_adjust_brightness_contrast(&ctx, firstSel, lastSel, brightness, contrast)) {
                                applied = true;
                            }
                        }
                        
                        if (hueShift != 0.0f || satFactor != 1.0f) {
                            if (realmpal_palette_adjust_hue_saturation(&ctx, firstSel, lastSel, hueShift, satFactor)) {
                                applied = true;
                            }
                        }
                        
                        if (applied) {
                            paletteModified = true;
                            statsValid = false;
                            statusMessage = "Color adjustments applied";
                            showStatus = true;
                            
                            // Reset sliders
                            brightness = contrast = hueShift = 0.0f;
                            satFactor = 1.0f;
                        }
                    }
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // IMPORT/EXPORT SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Import / Export")) {
                
                ImGui::Text("Import Palette From:");
                
                if (ImGui::Button("Browse File...", ImVec2(-1, 0))) {
                    OPENFILENAME ofn;
                    static char fileName[MAX_PATH] = "";
                    
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "Palette files (*.bmp;*.png;*.pcx)\0*.bmp;*.png;*.pcx\0All files (*.*)\0*.*\0\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    ofn.lpstrTitle = "Import Palette From File";
                    
                    if (GetOpenFileName(&ofn)) {
                        // Clear error state
                        realmpal_clear_error();
                        
                        int colors = realmpal_read_any_palette(fileName, workingPalette, 256);
                        if (colors > 0) {
                            lastImportFile = fileName;
                            paletteModified = true;
                            statsValid = false;
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            char msg[512];
                            sprintf(msg, "SUCCESS: Imported %d colors", colors);
                            statusMessage = msg;
                            showStatus = true;
                        } else {
                            const char* error = realmpal_get_last_error();
                            char msg[512];
                            sprintf(msg, "ERROR: Import failed - %s", error ? error : "Unknown error");
                            statusMessage = msg;
                            showStatus = true;
                        }
                    }
                }
                
                ImGui::Spacing();
                
                if (ImGui::Button("Export Palette...", ImVec2(-1, 0))) {
                    OPENFILENAME ofn;
                    static char fileName[MAX_PATH] = "palette.bmp";
                    
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "BMP Palette (*.bmp)\0*.bmp\0PCX Palette (*.pcx)\0*.pcx\0\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
                    ofn.lpstrTitle = "Export Palette To File";
                    
                    if (GetSaveFileName(&ofn)) {
                        // Create a simple 16x16 palette image for export
                        uint8_t indices[256];
                        for (int i = 0; i < 256; i++) {
                            indices[i] = i;
                        }
                        
                        // Clear error state
                        realmpal_clear_error();
                        
                        if (realmpal_write_auto(fileName, 16, 16, indices, workingPalette)) {
                            lastExportFile = fileName;
                            statusMessage = "SUCCESS: Palette exported";
                            showStatus = true;
                        } else {
                            const char* error = realmpal_get_last_error();
                            char msg[512];
                            sprintf(msg, "ERROR: Export failed - %s", error ? error : "Unknown error");
                            statusMessage = msg;
                            showStatus = true;
                        }
                    }
                }
                
                if (!lastImportFile.empty()) {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Last import: %s", lastImportFile.c_str());
                }
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right Column: Interactive Palette Grid (45% width)
        if (ImGui::BeginChild("PaletteColumn", ImVec2(0, 0), true)) {
            
            // =================================================================
            // INTERACTIVE PALETTE PREVIEW
            // =================================================================
            HeaderText("Interactive Palette Grid");
            ImGui::Separator();
            ImGui::Spacing();
            
            InfoText("Left-click = select, Drag = range, Ctrl+click = multi-select");
            ImGui::Spacing();
            
            // Palette grid display with selection
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.9f));
            if (ImGui::BeginChild("InteractivePalette", ImVec2(0, 0), true)) {
                
                const int COLORS_PER_ROW = 16;
                const float BUTTON_SIZE = 18.0f;
                const float SPACING_VAL = 2.0f;
                
                ImGuiIO& io = ImGui::GetIO();
                bool ctrlPressed = io.KeyCtrl;
                
                for (int row = 0; row < 16; row++) {
                    for (int col = 0; col < 16; col++) {
                        int colorIndex = row * COLORS_PER_ROW + col;
                        
                        char buttonId[16];
                        sprintf(buttonId, "##pal%d", colorIndex);
                        
                        RGB8 color = workingPalette[colorIndex];
                        float r = color.r / 255.0f;
                        float g = color.g / 255.0f;
                        float b = color.b / 255.0f;
                        
                        // Brighten selected colors
                        bool isSelected = selectedIndices[colorIndex];
                        if (isSelected) {
                            r = fmin(r + 0.3f, 1.0f);
                            g = fmin(g + 0.3f, 1.0f);
                            b = fmin(b + 0.3f, 1.0f);
                        }
                        
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fmin(r * 1.3f, 1.0f), fmin(g * 1.3f, 1.0f), fmin(b * 1.3f, 1.0f), 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.7f, g * 0.7f, b * 0.7f, 1.0f));
                        
                        ImVec2 buttonPos = ImGui::GetCursorScreenPos();
                        bool buttonClicked = ImGui::Button(buttonId, ImVec2(BUTTON_SIZE, BUTTON_SIZE));
                        
                        ImGui::PopStyleColor(3);
                        
                        // Handle selection
                        if (buttonClicked) {
                            if (ctrlPressed) {
                                // Toggle individual selection
                                selectedIndices[colorIndex] = !selectedIndices[colorIndex];
                            } else {
                                // Start new selection
                                if (!isDragging) {
                                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                    selectedIndices[colorIndex] = true;
                                    selectionStart = selectionEnd = colorIndex;
                                }
                            }
                        }
                        
                        // Handle drag selection
                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                            if (!isDragging) {
                                isDragging = true;
                                dragStart = colorIndex;
                                if (!ctrlPressed) {
                                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                }
                            }
                            
                            // Select range from dragStart to current
                            int start = min(dragStart, colorIndex);
                            int end = max(dragStart, colorIndex);
                            for (int i = start; i <= end; i++) {
                                selectedIndices[i] = true;
                            }
                            selectionStart = start;
                            selectionEnd = end;
                        }
                        
                        if (ImGui::IsMouseReleased(0)) {
                            isDragging = false;
                        }
                        
                        // Draw selection border
                        if (isSelected) {
                            ImDrawList* drawList = ImGui::GetWindowDrawList();
                            ImVec2 buttonMin = buttonPos;
                            ImVec2 buttonMax = ImVec2(buttonPos.x + BUTTON_SIZE, buttonPos.y + BUTTON_SIZE);
                            drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                        }
                        
                        // Tooltip
                        if (ImGui::IsItemHovered()) {
                            char tooltip[128];
                            sprintf(tooltip, "Index %d\nRGB(%d, %d, %d)%s", 
                                   colorIndex, color.r, color.g, color.b,
                                   isSelected ? "\n[SELECTED]" : "");
                            ImGui::SetTooltip("%s", tooltip);
                        }
                        
                        if (col < 15) {
                            ImGui::SameLine(0, SPACING_VAL);
                        }
                    }
                }
                
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();
    
    // =========================================================================
    // BOTTOM BUTTONS
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    float buttonWidth = availableWidth * 0.22f;
    
    if (ApplyButton("Apply & Close")) {
        // Apply working palette and close
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            for (int i = 0; i < 256; i++) {
                currentPalette->palData[i].red = workingPalette[i].r;
                currentPalette->palData[i].green = workingPalette[i].g;
                currentPalette->palData[i].blue = workingPalette[i].b;
                // Keep existing remap value
            }
            
            // Refresh display
            if (isPicture) {
                ShowCell(curCellIndex);
            } else {
                ShowLoopCell(curLoopIndex, curCellIndex);
            }
            
            datasaved = false;
            InvalidateRect(hWnd, NULL, TRUE);
            
            // Reset state for next time
            configInitialized = false;
        }
        
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
    }
    
    ImGui::SameLine();
    
    if (CloseButton("Close")) {
        // Reset state for next time dialog is opened
        configInitialized = false;
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
    }
    
    // Status message
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (statusMessage.find("SUCCESS") != std::string::npos) {
            SuccessText(statusMessage.c_str());
        } else if (statusMessage.find("ERROR") != std::string::npos) {
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