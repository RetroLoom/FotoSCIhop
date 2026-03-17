/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Realmpal Import Dialog
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

// ============================================================================
// FILE DIALOG STATE (set by Win32 file-open callbacks in FotoSCIhop.cpp)
// ============================================================================
std::string g_realmpalInputFile   = "";
std::string g_realmpalPaletteFile = "";
std::string g_realmpalExtraFile   = "";
bool g_requestInputDialog   = false;
bool g_requestPaletteDialog = false;
bool g_requestExtraDialog   = false;

// ============================================================================
// PALETTE PREVIEW STATE
// ============================================================================
static RGBQUAD g_previewPalette[256]  = {};
static bool    g_palettePreviewValid  = false;
static DWORD   g_previewDirtyTime     = 0;   // non-zero = needs refresh
static std::string g_lastPreviewFiles = "";  // tracks which files produced the preview

void CleanupPreviewResources()
{
    g_palettePreviewValid = false;
    g_previewDirtyTime    = 0;
    g_lastPreviewFiles    = "";
}

// ============================================================================
// HELPERS
// ============================================================================

// Returns just the filename portion of a path, or fallback if empty.
static const char* ShortPath(const std::string& path, const char* fallback)
{
    if (path.empty()) return fallback;
    const char* p = strrchr(path.c_str(), '\\');
    return p ? p + 1 : path.c_str();
}

// Inline file-picker row: label | truncated path field | Browse button.
// Returns true if Browse was clicked.
static bool FilePickerRow(const char* label, const std::string& currentPath,
                          const char* placeholder, const char* browseId,
                          float fieldWidth = 0.0f)
{
    using namespace FotoSCIhopStyles;
    ImGui::Text("%s", label);
    ImGui::SameLine();

    const char* display = ShortPath(currentPath, placeholder);
    char buf[MAX_PATH];
    strncpy(buf, display, MAX_PATH - 1);
    buf[MAX_PATH - 1] = '\0';

    if (fieldWidth > 0.0f) ImGui::SetNextItemWidth(fieldWidth);
    else                   ImGui::SetNextItemWidth(-90.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.18f, 0.9f));
    if (!currentPath.empty())
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

    char fieldId[64];
    sprintf(fieldId, "##field_%s", browseId);
    ImGui::InputText(fieldId, buf, MAX_PATH, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    char btnId[64];
    sprintf(btnId, "Browse##%s", browseId);
    return ImGui::Button(btnId, ImVec2(80.0f, 0));
}

// Trigger a palette preview refresh after a short debounce.
static void MarkPreviewDirty()
{
    g_previewDirtyTime = GetTickCount();
}

// Run the conversion just to extract the output palette.
static void RefreshPalettePreview(const RealmpalConfig& config)
{
    memset(g_previewPalette, 0, sizeof(g_previewPalette));
    g_palettePreviewValid = false;

    if (g_realmpalInputFile.empty()) return;

    char tempDir[MAX_PATH], tempFile[MAX_PATH];
    GetTempPath(MAX_PATH, tempDir);
    sprintf(tempFile, "%s\\rp_prev_%lu.bmp", tempDir, GetTickCount());

    RealmpalConfig cfg = config;
    cfg.input_file         = g_realmpalInputFile.c_str();
    cfg.output_file        = tempFile;
    cfg.palette_file       = g_realmpalPaletteFile.empty() ? nullptr : g_realmpalPaletteFile.c_str();
    cfg.extra_palette_file = g_realmpalExtraFile.empty()   ? nullptr : g_realmpalExtraFile.c_str();

    if (realmpal_convert_image(&cfg) == REALMPAL_SUCCESS) {
        FILE* f = fopen(tempFile, "rb");
        if (f) {
            BITMAPFILEHEADER fh; BITMAPINFOHEADER ih;
            if (fread(&fh, sizeof(fh), 1, f) == 1 &&
                fread(&ih, sizeof(ih), 1, f) == 1 &&
                fh.bfType == 0x4D42 && ih.biBitCount == 8)
            {
                g_palettePreviewValid = (fread(g_previewPalette, sizeof(RGBQUAD), 256, f) == 256);
            }
            fclose(f);
        }
        DeleteFile(tempFile);
    }

    // Remember which files produced this preview so we can detect staleness
    g_lastPreviewFiles = g_realmpalInputFile + "|" + g_realmpalPaletteFile + "|" + g_realmpalExtraFile;
}

