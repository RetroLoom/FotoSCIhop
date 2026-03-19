/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Palette Editor Dialog
 *
 *  Layout: palette grid (left, ~55%) | tabbed tools panel (right, ~45%)
 *
 *  Tabs:
 *    Edit   — selection, single-color RGB editor, flag controls, copy/paste
 *    Adjust — brightness/contrast/hue/sat sliders, color tint, swap ranges
 *    File   — import/export, palette analysis, header diagnostics
 */

#include "stdafx.h"
#include "FotoSCIhop.h"

// Global variables for palette manager file dialogs
std::string g_palMgrInputFile  = "";
std::string g_palMgrOutputFile = "";
bool g_requestPalMgrInputDialog  = false;
bool g_requestPalMgrOutputDialog = false;

// ============================================================================
// Inline HSL helpers (the realmpal versions are static, not accessible here)
// ============================================================================
static void PalMgr_RGBtoHSL(float r, float g, float b, float& h, float& s, float& l)
{
    float maxC = fmaxf(r, fmaxf(g, b));
    float minC = fminf(r, fminf(g, b));
    l = (maxC + minC) * 0.5f;
    float d = maxC - minC;
    if (d < 1e-6f) { h = 0.0f; s = 0.0f; return; }
    s = (l > 0.5f) ? d / (2.0f - maxC - minC) : d / (maxC + minC);
    if      (maxC == r) h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (maxC == g) h = (b - r) / d + 2.0f;
    else                h = (r - g) / d + 4.0f;
    h /= 6.0f;
}

