/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Enhanced ImGui Palette Manager Dialog with Cached Preview System
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

// Global variables for palette manager file dialogs (add to FotoSCIhop.h)
std::string g_palMgrInputFile = "";
std::string g_palMgrOutputFile = "";
bool g_requestPalMgrInputDialog = false;
bool g_requestPalMgrOutputDialog = false;

void RenderPaletteManagerDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state with cached palette system
    static bool configInitialized = false;
    static std::string statusMessage = "";
    static bool showStatus = false;
    static RGB8 workingPalette[256];      // Cached palette - where changes are made
    static RGB8 originalPalette[256];     // Original - preserved until Apply
    static bool paletteModified = false;
    static bool showOriginalPalette = false;  // Toggle: false = show cached, true = show original
    static int lastClickedIndex = -1;
    
    // Palette analysis results
    static RealmpalPaletteStats paletteStats;
    static bool statsValid = false;
    
    // Function to update display with current palette choice
    auto UpdatePaletteDisplay = [&]() {
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            // Choose which palette to display
            RGB8* displayPalette = showOriginalPalette ? originalPalette : workingPalette;
            
            // Apply to current palette for display
            for (int i = 0; i < 256; i++) {
                currentPalette->palData[i].red = displayPalette[i].r;
                currentPalette->palData[i].green = displayPalette[i].g;
                currentPalette->palData[i].blue = displayPalette[i].b;
            }
            
            // Force display refresh
            if (isPicture) {
                ShowCell(curCellIndex);
            } else {
                ShowLoopCell(curLoopIndex, curCellIndex);
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
    };
    
    // Selection state for interactive palette grid
    static std::vector<bool> selectedIndices(256, false);
    static int selectionStart = -1;
    static int selectionEnd = -1;
    
    // Clipboard for copy/paste operations
    static std::vector<RGB8> clipboardColors;
    static int clipboardStart = -1;
    static int clipboardSize = 0;
    
    // Swap buffer for two-step swapping
    static std::vector<RGB8> swapBuffer;
    static int swapStart = -1;
    static int swapEnd = -1;
    static bool hasSwapSelection = false;
    
    // Initialize working palette on first run or when dialog is reopened
    if (!configInitialized) {
        // Copy current palette to both working and original copies
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            for (int i = 0; i < 256; i++) {
                originalPalette[i].r = currentPalette->palData[i].red;
                originalPalette[i].g = currentPalette->palData[i].green;
                originalPalette[i].b = currentPalette->palData[i].blue;
                
                // Start with original as working copy
                workingPalette[i] = originalPalette[i];
            }
        } else {
            // Default grayscale palette if none available
            for (int i = 0; i < 256; i++) {
                originalPalette[i] = {(uint8_t)i, (uint8_t)i, (uint8_t)i};
                workingPalette[i] = originalPalette[i];
            }
        }
        
        configInitialized = true;
        paletteModified = false;
        showOriginalPalette = false;
        statsValid = false;
        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
        selectionStart = selectionEnd = -1;
        clipboardColors.clear();
        clipboardStart = clipboardSize = -1;
        swapBuffer.clear();
        hasSwapSelection = false;
    }
    
    // Calculate selection range once for the entire dialog
    int firstSel = -1, lastSel = -1;
    int selectedCount = 0;
    for (int i = 0; i < 256; i++) {
        if (selectedIndices[i]) {
            selectedCount++;
            if (firstSel == -1) firstSel = i;
            lastSel = i;
        }
    }
    bool hasSelection = (firstSel != -1);
    bool hasClipboard = !clipboardColors.empty();
    
    bool open = true;
    SetNextWindowSize(1200, 800);
    
    if (!BeginDialog("Enhanced Palette Manager", &open)) {
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
            HeaderText("Enhanced Palette Index Manager");
            ImGui::SameLine();
            if (paletteModified) {
                WarningText("* Modified");
            } else {
                DisabledText("- No changes");
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            bool showOriginalChanged = ImGui::Checkbox("Show Original", &showOriginalPalette);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle between original palette and modified version");
            }
            
            if (showOriginalChanged) {
                UpdatePaletteDisplay();
            }
            
            ImGui::SameLine();
            InfoText(showOriginalPalette ? "Viewing: Original Palette" : "Viewing: Modified Palette");
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            if (ApplyButton("Apply & Close")) {
                // Apply working palette back to the current file permanently and close
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
                    
                    // Mark file as needing save
                    datasaved = false;
                    
                    // Force display refresh with working palette
                    if (isPicture) {
                        ShowCell(curCellIndex);
                    } else {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    InvalidateRect(hWnd, NULL, TRUE);
                    
                    paletteModified = false;
                    statusMessage = "Palette applied and saved successfully!";
                    showStatus = true;
                } else {
                    statusMessage = "ERROR: No target palette found";
                    showStatus = true;
                }
                
                // Close dialog after applying
                configInitialized = false;
                ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
                EndDialog();
                return;
            }
            
            ImGui::SameLine();
            if (CancelButton("Revert")) {
                // Reset working palette to original
                for (int i = 0; i < 256; i++) {
                    workingPalette[i] = originalPalette[i];
                }
                paletteModified = false;
                statsValid = false;
                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                selectionStart = selectionEnd = -1;
                clipboardColors.clear();
                clipboardStart = clipboardSize = -1;
                swapBuffer.clear();
                hasSwapSelection = false;
                
                // Update display if showing working palette
                if (!showOriginalPalette) {
                    UpdatePaletteDisplay();
                }
                
                statusMessage = "Palette reverted to original";
                showStatus = true;
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
        
        // Left Column: Tools (60% width)
        if (ImGui::BeginChild("ToolsColumn", ImVec2(availableWidth * 0.60f, 0), true)) {
            
            // =================================================================
            // SELECTION INFO SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                if (selectedCount == 0) {
                    DisabledText("No indices selected - click palette grid to select");
                } else if (selectedCount == 1) {
                    char selText[64];
                    sprintf(selText, "Selected: Index %d", firstSel);
                    SuccessText(selText);
                } else {
                    char selText[128];
                    sprintf(selText, "Selected: %d indices (%d-%d)", selectedCount, firstSel, lastSel);
                    SuccessText(selText);
                }
                
                ImGui::Spacing();
                
                // Quick selection tools - only enabled when showing working palette
                if (showOriginalPalette) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Selection disabled when viewing original palette");
                    ImGui::PopStyleVar();
                } else {
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
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // COPY/PASTE AND SWAP OPERATIONS
            // =================================================================
            if (ImGui::CollapsingHeader("Copy, Paste & Swap", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                HeaderText("Copy & Paste:");
                
                // Copy to clipboard
                if (hasSelection) {
                    if (ImGui::Button("Copy Selected##clipboard", ImVec2(120, 0))) {
                        clipboardColors.clear();
                        clipboardStart = firstSel;
                        clipboardSize = 0;
                        
                        for (int i = firstSel; i <= lastSel; i++) {
                            if (selectedIndices[i]) {
                                clipboardColors.push_back(workingPalette[i]);
                                clipboardSize++;
                            }
                        }
                        
                        char msg[128];
                        sprintf(msg, "Copied %d colors to clipboard", clipboardSize);
                        statusMessage = msg;
                        showStatus = true;
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Copy Selected##clipboard", ImVec2(120, 0));
                    ImGui::PopStyleVar();
                }
                
                ImGui::SameLine();
                
                // Paste from clipboard
                if (hasClipboard && hasSelection) {
                    if (ImGui::Button("Paste Here##clipboard", ImVec2(120, 0))) {
                        // Paste colors starting at first selected index
                        int pasteCount = 0;
                        for (int i = 0; i < clipboardSize && (firstSel + i) < 256; i++) {
                            workingPalette[firstSel + i] = clipboardColors[i];
                            pasteCount++;
                        }
                        
                        paletteModified = true;
                        statsValid = false;
                        
                        // Update display if showing working palette
                        if (!showOriginalPalette) {
                            UpdatePaletteDisplay();
                        }
                        
                        char msg[128];
                        sprintf(msg, "Pasted %d colors at index %d", pasteCount, firstSel);
                        statusMessage = msg;
                        showStatus = true;
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Paste Here##clipboard", ImVec2(120, 0));
                    ImGui::PopStyleVar();
                }
                
                ImGui::Spacing();
                
                // Clipboard info
                if (hasClipboard) {
                    char clipInfo[128];
                    sprintf(clipInfo, "Clipboard: %d colors from %d-%d", clipboardSize, clipboardStart, clipboardStart + clipboardSize - 1);
                    InfoText(clipInfo);
                } else {
                    DisabledText("Clipboard empty");
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                HeaderText("Two-Step Swap:");
                
                // Mark for swap (step 1)
                if (hasSelection) {
                    if (ImGui::Button("Mark for Swap##twoStep", ImVec2(120, 0))) {
                        // Store current selection for swapping
                        swapBuffer.clear();
                        swapStart = firstSel;
                        swapEnd = lastSel;
                        
                        for (int i = firstSel; i <= lastSel; i++) {
                            if (selectedIndices[i]) {
                                swapBuffer.push_back(workingPalette[i]);
                            }
                        }
                        
                        hasSwapSelection = true;
                        
                        char msg[128];
                        sprintf(msg, "Marked range %d-%d for swapping", firstSel, lastSel);
                        statusMessage = msg;
                        showStatus = true;
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Mark for Swap##twoStep", ImVec2(120, 0));
                    ImGui::PopStyleVar();
                }
                
                ImGui::SameLine();
                
                // Swap with marked range (step 2)
                if (hasSwapSelection && hasSelection) {
                    if (ImGui::Button("Swap Here##twoStep", ImVec2(120, 0))) {
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.width = ctx.height = 0;
                        ctx.indices = nullptr;
                        ctx.update_indices = false; // Using cached palette system
                        
                        if (realmpal_palette_swap_ranges(&ctx, swapStart, swapEnd, firstSel, lastSel)) {
                            paletteModified = true;
                            statsValid = false;
                            hasSwapSelection = false; // Clear swap buffer after use
                            swapBuffer.clear();
                            
                            char msg[128];
                            sprintf(msg, "Swapped ranges %d-%d with %d-%d", swapStart, swapEnd, firstSel, lastSel);
                            statusMessage = msg;
                            showStatus = true;
                            
                            // Update display if showing working palette
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
                            }
                        } else {
                            statusMessage = "ERROR: Failed to swap ranges";
                            showStatus = true;
                        }
                    }
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Swap Here##twoStep", ImVec2(120, 0));
                    ImGui::PopStyleVar();
                }
                
                ImGui::Spacing();
                
                // Swap status info
                if (hasSwapSelection) {
                    char swapInfo[128];
                    sprintf(swapInfo, "Marked for swap: %d-%d (%d colors)", swapStart, swapEnd, (int)swapBuffer.size());
                    WarningText(swapInfo);
                } else {
                    DisabledText("No range marked for swapping");
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // RANGE OPERATIONS
            // =================================================================
            if (ImGui::CollapsingHeader("Range Operations", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices in the palette grid to enable operations");
                    ImGui::PopStyleVar();
                } else {
                    
                    // Shift operations
                    HeaderText("Move Range:");
                    
                    static int shiftTo = 0;
                    ImGui::PushItemWidth(100);
                    ImGui::InputInt("Move to index##shift", &shiftTo);
                    shiftTo = realmpal_clamp_int(shiftTo, 0, 255);
                    ImGui::PopItemWidth();
                    
                    if (ImGui::Button("Move Range##shift", ImVec2(-1, 0))) {
                        if (firstSel != -1 && lastSel != -1 && shiftTo != firstSel) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.width = ctx.height = 0;
                            ctx.indices = nullptr;
                            ctx.update_indices = false; // Using cached palette system now
                            
                            if (realmpal_palette_shift_range(&ctx, firstSel, lastSel, shiftTo)) {
                                paletteModified = true;
                                statsValid = false;
                                
                                // Update selection to follow the shift
                                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                int rangeSize = lastSel - firstSel + 1;
                                for (int i = 0; i < rangeSize && (shiftTo + i) < 256; i++) {
                                    selectedIndices[shiftTo + i] = true;
                                }
                                
                                statusMessage = "Range moved successfully";
                                showStatus = true;
                                
                                // Update display if showing working palette
                                if (!showOriginalPalette) {
                                    UpdatePaletteDisplay();
                                }
                            } else {
                                statusMessage = "ERROR: Failed to move range";
                                showStatus = true;
                            }
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Reverse operation
                    HeaderText("Other Operations:");
                    
                    if (ImGui::Button("Reverse Order##range", ImVec2(-1, 0))) {
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.width = ctx.height = 0;
                        ctx.indices = nullptr;
                        ctx.update_indices = false; // Using cached palette system
                        
                        if (realmpal_palette_reverse_range(&ctx, firstSel, lastSel)) {
                            paletteModified = true;
                            statsValid = false;
                            statusMessage = "Range reversed successfully";
                            showStatus = true;
                            
                            // Update display if showing working palette
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
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
            // SORTING AND ARRANGEMENT
            // =================================================================
            if (ImGui::CollapsingHeader("Sorting & Arrangement")) {
                
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices to sort or arrange");
                    ImGui::PopStyleVar();
                } else {
                    
                    HeaderText("Sort By:");
                    
                    static int sortCriteria = 0;
                    static bool sortAscending = true;
                    
                    const char* sortItems[] = { "Brightness", "Hue", "Saturation", "Red", "Green", "Blue" };
                    ImGui::PushItemWidth(120);
                    ImGui::Combo("##sort_criteria", &sortCriteria, sortItems, 6);
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    ImGui::Checkbox("Ascending", &sortAscending);
                    
                    if (ImGui::Button("Sort Range##sorting", ImVec2(-1, 0))) {
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.width = ctx.height = 0;
                        ctx.indices = nullptr;
                        ctx.update_indices = false; // Using cached palette system
                        
                        RealmpalSortCriteria criteria = (RealmpalSortCriteria)sortCriteria;
                        
                        if (realmpal_palette_sort_range(&ctx, firstSel, lastSel, criteria, sortAscending)) {
                            paletteModified = true;
                            statsValid = false;
                            statusMessage = "Range sorted successfully";
                            showStatus = true;
                            
                            // Update display if showing working palette
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
                            }
                        } else {
                            statusMessage = "ERROR: Failed to sort range";
                            showStatus = true;
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Gradient creation
                    HeaderText("Create Gradient:");
                    
                    static bool useHSL = true;
                    static bool autoGradient = false;
                    
                    if (ImGui::Checkbox("Use HSL interpolation", &useHSL)) {
                        // Update gradient immediately if auto-gradient is on and we have selection
                        if (autoGradient && hasSelection) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = nullptr;
                            ctx.width = ctx.height = 0;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_create_gradient(&ctx, firstSel, lastSel, useHSL)) {
                                paletteModified = true;
                                statsValid = false;
                                
                                if (!showOriginalPalette) {
                                    UpdatePaletteDisplay();
                                }
                            }
                        }
                    }
                    
                    if (ImGui::Checkbox("Auto-Gradient", &autoGradient)) {
                        // Apply gradient immediately when enabled if we have selection
                        if (autoGradient && hasSelection) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = nullptr;
                            ctx.width = ctx.height = 0;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_create_gradient(&ctx, firstSel, lastSel, useHSL)) {
                                paletteModified = true;
                                statsValid = false;
                                
                                if (!showOriginalPalette) {
                                    UpdatePaletteDisplay();
                                }
                            }
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Automatically create gradients when selection changes");
                    }
                    
                    // Manual gradient button (for when auto-gradient is off)
                    if (!autoGradient) {
                        if (ImGui::Button("Create Gradient##gradient", ImVec2(-1, 0))) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = nullptr;
                            ctx.width = ctx.height = 0;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_create_gradient(&ctx, firstSel, lastSel, useHSL)) {
                                paletteModified = true;
                                statsValid = false;
                                
                                if (!showOriginalPalette) {
                                    UpdatePaletteDisplay();
                                }
                            }
                        }
                    }
                    
                    // Auto-apply gradient when selection changes (if auto-gradient is enabled)
                    static int lastGradientFirstSel = -1;
                    static int lastGradientLastSel = -1;
                    if (autoGradient && hasSelection && 
                        (firstSel != lastGradientFirstSel || lastSel != lastGradientLastSel)) {
                        
                        RealmpalPaletteContext ctx;
                        ctx.palette = workingPalette;
                        ctx.indices = nullptr;
                        ctx.width = ctx.height = 0;
                        ctx.update_indices = false;
                        
                        if (realmpal_palette_create_gradient(&ctx, firstSel, lastSel, useHSL)) {
                            paletteModified = true;
                            statsValid = false;
                            
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
                            }
                        }
                        
                        lastGradientFirstSel = firstSel;
                        lastGradientLastSel = lastSel;
                    }
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // COLOR EFFECTS
            // =================================================================
            if (ImGui::CollapsingHeader("Color Effects")) {
                
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices to apply effects");
                    ImGui::PopStyleVar();
                } else {
                    
                    static float brightness = 0.0f;
                    static float contrast = 0.0f;
                    static float hueShift = 0.0f;
                    static float satFactor = 1.0f;
                    static float sepiaIntensity = 0.5f;
                    static float temperature = 0.0f;
                    static RGB8 effectsBackupPalette[256]; // Backup for real-time effects
                    static bool effectsBackupValid = false;
                    
                    // Create backup of selection when first adjusting
                    if (!effectsBackupValid && hasSelection) {
                        for (int i = firstSel; i <= lastSel; i++) {
                            if (selectedIndices[i]) {
                                effectsBackupPalette[i] = originalPalette[i]; // Use original as base
                            }
                        }
                        effectsBackupValid = true;
                    }
                    
                    bool anyEffectChanged = false;
                    
                    ImGui::PushItemWidth(150);
                    if (ImGui::SliderFloat("Brightness", &brightness, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                    if (ImGui::SliderFloat("Contrast", &contrast, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                    if (ImGui::SliderFloat("Hue Shift", &hueShift, -180.0f, 180.0f, "%.0f°")) anyEffectChanged = true;
                    if (ImGui::SliderFloat("Saturation", &satFactor, 0.0f, 2.0f, "%.2f")) anyEffectChanged = true;
                    if (ImGui::SliderFloat("Temperature", &temperature, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                    if (ImGui::SliderFloat("Sepia", &sepiaIntensity, 0.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                    ImGui::PopItemWidth();
                    
                    // Apply effects in real-time
                    if (anyEffectChanged && hasSelection && effectsBackupValid) {
                        // Start from backup and apply all effects
                        for (int i = firstSel; i <= lastSel; i++) {
                            if (selectedIndices[i]) {
                                RGB8 baseColor = effectsBackupPalette[i];
                                
                                float r = baseColor.r / 255.0f;
                                float g = baseColor.g / 255.0f;
                                float b = baseColor.b / 255.0f;
                                
                                // Apply temperature shift
                                if (temperature > 0) { // Warmer
                                    r *= 1.0f + (temperature * 0.3f);
                                    g *= 1.0f + (temperature * 0.1f);
                                    b *= 1.0f - (temperature * 0.2f);
                                } else if (temperature < 0) { // Cooler
                                    r *= 1.0f + (temperature * 0.2f);
                                    g *= 1.0f + (temperature * 0.1f);
                                    b *= 1.0f - (temperature * 0.3f);
                                }
                                
                                // Apply brightness
                                r *= (1.0f + brightness);
                                g *= (1.0f + brightness);
                                b *= (1.0f + brightness);
                                
                                // Apply contrast
                                if (contrast != 0.0f) {
                                    r = ((r - 0.5f) * (1.0f + contrast)) + 0.5f;
                                    g = ((g - 0.5f) * (1.0f + contrast)) + 0.5f;
                                    b = ((b - 0.5f) * (1.0f + contrast)) + 0.5f;
                                }
                                
                                // Apply saturation
                                if (satFactor != 1.0f) {
                                    float gray = 0.299f * r + 0.587f * g + 0.114f * b;
                                    r = gray + (r - gray) * satFactor;
                                    g = gray + (g - gray) * satFactor;
                                    b = gray + (b - gray) * satFactor;
                                }
                                
                                // Apply hue shift (simplified HSV approach)
                                if (hueShift != 0.0f) {
                                    float hueRad = hueShift * 3.14159f / 180.0f;
                                    float cosHue = cos(hueRad);
                                    float sinHue = sin(hueRad);
                                    
                                    float rNew = r * cosHue - g * sinHue;
                                    float gNew = r * sinHue + g * cosHue;
                                    r = rNew; g = gNew;
                                }
                                
                                // Apply sepia tone
                                if (sepiaIntensity > 0.0f) {
                                    float sepiaR = (r * 0.393f + g * 0.769f + b * 0.189f);
                                    float sepiaG = (r * 0.349f + g * 0.686f + b * 0.168f);
                                    float sepiaB = (r * 0.272f + g * 0.534f + b * 0.131f);
                                    
                                    r = r * (1.0f - sepiaIntensity) + sepiaR * sepiaIntensity;
                                    g = g * (1.0f - sepiaIntensity) + sepiaG * sepiaIntensity;
                                    b = b * (1.0f - sepiaIntensity) + sepiaB * sepiaIntensity;
                                }
                                
                                workingPalette[i].r = (uint8_t)realmpal_clamp_int((int)(r * 255), 0, 255);
                                workingPalette[i].g = (uint8_t)realmpal_clamp_int((int)(g * 255), 0, 255);
                                workingPalette[i].b = (uint8_t)realmpal_clamp_int((int)(b * 255), 0, 255);
                            }
                        }
                        
                        paletteModified = true;
                        statsValid = false;
                        
                        // Update display if showing working palette
                        if (!showOriginalPalette) {
                            UpdatePaletteDisplay();
                        }
                    }
                    
                    ImGui::Spacing();
                    // Reset effects button
                    if (ImGui::Button("Reset Effects##effects", ImVec2(120, 0))) {
                        brightness = contrast = hueShift = temperature = 0.0f;
                        satFactor = sepiaIntensity = 1.0f;
                        effectsBackupValid = false; // Force backup refresh
                        
                        // Restore original colors for selection
                        if (hasSelection) {
                            for (int i = firstSel; i <= lastSel; i++) {
                                if (selectedIndices[i]) {
                                    workingPalette[i] = originalPalette[i];
                                }
                            }
                            paletteModified = true;
                            statsValid = false;
                            
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
                            }
                        }
                    }
                    
                    ImGui::SameLine();
                    
                    // Quick effect buttons
                    if (ImGui::Button("Grayscale##effects", ImVec2(90, 0))) {
                        if (hasSelection) {
                            RealmpalPaletteContext ctx;
                            ctx.palette = workingPalette;
                            ctx.indices = nullptr;
                            ctx.width = ctx.height = 0;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_to_grayscale(&ctx, firstSel, lastSel)) {
                                paletteModified = true;
                                statsValid = false;
                                effectsBackupValid = false; // Force backup refresh
                                
                                if (!showOriginalPalette) {
                                    UpdatePaletteDisplay();
                                }
                            }
                        }
                    }
                    
                    // Reset backup when selection changes
                    static int lastFirstSel = -1;
                    static int lastLastSel = -1;
                    if (firstSel != lastFirstSel || lastSel != lastLastSel) {
                        effectsBackupValid = false;
                        lastFirstSel = firstSel;
                        lastLastSel = lastSel;
                    }
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // IMPORT/EXPORT SECTION
            // =================================================================
            if (ImGui::CollapsingHeader("Import / Export")) {
                
                ImGui::Text("Import Palette From:");
                
                // Show current input file
                if (!g_palMgrInputFile.empty()) {
                    const char* fileName = strrchr(g_palMgrInputFile.c_str(), '\\');
                    if (fileName) {
                        char displayName[64];
                        strncpy(displayName, fileName + 1, 60);
                        displayName[60] = '\0';
                        InfoText(displayName);
                    }
                } else {
                    DisabledText("No file selected");
                }
                
                if (ImGui::Button("Browse File...##import", ImVec2(-1, 0))) {
                    g_requestPalMgrInputDialog = true;
                }
                
                ImGui::Spacing();
                
                // Import button
                if (!g_palMgrInputFile.empty()) {
                    if (ImGui::Button("Import Palette##import", ImVec2(-1, 0))) {
                        // Clear error state
                        realmpal_clear_error();
                        
                        int colors = realmpal_read_any_palette(g_palMgrInputFile.c_str(), workingPalette, 256);
                        if (colors > 0) {
                            paletteModified = true;
                            statsValid = false;
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                            
                            // Update display if showing working palette
                            if (!showOriginalPalette) {
                                UpdatePaletteDisplay();
                            }
                            
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
                } else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Import Palette##import", ImVec2(-1, 0));
                    ImGui::PopStyleVar();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Export Palette To:");
                
                if (ImGui::Button("Export Palette...##export", ImVec2(-1, 0))) {
                    g_requestPalMgrOutputDialog = true;
                }
                
                // Show export progress if file is selected
                if (!g_palMgrOutputFile.empty()) {
                    const char* fileName = strrchr(g_palMgrOutputFile.c_str(), '\\');
                    if (fileName) {
                        char displayName[64];
                        strncpy(displayName, fileName + 1, 60);
                        displayName[60] = '\0';
                        InfoText(displayName);
                        
                        if (ImGui::Button("Export Now##export", ImVec2(-1, 0))) {
                            // Create a simple 16x16 palette image for export
                            uint8_t indices[256];
                            for (int i = 0; i < 256; i++) {
                                indices[i] = i;
                            }
                            
                            // Clear error state
                            realmpal_clear_error();
                            
                            // Use working palette for export
                            if (realmpal_write_auto(g_palMgrOutputFile.c_str(), 16, 16, indices, workingPalette)) {
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
                }
            }
            
            ImGui::Spacing();
            
            // =================================================================
            // PALETTE ANALYSIS
            // =================================================================
            if (ImGui::CollapsingHeader("Palette Analysis")) {
                
                if (ImGui::Button("Analyze Palette##analysis", ImVec2(-1, 0))) {
                    uint8_t* indices = nullptr;
                    int pixelCount = 0;
                    
                    // Get pixel data if available for usage analysis
                    if (curCell && (*curCell)) {
                        // Ensure image data is loaded
                        if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
                            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
                        }
                        
                        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
                            indices = (*curCell)->bmImage;
                            pixelCount = (*curCell)->bmInfo->bmiHeader.biWidth * 
                                        abs((*curCell)->bmInfo->bmiHeader.biHeight);
                        }
                    }
                    
                    if (realmpal_palette_analyze(workingPalette, indices, pixelCount, &paletteStats)) {
                        statsValid = true;
                        statusMessage = "Palette analysis complete";
                        showStatus = true;
                    } else {
                        statusMessage = "ERROR: Failed to analyze palette";
                        showStatus = true;
                    }
                }
                
                if (statsValid) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    char statsText[128];
                    sprintf(statsText, "Unique colors: %d", paletteStats.unique_colors);
                    InfoText(statsText);
                    
                    if (paletteStats.used_colors > 0) {
                        sprintf(statsText, "Used in image: %d", paletteStats.used_colors);
                        InfoText(statsText);
                    }
                    
                    sprintf(statsText, "Avg luminance: %.2f", paletteStats.average_luminance);
                    InfoText(statsText);
                    
                    sprintf(statsText, "Darkest: %d, Brightest: %d", 
                           paletteStats.darkest_index, paletteStats.brightest_index);
                    InfoText(statsText);
                }
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right Column: Interactive Palette Grid (40% width)
        if (ImGui::BeginChild("PaletteColumn", ImVec2(0, 0), true)) {
            
            // =================================================================
            // INTERACTIVE PALETTE PREVIEW
            // =================================================================
            HeaderText("Interactive Palette Grid");
            ImGui::Separator();
            ImGui::Spacing();
            
            InfoText("Left-click = select, Shift+click = range, Ctrl+click = multi-select");
            ImGui::Spacing();
            
            // Choose which palette to display based on toggle
            RGB8* displayPalette = showOriginalPalette ? originalPalette : workingPalette;
            
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
                        
                        RGB8 color = displayPalette[colorIndex];
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
                        
                        // Handle selection - only allow if showing working palette
                        if (buttonClicked && !showOriginalPalette) {
                            ImGuiIO& io = ImGui::GetIO();
                            bool ctrlPressed = io.KeyCtrl;
                            bool shiftPressed = io.KeyShift;
                            
                            if (shiftPressed && lastClickedIndex != -1) {
                                // Shift+click: Select range from last clicked to current
                                int rangeStart = min(lastClickedIndex, colorIndex);
                                int rangeEnd = max(lastClickedIndex, colorIndex);
                                
                                if (!ctrlPressed) {
                                    // Clear existing selection unless Ctrl is also held
                                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                }
                                
                                // Select the range
                                for (int i = rangeStart; i <= rangeEnd; i++) {
                                    selectedIndices[i] = true;
                                }
                                
                                selectionStart = rangeStart;
                                selectionEnd = rangeEnd;
                                
                                // Don't update lastClickedIndex when shift-clicking to allow extending ranges
                            } else if (ctrlPressed) {
                                // Ctrl+click: Toggle individual selection
                                selectedIndices[colorIndex] = !selectedIndices[colorIndex];
                                lastClickedIndex = colorIndex;
                            } else {
                                // Plain click: Start new selection
                                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                selectedIndices[colorIndex] = true;
                                selectionStart = selectionEnd = colorIndex;
                                lastClickedIndex = colorIndex;
                            }
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
                            sprintf(tooltip, "Index %d\nRGB(%d, %d, %d)%s%s", 
                                colorIndex, color.r, color.g, color.b,
                                isSelected ? "\n[SELECTED]" : "",
                                showOriginalPalette ? "\n(Original)" : "");
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
    
    // Bottom buttons are now handled in the header section
    // Status message
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (statusMessage.find("SUCCESS") != std::string::npos || statusMessage.find("successfully") != std::string::npos) {
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