/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

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