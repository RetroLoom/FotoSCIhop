#pragma once
#include <windows.h>
#include <cfloat>
#include <climits>
#include <vector>

// ImGui constants we need
#define IMGUI_STYLE_VAR_ALPHA 0
#define IMGUI_STYLE_VAR_WINDOW_PADDING 1
#define IMGUI_STYLE_VAR_WINDOW_ROUNDING 2
#define IMGUI_STYLE_VAR_FRAME_PADDING 3
#define IMGUI_STYLE_VAR_FRAME_ROUNDING 4
#define IMGUI_STYLE_VAR_ITEM_SPACING 5
#define IMGUI_STYLE_VAR_ITEM_INNER_SPACING 6
#define IMGUI_STYLE_VAR_INDENT_SPACING 7

// Color constants
#define IMGUI_COL_TEXT 0
#define IMGUI_COL_TEXT_DISABLED 1
#define IMGUI_COL_WINDOW_BG 2
#define IMGUI_COL_CHILD_BG 3
#define IMGUI_COL_POPUP_BG 4
#define IMGUI_COL_BORDER 5
#define IMGUI_COL_FRAME_BG 6
#define IMGUI_COL_FRAME_BG_HOVERED 7
#define IMGUI_COL_FRAME_BG_ACTIVE 8
#define IMGUI_COL_TITLE_BG 9
#define IMGUI_COL_TITLE_BG_ACTIVE 10
#define IMGUI_COL_BUTTON 11
#define IMGUI_COL_BUTTON_HOVERED 12
#define IMGUI_COL_BUTTON_ACTIVE 13
#define IMGUI_COL_HEADER 14
#define IMGUI_COL_HEADER_HOVERED 15
#define IMGUI_COL_HEADER_ACTIVE 16

// Simple callback type - your dialog rendering functions
typedef void (*ImGuiDialogCallback)(void);

// Color structure for easier color management
struct ImGuiColor {
    float r, g, b, a;
    ImGuiColor(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) 
        : r(r), g(g), b(b), a(a) {}
};

namespace ImGuiDialogs {
    
    // =========================================================================
    // ORGANIZED DIALOG MANAGEMENT
    // =========================================================================
    
    enum DialogType {
        DIALOG_PROPERTIES = 0,
        DIALOG_ABOUT = 1,
        DIALOG_CLUT_GENERATOR = 2,
        DIALOG_COUNT
    };
    
    // NEW: Organized dialog system (optional - can still use old way)
    void RegisterDialog(DialogType type, const char* title, ImGuiDialogCallback callback);
    void ShowDialog(DialogType type);
    void HideDialog(DialogType type);
    bool IsDialogOpen(DialogType type);
    
    // =========================================================================
    // All your current functions stay exactly the same
    // =========================================================================
    
    // Initialize/cleanup the ImGui system
    bool Initialize(HWND parent);
    void Shutdown();

    void Hide(); // Hides all dialogs
    
    // Check if any dialog is open (EXISTING - still works)
    bool IsAnyDialogOpen();
    
    // Call from your main message loop and timer (EXISTING - still works)
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render();
    
    // =========================================================================
    // STYLE AND THEMING - All unchanged
    // =========================================================================
    
    // Predefined themes
    enum Theme {
        THEME_DARK = 0,
        THEME_LIGHT = 1,
        THEME_CLASSIC = 2,
        THEME_PHOTOSHOP = 3, // Custom theme for your app
        THEME_HIGH_CONTRAST = 4
    };
    
    // Apply a predefined theme
    void ApplyTheme(Theme theme);
    
    // Style management
    void PushStyleVar(int var, float value);
    void PushStyleVar2(int var, float x, float y);
    void PushStyleColor(int colorId, ImGuiColor color);
    void PushStyleColor(int colorId, float r, float g, float b, float a = 1.0f);
    void PopStyleVar(int count = 1);
    void PopStyleColor(int count = 1);
    
    // Custom styling helpers
    void SetWindowRounding(float rounding);
    void SetFrameRounding(float rounding);
    void SetScrollbarRounding(float rounding);
    void SetGrabRounding(float rounding);
    
    // =========================================================================
    // BASIC LAYOUT AND WIDGETS - All unchanged
    // =========================================================================
    
