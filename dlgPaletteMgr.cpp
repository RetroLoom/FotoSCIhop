/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

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