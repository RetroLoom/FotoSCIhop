/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Complete Palette Manager Dialog - Direct Edit with All Features
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

// Global variables for palette manager file dialogs
std::string g_palMgrInputFile = "";
std::string g_palMgrOutputFile = "";
bool g_requestPalMgrInputDialog = false;
bool g_requestPalMgrOutputDialog = false;

void RenderPaletteManagerDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state - NO CACHING, DIRECT EDIT
    static bool configInitialized = false;
    static std::string statusMessage = "";
    static bool showStatus = false;
    static RGB8 originalPalette[256];     // Only store original for revert
    static bool paletteModified = false;
    static int lastClickedIndex = -1;
    static bool shouldCloseDialog = false;
    
    // Palette analysis results
    static RealmpalPaletteStats paletteStats;
    static bool statsValid = false;
    
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
    
    // Initialize - store original palette on first run
    if (!configInitialized) {
        // Get current global palette
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            // Store original palette for revert functionality
            for (int i = 0; i < 256; i++) {
                originalPalette[i].r = currentPalette->palData[i].red;
                originalPalette[i].g = currentPalette->palData[i].green;
                originalPalette[i].b = currentPalette->palData[i].blue;
            }
        } else {
            // Default grayscale palette if none available
            for (int i = 0; i < 256; i++) {
                originalPalette[i] = {(uint8_t)i, (uint8_t)i, (uint8_t)i};
            }
        }
        
        configInitialized = true;
        paletteModified = false;
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
    
    if (!BeginDialog("Enhanced Palette Manager - Direct Edit", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open) {
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
                WarningText("* Modified - changes are live");
            } else {
                DisabledText("- No changes");
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            if (ImGui::Button("Revert All Changes")) {
                // Restore original palette to global palette
                Palette* currentPalette = nullptr;
                if (globalView && globalView->palSCI) {
                    currentPalette = globalView->palSCI;
                } else if (globalPicture && globalPicture->palSCI) {
                    currentPalette = globalPicture->palSCI;
                }
                
                if (currentPalette) {
                    for (int i = 0; i < 256; i++) {
                        currentPalette->palData[i].red = originalPalette[i].r;
                        currentPalette->palData[i].green = originalPalette[i].g;
                        currentPalette->palData[i].blue = originalPalette[i].b;
                    }
                    
                    // Force display refresh
                    if (isPicture && globalPicture && curCell && (*curCell)) {
                        // For P56 files, clear cached image data to force regeneration
                        (*curCell)->bmInfo = nullptr;
                        (*curCell)->bmImage = nullptr;
                        ShowCell(curCellIndex);
                    } else if (globalView) {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    
                    InvalidateRect(hWnd, NULL, TRUE);
                    
                    paletteModified = false;
                    statsValid = false;
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    selectionStart = selectionEnd = -1;
                    clipboardColors.clear();
                    clipboardStart = clipboardSize = -1;
                    swapBuffer.clear();
                    hasSwapSelection = false;
                    
                    statusMessage = "All changes reverted to original palette";
                    showStatus = true;
                }
            }
            
            ImGui::SameLine();
            if (ApplyButton("Apply & Close")) {
                // Changes are already applied to global palette, just mark as saved
                if (paletteModified) {
                    // Update the stored original to current state (makes changes permanent)
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
                        }
                    }
                    
                    // Mark file as needing save
                    datasaved = false;
                    
                    statusMessage = "Palette changes applied and made permanent!";
                    showStatus = true;
                }
                
                // Set flag to close dialog after UI is complete
                shouldCloseDialog = true;
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
                        
                        // Get current global palette
                        Palette* currentPalette = nullptr;
                        if (globalView && globalView->palSCI) {
                            currentPalette = globalView->palSCI;
                        } else if (globalPicture && globalPicture->palSCI) {
                            currentPalette = globalPicture->palSCI;
                        }
                        
                        if (currentPalette) {
                            for (int i = firstSel; i <= lastSel; i++) {
                                if (selectedIndices[i]) {
                                    RGB8 color = {currentPalette->palData[i].red, currentPalette->palData[i].green, currentPalette->palData[i].blue};
                                    clipboardColors.push_back(color);
                                    clipboardSize++;
                                }
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
                        // Get current global palette
                        Palette* currentPalette = nullptr;
                        if (globalView && globalView->palSCI) {
                            currentPalette = globalView->palSCI;
                        } else if (globalPicture && globalPicture->palSCI) {
                            currentPalette = globalPicture->palSCI;
                        }
                        
                        if (currentPalette) {
                            // Paste colors starting at first selected index
                            int pasteCount = 0;
                            for (int i = 0; i < clipboardSize && (firstSel + i) < 256; i++) {
                                currentPalette->palData[firstSel + i].red = clipboardColors[i].r;
                                currentPalette->palData[firstSel + i].green = clipboardColors[i].g;
                                currentPalette->palData[firstSel + i].blue = clipboardColors[i].b;
                                pasteCount++;
                            }
                            
                            paletteModified = true;
                            statsValid = false;
                            
                            // Force immediate display update
                            if (isPicture && globalPicture && curCell && (*curCell)) {
                                (*curCell)->bmInfo = nullptr;
                                (*curCell)->bmImage = nullptr;
                                ShowCell(curCellIndex);
                            } else if (globalView) {
                                ShowLoopCell(curLoopIndex, curCellIndex);
                            }
                            InvalidateRect(hWnd, NULL, TRUE);
                            
                            char msg[128];
                            sprintf(msg, "Pasted %d colors at index %d", pasteCount, firstSel);
                            statusMessage = msg;
                            showStatus = true;
                        }
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
                        
                        // Get current global palette
                        Palette* currentPalette = nullptr;
                        if (globalView && globalView->palSCI) {
                            currentPalette = globalView->palSCI;
                        } else if (globalPicture && globalPicture->palSCI) {
                            currentPalette = globalPicture->palSCI;
                        }
                        
                        if (currentPalette) {
                            for (int i = firstSel; i <= lastSel; i++) {
                                if (selectedIndices[i]) {
                                    RGB8 color = {currentPalette->palData[i].red, currentPalette->palData[i].green, currentPalette->palData[i].blue};
                                    swapBuffer.push_back(color);
                                }
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
                        // Get current global palette
                        Palette* currentPalette = nullptr;
                        if (globalView && globalView->palSCI) {
                            currentPalette = globalView->palSCI;
                        } else if (globalPicture && globalPicture->palSCI) {
                            currentPalette = globalPicture->palSCI;
                        }
                        
                        if (currentPalette) {
                            // Convert global palette to realmpal format for swapping
                            RGB8 globalPalette[256];
                            for (int i = 0; i < 256; i++) {
                                globalPalette[i].r = currentPalette->palData[i].red;
                                globalPalette[i].g = currentPalette->palData[i].green;
                                globalPalette[i].b = currentPalette->palData[i].blue;
                            }
                            
                            RealmpalPaletteContext ctx;
                            ctx.palette = globalPalette;
                            ctx.width = ctx.height = 0;
                            ctx.indices = nullptr;
                            ctx.update_indices = false;
                            
                            if (realmpal_palette_swap_ranges(&ctx, swapStart, swapEnd, firstSel, lastSel)) {
                                // Copy back to global palette
                                for (int i = 0; i < 256; i++) {
                                    currentPalette->palData[i].red = globalPalette[i].r;
                                    currentPalette->palData[i].green = globalPalette[i].g;
                                    currentPalette->palData[i].blue = globalPalette[i].b;
                                }
                                
                                paletteModified = true;
                                statsValid = false;
                                hasSwapSelection = false;
                                swapBuffer.clear();
                                
                                // Force display update
                                if (isPicture && globalPicture && curCell && (*curCell)) {
                                    (*curCell)->bmInfo = nullptr;
                                    (*curCell)->bmImage = nullptr;
                                    ShowCell(curCellIndex);
                                } else if (globalView) {
                                    ShowLoopCell(curLoopIndex, curCellIndex);
                                }
                                InvalidateRect(hWnd, NULL, TRUE);
                                
                                char msg[128];
                                sprintf(msg, "Swapped ranges %d-%d with %d-%d", swapStart, swapEnd, firstSel, lastSel);
                                statusMessage = msg;
                                showStatus = true;
                            } else {
                                statusMessage = "ERROR: Failed to swap ranges";
                                showStatus = true;
                            }
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
            // ATMOSPHERIC EFFECTS
            // =================================================================
            if (ImGui::CollapsingHeader("Atmospheric Effects##main_section")) {
    
                if (!hasSelection) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select indices to apply effects");
                    ImGui::PopStyleVar();
                } else {
                    
                    // Effect variables
                    static float brightness = 0.0f;
                    static float contrast = 0.0f;
                    static float hueShift = 0.0f;
                    static float satFactor = 1.0f;
                    static float sepiaIntensity = 0.0f;
                    static float temperature = 0.0f;
                    static float gamma = 1.0f;
                    static float exposure = 0.0f;
                    static float shadows = 0.0f;
                    static float highlights = 0.0f;
                    static float fogIntensity = 0.0f;
                    static float colorTintR = 0.0f;
                    static float colorTintG = 0.0f;
                    static float colorTintB = 0.0f;
                    static float vibrance = 0.0f;
                    static float blackPoint = 0.0f;
                    static float whitePoint = 1.0f;
                    
                    static RGB8 effectsBackupPalette[256];
                    static bool effectsBackupValid = false;
                    
                    // Create backup when first adjusting
                    if (!effectsBackupValid && hasSelection) {
                        for (int i = firstSel; i <= lastSel; i++) {
                            if (selectedIndices[i]) {
                                effectsBackupPalette[i] = originalPalette[i];
                            }
                        }
                        effectsBackupValid = true;
                    }
                    
                    bool anyEffectChanged = false;
                    
                    // Organize effects into collapsible sections
                    if (ImGui::TreeNodeEx("Basic Adjustments", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::PushItemWidth(150);
                        if (ImGui::SliderFloat("Brightness", &brightness, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Contrast", &contrast, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Gamma", &gamma, 0.1f, 3.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Exposure", &exposure, -3.0f, 3.0f, "%.2f")) anyEffectChanged = true;
                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }
                    
                    if (ImGui::TreeNodeEx("Shadow & Highlight", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::PushItemWidth(150);
                        if (ImGui::SliderFloat("Shadows", &shadows, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjust dark areas only");
                        
                        if (ImGui::SliderFloat("Highlights", &highlights, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjust bright areas only");
                        
                        if (ImGui::SliderFloat("Black Point", &blackPoint, 0.0f, 0.5f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("White Point", &whitePoint, 0.5f, 1.0f, "%.2f")) anyEffectChanged = true;
                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }
                    
                    if (ImGui::TreeNodeEx("Color & Saturation", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::PushItemWidth(150);
                        if (ImGui::SliderFloat("Hue Shift", &hueShift, -180.0f, 180.0f, "%.0f°")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Saturation", &satFactor, 0.0f, 2.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Vibrance", &vibrance, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Smart saturation that preserves skin tones");
                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }
                    
                    if (ImGui::TreeNodeEx("Atmospheric Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::PushItemWidth(150);
                        if (ImGui::SliderFloat("Temperature", &temperature, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cool (blue) to Warm (orange)");
                        
                        if (ImGui::SliderFloat("Fog/Haze", &fogIntensity, 0.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulates atmospheric perspective");
                        
                        if (ImGui::SliderFloat("Sepia Tone", &sepiaIntensity, 0.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }
                    
                    if (ImGui::TreeNode("Color Tinting")) {
                        ImGui::Text("Custom Color Tint:");
                        ImGui::PushItemWidth(120);
                        if (ImGui::SliderFloat("Red Tint", &colorTintR, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Green Tint", &colorTintG, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        if (ImGui::SliderFloat("Blue Tint", &colorTintB, -1.0f, 1.0f, "%.2f")) anyEffectChanged = true;
                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }
                    
                    ImGui::Spacing();
                    
                    // Preset Effects for Common Atmospheres
                    struct AtmosphericPreset {
                        const char* name;
                        const char* category;
                        float brightness, contrast, gamma, exposure, shadows, highlights;
                        float hueShift, satFactor, vibrance, blackPoint, whitePoint;
                        float temperature, fogIntensity, sepiaIntensity;
                        float colorTintR, colorTintG, colorTintB;
                        const char* description;
                    };

                    // Comprehensive preset database
                    static AtmosphericPreset g_atmosphericPresets[] = {
                        // === TIME OF DAY ===
                        {"Dawn", "Time of Day", 0.2f, 0.1f, 1.1f, 0.3f, 0.1f, 0.0f, 
                        10.0f, 1.1f, 0.2f, 0.0f, 1.0f, 0.2f, 0.1f, 0.0f, 
                        0.3f, 0.2f, 0.1f, "Soft morning light with warm orange glow"},
                        
                        {"Morning", "Time of Day", 0.3f, 0.2f, 1.0f, 0.2f, 0.2f, 0.0f,
                        0.0f, 1.2f, 0.1f, 0.0f, 1.0f, 0.1f, 0.0f, 0.0f,
                        0.2f, 0.1f, 0.0f, "Bright, clear morning atmosphere"},
                        
                        {"Noon", "Time of Day", 0.4f, 0.3f, 0.9f, 0.1f, 0.0f, 0.1f,
                        0.0f, 1.3f, 0.2f, 0.0f, 1.0f, -0.1f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, "High contrast midday sun"},
                        
                        {"Dusk", "Time of Day", 0.0f, 0.2f, 1.2f, 0.0f, -0.1f, -0.2f,
                        15.0f, 1.0f, -0.1f, 0.1f, 0.9f, 0.4f, 0.0f, 0.1f,
                        0.4f, 0.2f, -0.1f, "Golden hour with warm sunset colors"},
                        
                        {"Night", "Time of Day", -0.3f, 0.2f, 1.3f, -0.5f, 0.0f, -0.3f,
                        0.0f, 0.7f, -0.2f, 0.2f, 0.8f, -0.4f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.2f, "Dark night with reduced saturation"},
                        
                        {"Moonlight", "Time of Day", -0.4f, 0.1f, 1.4f, -0.3f, 0.1f, -0.4f,
                        0.0f, 0.6f, -0.3f, 0.1f, 0.9f, -0.5f, 0.2f, 0.0f,
                        -0.2f, -0.1f, 0.3f, "Cool moonlit atmosphere"},

                        // === WEATHER CONDITIONS ===
                        {"Foggy Morning", "Weather", 0.2f, -0.3f, 1.1f, 0.2f, -0.1f, 0.0f,
                        0.0f, 0.6f, -0.2f, 0.0f, 0.9f, 0.0f, 0.6f, 0.0f,
                        0.0f, 0.0f, 0.0f, "Misty, low contrast morning fog"},
                        
                        {"Heavy Fog", "Weather", 0.1f, -0.5f, 1.2f, 0.1f, -0.2f, 0.0f,
                        0.0f, 0.4f, -0.4f, 0.1f, 0.8f, 0.0f, 0.8f, 0.0f,
                        0.0f, 0.0f, 0.0f, "Dense, visibility-reducing fog"},
                        
                        {"Rainy Day", "Weather", -0.2f, -0.2f, 1.1f, -0.1f, 0.0f, -0.1f,
                        0.0f, 0.8f, -0.1f, 0.0f, 0.9f, -0.2f, 0.3f, 0.0f,
                        -0.1f, 0.0f, 0.1f, "Overcast, muted colors"},
                        
                        {"Storm", "Weather", -0.4f, 0.4f, 1.3f, -0.2f, 0.2f, -0.2f,
                        0.0f, 0.7f, -0.2f, 0.2f, 0.8f, -0.3f, 0.1f, 0.0f,
                        0.0f, 0.0f, 0.2f, "Dark, dramatic storm atmosphere"},
                        
                        {"Blizzard", "Weather", 0.3f, -0.4f, 1.0f, 0.3f, -0.3f, 0.1f,
                        0.0f, 0.3f, -0.5f, 0.0f, 1.0f, -0.6f, 0.7f, 0.0f,
                        -0.3f, -0.1f, 0.0f, "White-out snow conditions"},
                        
                        {"Heat Haze", "Weather", 0.3f, -0.3f, 0.8f, 0.4f, 0.0f, 0.2f,
                        10.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.6f, 0.4f, 0.0f,
                        0.3f, 0.2f, 0.0f, "Shimmering desert heat"},

                        // === ENVIRONMENTS ===
                        {"Deep Cave", "Environment", -0.5f, 0.5f, 1.5f, -0.4f, 0.3f, -0.5f,
                        0.0f, 0.5f, -0.4f, 0.3f, 0.7f, -0.2f, 0.1f, 0.0f,
                        0.0f, 0.0f, 0.1f, "Dark, high contrast underground"},
                        
                        {"Forest Canopy", "Environment", -0.1f, 0.1f, 1.1f, -0.1f, 0.1f, -0.1f,
                        0.0f, 1.1f, 0.1f, 0.0f, 1.0f, 0.0f, 0.2f, 0.0f,
                        -0.1f, 0.2f, 0.0f, "Dappled green forest light"},
                        
                        {"Deep Ocean", "Environment", -0.3f, -0.1f, 1.2f, -0.2f, 0.0f, -0.3f,
                        0.0f, 0.8f, 0.0f, 0.1f, 0.9f, -0.5f, 0.4f, 0.0f,
                        -0.2f, 0.0f, 0.4f, "Deep underwater blue atmosphere"},
                        
                        {"Mountain Peak", "Environment", 0.2f, 0.4f, 0.9f, 0.2f, 0.0f, 0.1f,
                        0.0f, 1.2f, 0.3f, 0.0f, 1.0f, -0.3f, 0.1f, 0.0f,
                        -0.2f, -0.1f, 0.2f, "Clear, crisp high altitude air"},
                        
                        {"Swampland", "Environment", -0.1f, 0.0f, 1.1f, 0.0f, 0.0f, -0.1f,
                        20.0f, 0.9f, -0.1f, 0.1f, 0.9f, 0.1f, 0.3f, 0.0f,
                        -0.1f, 0.2f, 0.1f, "Murky, green-tinted wetland"},
                        
                        {"Desert Noon", "Environment", 0.4f, 0.5f, 0.8f, 0.5f, 0.0f, 0.3f,
                        5.0f, 1.1f, 0.1f, 0.0f, 1.0f, 0.7f, 0.2f, 0.1f,
                        0.4f, 0.2f, 0.0f, "Harsh, bleaching desert sun"},

                        // === MAGICAL/FANTASY ===
                        {"Poison Cloud", "Magical", 0.0f, 0.2f, 1.1f, 0.0f, 0.0f, 0.0f,
                        30.0f, 1.4f, 0.3f, 0.0f, 1.0f, 0.0f, 0.1f, 0.0f,
                        -0.2f, 0.6f, -0.1f, "Toxic green magical atmosphere"},
                        
                        {"Fire Realm", "Magical", 0.2f, 0.4f, 0.9f, 0.3f, 0.1f, 0.1f,
                        15.0f, 1.3f, 0.2f, 0.0f, 1.0f, 0.8f, 0.0f, 0.0f,
                        0.5f, 0.2f, 0.0f, "Blazing magical fire environment"},
                        
                        {"Ice Cavern", "Magical", 0.1f, 0.3f, 1.2f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.9f, -0.1f, 0.0f, 1.0f, -0.6f, 0.2f, 0.0f,
                        -0.3f, 0.0f, 0.3f, "Crystalline blue ice magic"},
                        
                        {"Shadow Realm", "Magical", -0.6f, 0.6f, 1.4f, -0.3f, 0.4f, -0.6f,
                        0.0f, 0.4f, -0.5f, 0.4f, 0.6f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.1f, "Dark magical shadow dimension"},
                        
                        {"Holy Light", "Magical", 0.5f, -0.2f, 0.8f, 0.4f, -0.2f, 0.3f,
                        10.0f, 1.0f, 0.1f, 0.0f, 1.0f, 0.3f, 0.3f, 0.0f,
                        0.2f, 0.2f, 0.1f, "Divine golden radiance"},
                        
                        {"Arcane Energy", "Magical", 0.0f, 0.3f, 1.0f, 0.1f, 0.0f, 0.0f,
                        -30.0f, 1.2f, 0.4f, 0.0f, 1.0f, -0.2f, 0.0f, 0.0f,
                        0.0f, -0.1f, 0.4f, "Purple-blue magical energy"},

                        // === SCI-FI/CYBERPUNK ===
                        {"Neon Night", "Sci-Fi", -0.2f, 0.4f, 1.1f, 0.0f, 0.2f, -0.1f,
                        0.0f, 1.5f, 0.5f, 0.1f, 0.9f, -0.3f, 0.0f, 0.0f,
                        0.0f, 0.1f, 0.3f, "Cyberpunk neon-lit streets"},
                        
                        {"Space Station", "Sci-Fi", -0.1f, 0.2f, 1.2f, -0.1f, 0.1f, -0.1f,
                        0.0f, 0.8f, -0.2f, 0.0f, 1.0f, -0.4f, 0.0f, 0.0f,
                        -0.1f, 0.0f, 0.2f, "Sterile artificial lighting"},
                        
                        {"Alien World", "Sci-Fi", 0.1f, 0.2f, 1.0f, 0.2f, 0.0f, 0.0f,
                        45.0f, 1.2f, 0.2f, 0.0f, 1.0f, 0.2f, 0.1f, 0.0f,
                        0.2f, 0.3f, 0.2f, "Strange otherworldly atmosphere"},
                        
                        {"Nuclear Glow", "Sci-Fi", 0.2f, 0.1f, 1.1f, 0.3f, 0.0f, 0.0f,
                        20.0f, 1.1f, 0.1f, 0.0f, 1.0f, 0.1f, 0.2f, 0.0f,
                        0.1f, 0.4f, 0.0f, "Radioactive green luminescence"},

                        // === HORROR/MOOD ===
                        {"Gothic Horror", "Horror/Mood", -0.4f, 0.5f, 1.3f, -0.2f, 0.3f, -0.4f,
                        0.0f, 0.6f, -0.3f, 0.3f, 0.7f, 0.0f, 0.1f, 0.3f,
                        0.0f, 0.0f, 0.0f, "Dark, foreboding Gothic atmosphere"},
                        
                        {"Blood Moon", "Horror/Mood", -0.2f, 0.3f, 1.2f, 0.0f, 0.1f, -0.2f,
                        0.0f, 1.0f, 0.0f, 0.1f, 0.9f, 0.2f, 0.0f, 0.0f,
                        0.4f, -0.1f, -0.1f, "Ominous red-tinted moonlight"},
                        
                        {"Peaceful Meadow", "Horror/Mood", 0.3f, -0.1f, 0.9f, 0.2f, -0.1f, 0.1f,
                        5.0f, 1.1f, 0.2f, 0.0f, 1.0f, 0.1f, 0.0f, 0.0f,
                        0.1f, 0.2f, 0.0f, "Soft, calming natural light"},
                        
                        {"Romantic Sunset", "Horror/Mood", 0.1f, 0.0f, 1.0f, 0.2f, -0.1f, 0.0f,
                        25.0f, 1.2f, 0.1f, 0.0f, 1.0f, 0.5f, 0.0f, 0.2f,
                        0.3f, 0.2f, 0.1f, "Warm, golden romantic atmosphere"},

                        // === SEASONAL ===
                        {"Spring Fresh", "Seasonal", 0.2f, 0.1f, 0.9f, 0.2f, -0.1f, 0.1f,
                        0.0f, 1.2f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.2f, 0.0f, "Fresh, vibrant spring colors"},
                        
                        {"Summer Heat", "Seasonal", 0.3f, 0.2f, 0.8f, 0.4f, 0.0f, 0.2f,
                        0.0f, 1.1f, 0.1f, 0.0f, 1.0f, 0.5f, 0.2f, 0.0f,
                        0.2f, 0.1f, 0.0f, "Intense summer sunshine"},
                        
                        {"Autumn Leaves", "Seasonal", 0.1f, 0.2f, 1.0f, 0.1f, 0.0f, 0.0f,
                        20.0f, 1.1f, 0.0f, 0.0f, 1.0f, 0.3f, 0.0f, 0.2f,
                        0.3f, 0.2f, 0.0f, "Warm autumn foliage colors"},
                        
                        {"Winter Twilight", "Seasonal", -0.2f, 0.1f, 1.2f, -0.1f, 0.0f, -0.2f,
                        0.0f, 0.8f, -0.2f, 0.0f, 0.9f, -0.3f, 0.1f, 0.0f,
                        -0.2f, 0.0f, 0.2f, "Cold, crisp winter evening"}
                    };

                    // Atmospheric Presets section
                    if (ImGui::CollapsingHeader("Atmospheric Presets##presets_section")) {
                        
                        static int selectedPresetIndex = -1;
                        static char filterText[128] = "";
                        static char categoryFilter[64] = "All Categories";
                        
                        // Category filter
                        ImGui::Text("Category Filter:");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(180);
                        if (ImGui::BeginCombo("##category_filter", categoryFilter)) {
                            if (ImGui::Selectable("All Categories", strcmp(categoryFilter, "All Categories") == 0)) {
                                strcpy(categoryFilter, "All Categories");
                            }
                            
                            // Get unique categories
                            std::set<std::string> categories;
                            for (size_t i = 0; i < sizeof(g_atmosphericPresets) / sizeof(g_atmosphericPresets[0]); i++) {
                                categories.insert(g_atmosphericPresets[i].category);
                            }
                            
                            for (const auto& category : categories) {
                                if (ImGui::Selectable(category.c_str(), strcmp(categoryFilter, category.c_str()) == 0)) {
                                    strcpy(categoryFilter, category.c_str());
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();
                        
                        ImGui::SameLine();
                        ImGui::Text("Search:");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(150);
                        ImGui::InputText("##preset_search", filterText, sizeof(filterText));
                        ImGui::PopItemWidth();
                        
                        ImGui::Spacing();
                        
                        // Preset list
                        ImGui::PushItemWidth(-1);
                        if (ImGui::BeginListBox("##preset_list", ImVec2(-1, 200))) {
                            
                            int presetCount = sizeof(g_atmosphericPresets) / sizeof(g_atmosphericPresets[0]);
                            for (int i = 0; i < presetCount; i++) {
                                AtmosphericPreset& preset = g_atmosphericPresets[i];
                                
                                // Apply filters
                                bool matchesCategory = (strcmp(categoryFilter, "All Categories") == 0) || 
                                                    (strcmp(categoryFilter, preset.category) == 0);
                                bool matchesSearch = (strlen(filterText) == 0) || 
                                                (strstr(preset.name, filterText) != nullptr) ||
                                                (strstr(preset.description, filterText) != nullptr);
                                
                                if (matchesCategory && matchesSearch) {
                                    char displayText[512];
                                    sprintf(displayText, "[%s] %s", preset.category, preset.name);
                                    
                                    if (ImGui::Selectable(displayText, selectedPresetIndex == i)) {
                                        selectedPresetIndex = i;
                                    }
                                    
                                    if (ImGui::IsItemHovered()) {
                                        ImGui::SetTooltip("%s", preset.description);
                                    }
                                }
                            }
                            
                            ImGui::EndListBox();
                        }
                        ImGui::PopItemWidth();
                        
                        ImGui::Spacing();
                        
                        // Apply preset button
                        if (selectedPresetIndex >= 0) {
                            AtmosphericPreset& preset = g_atmosphericPresets[selectedPresetIndex];
                            
                            char buttonText[256];
                            sprintf(buttonText, "Apply: %s", preset.name);
                            
                            if (ImGui::Button(buttonText, ImVec2(-1, 0))) {
                                // Apply all preset values
                                brightness = preset.brightness;
                                contrast = preset.contrast;
                                gamma = preset.gamma;
                                exposure = preset.exposure;
                                shadows = preset.shadows;
                                highlights = preset.highlights;
                                hueShift = preset.hueShift;
                                satFactor = preset.satFactor;
                                vibrance = preset.vibrance;
                                blackPoint = preset.blackPoint;
                                whitePoint = preset.whitePoint;
                                temperature = preset.temperature;
                                fogIntensity = preset.fogIntensity;
                                sepiaIntensity = preset.sepiaIntensity;
                                colorTintR = preset.colorTintR;
                                colorTintG = preset.colorTintG;
                                colorTintB = preset.colorTintB;
                                
                                anyEffectChanged = true;
                                
                                char msg[256];
                                sprintf(msg, "Applied preset: %s", preset.name);
                                statusMessage = msg;
                                showStatus = true;
                            }
                            
                            // Show preset description
                            ImGui::Spacing();
                            ImGui::TextWrapped("Description: %s", preset.description);
                            
                        } else {
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                            ImGui::Button("Select a preset to apply", ImVec2(-1, 0));
                            ImGui::PopStyleVar();
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Control buttons
                    if (ImGui::Button("Reset All Effects", ImVec2(120, 0))) {
                        brightness = contrast = hueShift = exposure = shadows = highlights = 0.0f;
                        gamma = satFactor = whitePoint = 1.0f;
                        temperature = sepiaIntensity = fogIntensity = vibrance = blackPoint = 0.0f;
                        colorTintR = colorTintG = colorTintB = 0.0f;
                        effectsBackupValid = false;
                        
                        if (hasSelection) {
                            // Get current global palette
                            Palette* currentPalette = nullptr;
                            if (globalView && globalView->palSCI) {
                                currentPalette = globalView->palSCI;
                            } else if (globalPicture && globalPicture->palSCI) {
                                currentPalette = globalPicture->palSCI;
                            }
                            
                            if (currentPalette) {
                                for (int i = firstSel; i <= lastSel; i++) {
                                    if (selectedIndices[i]) {
                                        currentPalette->palData[i].red = originalPalette[i].r;
                                        currentPalette->palData[i].green = originalPalette[i].g;
                                        currentPalette->palData[i].blue = originalPalette[i].b;
                                    }
                                }
                                paletteModified = true;
                                statsValid = false;
                                
                                // Force display update
                                if (isPicture && globalPicture && curCell && (*curCell)) {
                                    (*curCell)->bmInfo = nullptr;
                                    (*curCell)->bmImage = nullptr;
                                    ShowCell(curCellIndex);
                                } else if (globalView) {
                                    ShowLoopCell(curLoopIndex, curCellIndex);
                                }
                                InvalidateRect(hWnd, NULL, TRUE);
                            }
                        }
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Grayscale", ImVec2(90, 0))) {
                        if (hasSelection) {
                            // Get current global palette
                            Palette* currentPalette = nullptr;
                            if (globalView && globalView->palSCI) {
                                currentPalette = globalView->palSCI;
                            } else if (globalPicture && globalPicture->palSCI) {
                                currentPalette = globalPicture->palSCI;
                            }
                            
                            if (currentPalette) {
                                RGB8 globalPalette[256];
                                for (int i = 0; i < 256; i++) {
                                    globalPalette[i].r = currentPalette->palData[i].red;
                                    globalPalette[i].g = currentPalette->palData[i].green;
                                    globalPalette[i].b = currentPalette->palData[i].blue;
                                }
                                
                                RealmpalPaletteContext ctx;
                                ctx.palette = globalPalette;
                                ctx.indices = nullptr;
                                ctx.width = ctx.height = 0;
                                ctx.update_indices = false;
                                
                                if (realmpal_palette_to_grayscale(&ctx, firstSel, lastSel)) {
                                    for (int i = 0; i < 256; i++) {
                                        currentPalette->palData[i].red = globalPalette[i].r;
                                        currentPalette->palData[i].green = globalPalette[i].g;
                                        currentPalette->palData[i].blue = globalPalette[i].b;
                                    }
                                    
                                    paletteModified = true;
                                    statsValid = false;
                                    effectsBackupValid = false;
                                    
                                    // Force display update
                                    if (isPicture && globalPicture && curCell && (*curCell)) {
                                        (*curCell)->bmInfo = nullptr;
                                        (*curCell)->bmImage = nullptr;
                                        ShowCell(curCellIndex);
                                    } else if (globalView) {
                                        ShowLoopCell(curLoopIndex, curCellIndex);
                                    }
                                    InvalidateRect(hWnd, NULL, TRUE);
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
                    
                    // Apply all effects in real-time to global palette
                    if (anyEffectChanged && hasSelection && effectsBackupValid) {
                        // Get current global palette
                        Palette* currentPalette = nullptr;
                        if (globalView && globalView->palSCI) {
                            currentPalette = globalView->palSCI;
                        } else if (globalPicture && globalPicture->palSCI) {
                            currentPalette = globalPicture->palSCI;
                        }
                        
                        if (currentPalette) {
                            for (int i = firstSel; i <= lastSel; i++) {
                                if (selectedIndices[i]) {
                                    RGB8 baseColor = effectsBackupPalette[i];
                                    
                                    float r = baseColor.r / 255.0f;
                                    float g = baseColor.g / 255.0f;
                                    float b = baseColor.b / 255.0f;
                                    
                                    // Helper functions for color space conversion
                                    auto rgb_to_hsl = [](float r, float g, float b, float& h, float& s, float& l) {
                                        float max = fmax(fmax(r, g), b);
                                        float min = fmin(fmin(r, g), b);
                                        float delta = max - min;
                                        
                                        l = (max + min) / 2.0f;
                                        
                                        if (delta < 0.001f) {
                                            s = 0.0f;
                                            h = 0.0f;
                                        } else {
                                            s = (l > 0.5f) ? delta / (2.0f - max - min) : delta / (max + min);
                                            
                                            if (max == r) {
                                                h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
                                            } else if (max == g) {
                                                h = (b - r) / delta + 2.0f;
                                            } else {
                                                h = (r - g) / delta + 4.0f;
                                            }
                                            h /= 6.0f;
                                        }
                                    };

                                    auto hsl_to_rgb = [](float h, float s, float l, float& r, float& g, float& b) {
                                        if (s < 0.001f) {
                                            r = g = b = l;
                                        } else {
                                            auto hue_to_rgb = [](float p, float q, float t) {
                                                if (t < 0.0f) t += 1.0f;
                                                if (t > 1.0f) t -= 1.0f;
                                                if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
                                                if (t < 1.0f/2.0f) return q;
                                                if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
                                                return p;
                                            };
                                            
                                            float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
                                            float p = 2.0f * l - q;
                                            
                                            r = hue_to_rgb(p, q, h + 1.0f/3.0f);
                                            g = hue_to_rgb(p, q, h);
                                            b = hue_to_rgb(p, q, h - 1.0f/3.0f);
                                        }
                                    };

                                    // Apply gamma correction
                                    if (gamma != 1.0f) {
                                        r = powf(fmax(r, 0.0f), 1.0f / gamma);
                                        g = powf(fmax(g, 0.0f), 1.0f / gamma);
                                        b = powf(fmax(b, 0.0f), 1.0f / gamma);
                                    }

                                    // Apply exposure (multiplicative)
                                    if (exposure != 0.0f) {
                                        float exposureMult = powf(2.0f, exposure);
                                        r *= exposureMult;
                                        g *= exposureMult;
                                        b *= exposureMult;
                                    }

                                    // Apply brightness (additive)
                                    if (brightness != 0.0f) {
                                        r += brightness;
                                        g += brightness;
                                        b += brightness;
                                    }

                                    // Apply contrast
                                    if (contrast != 0.0f) {
                                        float contrastFactor = (1.0f + contrast);
                                        r = (r - 0.5f) * contrastFactor + 0.5f;
                                        g = (g - 0.5f) * contrastFactor + 0.5f;
                                        b = (b - 0.5f) * contrastFactor + 0.5f;
                                    }

                                    // Apply black/white point adjustment
                                    if (blackPoint != 0.0f || whitePoint != 1.0f) {
                                        float range = whitePoint - blackPoint;
                                        if (range > 0.001f) {
                                            r = (fmax(r - blackPoint, 0.0f)) / range;
                                            g = (fmax(g - blackPoint, 0.0f)) / range;
                                            b = (fmax(b - blackPoint, 0.0f)) / range;
                                        }
                                    }

                                    // Apply shadow/highlight adjustments
                                    if (shadows != 0.0f || highlights != 0.0f) {
                                        float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
                                        
                                        // Shadow adjustment (affects darker areas more)
                                        if (shadows != 0.0f) {
                                            float shadowMask = 1.0f - luminance;
                                            shadowMask = shadowMask * shadowMask; // Quadratic falloff
                                            float shadowAdjust = shadows * shadowMask;
                                            r += shadowAdjust;
                                            g += shadowAdjust;
                                            b += shadowAdjust;
                                        }
                                        
                                        // Highlight adjustment (affects brighter areas more)
                                        if (highlights != 0.0f) {
                                            float highlightMask = luminance;
                                            highlightMask = highlightMask * highlightMask; // Quadratic falloff
                                            float highlightAdjust = highlights * highlightMask;
                                            r += highlightAdjust;
                                            g += highlightAdjust;
                                            b += highlightAdjust;
                                        }
                                    }

                                    // Color temperature adjustment
                                    if (temperature != 0.0f) {
                                        if (temperature > 0.0f) {
                                            // Warmer (more orange/red)
                                            r += temperature * 0.3f;
                                            g += temperature * 0.1f;
                                            b -= temperature * 0.2f;
                                        } else {
                                            // Cooler (more blue)
                                            r += temperature * 0.2f;
                                            g += temperature * 0.1f;
                                            b -= temperature * 0.3f;
                                        }
                                    }

                                    // Custom color tinting
                                    r += colorTintR;
                                    g += colorTintG;
                                    b += colorTintB;

                                    // Convert to HSL for hue/saturation adjustments
                                    float h, s, l;
                                    rgb_to_hsl(fmax(0.0f, fmin(1.0f, r)), 
                                              fmax(0.0f, fmin(1.0f, g)), 
                                              fmax(0.0f, fmin(1.0f, b)), h, s, l);

                                    // Apply hue shift
                                    if (hueShift != 0.0f) {
                                        h += hueShift / 360.0f;
                                        while (h < 0.0f) h += 1.0f;
                                        while (h > 1.0f) h -= 1.0f;
                                    }

                                    // Apply saturation
                                    if (satFactor != 1.0f) {
                                        s *= satFactor;
                                        s = fmax(0.0f, fmin(1.0f, s));
                                    }

                                    // Apply vibrance (smart saturation that preserves skin tones)
                                    if (vibrance != 0.0f) {
                                        float maxSat = fmax(s, 0.5f); // Reduce effect on highly saturated colors
                                        float vibranceAdjust = vibrance * (1.0f - maxSat);
                                        s += vibranceAdjust;
                                        s = fmax(0.0f, fmin(1.0f, s));
                                    }

                                    // Convert back to RGB
                                    hsl_to_rgb(h, s, l, r, g, b);

                                    // Apply fog/haze effect (desaturates and brightens)
                                    if (fogIntensity > 0.0f) {
                                        float fogR = 0.9f, fogG = 0.95f, fogB = 1.0f; // Slightly blue-tinted fog
                                        r = r * (1.0f - fogIntensity) + fogR * fogIntensity;
                                        g = g * (1.0f - fogIntensity) + fogG * fogIntensity;
                                        b = b * (1.0f - fogIntensity) + fogB * fogIntensity;
                                    }

                                    // Apply sepia effect
                                    if (sepiaIntensity > 0.0f) {
                                        float sepiaR = (r * 0.393f) + (g * 0.769f) + (b * 0.189f);
                                        float sepiaG = (r * 0.349f) + (g * 0.686f) + (b * 0.168f);
                                        float sepiaB = (r * 0.272f) + (g * 0.534f) + (b * 0.131f);
                                        
                                        r = r * (1.0f - sepiaIntensity) + sepiaR * sepiaIntensity;
                                        g = g * (1.0f - sepiaIntensity) + sepiaG * sepiaIntensity;
                                        b = b * (1.0f - sepiaIntensity) + sepiaB * sepiaIntensity;
                                    }

                                    // Final clamping
                                    r = fmax(0.0f, fmin(1.0f, r));
                                    g = fmax(0.0f, fmin(1.0f, g));
                                    b = fmax(0.0f, fmin(1.0f, b));
                                    
                                    // Set final values to global palette
                                    currentPalette->palData[i].red = (uint8_t)realmpal_clamp_int((int)(r * 255), 0, 255);
                                    currentPalette->palData[i].green = (uint8_t)realmpal_clamp_int((int)(g * 255), 0, 255);
                                    currentPalette->palData[i].blue = (uint8_t)realmpal_clamp_int((int)(b * 255), 0, 255);
                                }
                            }
                            
                            paletteModified = true;
                            statsValid = false;
                            
                            // Force display update
                            if (isPicture && globalPicture && curCell && (*curCell)) {
                                (*curCell)->bmInfo = nullptr;
                                (*curCell)->bmImage = nullptr;
                                ShowCell(curCellIndex);
                            } else if (globalView) {
                                ShowLoopCell(curLoopIndex, curCellIndex);
                            }
                            InvalidateRect(hWnd, NULL, TRUE);
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
                        
                        RGB8 importedPalette[256];
                        int colors = realmpal_read_any_palette(g_palMgrInputFile.c_str(), importedPalette, 256);
                        if (colors > 0) {
                            // Get current global palette
                            Palette* currentPalette = nullptr;
                            if (globalView && globalView->palSCI) {
                                currentPalette = globalView->palSCI;
                            } else if (globalPicture && globalPicture->palSCI) {
                                currentPalette = globalPicture->palSCI;
                            }
                            
                            if (currentPalette) {
                                // Import directly to global palette
                                for (int i = 0; i < colors; i++) {
                                    currentPalette->palData[i].red = importedPalette[i].r;
                                    currentPalette->palData[i].green = importedPalette[i].g;
                                    currentPalette->palData[i].blue = importedPalette[i].b;
                                }
                                
                                paletteModified = true;
                                statsValid = false;
                                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                
                                // Force display update
                                if (isPicture && globalPicture && curCell && (*curCell)) {
                                    (*curCell)->bmInfo = nullptr;
                                    (*curCell)->bmImage = nullptr;
                                    ShowCell(curCellIndex);
                                } else if (globalView) {
                                    ShowLoopCell(curLoopIndex, curCellIndex);
                                }
                                InvalidateRect(hWnd, NULL, TRUE);
                                
                                char msg[512];
                                sprintf(msg, "SUCCESS: Imported %d colors", colors);
                                statusMessage = msg;
                                showStatus = true;
                            }
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
                            
                            // Get current global palette for export
                            RGB8 exportPalette[256];
                            Palette* currentPalette = nullptr;
                            if (globalView && globalView->palSCI) {
                                currentPalette = globalView->palSCI;
                            } else if (globalPicture && globalPicture->palSCI) {
                                currentPalette = globalPicture->palSCI;
                            }
                            
                            if (currentPalette) {
                                for (int i = 0; i < 256; i++) {
                                    exportPalette[i].r = currentPalette->palData[i].red;
                                    exportPalette[i].g = currentPalette->palData[i].green;
                                    exportPalette[i].b = currentPalette->palData[i].blue;
                                }
                                
                                if (realmpal_write_auto(g_palMgrOutputFile.c_str(), 16, 16, indices, exportPalette)) {
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
                    
                    // Get current global palette
                    RGB8 analysisPalette[256];
                    Palette* currentPalette = nullptr;
                    if (globalView && globalView->palSCI) {
                        currentPalette = globalView->palSCI;
                    } else if (globalPicture && globalPicture->palSCI) {
                        currentPalette = globalPicture->palSCI;
                    }
                    
                    if (currentPalette) {
                        for (int i = 0; i < 256; i++) {
                            analysisPalette[i].r = currentPalette->palData[i].red;
                            analysisPalette[i].g = currentPalette->palData[i].green;
                            analysisPalette[i].b = currentPalette->palData[i].blue;
                        }
                        
                        if (realmpal_palette_analyze(analysisPalette, indices, pixelCount, &paletteStats)) {
                            statsValid = true;
                            statusMessage = "Palette analysis complete";
                            showStatus = true;
                        } else {
                            statusMessage = "ERROR: Failed to analyze palette";
                            showStatus = true;
                        }
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
            
            HeaderText("Direct Edit Palette Grid");
            ImGui::Separator();
            ImGui::Spacing();
            
            InfoText("Left-click = select, Shift+click = range, Ctrl+click = multi-select");
            InfoText("Changes are applied immediately to live palette");
            ImGui::Spacing();
            
            // Get current global palette for display
            Palette* currentPalette = nullptr;
            if (globalView && globalView->palSCI) {
                currentPalette = globalView->palSCI;
            } else if (globalPicture && globalPicture->palSCI) {
                currentPalette = globalPicture->palSCI;
            }
            
            // Palette grid display with selection
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.9f));
            if (ImGui::BeginChild("DirectEditPalette", ImVec2(0, 0), true)) {
                
                if (currentPalette) {
                    const int COLORS_PER_ROW = 16;
                    const float BUTTON_SIZE = 18.0f;
                    const float SPACING_VAL = 2.0f;
                    
                    ImGuiIO& io = ImGui::GetIO();
                    
                    for (int row = 0; row < 16; row++) {
                        for (int col = 0; col < 16; col++) {
                            int colorIndex = row * COLORS_PER_ROW + col;
                            
                            char buttonId[16];
                            sprintf(buttonId, "##pal%d", colorIndex);
                            
                            float r = currentPalette->palData[colorIndex].red / 255.0f;
                            float g = currentPalette->palData[colorIndex].green / 255.0f;
                            float b = currentPalette->palData[colorIndex].blue / 255.0f;
                            
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
                                sprintf(tooltip, "Index %d\nRGB(%d, %d, %d)%s", 
                                    colorIndex, 
                                    currentPalette->palData[colorIndex].red, 
                                    currentPalette->palData[colorIndex].green, 
                                    currentPalette->palData[colorIndex].blue,
                                    isSelected ? "\n[SELECTED]" : "");
                                ImGui::SetTooltip("%s", tooltip);
                            }
                            
                            if (col < 15) {
                                ImGui::SameLine(0, SPACING_VAL);
                            }
                        }
                    }
                } else {
                    ErrorText("No palette available");
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();
    
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

    // Handle deferred dialog close to prevent ImGui crashes
    if (shouldCloseDialog) {
        configInitialized = false;
        shouldCloseDialog = false;
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
        return;
    }
}