    // Dialog management (backward compatible)
    bool BeginDialog(const char* title, bool* open);
    void EndDialog();
    
    // Enhanced layout functions
    bool BeginChild(const char* id, float width = 0, float height = 0, bool border = false);
    void EndChild();
    bool BeginGroup();
    void EndGroup();
    
    // Spacing and alignment
    void Separator();
    void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
    void NewLine();
    void Spacing();
    void Dummy(float width, float height);
    void Indent(float indent_w = 0.0f);
    void Unindent(float indent_w = 0.0f);
    
    // Alignment helpers
    void AlignTextToFramePadding();
    void CenterNextItem(float itemWidth);
    void RightAlignNextItem(float itemWidth);
    
    // =========================================================================
    // TEXT AND LABELS - All unchanged
    // =========================================================================
    
    // Basic text (backward compatible)
    void Text(const char* text);
    void TextFormatted(const char* fmt, ...);
    
    // Enhanced text functions
    void TextColored(ImGuiColor color, const char* text);
    void TextColored(float r, float g, float b, float a, const char* text);
    void TextDisabled(const char* text);
    void TextWrapped(const char* text);
    void LabelText(const char* label, const char* text);
    void BulletText(const char* text);
    
    // Headers and sections
    bool CollapsingHeader(const char* label, bool defaultOpen = false);
    bool TreeNode(const char* label);
    bool TreeNodeEx(const char* label, bool defaultOpen = false);
    void TreePop();
    
    // =========================================================================
    // BUTTONS AND INTERACTABLES - All unchanged
    // =========================================================================
    
    // Basic buttons (backward compatible)
    bool Button(const char* label);
    bool Button(const char* label, float width, float height);
    
    // Enhanced button functions
    bool SmallButton(const char* label);
    bool InvisibleButton(const char* str_id, float width, float height);
    bool ArrowButton(const char* str_id, int dir); // 0=left, 1=right, 2=up, 3=down
    bool ImageButton(const char* str_id, void* texture_id, float width, float height);
    
    // Button styling helpers
    bool ButtonColored(const char* label, ImGuiColor color);
    bool ButtonColored(const char* label, float r, float g, float b, float a = 1.0f);
    bool ButtonColored(const char* label, float r, float g, float b, float a, float width, float height);
    bool ButtonColored(const char* label, ImGuiColor color, float width, float height);
    
    // =========================================================================
    // INPUT WIDGETS - All unchanged
    // =========================================================================
    
    // Basic inputs (backward compatible)
    bool Checkbox(const char* label, bool* value);
    