static float PalMgr_HueToRGB(float p, float q, float t)
{
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

static void PalMgr_HSLtoRGB(float h, float s, float l, float& r, float& g, float& b)
{
    if (s < 1e-6f) { r = g = b = l; return; }
    float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;
    r = PalMgr_HueToRGB(p, q, h + 1.0f/3.0f);
    g = PalMgr_HueToRGB(p, q, h);
    b = PalMgr_HueToRGB(p, q, h - 1.0f/3.0f);
}

// ============================================================================
// Helper: get the current open palette (view or picture)
// ============================================================================
static Palette* GetCurrentPalette()
{
    if (globalView    && globalView->palSCI)    return globalView->palSCI;
    if (globalPicture && globalPicture->palSCI) return globalPicture->palSCI;
    return nullptr;
}

// ============================================================================
// Helper: force a display refresh after palette changes
// ============================================================================
static void RefreshDisplay()
{
    if (isPicture && globalPicture && curCell && (*curCell)) {
        (*curCell)->bmInfo  = nullptr;
        (*curCell)->bmImage = nullptr;
        ShowCell(curCellIndex);
    } else if (globalView) {
        ShowLoopCell(curLoopIndex, curCellIndex);
    }
    InvalidateRect(hWnd, NULL, TRUE);
}

// ============================================================================
// Helper: convert RGB bytes to a hex string like "FF8000"
// ============================================================================
static void RGBToHex(uint8_t r, uint8_t g, uint8_t b, char out[7])
{
    sprintf(out, "%02X%02X%02X", r, g, b);
}

// ============================================================================
// Helper: parse a 6-char hex string to RGB (returns false on invalid input)
// ============================================================================
static bool HexToRGB(const char* hex, uint8_t& r, uint8_t& g, uint8_t& b)
{
    if (!hex) return false;
    // Strip leading '#' if present
    if (hex[0] == '#') hex++;
    if (strlen(hex) < 6) return false;
    unsigned int ri = 0, gi = 0, bi = 0;
    if (sscanf(hex, "%02x%02x%02x", &ri, &gi, &bi) != 3) return false;
    r = (uint8_t)ri; g = (uint8_t)gi; b = (uint8_t)bi;
    return true;
}

void RenderPaletteManagerDialog()
{
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;

    FotoSCIhopStyles::RefreshTheme();

    // -------------------------------------------------------------------------
    // Per-session state
    // -------------------------------------------------------------------------
    struct PalEntrySnapshot { uint8_t r, g, b, remap; };
    static bool              configInitialized  = false;
    static std::string       statusMessage      = "";
    static bool              showStatus         = false;
    static int               statusCounter      = 0;
    static PalEntrySnapshot  originalPalette[256];
    static bool              paletteModified    = false;
    static bool              shouldCloseDialog  = false;

    // Selection state
    static std::vector<bool> selectedIndices(256, false);
    static int               lastClickedIndex   = -1;

    // Clipboard
    static std::vector<PalEntrySnapshot> clipboardColors;
    static int               clipboardStart     = -1;

    // Palette analysis results
    static RealmpalPaletteStats paletteStats;
    static bool              statsValid         = false;

    // Active tab
    static int               activeTab          = 0;  // 0=Edit 1=Adjust 2=File

    // -------------------------------------------------------------------------
    // Initialise on first open
    // -------------------------------------------------------------------------
    if (!configInitialized) {
        Palette* pal = GetCurrentPalette();
        for (int i = 0; i < 256; i++) {
            if (pal) {
                originalPalette[i] = { pal->palData[i].red,
                                       pal->palData[i].green,
                                       pal->palData[i].blue,
                                       pal->palData[i].remap };
            } else {
                originalPalette[i] = { (uint8_t)i, (uint8_t)i, (uint8_t)i, 1 };
            }
        }
        configInitialized = true;
        paletteModified   = false;
        statsValid        = false;
        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
        lastClickedIndex  = -1;
        clipboardColors.clear();
        clipboardStart    = -1;
    }

    // -------------------------------------------------------------------------
    // Compute selection range once per frame
    // -------------------------------------------------------------------------
    int firstSel = -1, lastSel = -1, selectedCount = 0;
    for (int i = 0; i < 256; i++) {
        if (selectedIndices[i]) {
            selectedCount++;
            if (firstSel == -1) firstSel = i;
            lastSel = i;
        }
    }
    bool hasSelection  = (firstSel != -1);
    bool hasClipboard  = !clipboardColors.empty();

    // -------------------------------------------------------------------------
    // Window
    // -------------------------------------------------------------------------
    bool open = true;
    SetNextWindowSize(1400, 900);
    if (!BeginDialog("Palette Editor", &open)) {
        EndDialog();
        return;
    }
    if (!open) {
        configInitialized = false;
        HideDialog(DIALOG_PALETTE_MANAGER);
        EndDialog();
        return;
    }

    float availW = ImGui::GetContentRegionAvail().x;

    // -------------------------------------------------------------------------
    // Header bar
    // -------------------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.9f));
    if (ImGui::BeginChild("PalHdr", ImVec2(0, 50), true)) {
        HeaderText("Palette Editor");
        ImGui::SameLine();
        if (paletteModified) {
            WarningText("* Modified");
        } else {
            DisabledText("- No changes");
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();

        if (ImGui::Button("Revert")) {
            Palette* pal = GetCurrentPalette();
            if (pal) {
                for (int i = 0; i < 256; i++) {
                    pal->palData[i].red   = originalPalette[i].r;
                    pal->palData[i].green = originalPalette[i].g;
                    pal->palData[i].blue  = originalPalette[i].b;
                    pal->palData[i].remap = originalPalette[i].remap;
                }
                RefreshDisplay();
                paletteModified = false;
                statsValid      = false;
                std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                clipboardColors.clear();
                clipboardStart = -1;
                statusMessage = "Reverted to original palette";
                showStatus = true;
            }
        }
        ImGui::SameLine();
        if (ApplyButton("Apply & Close")) {
            Palette* pal = GetCurrentPalette();
            if (pal && paletteModified) {
                for (int i = 0; i < 256; i++) {
                    originalPalette[i] = { pal->palData[i].red,
                                           pal->palData[i].green,
                                           pal->palData[i].blue,
                                           pal->palData[i].remap };
                }
                datasaved = false;
            }
            shouldCloseDialog = true;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // =========================================================================
    // MAIN CONTENT — left: palette grid | right: tools
    // =========================================================================
    if (ImGui::BeginChild("PalMain", ImVec2(0, -50))) {

        const float GRID_W   = availW * 0.55f;
        const float TOOLS_W  = availW - GRID_W - 8.0f;

        // =====================================================================
        // LEFT — Palette grid
        // =====================================================================
        if (ImGui::BeginChild("PalGridOuter", ImVec2(GRID_W, 0), true)) {

            Palette* pal = GetCurrentPalette();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.08f, 1.0f));
            if (ImGui::BeginChild("PalGridInner", ImVec2(0, 0), false)) {

                const int   COLS        = 16;
                const float SWATCH      = 28.0f;
                const float GAP         = 2.0f;
                const float LABEL_W     = 30.0f;
                const float LABEL_H     = 18.0f;

                ImGuiIO& io = ImGui::GetIO();

                if (pal) {
                    // Column labels (hex 0–F)
                    ImGui::Dummy(ImVec2(LABEL_W, LABEL_H));
                    for (int col = 0; col < COLS; col++) {
                        ImGui::SameLine(LABEL_W + col * (SWATCH + GAP) + SWATCH * 0.3f);
                        char lbl[4]; sprintf(lbl, "%X", col);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
                        ImGui::Text("%s", lbl);
                        ImGui::PopStyleColor();
                    }

                    for (int row = 0; row < 16; row++) {
                        // Row label
                        char rowLbl[8]; sprintf(rowLbl, " %X0", row);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
                        ImGui::Text("%s", rowLbl);
                        ImGui::PopStyleColor();

                        for (int col = 0; col < COLS; col++) {
                            int idx = row * COLS + col;
                            bool isSelected = selectedIndices[idx];
                            bool isUsed     = (pal->palData[idx].remap != 0);

                            float r = pal->palData[idx].red   / 255.0f;
                            float g = pal->palData[idx].green / 255.0f;
                            float b = pal->palData[idx].blue  / 255.0f;

                            // Brighten selected
                            if (isSelected) {
                                r = fminf(r + 0.28f, 1.0f);
                                g = fminf(g + 0.28f, 1.0f);
                                b = fminf(b + 0.28f, 1.0f);
                            }

                            // Dim unused entries so they read as "not in play"
                            float dimFactor = isUsed ? 1.0f : 0.45f;

                            ImGui::SameLine(LABEL_W + col * (SWATCH + GAP), 0.0f);

                            char btnId[16]; sprintf(btnId, "##pc%d", idx);
                            ImGui::PushStyleColor(ImGuiCol_Button,
                                ImVec4(r * dimFactor, g * dimFactor, b * dimFactor, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                ImVec4(fminf(r * 1.25f, 1.0f), fminf(g * 1.25f, 1.0f), fminf(b * 1.25f, 1.0f), 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                ImVec4(r * 0.75f, g * 0.75f, b * 0.75f, 1.0f));

                            ImVec2 btnPos = ImGui::GetCursorScreenPos();
                            bool clicked  = ImGui::Button(btnId, ImVec2(SWATCH, SWATCH));
                            ImGui::PopStyleColor(3);

                            // Handle left-click selection
                            if (clicked) {
                                if (io.KeyShift && lastClickedIndex != -1) {
                                    int lo = (lastClickedIndex < idx) ? lastClickedIndex : idx;
                                    int hi = (lastClickedIndex > idx) ? lastClickedIndex : idx;
                                    if (!io.KeyCtrl)
                                        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                    for (int i = lo; i <= hi; i++) selectedIndices[i] = true;
                                } else if (io.KeyCtrl) {
                                    selectedIndices[idx] = !selectedIndices[idx];
                                    lastClickedIndex = idx;
                                } else {
                                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                    selectedIndices[idx] = true;
                                    lastClickedIndex = idx;
                                }
                            }

                            // Right-click context menu
                            char ctxId[24]; sprintf(ctxId, "##ctx%d", idx);
                            if (ImGui::BeginPopupContextItem(ctxId)) {
                                char ctxTitle[32]; sprintf(ctxTitle, "Index 0x%02X (%d)", idx, idx);
                                ImGui::Text("%s", ctxTitle);
                                ImGui::Separator();

                                if (ImGui::MenuItem("Select This")) {
                                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                                    selectedIndices[idx] = true;
                                    lastClickedIndex = idx;
                                    activeTab = 0;
                                }
                                if (ImGui::MenuItem(pal->palData[idx].remap ? "Mark as Unused" : "Mark as Used")) {
                                    pal->palData[idx].remap = pal->palData[idx].remap ? 0 : 1;
                                    paletteModified = true;
                                    datasaved = false;
                                    RefreshDisplay();
                                }
                                ImGui::Separator();
                                if (ImGui::MenuItem("Copy Color")) {
                                    clipboardColors.clear();
                                    clipboardColors.push_back({ pal->palData[idx].red,
                                                                pal->palData[idx].green,
                                                                pal->palData[idx].blue,
                                                                pal->palData[idx].remap });
                                    clipboardStart = idx;
                                }
                                if (hasClipboard && ImGui::MenuItem("Paste Color Here")) {
                                    pal->palData[idx].red   = clipboardColors[0].r;
                                    pal->palData[idx].green = clipboardColors[0].g;
                                    pal->palData[idx].blue  = clipboardColors[0].b;
                                    paletteModified = true;
                                    datasaved = false;
                                    RefreshDisplay();
                                }
                                ImGui::EndPopup();
                            }

                            // Draw selection border (yellow)
                            if (isSelected) {
                                ImDrawList* dl = ImGui::GetWindowDrawList();
                                dl->AddRect(btnPos,
                                    ImVec2(btnPos.x + SWATCH, btnPos.y + SWATCH),
                                    IM_COL32(255, 220, 0, 255), 0.0f, 0, 2.0f);
                            }

                            // Unused slots get a diagonal strikethrough so it's obvious at a glance
                            if (!isUsed) {
                                ImDrawList* dl = ImGui::GetWindowDrawList();
                                // Single diagonal line corner to corner, semi-transparent dark
                                dl->AddLine(
                                    ImVec2(btnPos.x + 2.0f,          btnPos.y + 2.0f),
                                    ImVec2(btnPos.x + SWATCH - 2.0f, btnPos.y + SWATCH - 2.0f),
                                    IM_COL32(0, 0, 0, 160), 1.5f);
                                // Small "unused" dot in bottom-right corner
                                dl->AddCircleFilled(
                                    ImVec2(btnPos.x + SWATCH - 5.0f, btnPos.y + SWATCH - 5.0f),
                                    3.0f, IM_COL32(180, 50, 50, 220));
                            }

                            // Tooltip
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip(
                                    "Index 0x%02X (%d)\nRGB(%d, %d, %d)\n%s%s",
                                    idx, idx,
                                    pal->palData[idx].red,
                                    pal->palData[idx].green,
                                    pal->palData[idx].blue,
                                    isUsed
                                        ? "Used  — included in game palette"
                                        : "Unused  — skipped by game (right-click to mark as Used)",
                                    isSelected ? "\n[SELECTED]" : "");
                            }
                        } // col
                    } // row

                    // Legend
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    DisabledText("Click=select  Shift+click=range  Ctrl+click=add  Right-click=menu");

                    // Unused swatch example
                    ImGui::Spacing();
                    {
                        const float SZ = 14.0f;
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        // dimmed gray swatch
                        dl->AddRectFilled(p, ImVec2(p.x + SZ, p.y + SZ), IM_COL32(60, 60, 60, 255));
                        dl->AddRect      (p, ImVec2(p.x + SZ, p.y + SZ), IM_COL32(100,100,100,200));
                        // diagonal
                        dl->AddLine(ImVec2(p.x+1, p.y+1), ImVec2(p.x+SZ-1, p.y+SZ-1), IM_COL32(0,0,0,160), 1.5f);
                        // red dot
                        dl->AddCircleFilled(ImVec2(p.x+SZ-4, p.y+SZ-4), 3.0f, IM_COL32(180,50,50,220));
                        ImGui::Dummy(ImVec2(SZ + 4, SZ));
                        ImGui::SameLine();
                        DisabledText("= Unused  (game skips this slot)");

                        ImGui::SameLine(0, 16.0f);
                        ImVec2 p2 = ImGui::GetCursorScreenPos();
                        dl->AddRectFilled(p2, ImVec2(p2.x + SZ, p2.y + SZ), IM_COL32(80, 140, 80, 255));
                        dl->AddRect      (p2, ImVec2(p2.x + SZ, p2.y + SZ), IM_COL32(100,200,100,200));
                        ImGui::Dummy(ImVec2(SZ + 4, SZ));
                        ImGui::SameLine();
                        DisabledText("= Used  (game includes this color)");
                    }

                } else {
                    ErrorText("No palette loaded");
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        ImGui::EndChild(); // PalGridOuter

        ImGui::SameLine();

        // =====================================================================
        // RIGHT — Tab panel
        // =====================================================================
        if (ImGui::BeginChild("PalTools", ImVec2(TOOLS_W, 0), true)) {

            Palette* pal = GetCurrentPalette();

            const char* tabLabels[] = { "Edit", "Adjust", "File" };
            for (int t = 0; t < 3; t++) {
                if (t > 0) ImGui::SameLine();
                bool active = (activeTab == t);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.75f, 1.0f));
                if (ImGui::Button(tabLabels[t], ImVec2(TOOLS_W / 3.0f - 6.0f, 30.0f)))
                    activeTab = t;
                if (active) ImGui::PopStyleColor();
            }
            ImGui::Separator();
            ImGui::Spacing();

            // =================================================================
            // TAB 0 — Edit
            // =================================================================
            if (activeTab == 0) {

                // --- Selection info ---
                if (selectedCount == 0) {
                    DisabledText("No selection — click palette grid");
                } else if (selectedCount == 1) {
                    char buf[48]; sprintf(buf, "Selected: Index 0x%02X (%d)", firstSel, firstSel);
                    SuccessText(buf);
                } else {
                    char buf[64]; sprintf(buf, "Selected: %d indices  (0x%02X – 0x%02X)",
                                          selectedCount, firstSel, lastSel);
                    SuccessText(buf);
                }
                ImGui::Spacing();

                // Quick selection buttons
                if (ImGui::Button("All",    ImVec2(50, 0))) std::fill(selectedIndices.begin(), selectedIndices.end(), true);
                ImGui::SameLine();
                if (ImGui::Button("None",   ImVec2(50, 0))) {
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    lastClickedIndex = -1;
                }
                ImGui::SameLine();
                if (ImGui::Button("Invert", ImVec2(55, 0)))
                    for (int i = 0; i < 256; i++) selectedIndices[i] = !selectedIndices[i];

                ImGui::Spacing();

                // Range selection
                static int rangeA = 0, rangeB = 255;
                ImGui::Text("Range:");
                ImGui::PushItemWidth(60);
                ImGui::SameLine();
                if (ImGui::InputInt("##rA", &rangeA)) rangeA = realmpal_clamp_int(rangeA, 0, 255);
                ImGui::SameLine(); ImGui::Text("–");
                ImGui::SameLine();
                if (ImGui::InputInt("##rB", &rangeB)) rangeB = realmpal_clamp_int(rangeB, rangeA, 255);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Select##range")) {
                    std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                    for (int i = rangeA; i <= rangeB; i++) selectedIndices[i] = true;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // --- Single-color editor (shown when exactly 1 index selected) ---
                if (selectedCount == 1 && pal) {
                    HeaderText("Color Editor");
                    ImGui::Spacing();

                    uint8_t& cr = pal->palData[firstSel].red;
                    uint8_t& cg = pal->palData[firstSel].green;
                    uint8_t& cb = pal->palData[firstSel].blue;

                    // Color preview swatch
                    ImVec2 swatchPos = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(48, 48));
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(swatchPos, ImVec2(swatchPos.x + 48, swatchPos.y + 48),
                        IM_COL32(cr, cg, cb, 255));
                    dl->AddRect(swatchPos, ImVec2(swatchPos.x + 48, swatchPos.y + 48),
                        IM_COL32(160, 160, 160, 255));
                    ImGui::SameLine();

                    // RGB sliders
                    ImGui::BeginGroup();
                    int r = cr, g = cg, b = cb;
                    bool changed = false;
                    ImGui::PushItemWidth(140);
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.9f,0.3f,0.3f,1.0f));
                    if (ImGui::SliderInt("R##ce", &r, 0, 255)) changed = true;
                    ImGui::PopStyleColor();
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.3f,0.9f,0.3f,1.0f));
                    if (ImGui::SliderInt("G##ce", &g, 0, 255)) changed = true;
                    ImGui::PopStyleColor();
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.3f,0.5f,1.0f,1.0f));
                    if (ImGui::SliderInt("B##ce", &b, 0, 255)) changed = true;
                    ImGui::PopStyleColor();
                    ImGui::PopItemWidth();
                    if (changed) {
                        cr = (uint8_t)r; cg = (uint8_t)g; cb = (uint8_t)b;
                        paletteModified = true;
                        datasaved = false;
                        RefreshDisplay();
                    }
                    ImGui::EndGroup();

                    // Hex input
                    ImGui::Spacing();
                    static char hexBuf[8] = "";
                    RGBToHex(cr, cg, cb, hexBuf);
                    ImGui::Text("#"); ImGui::SameLine(0,2);
                    ImGui::PushItemWidth(65);
                    if (ImGui::InputText("##hexin", hexBuf, 7, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        uint8_t nr, ng, nb;
                        if (HexToRGB(hexBuf, nr, ng, nb)) {
                            cr = nr; cg = ng; cb = nb;
                            paletteModified = true;
                            datasaved = false;
                            RefreshDisplay();
                        }
                    }
                    ImGui::PopItemWidth();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }

                // --- Used / Unused controls ---
                HeaderText("Color Slot Usage");
                ImGui::Spacing();
                InfoText("Used = game includes this color.  Unused = game skips it.");
                ImGui::Spacing();

                if (pal) {
                    // Show usage stats for current selection
                    if (hasSelection) {
                        int usedCount = 0;
                        for (int i = 0; i < 256; i++)
                            if (selectedIndices[i] && pal->palData[i].remap != 0) usedCount++;
                        char buf[80];
                        sprintf(buf, "%d / %d selected are Used", usedCount, selectedCount);
                        InfoText(buf);
                        ImGui::Spacing();
                    }

                    // Per-selection buttons
                    bool canEditSel = hasSelection;
                    if (!canEditSel) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);

                    if (ImGui::Button("Mark Selected as Used", ImVec2(-1, 0)) && canEditSel) {
                        for (int i = 0; i < 256; i++)
                            if (selectedIndices[i]) pal->palData[i].remap = 1;
                        paletteModified = true; datasaved = false; RefreshDisplay();
                        statusMessage = "Selected slots marked as Used"; showStatus = true;
                    }
                    if (ImGui::Button("Mark Selected as Unused", ImVec2(-1, 0)) && canEditSel) {
                        for (int i = 0; i < 256; i++)
                            if (selectedIndices[i]) pal->palData[i].remap = 0;
                        paletteModified = true; datasaved = false; RefreshDisplay();
                        statusMessage = "Selected slots marked as Unused"; showStatus = true;
                    }

                    if (!canEditSel) ImGui::PopStyleVar();

                    ImGui::Spacing();

                    // Whole-palette operations
                    if (ImGui::Button("Auto-Detect Used From Current Cell", ImVec2(-1, 0))) {
                        if (curCell && *curCell) {
                            Cell* cell = *curCell;
                            if (!cell->bmInfo || !cell->bmImage)
                                cell->GetImage(&cell->bmInfo, &cell->bmImage);
                            if (cell->bmImage && cell->bmInfo) {
                                bool used[256] = {};
                                int w = cell->bmInfo->bmiHeader.biWidth;
                                int h = abs(cell->bmInfo->bmiHeader.biHeight);
                                int rowStride = (w + 3) & ~3;
                                for (int row = 0; row < h; row++)
                                    for (int col = 0; col < w; col++)
                                        used[cell->bmImage[row * rowStride + col]] = true;
                                pal->ApplyUsageFlags(used);
                                pal->RecalculateHeaderRange();
                                pal->EnsureValidPalCount();
                                paletteModified = true; datasaved = false; RefreshDisplay();
                                statusMessage = "Usage auto-detected from current cell pixels";
                                showStatus = true;
                            } else {
                                statusMessage = "No cell image data available";
                                showStatus = true;
                            }
                        }
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Scans the current cell's pixels and marks slots\n"
                                          "as Used only if the image actually references them.\n"
                                          "All other slots are marked Unused.");

                    if (ImGui::Button("Mark All 256 as Used", ImVec2(-1, 0))) {
                        for (int i = 0; i < 256; i++) pal->palData[i].remap = 1;
                        pal->Head.startOffset = 0;
                        pal->Head.nColors     = 256;
                        pal->EnsureValidPalCount();
                        paletteModified = true; datasaved = false; RefreshDisplay();
                        statusMessage = "All 256 slots marked as Used"; showStatus = true;
                    }
                    if (ImGui::Button("Mark All 256 as Unused", ImVec2(-1, 0))) {
                        for (int i = 0; i < 256; i++) pal->palData[i].remap = 0;
                        paletteModified = true; datasaved = false; RefreshDisplay();
                        statusMessage = "All slots marked as Unused"; showStatus = true;
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // --- Copy / Paste ---
                HeaderText("Copy / Paste");
                ImGui::Spacing();

                bool canCopy  = hasSelection && pal;
                bool canPaste = hasClipboard && hasSelection && pal;

                if (!canCopy) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
                if (ImGui::Button("Copy Selected##cp", ImVec2(110, 0)) && canCopy) {
                    clipboardColors.clear();
                    clipboardStart = firstSel;
                    for (int i = firstSel; i <= lastSel; i++) {
                        if (selectedIndices[i])
                            clipboardColors.push_back({ pal->palData[i].red,
                                                        pal->palData[i].green,
                                                        pal->palData[i].blue,
                                                        pal->palData[i].remap });
                    }
                    char buf[64]; sprintf(buf, "Copied %d colors", (int)clipboardColors.size());
                    statusMessage = buf; showStatus = true;
                }
                if (!canCopy) ImGui::PopStyleVar();

                ImGui::SameLine();

                if (!canPaste) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
                if (ImGui::Button("Paste Here##cp", ImVec2(110, 0)) && canPaste) {
                    int pasteCount = 0;
                    for (int i = 0; i < (int)clipboardColors.size() && (firstSel + i) < 256; i++) {
                        pal->palData[firstSel + i].red   = clipboardColors[i].r;
                        pal->palData[firstSel + i].green = clipboardColors[i].g;
                        pal->palData[firstSel + i].blue  = clipboardColors[i].b;
                        pasteCount++;
                    }
                    paletteModified = true; datasaved = false; RefreshDisplay();
                    char buf[64]; sprintf(buf, "Pasted %d colors at index %d", pasteCount, firstSel);
                    statusMessage = buf; showStatus = true;
                }
                if (!canPaste) ImGui::PopStyleVar();

                if (hasClipboard) {
                    ImGui::Spacing();
                    char buf[64];
                    sprintf(buf, "Clipboard: %d colors from index %d", (int)clipboardColors.size(), clipboardStart);
                    InfoText(buf);
                } else {
                    ImGui::Spacing();
                    DisabledText("Clipboard empty");
                }

            } // TAB 0 — Edit

            // =================================================================
            // TAB 1 — Adjust
            // =================================================================
            else if (activeTab == 1) {

                if (!hasSelection || !pal) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    DisabledText("Select palette indices to adjust");
                    ImGui::PopStyleVar();
                } else {

                    static float brightness   = 0.0f;
                    static float contrast     = 0.0f;
                    static float hueShift     = 0.0f;
                    static float satFactor    = 1.0f;
                    static float colorTintR   = 0.0f;
                    static float colorTintG   = 0.0f;
                    static float colorTintB   = 0.0f;
                    static RGB8  adjustBackup[256];
                    static bool  adjustBackupValid = false;

                    // Store backup when first entering this tab with a selection
                    if (!adjustBackupValid) {
                        for (int i = 0; i < 256; i++) {
                            adjustBackup[i] = { pal->palData[i].red,
                                                pal->palData[i].green,
                                                pal->palData[i].blue };
                        }
                        adjustBackupValid = true;
                    }

                    HeaderText("Adjustments");
                    ImGui::Spacing();

                    bool anyChanged = false;
                    ImGui::PushItemWidth(160);
                    if (ImGui::SliderFloat("Brightness",  &brightness, -1.0f, 1.0f, "%.2f")) anyChanged = true;
                    if (ImGui::SliderFloat("Contrast",    &contrast,   -1.0f, 1.0f, "%.2f")) anyChanged = true;
                    if (ImGui::SliderFloat("Hue Shift",   &hueShift,  -180.0f, 180.0f, "%.0f°")) anyChanged = true;
                    if (ImGui::SliderFloat("Saturation",  &satFactor,  0.0f, 2.0f, "%.2f")) anyChanged = true;
                    ImGui::Spacing();
                    ImGui::Text("Color Tint:");
                    if (ImGui::SliderFloat("Red##tint",   &colorTintR, -1.0f, 1.0f, "%.2f")) anyChanged = true;
                    if (ImGui::SliderFloat("Green##tint", &colorTintG, -1.0f, 1.0f, "%.2f")) anyChanged = true;
                    if (ImGui::SliderFloat("Blue##tint",  &colorTintB, -1.0f, 1.0f, "%.2f")) anyChanged = true;
                    ImGui::PopItemWidth();

                    if (anyChanged) {
                        // Apply from backup so sliders are non-destructive
                        for (int i = 0; i < 256; i++) {
                            if (!selectedIndices[i]) continue;

                            float r = adjustBackup[i].r / 255.0f;
                            float g = adjustBackup[i].g / 255.0f;
                            float b = adjustBackup[i].b / 255.0f;

                            r += brightness; g += brightness; b += brightness;

                            if (contrast != 0.0f) {
                                float cf = 1.0f + contrast;
                                r = (r - 0.5f) * cf + 0.5f;
                                g = (g - 0.5f) * cf + 0.5f;
                                b = (b - 0.5f) * cf + 0.5f;
                            }

                            r += colorTintR; g += colorTintG; b += colorTintB;

                            r = fmaxf(0.0f, fminf(1.0f, r));
                            g = fmaxf(0.0f, fminf(1.0f, g));
                            b = fmaxf(0.0f, fminf(1.0f, b));

                            if (hueShift != 0.0f || satFactor != 1.0f) {
                                float h, s, l;
                                PalMgr_RGBtoHSL(r, g, b, h, s, l);
                                h += hueShift / 360.0f;
                                while (h < 0.0f) h += 1.0f;
                                while (h > 1.0f) h -= 1.0f;
                                s *= satFactor;
                                s = fmaxf(0.0f, fminf(1.0f, s));
                                PalMgr_HSLtoRGB(h, s, l, r, g, b);
                            }

                            pal->palData[i].red   = (uint8_t)(r * 255);
                            pal->palData[i].green = (uint8_t)(g * 255);
                            pal->palData[i].blue  = (uint8_t)(b * 255);
                        }
                        paletteModified = true;
                        datasaved = false;
                        RefreshDisplay();
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Reset Adjustments", ImVec2(-1, 0))) {
                        brightness = 0.0f; contrast = 0.0f;
                        hueShift   = 0.0f; satFactor = 1.0f;
                        colorTintR = 0.0f; colorTintG = 0.0f; colorTintB = 0.0f;
                        // Restore backup for selected indices
                        for (int i = 0; i < 256; i++) {
                            if (!selectedIndices[i]) continue;
                            pal->palData[i].red   = adjustBackup[i].r;
                            pal->palData[i].green = adjustBackup[i].g;
                            pal->palData[i].blue  = adjustBackup[i].b;
                        }
                        adjustBackupValid = false;
                        paletteModified = true; datasaved = false; RefreshDisplay();
                    }

                    // Re-seed backup if selection changes
                    static int prevSelCount = -1;
                    if (selectedCount != prevSelCount) {
                        adjustBackupValid = false;
                        prevSelCount = selectedCount;
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // --- Quick presets ---
                    HeaderText("Quick Presets");
                    ImGui::Spacing();

                    struct SimplePreset { const char* name; float br, co, hs, sa, tr, tg, tb; };
                    static const SimplePreset presets[] = {
                        { "Normal",       0.0f, 0.0f,   0.0f, 1.0f,  0.0f, 0.0f, 0.0f },
                        { "Darken",      -0.25f, 0.1f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f },
                        { "Brighten",     0.25f,-0.1f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f },
                        { "Desaturate",   0.0f, 0.0f,   0.0f, 0.0f,  0.0f, 0.0f, 0.0f },
                        { "Warm",         0.05f,0.0f,  10.0f, 1.0f,  0.15f,0.05f,-0.1f},
                        { "Cool",        -0.05f,0.0f,  -10.0f,1.0f, -0.1f,0.0f, 0.15f },
                        { "Night",       -0.35f,0.15f,  0.0f, 0.55f,-0.05f,0.0f, 0.1f },
                    };
                    for (auto& p : presets) {
                        if (ImGui::Button(p.name, ImVec2(80, 0))) {
                            brightness = p.br; contrast = p.co;
                            hueShift   = p.hs; satFactor = p.sa;
                            colorTintR = p.tr; colorTintG = p.tg; colorTintB = p.tb;
                            adjustBackupValid = false; // re-seed from current state
                        }
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // --- Range swap ---
                    HeaderText("Swap Ranges");
                    ImGui::Spacing();
                    InfoText("Swap two palette ranges of equal size");
                    ImGui::Spacing();

                    static int swapA1 = 0,  swapA2 = 15;
                    static int swapB1 = 16, swapB2 = 31;
                    ImGui::PushItemWidth(55);
                    ImGui::Text("Range A:"); ImGui::SameLine();
                    if (ImGui::InputInt("##sa1", &swapA1)) swapA1 = realmpal_clamp_int(swapA1, 0, 255);
                    ImGui::SameLine(); ImGui::Text("–"); ImGui::SameLine();
                    if (ImGui::InputInt("##sa2", &swapA2)) swapA2 = realmpal_clamp_int(swapA2, swapA1, 255);
                    ImGui::Text("Range B:"); ImGui::SameLine();
                    if (ImGui::InputInt("##sb1", &swapB1)) swapB1 = realmpal_clamp_int(swapB1, 0, 255);
                    ImGui::SameLine(); ImGui::Text("–"); ImGui::SameLine();
                    if (ImGui::InputInt("##sb2", &swapB2)) swapB2 = realmpal_clamp_int(swapB2, swapB1, 255);
                    ImGui::PopItemWidth();

                    int sizeA = swapA2 - swapA1 + 1;
                    int sizeB = swapB2 - swapB1 + 1;
                    bool canSwap = (sizeA == sizeB) && pal &&
                                   !(swapA1 <= swapB2 && swapB1 <= swapA2); // no overlap
                    if (!canSwap) {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
                        if (sizeA != sizeB) DisabledText("Ranges must be the same size");
                        else if (swapA1 <= swapB2 && swapB1 <= swapA2) DisabledText("Ranges overlap");
                    }
                    if (ImGui::Button("Swap A \xe2\x86\x94 B", ImVec2(-1, 0)) && canSwap) {
                        // Simple in-place swap
                        for (int i = 0; i < sizeA; i++) {
                            PalEntry tmp               = pal->palData[swapA1 + i];
                            pal->palData[swapA1 + i]   = pal->palData[swapB1 + i];
                            pal->palData[swapB1 + i]   = tmp;
                        }
                        paletteModified = true; datasaved = false; RefreshDisplay();
                        char msg[64]; sprintf(msg, "Swapped %d-%d with %d-%d", swapA1, swapA2, swapB1, swapB2);
                        statusMessage = msg; showStatus = true;
                    }
                    if (!canSwap) ImGui::PopStyleVar();

                } // hasSelection

            } // TAB 1 — Adjust

            // =================================================================
            // TAB 2 — File
            // =================================================================
            else if (activeTab == 2) {

                // --- Import ---
                HeaderText("Import Palette");
                ImGui::Spacing();

                if (!g_palMgrInputFile.empty()) {
                    const char* fn = strrchr(g_palMgrInputFile.c_str(), '\\');
                    InfoText(fn ? fn + 1 : g_palMgrInputFile.c_str());
                } else {
                    DisabledText("No file selected");
                }
                if (ImGui::Button("Browse...##import", ImVec2(-1, 0)))
                    g_requestPalMgrInputDialog = true;

                ImGui::Spacing();
                bool canImport = !g_palMgrInputFile.empty() && pal;
                if (!canImport) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
                if (ImGui::Button("Import Palette##do", ImVec2(-1, 0)) && canImport) {
                    realmpal_clear_error();
                    RGB8 importedPal[256];
                    int colors = realmpal_read_any_palette(g_palMgrInputFile.c_str(), importedPal, 256);
                    if (colors > 0) {
                        for (int i = 0; i < colors; i++) {
                            pal->palData[i].red   = importedPal[i].r;
                            pal->palData[i].green = importedPal[i].g;
                            pal->palData[i].blue  = importedPal[i].b;
                        }
                        paletteModified = true; datasaved = false;
                        std::fill(selectedIndices.begin(), selectedIndices.end(), false);
                        RefreshDisplay();
                        char msg[64]; sprintf(msg, "Imported %d colors", colors);
                        statusMessage = msg; showStatus = true;
                    } else {
                        const char* err = realmpal_get_last_error();
                        char msg[256]; sprintf(msg, "Import failed: %s", err ? err : "unknown error");
                        statusMessage = msg; showStatus = true;
                    }
                }
                if (!canImport) ImGui::PopStyleVar();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // --- Export ---
                HeaderText("Export Palette");
                ImGui::Spacing();

                if (ImGui::Button("Export To...##export", ImVec2(-1, 0)))
                    g_requestPalMgrOutputDialog = true;

                if (!g_palMgrOutputFile.empty() && pal) {
                    const char* fn = strrchr(g_palMgrOutputFile.c_str(), '\\');
                    InfoText(fn ? fn + 1 : g_palMgrOutputFile.c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Export Now##do", ImVec2(-1, 0))) {
                        uint8_t indices[256];
                        RGB8 exportPal[256];
                        for (int i = 0; i < 256; i++) {
                            indices[i]    = (uint8_t)i;
                            exportPal[i]  = { pal->palData[i].red,
                                              pal->palData[i].green,
                                              pal->palData[i].blue };
                        }
                        realmpal_clear_error();
                        if (realmpal_write_auto(g_palMgrOutputFile.c_str(), 16, 16, indices, exportPal)) {
                            statusMessage = "Palette exported"; showStatus = true;
                        } else {
                            const char* err = realmpal_get_last_error();
                            char msg[256]; sprintf(msg, "Export failed: %s", err ? err : "unknown error");
                            statusMessage = msg; showStatus = true;
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // --- Palette analysis ---
                HeaderText("Palette Analysis");
                ImGui::Spacing();

                if (ImGui::Button("Analyze Current Cell", ImVec2(-1, 0)) && pal) {
                    uint8_t* pixData  = nullptr;
                    int pixCount = 0;
                    if (curCell && *curCell) {
                        if (!(*curCell)->bmInfo || !(*curCell)->bmImage)
                            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
                        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
                            pixData  = (*curCell)->bmImage;
                            pixCount = (*curCell)->bmInfo->bmiHeader.biWidth *
                                       abs((*curCell)->bmInfo->bmiHeader.biHeight);
                        }
                    }
                    RGB8 analysisPal[256];
                    for (int i = 0; i < 256; i++) {
                        analysisPal[i] = { pal->palData[i].red,
                                           pal->palData[i].green,
                                           pal->palData[i].blue };
                    }
                    if (realmpal_palette_analyze(analysisPal, pixData, pixCount, &paletteStats))
                        statsValid = true;
                    else
                        statusMessage = "Analysis failed"; showStatus = true;
                }

                if (statsValid) {
                    ImGui::Spacing();
                    char buf[80];
                    sprintf(buf, "Unique colors:    %d", paletteStats.unique_colors);  InfoText(buf);
                    if (paletteStats.used_colors > 0) {
                        sprintf(buf, "Used by image:    %d", paletteStats.used_colors); InfoText(buf);
                    }
                    sprintf(buf, "Avg luminance:    %.2f", paletteStats.average_luminance); InfoText(buf);
                    sprintf(buf, "Darkest index:    %d",   paletteStats.darkest_index);     InfoText(buf);
                    sprintf(buf, "Brightest index:  %d",   paletteStats.brightest_index);   InfoText(buf);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // --- Palette header diagnostics ---
                HeaderText("Header Info");
                ImGui::Spacing();
                if (pal) {
                    char buf[80];
                    sprintf(buf, "palCount:     %d%s", (int)(unsigned char)pal->Head.palCount,
                            pal->Head.palCount == 0 ? "  *** ZERO — game won't load! ***" : "");
                    if (pal->Head.palCount == 0) WarningText(buf); else InfoText(buf);

                    sprintf(buf, "valid:        %u%s", pal->Head.valid,
                            pal->Head.valid == 0 ? "  *** ZERO — palette may not load! ***" : "");
                    if (pal->Head.valid == 0) WarningText(buf); else InfoText(buf);

                    sprintf(buf, "type:         %d  (%s)", (int)pal->Head.type,
                            pal->Head.type == 0 ? "per-entry flags" : "shared def flag");
                    InfoText(buf);

                    sprintf(buf, "startOffset:  %d", (int)pal->Head.startOffset); InfoText(buf);
                    sprintf(buf, "nColors:      %d", (int)pal->Head.nColors);     InfoText(buf);

                    // Count used entries
                    int usedCount = 0;
                    for (int i = 0; i < 256; i++)
                        if (pal->palData[i].remap != 0) usedCount++;
                    sprintf(buf, "Used slots:  %d / 256", usedCount);
                    InfoText(buf);

                    ImGui::Spacing();
                    if (ImGui::Button("Fix Header (palCount/valid)", ImVec2(-1, 0))) {
                        pal->EnsureValidPalCount();
                        pal->RecalculateHeaderRange();
                        paletteModified = true; datasaved = false;
                        statusMessage = "Header fields corrected"; showStatus = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Sets palCount=1, valid=1, type=0 and recalculates\n"
                                          "startOffset/nColors to cover only Used slots.");
                }

            } // TAB 2 — File

            ImGui::Spacing();

            // Status message (auto-hide)
            if (showStatus && !statusMessage.empty()) {
                ImGui::Separator();
                ImGui::Spacing();
                if (statusMessage.find("fail") != std::string::npos ||
                    statusMessage.find("Error") != std::string::npos ||
                    statusMessage.find("ERROR") != std::string::npos)
                    ErrorText(statusMessage.c_str());
                else if (statusMessage.find("***") != std::string::npos)
                    WarningText(statusMessage.c_str());
                else
                    SuccessText(statusMessage.c_str());

                statusCounter++;
                if (statusCounter > 300) {
                    showStatus = false;
                    statusMessage = "";
                    statusCounter = 0;
                }
            }

        }
        ImGui::EndChild(); // PalTools

    }
    ImGui::EndChild(); // PalMain

    EndDialog();

    if (shouldCloseDialog) {
        configInitialized = false;
        shouldCloseDialog = false;
        HideDialog(DIALOG_PALETTE_MANAGER);
    }
}
