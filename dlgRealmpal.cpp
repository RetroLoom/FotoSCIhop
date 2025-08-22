/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  ImGui Dialog implementations
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

std::string g_realmpalInputFile = "";
std::string g_realmpalPaletteFile = "";
std::string g_realmpalExtraFile = "";
bool g_requestInputDialog = false;
bool g_requestPaletteDialog = false; 
bool g_requestExtraDialog = false;

// Enhanced RenderRealmpalDialog with Image Preview and Live Palette Grid
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