    // Enhanced input functions
    bool InputInt(const char* label, int* value, int step = 1, int step_fast = 100);
    bool InputFloat(const char* label, float* value, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1);
    bool InputDouble(const char* label, double* value, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f");
    bool InputText(const char* label, char* buf, size_t buf_size);
    bool InputTextMultiline(const char* label, char* buf, size_t buf_size, float width = 0, float height = 0);
    
    // Sliders
    bool SliderInt(const char* label, int* value, int min_value, int max_value);
    bool SliderFloat(const char* label, float* value, float min_value, float max_value);
    bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
    
    // Drag inputs
    bool DragInt(const char* label, int* value, float v_speed = 1.0f, int v_min = 0, int v_max = 0);
    bool DragFloat(const char* label, float* value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);
    
    // Color inputs
    bool ColorEdit3(const char* label, float col[3]);
    bool ColorEdit4(const char* label, float col[4]);
    bool ColorPicker3(const char* label, float col[3]);
    bool ColorPicker4(const char* label, float col[4]);
    
    // =========================================================================
    // SELECTION WIDGETS - All unchanged
    // =========================================================================
    
    // Combo boxes
    bool BeginCombo(const char* label, const char* preview_value);
    void EndCombo();
    bool Combo(const char* label, int* current_item, const char* const items[], int items_count);
    
    // List boxes
    bool BeginListBox(const char* label, float width = 0, float height = 0);
    void EndListBox();
    bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
    
    // Selectables
    bool Selectable(const char* label, bool selected = false);
    bool Selectable(const char* label, bool* p_selected);
    
    // Radio buttons
    bool RadioButton(const char* label, bool active);
    bool RadioButton(const char* label, int* v, int v_button);
    
    // =========================================================================
    // TOOLTIPS AND POPUPS - All unchanged
    // =========================================================================
    
    // Tooltips
    void SetTooltip(const char* text);
    void SetTooltipFormatted(const char* fmt, ...);
    bool BeginTooltip();
    void EndTooltip();
    
    // Popups
    bool BeginPopup(const char* str_id);
    bool BeginPopupModal(const char* name, bool* p_open = nullptr);
    void EndPopup();
    void OpenPopup(const char* str_id);
    void CloseCurrentPopup();
    
    // =========================================================================
    // TABLES - All unchanged
    // =========================================================================
    
    bool BeginTable(const char* str_id, int column_count);
    void EndTable();
    void TableNextRow();
    bool TableNextColumn();
    void TableSetupColumn(const char* label);
    void TableHeadersRow();
    
    // =========================================================================
    // UTILITY FUNCTIONS - All unchanged
    // =========================================================================
    
    // Window management
    void SetNextWindowPos(float x, float y);
    void SetNextWindowSize(float width, float height);
    void SetNextWindowFocus();
    
    // Cursor and positioning
    void SetCursorPosX(float local_x);
    void SetCursorPosY(float local_y);
    void SetCursorPos(float local_x, float local_y);
    float GetCursorPosX();
    float GetCursorPosY();
    
    // Content region
    float GetContentRegionAvailWidth();
    float GetContentRegionAvailHeight();
    float GetWindowWidth();
    float GetWindowHeight();
    
    // Item queries
    bool IsItemHovered();
    bool IsItemActive();
    bool IsItemClicked(int mouse_button = 0);
    bool IsItemVisible();
    bool IsItemEdited();
    bool IsItemActivated();
    bool IsItemDeactivated();
    bool IsItemDeactivatedAfterEdit();
    
    // Focus management
    void SetItemDefaultFocus();
    void SetKeyboardFocusHere(int offset = 0);

    // Item width control functions
    void SetNextItemWidth(float item_width);
    void PushItemWidth(float item_width);
    void PopItemWidth();
    float CalcItemWidth();
    
    // =========================================================================
    // DRAWING AND GRAPHICS - All unchanged
    // =========================================================================
    
    // Custom drawing
    void DrawLine(float x1, float y1, float x2, float y2, ImGuiColor color, float thickness = 1.0f);
    void DrawRect(float x, float y, float width, float height, ImGuiColor color, float rounding = 0.0f, float thickness = 1.0f);
    void DrawRectFilled(float x, float y, float width, float height, ImGuiColor color, float rounding = 0.0f);
    void DrawCircle(float center_x, float center_y, float radius, ImGuiColor color, int num_segments = 12, float thickness = 1.0f);
    void DrawCircleFilled(float center_x, float center_y, float radius, ImGuiColor color, int num_segments = 12);
    void DrawText(float x, float y, ImGuiColor color, const char* text);
    
    // =========================================================================
    // CONVENIENCE FUNCTIONS - All unchanged
    // =========================================================================
    
    // Property editing helpers
    bool PropertyInt(const char* label, int* value, int min_val = INT_MIN, int max_val = INT_MAX);
    bool PropertyFloat(const char* label, float* value, float min_val = -3.402823466e+38F, float max_val = 3.402823466e+38F);
    bool PropertyBool(const char* label, bool* value);
    bool PropertyText(const char* label, char* buffer, size_t buffer_size);
    
    // Section helpers for better organization
    bool BeginPropertySection(const char* name, bool defaultOpen = true);
    void EndPropertySection();
    
    // Status and feedback
    void ShowStatus(const char* text, ImGuiColor color = ImGuiColor(1.0f, 1.0f, 1.0f, 1.0f));
    void ShowError(const char* text);
    void ShowWarning(const char* text);
    void ShowSuccess(const char* text);
    
    // Help and documentation
    void HelpMarker(const char* desc);
    void HelpTooltip(const char* desc);
    
    // Confirmation dialogs
    bool ConfirmationDialog(const char* title, const char* message, bool* open);
    
    // Progress indicators
    void ProgressBar(float fraction, float width = -1.0f, float height = 0.0f);
    void ProgressBar(float fraction, const char* overlay, float width = -1.0f, float height = 0.0f);
}