// ============================================================================
// PALETTE GRID (right panel)
// ============================================================================
static void RenderPaletteGrid(const RealmpalConfig& config,
                               std::vector<bool>& selectedIndices,
                               bool constraintEditMode,
                               bool* isDragging, int* dragStart)
{
    using namespace FotoSCIhopStyles;

    const int   COLS        = 16;
    const float SZ          = 18.0f;
    const float GAP         = 2.0f;
    const float ROUNDING    = 3.0f;
    ImGuiIO&    io          = ImGui::GetIO();

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < COLS; col++) {
            const int idx = row * COLS + col;

            bool isTransp     = (idx == config.transparency_index);
            bool isSelected   = constraintEditMode && selectedIndices[idx];
            bool isAllowed    = !constraintEditMode &&
                                realmpal_index_allowed_by_constraints(
                                    idx, &config.index_constraints, config.transparency_index);

            float r, g, b;
            if (g_palettePreviewValid) {
                r = g_previewPalette[idx].rgbRed   / 255.0f;
                g = g_previewPalette[idx].rgbGreen / 255.0f;
                b = g_previewPalette[idx].rgbBlue  / 255.0f;
            } else {
                r = g = b = idx / 255.0f; // grayscale placeholder
            }

            // Dim blocked indices when constraints are active
            if (!constraintEditMode && config.index_constraints.enabled &&
                config.index_constraints.count > 0 && !isAllowed && !isTransp)
            {
                r *= 0.35f; g *= 0.35f; b *= 0.35f;
            }
            // Brighten selected
            if (isSelected) {
                r = fmin(r + 0.25f, 1.0f);
                g = fmin(g + 0.25f, 1.0f);
                b = fmin(b + 0.25f, 1.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(r, g, b, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fmin(r*1.3f,1.f), fmin(g*1.3f,1.f), fmin(b*1.3f,1.f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(r*0.7f, g*0.7f, b*0.7f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ROUNDING);

            char id[16]; sprintf(id, "##p%d", idx);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            bool clicked = ImGui::Button(id, ImVec2(SZ, SZ));

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            // Constraint drag-select
            if (constraintEditMode) {
                if (clicked) {
                    if (io.KeyCtrl) {
                        selectedIndices[idx] = !selectedIndices[idx];
                    } else if (!(*isDragging)) {
                        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                        selectedIndices[idx] = true;
                    }
                }
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                    if (!(*isDragging)) {
                        *isDragging = true; *dragStart = idx;
                        if (!io.KeyCtrl)
                            std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    }
                    int lo = min(*dragStart, idx), hi = max(*dragStart, idx);
                    for (int i = lo; i <= hi; i++) selectedIndices[i] = true;
                }
                if (ImGui::IsMouseReleased(0)) *isDragging = false;
            }

            // Borders
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 pmax = ImVec2(pos.x + SZ, pos.y + SZ);
            if (isTransp)
                dl->AddRect(pos, pmax, IM_COL32(255,255,0,255), ROUNDING, 0, 2.0f);
            else if (constraintEditMode && isSelected)
                dl->AddRect(pos, pmax, IM_COL32(255,220,0,255), ROUNDING, 0, 2.0f);
            else if (!constraintEditMode && config.index_constraints.enabled &&
                     config.index_constraints.count > 0 && isAllowed)
                dl->AddRect(pos, pmax, IM_COL32(80,220,80,200), ROUNDING, 0, 1.5f);

            // Tooltip
            if (ImGui::IsItemHovered()) {
                char tip[128];
                if (g_palettePreviewValid)
                    sprintf(tip, "#%d  RGB(%d,%d,%d)", idx,
                            g_previewPalette[idx].rgbRed,
                            g_previewPalette[idx].rgbGreen,
                            g_previewPalette[idx].rgbBlue);
                else
                    sprintf(tip, "#%d", idx);
                if (isTransp)           strcat(tip, "  [transparency]");
                else if (isSelected)    strcat(tip, "  [selected]");
                ImGui::SetTooltip("%s", tip);
            }

            if (col < 15) ImGui::SameLine(0, GAP);
        }
    }
}

// ============================================================================
// MAIN DIALOG
// ============================================================================
void RenderRealmpalDialog()
{
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;

    FotoSCIhopStyles::RefreshTheme();

    // --- persistent state ---
    static RealmpalConfig config;
    static bool  configInitialized = false;
    static int   selectedPreset    = 0;
    static bool  constraintEdit    = false;   // true = palette grid is in drag-select mode
    static std::vector<bool> selIdx(256, false);
    static bool  isDragging        = false;
    static int   dragStart         = -1;
    static std::string importStatus = "";
    static DWORD importStatusTime   = 0;
    static bool  isConverting       = false;

    static const char* presetNames[] = {
        "Custom", "Auto (256)", "Hybrid (108+128)",
        "Palette", "P56 Neutral (128)", "Icon (64)",
        "Full Screen", "Sprite (128)"
    };

    if (!configInitialized) {
        realmpal_config_init(&config);
        config.num_colors         = 256;
        config.dither             = REALMPAL_DITHER_FS_SERP;
        config.fs_strength        = 0.8;
        config.transparency_index = 255;
        config.alpha_threshold    = 128;
        config.ordered_matrix_size = 4;
        configInitialized = true;
    }

    // Auto-refresh palette preview after 600 ms debounce
    if (g_previewDirtyTime != 0 && (GetTickCount() - g_previewDirtyTime) > 600) {
        g_previewDirtyTime = 0;
        RefreshPalettePreview(config);
    }

    // Mark dirty when files change
    {
        std::string curFiles = g_realmpalInputFile + "|" + g_realmpalPaletteFile + "|" + g_realmpalExtraFile;
        if (curFiles != g_lastPreviewFiles && g_previewDirtyTime == 0)
            MarkPreviewDirty();
    }

    bool open = true;
    SetNextWindowSize(1000, 720);
    if (!BeginDialog("Realmpal Import", &open)) { EndDialog(); return; }
    if (!open) {
        CleanupPreviewResources();
        HideDialog(DIALOG_REALMPAL);
        EndDialog();
        return;
    }

    const float W = ImGui::GetContentRegionAvail().x;

    // =========================================================================
    // TOP BAR: preset + file pickers
    // =========================================================================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.13f, 1.0f));
    if (ImGui::BeginChild("TopBar", ImVec2(0, 110), true)) {

        // Preset row
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::Combo("Preset##preset", &selectedPreset, presetNames, 8)) {
            switch (selectedPreset) {
                case 1: config.mode=REALMPAL_MODE_AUTO;    config.num_colors=256; config.index_offset=0;   config.quantizer=REALMPAL_QUANT_WU; config.fs_strength=0.8; config.extra_offset=-1; config.extra_colors=-1; break;
                case 2: config.mode=REALMPAL_MODE_AUTO;    config.num_colors=108; config.index_offset=128; config.quantizer=REALMPAL_QUANT_WU; config.fs_strength=0.8; config.extra_offset=0;  config.extra_colors=128; break;
                case 3: config.mode=REALMPAL_MODE_PALETTE; config.num_colors=256; config.index_offset=0;   config.fs_strength=0.8; config.extra_offset=-1; config.extra_colors=-1; break;
                case 4: config.mode=REALMPAL_MODE_PALETTE; config.num_colors=128; config.index_offset=0;   config.fs_strength=0.8; config.extra_offset=-1; config.extra_colors=-1; break;
                case 5: config.mode=REALMPAL_MODE_AUTO;    config.num_colors=64;  config.index_offset=0;   config.quantizer=REALMPAL_QUANT_WU; config.fs_strength=1.0; config.extra_offset=-1; config.extra_colors=-1; break;
                case 6: config.mode=REALMPAL_MODE_AUTO;    config.num_colors=256; config.index_offset=0;   config.quantizer=REALMPAL_QUANT_WU; config.fs_strength=0.6; config.extra_offset=-1; config.extra_colors=-1; break;
                case 7: config.mode=REALMPAL_MODE_AUTO;    config.num_colors=128; config.index_offset=0;   config.quantizer=REALMPAL_QUANT_WU; config.fs_strength=0.9; config.extra_offset=-1; config.extra_colors=-1; break;
                default: break;
            }
            MarkPreviewDirty();
        }

        ImGui::Spacing();

        // File pickers — three compact rows
        const float labelW = 90.0f;
        ImGui::SetNextItemWidth(labelW); ImGui::Text("Source:");
        ImGui::SameLine(labelW);
        if (FilePickerRow("", g_realmpalInputFile, "no file selected", "input"))
            g_requestInputDialog = true;

        ImGui::SetNextItemWidth(labelW); ImGui::Text("Palette:");
        ImGui::SameLine(labelW);
        if (FilePickerRow("", g_realmpalPaletteFile, "optional", "palette"))
            g_requestPaletteDialog = true;

        ImGui::SetNextItemWidth(labelW); ImGui::Text("Extra pal:");
        ImGui::SameLine(labelW);
        if (FilePickerRow("", g_realmpalExtraFile, "optional", "extra"))
            g_requestExtraDialog = true;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // =========================================================================
    // MIDDLE: settings tabs (left) + palette grid (right)
    // =========================================================================
    if (ImGui::BeginChild("Middle", ImVec2(0, -120))) {

        // Left: settings tabs
        if (ImGui::BeginChild("SettingsPane", ImVec2(W * 0.45f, 0), true)) {

            if (ImGui::BeginTabBar("SettingsTabs")) {

                // ---- QUANTIZATION TAB ----
                if (ImGui::BeginTabItem("Quantization")) {
                    ImGui::Spacing();

                    ImGui::Text("Mode:");
                    ImGui::SetNextItemWidth(-1);
                    int modeIdx = (config.mode == REALMPAL_MODE_AUTO) ? 0 : 1;
                    const char* modes[] = { "Auto (generate palette)", "Palette (use base palette)" };
                    if (ImGui::Combo("##mode", &modeIdx, modes, 2)) {
                        config.mode = modeIdx ? REALMPAL_MODE_PALETTE : REALMPAL_MODE_AUTO;
                        selectedPreset = 0; MarkPreviewDirty();
                    }

                    ImGui::Spacing();
                    ImGui::Text("Colors:"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(80); int nc = config.num_colors;
                    if (ImGui::InputInt("##nc", &nc)) { config.num_colors = realmpal_clamp_int(nc,1,256); selectedPreset=0; MarkPreviewDirty(); }

                    ImGui::Text("Start index:"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(80); int si = config.index_offset;
                    if (ImGui::InputInt("##si", &si)) { config.index_offset = realmpal_clamp_int(si,0,255); selectedPreset=0; MarkPreviewDirty(); }

                    if (config.mode == REALMPAL_MODE_AUTO) {
                        ImGui::Spacing();
                        ImGui::Text("Algorithm:");
                        ImGui::SetNextItemWidth(-1);
                        int qi = (config.quantizer == REALMPAL_QUANT_WU) ? 0 : 1;
                        const char* quants[] = { "Wu (recommended)", "Median cut" };
                        if (ImGui::Combo("##quant", &qi, quants, 2)) {
                            config.quantizer = qi ? REALMPAL_QUANT_MEDIAN : REALMPAL_QUANT_WU;
                            selectedPreset=0; MarkPreviewDirty();
                        }
                    }

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                    ImGui::Text("Dither:");
                    ImGui::SetNextItemWidth(-1);
                    int di = (int)config.dither;
                    const char* dithers[] = { "None", "Floyd-Steinberg", "FS Serpentine", "Ordered" };
                    if (ImGui::Combo("##dither", &di, dithers, 4)) {
                        config.dither = (RealmpalDitherMode)di; selectedPreset=0; MarkPreviewDirty();
                    }
                    if (config.dither == REALMPAL_DITHER_FS || config.dither == REALMPAL_DITHER_FS_SERP) {
                        ImGui::Text("Strength:"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(-1);
                        float fsStrength = (float)config.fs_strength;
                        if (ImGui::SliderFloat("##fs", &fsStrength, 0.0f, 1.0f, "%.2f")) {
                            config.fs_strength = fsStrength;
                            selectedPreset=0; MarkPreviewDirty();
                        }
                    }
                    if (config.dither == REALMPAL_DITHER_ORDERED) {
                        ImGui::Text("Matrix:"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(-1);
                        int ms = config.ordered_matrix_size;
                        if (ImGui::InputInt("##ms", &ms)) {
                            config.ordered_matrix_size = realmpal_clamp_int(ms,2,8);
                            selectedPreset=0; MarkPreviewDirty();
                        }
                    }

                    ImGui::EndTabItem();
                }

                // ---- TRANSPARENCY TAB ----
                if (ImGui::BeginTabItem("Transparency")) {
                    ImGui::Spacing();

                    ImGui::Text("Index (-1 = none):"); ImGui::SameLine();
                    ImGui::SetNextItemWidth(80); int ti = config.transparency_index;
                    if (ImGui::InputInt("##ti", &ti)) {
                        config.transparency_index = realmpal_clamp_int(ti,-1,255);
                        selectedPreset=0; MarkPreviewDirty();
                    }

                    if (config.transparency_index >= 0) {
                        ImGui::Text("Alpha threshold:"); ImGui::SameLine();
                        ImGui::SetNextItemWidth(80); int at = config.alpha_threshold;
                        if (ImGui::InputInt("##at", &at)) {
                            config.alpha_threshold = realmpal_clamp_int(at,0,255);
                            selectedPreset=0; MarkPreviewDirty();
                        }

                        ImGui::Spacing();
                        if (ImGui::Checkbox("Replace transparent pixels with color", &config.use_alpha_color))
                            MarkPreviewDirty();
                        if (config.use_alpha_color) {
                            int cr=config.alpha_color.r, cg=config.alpha_color.g, cb=config.alpha_color.b;
                            ImGui::SetNextItemWidth(70);
                            if (ImGui::InputInt("R##ar",&cr)) { config.alpha_color.r=realmpal_clamp_int(cr,0,255); MarkPreviewDirty(); }
                            ImGui::SameLine(); ImGui::SetNextItemWidth(70);
                            if (ImGui::InputInt("G##ag",&cg)) { config.alpha_color.g=realmpal_clamp_int(cg,0,255); MarkPreviewDirty(); }
                            ImGui::SameLine(); ImGui::SetNextItemWidth(70);
                            if (ImGui::InputInt("B##ab",&cb)) { config.alpha_color.b=realmpal_clamp_int(cb,0,255); MarkPreviewDirty(); }
                        }
                    }

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                    ImGui::Text("Quick:");
                    if (ImGui::Button("SCI default (255, magenta)")) {
                        config.transparency_index=255; config.alpha_threshold=128;
                        config.use_alpha_color=true; config.alpha_color={255,0,255};
                        selectedPreset=0; MarkPreviewDirty();
                    }
                    if (ImGui::Button("Disable transparency")) {
                        config.transparency_index=-1; config.use_alpha_color=false;
                        selectedPreset=0; MarkPreviewDirty();
                    }

                    ImGui::EndTabItem();
                }

                // ---- CONSTRAINTS TAB ----
                if (ImGui::BeginTabItem("Constraints")) {
                    ImGui::Spacing();

                    bool en = (config.index_constraints.enabled != 0);
                    if (ImGui::Checkbox("Restrict palette index mapping", &en)) {
                        config.index_constraints.enabled = en ? 1 : 0;
                        if (!en) {
                            constraintEdit = false;
                            std::fill(selIdx.begin(), selIdx.end(), false);
                        }
                        selectedPreset=0; MarkPreviewDirty();
                    }

                    if (config.index_constraints.enabled) {
                        ImGui::Spacing();

                        // Summary
                        int total = realmpal_count_constraint_indices(&config.index_constraints, config.transparency_index);
                        char summary[64]; sprintf(summary, "%d ranges, %d indices available", config.index_constraints.count, total);
                        SuccessText(summary);

                        ImGui::Spacing();
                        ImGui::Text("Click and drag on the palette grid to select allowed ranges.");
                        ImGui::Spacing();

                        // Edit mode toggle
                        if (!constraintEdit) {
                            if (ImGui::Button("Edit constraints on grid")) {
                                constraintEdit = true;
                                // Sync current constraints -> selection
                                std::fill(selIdx.begin(), selIdx.end(), false);
                                for (int i = 0; i < config.index_constraints.count; i++) {
                                    for (int k = config.index_constraints.ranges[i].start;
                                         k <= config.index_constraints.ranges[i].end && k < 256; k++)
                                        selIdx[k] = true;
                                }
                            }
                        } else {
                            // Count selected
                            int selCount = 0;
                            for (int i = 0; i < 256; i++) if (selIdx[i]) selCount++;
                            char selInfo[64]; sprintf(selInfo, "%d indices selected", selCount);
                            WarningText(selInfo);
                            ImGui::Spacing();

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f,0.65f,0.15f,1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f,0.8f,0.2f,1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f,0.5f,0.1f,1.0f));
                            if (ImGui::Button("Apply##cApply")) {
                                config.index_constraints.count = 0;
                                for (int i = 0; i < 256; ) {
                                    if (selIdx[i]) {
                                        int s = i;
                                        while (i < 256 && selIdx[i]) i++;
                                        if (config.index_constraints.count < 32) {
                                            config.index_constraints.ranges[config.index_constraints.count].start = s;
                                            config.index_constraints.ranges[config.index_constraints.count].end   = i - 1;
                                            config.index_constraints.count++;
                                        }
                                    } else { i++; }
                                }
                                constraintEdit = false; selectedPreset=0; MarkPreviewDirty();
                            }
                            ImGui::PopStyleColor(3);
                            ImGui::SameLine();
                            if (ImGui::Button("Clear##cClear")) {
                                std::fill(selIdx.begin(), selIdx.end(), false);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel##cCancel")) {
                                constraintEdit = false;
                            }
                        }

                        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                        ImGui::Text("Presets:");
                        auto applyRange = [&](int s, int e) {
                            config.index_constraints.count = 1;
                            config.index_constraints.ranges[0] = {s, e};
                            std::fill(selIdx.begin(), selIdx.end(), false);
                            for (int i = s; i <= e; i++) selIdx[i] = true;
                            selectedPreset=0; MarkPreviewDirty();
                        };
                        if (ImGui::Button("Upper half (128-254)")) applyRange(128,254);
                        ImGui::SameLine();
                        if (ImGui::Button("Lower half (0-127)"))   applyRange(0,127);
                        if (ImGui::Button("Safe range (16-239)"))  applyRange(16,239);
                        ImGui::SameLine();
                        if (ImGui::Button("Clear all")) {
                            config.index_constraints.count = 0;
                            std::fill(selIdx.begin(), selIdx.end(), false);
                            selectedPreset=0; MarkPreviewDirty();
                        }
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right: palette grid
        if (ImGui::BeginChild("PalettePane", ImVec2(0, 0), true)) {
            HeaderText("Palette Preview");
            ImGui::SameLine();
            if (g_previewDirtyTime != 0) {
                WarningText("(updating...)");
            } else if (g_palettePreviewValid) {
                SuccessText("(256 colors)");
            } else if (!g_realmpalInputFile.empty()) {
                ErrorText("(preview failed)");
            } else {
                DisabledText("(select a source image)");
            }
            ImGui::Separator(); ImGui::Spacing();

            if (constraintEdit)
                InfoText("Drag to select allowed indices. Yellow = transparency.");
            else if (config.index_constraints.enabled && config.index_constraints.count > 0)
                InfoText("Green border = allowed by constraints. Yellow = transparency.");

            ImGui::Spacing();
            RenderPaletteGrid(config, selIdx, constraintEdit, &isDragging, &dragStart);

            // Range info
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f,0.9f,1.0f,1.0f));
            char info[128];
            sprintf(info, "Base: %d-%d (%d colors)", config.index_offset,
                    config.index_offset + config.num_colors - 1, config.num_colors);
            ImGui::Text("%s", info);
            if (config.extra_offset >= 0) {
                sprintf(info, "Inject: %d-%d (%d colors)", config.extra_offset,
                        config.extra_offset + config.extra_colors - 1, config.extra_colors);
                ImGui::Text("%s", info);
            }
            if (config.transparency_index >= 0) {
                sprintf(info, "Transparent: index %d", config.transparency_index);
                ImGui::Text("%s", info);
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    // =========================================================================
    // BOTTOM: status bar + action buttons
    // =========================================================================
    ImGui::Separator(); ImGui::Spacing();

    bool needsPalette = (config.mode == REALMPAL_MODE_PALETTE);
    bool canImport    = (curCell && *curCell) &&
                        !g_realmpalInputFile.empty() &&
                        (!needsPalette || !g_realmpalPaletteFile.empty());

    // Status chips
    ImGui::BeginGroup();
    {
        if (!g_realmpalInputFile.empty()) SuccessText("Source ready");
        else                              ErrorText("No source");
        ImGui::SameLine(); ImGui::Text("|");
        ImGui::SameLine();
        if (needsPalette) {
            if (!g_realmpalPaletteFile.empty()) SuccessText("Palette ready");
            else                                ErrorText("Palette required");
        } else {
            DisabledText("Palette optional");
        }
        ImGui::SameLine(); ImGui::Text("|");
        ImGui::SameLine();
        if (curCell && *curCell) SuccessText("Target cell ready");
        else                     ErrorText("No target cell");
    }
    ImGui::EndGroup();

    ImGui::Spacing();

    // Action buttons
    const float btnH = 44.0f;
    const float btnW = (W - 12.0f) * 0.5f;

    if (isConverting) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,0.8f,0.2f,1.0f));
        ImGui::Text("Converting...");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        canImport ? ImVec4(0.12f,0.65f,0.12f,1.0f) : ImVec4(0.28f,0.28f,0.28f,0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, canImport ? ImVec4(0.18f,0.78f,0.18f,1.0f) : ImVec4(0.32f,0.32f,0.32f,0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  canImport ? ImVec4(0.08f,0.52f,0.08f,1.0f) : ImVec4(0.22f,0.22f,0.22f,0.6f));
        if (ImGui::Button("Convert & Import", ImVec2(btnW, btnH)) && canImport) {
            isConverting = true;
            char tempDir[MAX_PATH], tempFile[MAX_PATH];
            GetTempPath(MAX_PATH, tempDir);
            sprintf(tempFile, "%s\\rp_import_%lu.bmp", tempDir, GetTickCount());

            config.input_file         = g_realmpalInputFile.c_str();
            config.output_file        = tempFile;
            config.palette_file       = g_realmpalPaletteFile.empty() ? nullptr : g_realmpalPaletteFile.c_str();
            config.extra_palette_file = g_realmpalExtraFile.empty()   ? nullptr : g_realmpalExtraFile.c_str();

            int result = realmpal_convert_image(&config);
            isConverting = false;

            if (result == REALMPAL_SUCCESS &&
                GetFileAttributes(tempFile) != INVALID_FILE_ATTRIBUTES)
            {
                if (ImportBMPToCurrentCell(tempFile, TRUE)) {
                    Palette* gpal = isPicture ? globalPicture->palSCI : globalView->palSCI;
                    if (gpal && ImportPaletteFromBMP(tempFile, gpal))
                        importStatus = "Image and palette imported.";
                    else
                        importStatus = "Image imported (palette import failed).";

                    if (isPicture) ShowCell(curCellIndex);
                    else           ShowLoopCell(curLoopIndex, curCellIndex);
                    datasaved = false;
                    InvalidateRect(hWnd, NULL, TRUE);
                } else {
                    importStatus = "Conversion succeeded but cell import failed.";
                }
            } else {
                const char* err = realmpal_get_last_error();
                char buf[512]; sprintf(buf, "Conversion failed: %s", err ? err : "unknown error");
                importStatus = buf;
            }
            DeleteFile(tempFile);
            importStatusTime = GetTickCount();
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f,0.35f,0.55f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f,0.45f,0.70f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.28f,0.28f,0.44f,1.0f));
    if (ImGui::Button("Close", ImVec2(btnW, btnH))) {
        CleanupPreviewResources();
        HideDialog(DIALOG_REALMPAL);
    }
    ImGui::PopStyleColor(3);

    // Import status — auto-hide after 4 seconds
    if (!importStatus.empty() && importStatusTime != 0) {
        if ((GetTickCount() - importStatusTime) < 4000) {
            ImGui::Spacing();
            if (importStatus.find("failed") != std::string::npos ||
                importStatus.find("Failed") != std::string::npos)
                ErrorText(importStatus.c_str());
            else if (importStatus.find("palette import failed") != std::string::npos)
                WarningText(importStatus.c_str());
            else
                SuccessText(importStatus.c_str());
        } else {
            importStatus = "";
            importStatusTime = 0;
        }
    }

    EndDialog();
}
