#pragma once
#include <windows.h>

// ============================================================================
// DIRECT IMGUI INTEGRATION WITH COMPLETE COMPATIBILITY LAYER
// ============================================================================
// This approach allows both ImGuiDialogs:: and ImGui:: to be used simultaneously
// Existing code keeps working, new code can use ImGui:: directly

// Include ImGui directly - now available everywhere
#include "imgui.h"

// Simple callback type - your dialog rendering functions
typedef void (*ImGuiDialogCallback)(void);

namespace ImGuiDialogs {
    
    // =========================================================================
    // DIALOG MANAGEMENT - Keep this minimal system
    // =========================================================================
    
    enum DialogType {
        DIALOG_PROPERTIES = 0,
        DIALOG_ABOUT = 1,
        DIALOG_CLUT_GENERATOR = 2,
        DIALOG_COUNT
    };
    
    // Core dialog system (unchanged)
    void RegisterDialog(DialogType type, const char* title, ImGuiDialogCallback callback);
    void ShowDialog(DialogType type);
    void HideDialog(DialogType type);
    bool IsDialogOpen(DialogType type);
    bool IsAnyDialogOpen();
    
    // =========================================================================
    // CORE ENGINE FUNCTIONS - Keep these
    // =========================================================================
    
    // Initialize/cleanup the ImGui system
    bool Initialize(HWND parent);
    void Shutdown();
    void Hide(); // Hides all dialogs
    
    // Call from your main message loop and timer
    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render();
    
    // =========================================================================
    // BASIC WINDOW MANAGEMENT - Keep minimal set
    // =========================================================================
    
    // Window management for dialog sizing
    void SetNextWindowSize(float width, float height);
    void SetNextWindowPos(float x, float y);
    void SetNextWindowFocus();
    
    // Dialog frame management - simplified
    bool BeginDialog(const char* title, bool* open);
    void EndDialog();
    
    // =========================================================================
    // THEME SYSTEM - Keep this for convenience
    // =========================================================================
    
    enum Theme {
        THEME_DARK = 0,
        THEME_LIGHT = 1,
        THEME_CLASSIC = 2,
        THEME_PHOTOSHOP = 3,
        THEME_HIGH_CONTRAST = 4
    };
    
    void ApplyTheme(Theme theme);
    
    // =========================================================================
    // COMPATIBILITY CONSTANTS - Map old wrapper constants to ImGui constants
    // =========================================================================
    // These allow existing code using IMGUI_COL_* and IMGUI_STYLE_VAR_* to work
    
    // Style Variable Constants (for PushStyleVar)
    #define IMGUI_STYLE_VAR_ALPHA                   ImGuiStyleVar_Alpha
    #define IMGUI_STYLE_VAR_WINDOW_PADDING          ImGuiStyleVar_WindowPadding
    #define IMGUI_STYLE_VAR_WINDOW_ROUNDING         ImGuiStyleVar_WindowRounding
    #define IMGUI_STYLE_VAR_FRAME_PADDING           ImGuiStyleVar_FramePadding
    #define IMGUI_STYLE_VAR_FRAME_ROUNDING          ImGuiStyleVar_FrameRounding
    #define IMGUI_STYLE_VAR_ITEM_SPACING            ImGuiStyleVar_ItemSpacing
    #define IMGUI_STYLE_VAR_ITEM_INNER_SPACING      ImGuiStyleVar_ItemInnerSpacing
    #define IMGUI_STYLE_VAR_INDENT_SPACING          ImGuiStyleVar_IndentSpacing
    
    // Color Constants (for PushStyleColor)
    #define IMGUI_COL_TEXT                          ImGuiCol_Text
    #define IMGUI_COL_TEXT_DISABLED                 ImGuiCol_TextDisabled
    #define IMGUI_COL_WINDOW_BG                     ImGuiCol_WindowBg
    #define IMGUI_COL_CHILD_BG                      ImGuiCol_ChildBg
    #define IMGUI_COL_POPUP_BG                      ImGuiCol_PopupBg
    #define IMGUI_COL_BORDER                        ImGuiCol_Border
    #define IMGUI_COL_FRAME_BG                      ImGuiCol_FrameBg
    #define IMGUI_COL_FRAME_BG_HOVERED              ImGuiCol_FrameBgHovered
    #define IMGUI_COL_FRAME_BG_ACTIVE               ImGuiCol_FrameBgActive
    #define IMGUI_COL_TITLE_BG                      ImGuiCol_TitleBg
    #define IMGUI_COL_TITLE_BG_ACTIVE               ImGuiCol_TitleBgActive
    #define IMGUI_COL_BUTTON                        ImGuiCol_Button
    #define IMGUI_COL_BUTTON_HOVERED                ImGuiCol_ButtonHovered
    #define IMGUI_COL_BUTTON_ACTIVE                 ImGuiCol_ButtonActive
    #define IMGUI_COL_HEADER                        ImGuiCol_Header
    #define IMGUI_COL_HEADER_HOVERED                ImGuiCol_HeaderHovered
    #define IMGUI_COL_HEADER_ACTIVE                 ImGuiCol_HeaderActive
    
    // Legacy color structure for compatibility
    struct ImGuiColor {
        float r, g, b, a;
        ImGuiColor(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) 
            : r(r), g(g), b(b), a(a) {}
    };
    
    // =========================================================================
    // COMPATIBILITY HELPER FUNCTIONS
    // =========================================================================
    
    // These functions existed in the old wrapper but aren't part of core ImGui
    inline void SetWindowRounding(float rounding) {
        ImGui::GetStyle().WindowRounding = rounding;
    }
    
    inline void SetFrameRounding(float rounding) {
        ImGui::GetStyle().FrameRounding = rounding;
    }
    
    inline void SetScrollbarRounding(float rounding) {
        ImGui::GetStyle().ScrollbarRounding = rounding;
    }
    
    inline void SetGrabRounding(float rounding) {
        ImGui::GetStyle().GrabRounding = rounding;
    }
    
    // =========================================================================
    // COMPATIBILITY LAYER - Inline pass-through functions
    // =========================================================================
    // These allow existing ImGuiDialogs:: code to keep working
    // while providing direct access to ImGui functionality
    
    // -------------------------------------------------------------------------
    // LAYOUT FUNCTIONS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline bool BeginChild(const char* id, float width = 0, float height = 0, bool border = false) {
        return ImGui::BeginChild(id, ImVec2(width, height), border);
    }
    
    inline void EndChild() {
        ImGui::EndChild();
    }
    
    inline void BeginGroup() {
        ImGui::BeginGroup();
    }
    
    inline void EndGroup() {
        ImGui::EndGroup();
    }
    
    inline void Separator() {
        ImGui::Separator();
    }
    
    inline void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f) {
        ImGui::SameLine(offset_from_start_x, spacing);
    }
    
    inline void NewLine() {
        ImGui::NewLine();
    }
    
    inline void Spacing() {
        ImGui::Spacing();
    }
    
    inline void Dummy(float width, float height) {
        ImGui::Dummy(ImVec2(width, height));
    }
    
    inline void Indent(float indent_w = 0.0f) {
        ImGui::Indent(indent_w);
    }
    
    inline void Unindent(float indent_w = 0.0f) {
        ImGui::Unindent(indent_w);
    }
    
    inline void AlignTextToFramePadding() {
        ImGui::AlignTextToFramePadding();
    }
    
    // -------------------------------------------------------------------------
    // TEXT FUNCTIONS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline void Text(const char* text) {
        ImGui::Text("%s", text);
    }
    
    inline void TextColored(float r, float g, float b, float a, const char* text) {
        ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
    }
    
    inline void TextColored(ImGuiColor color, const char* text) {
        ImGui::TextColored(ImVec4(color.r, color.g, color.b, color.a), "%s", text);
    }
    
    inline void TextDisabled(const char* text) {
        ImGui::TextDisabled("%s", text);
    }
    
    inline void TextWrapped(const char* text) {
        ImGui::TextWrapped("%s", text);
    }
    
    inline void LabelText(const char* label, const char* text) {
        ImGui::LabelText(label, "%s", text);
    }
    
    inline void BulletText(const char* text) {
        ImGui::BulletText("%s", text);
    }
    
    inline bool CollapsingHeader(const char* label, bool defaultOpen = false) {
        ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::CollapsingHeader(label, flags);
    }
    
    inline bool TreeNode(const char* label) {
        return ImGui::TreeNode(label);
    }
    
    inline bool TreeNodeEx(const char* label, bool defaultOpen = false) {
        ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::TreeNodeEx(label, flags);
    }
    
    inline void TreePop() {
        ImGui::TreePop();
    }
    
    // -------------------------------------------------------------------------
    // BUTTON FUNCTIONS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline bool Button(const char* label) {
        return ImGui::Button(label);
    }
    
    inline bool Button(const char* label, float width, float height) {
        return ImGui::Button(label, ImVec2(width, height));
    }
    
    inline bool SmallButton(const char* label) {
        return ImGui::SmallButton(label);
    }
    
    inline bool InvisibleButton(const char* str_id, float width, float height) {
        return ImGui::InvisibleButton(str_id, ImVec2(width, height));
    }
    
    inline bool ArrowButton(const char* str_id, int dir) {
        return ImGui::ArrowButton(str_id, (ImGuiDir)dir);
    }
    
    // Enhanced button with color - simplified implementation
    inline bool ButtonColored(const char* label, float r, float g, float b, float a = 1.0f) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, a));
        bool result = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return result;
    }
    
    inline bool ButtonColored(const char* label, float r, float g, float b, float a, float width, float height) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, a));
        bool result = ImGui::Button(label, ImVec2(width, height));
        ImGui::PopStyleColor(3);
        return result;
    }
    
    // -------------------------------------------------------------------------
    // INPUT WIDGETS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline bool InputInt(const char* label, int* value, int step = 1, int step_fast = 100) {
        return ImGui::InputInt(label, value, step, step_fast);
    }
    
    inline bool InputFloat(const char* label, float* value, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1) {
        return ImGui::InputFloat(label, value, step, step_fast, "%.3f");
    }
    
    inline bool InputDouble(const char* label, double* value, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f") {
        return ImGui::InputDouble(label, value, step, step_fast, format);
    }
    
    inline bool InputText(const char* label, char* buf, size_t buf_size) {
        return ImGui::InputText(label, buf, buf_size);
    }
    
    inline bool InputTextMultiline(const char* label, char* buf, size_t buf_size, float width = 0, float height = 0) {
        return ImGui::InputTextMultiline(label, buf, buf_size, ImVec2(width, height));
    }
    
    inline bool Checkbox(const char* label, bool* value) {
        return ImGui::Checkbox(label, value);
    }
    
    inline bool SliderInt(const char* label, int* value, int min_value, int max_value) {
        return ImGui::SliderInt(label, value, min_value, max_value);
    }
    
    inline bool SliderFloat(const char* label, float* value, float min_value, float max_value) {
        return ImGui::SliderFloat(label, value, min_value, max_value);
    }
    
    inline bool SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f) {
        return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max);
    }
    
    inline bool DragInt(const char* label, int* value, float v_speed = 1.0f, int v_min = 0, int v_max = 0) {
        return ImGui::DragInt(label, value, v_speed, v_min, v_max);
    }
    
    inline bool DragFloat(const char* label, float* value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f) {
        return ImGui::DragFloat(label, value, v_speed, v_min, v_max);
    }
    
    inline bool ColorEdit3(const char* label, float col[3]) {
        return ImGui::ColorEdit3(label, col);
    }
    
    inline bool ColorEdit4(const char* label, float col[4]) {
        return ImGui::ColorEdit4(label, col);
    }
    
    inline bool ColorPicker3(const char* label, float col[3]) {
        return ImGui::ColorPicker3(label, col);
    }
    
    inline bool ColorPicker4(const char* label, float col[4]) {
        return ImGui::ColorPicker4(label, col);
    }
    
    // -------------------------------------------------------------------------
    // SELECTION WIDGETS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline bool BeginCombo(const char* label, const char* preview_value) {
        return ImGui::BeginCombo(label, preview_value);
    }
    
    inline void EndCombo() {
        ImGui::EndCombo();
    }
    
    inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count) {
        return ImGui::Combo(label, current_item, items, items_count);
    }
    
    inline bool BeginListBox(const char* label, float width = 0, float height = 0) {
        return ImGui::BeginListBox(label, ImVec2(width, height));
    }
    
    inline void EndListBox() {
        ImGui::EndListBox();
    }
    
    inline bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1) {
        return ImGui::ListBox(label, current_item, items, items_count, height_in_items);
    }
    
    inline bool Selectable(const char* label, bool selected = false) {
        return ImGui::Selectable(label, selected);
    }
    
    inline bool Selectable(const char* label, bool* p_selected) {
        return ImGui::Selectable(label, p_selected);
    }
    
    inline bool RadioButton(const char* label, bool active) {
        return ImGui::RadioButton(label, active);
    }
    
    inline bool RadioButton(const char* label, int* v, int v_button) {
        return ImGui::RadioButton(label, v, v_button);
    }
    
    // -------------------------------------------------------------------------
    // TOOLTIPS AND POPUPS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline void SetTooltip(const char* text) {
        ImGui::SetTooltip("%s", text);
    }
    
    inline bool BeginTooltip() {
        ImGui::BeginTooltip();
        return true;
    }
    
    inline void EndTooltip() {
        ImGui::EndTooltip();
    }
    
    inline bool BeginPopup(const char* str_id) {
        return ImGui::BeginPopup(str_id);
    }
    
    inline bool BeginPopupModal(const char* name, bool* p_open = nullptr) {
        return ImGui::BeginPopupModal(name, p_open);
    }
    
    inline void EndPopup() {
        ImGui::EndPopup();
    }
    
    inline void OpenPopup(const char* str_id) {
        ImGui::OpenPopup(str_id);
    }
    
    inline void CloseCurrentPopup() {
        ImGui::CloseCurrentPopup();
    }
    
    // -------------------------------------------------------------------------
    // TABLES - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline bool BeginTable(const char* str_id, int column_count) {
        return ImGui::BeginTable(str_id, column_count);
    }
    
    inline void EndTable() {
        ImGui::EndTable();
    }
    
    inline void TableNextRow() {
        ImGui::TableNextRow();
    }
    
    inline bool TableNextColumn() {
        return ImGui::TableNextColumn();
    }
    
    inline void TableSetupColumn(const char* label) {
        ImGui::TableSetupColumn(label);
    }
    
    inline void TableHeadersRow() {
        ImGui::TableHeadersRow();
    }
    
    // -------------------------------------------------------------------------
    // UTILITY FUNCTIONS - Direct pass-through to ImGui
    // -------------------------------------------------------------------------
    
    inline void SetCursorPosX(float local_x) {
        ImGui::SetCursorPosX(local_x);
    }
    
    inline void SetCursorPosY(float local_y) {
        ImGui::SetCursorPosY(local_y);
    }
    
    inline void SetCursorPos(float local_x, float local_y) {
        ImGui::SetCursorPos(ImVec2(local_x, local_y));
    }
    
    inline float GetCursorPosX() {
        return ImGui::GetCursorPosX();
    }
    
    inline float GetCursorPosY() {
        return ImGui::GetCursorPosY();
    }
    
    inline float GetContentRegionAvailWidth() {
        return ImGui::GetContentRegionAvail().x;
    }
    
    inline float GetContentRegionAvailHeight() {
        return ImGui::GetContentRegionAvail().y;
    }
    
    inline float GetWindowWidth() {
        return ImGui::GetWindowSize().x;
    }
    
    inline float GetWindowHeight() {
        return ImGui::GetWindowSize().y;
    }
    
    inline bool IsItemHovered() {
        return ImGui::IsItemHovered();
    }
    
    inline bool IsItemActive() {
        return ImGui::IsItemActive();
    }
    
    inline bool IsItemClicked(int mouse_button = 0) {
        return ImGui::IsItemClicked(mouse_button);
    }
    
    inline bool IsItemVisible() {
        return ImGui::IsItemVisible();
    }
    
    inline bool IsItemEdited() {
        return ImGui::IsItemEdited();
    }
    
    inline bool IsItemActivated() {
        return ImGui::IsItemActivated();
    }
    
    inline bool IsItemDeactivated() {
        return ImGui::IsItemDeactivated();
    }
    
    inline bool IsItemDeactivatedAfterEdit() {
        return ImGui::IsItemDeactivatedAfterEdit();
    }
    
    inline void SetItemDefaultFocus() {
        ImGui::SetItemDefaultFocus();
    }
    
    inline void SetKeyboardFocusHere(int offset = 0) {
        ImGui::SetKeyboardFocusHere(offset);
    }
    
    inline void SetNextItemWidth(float item_width) {
        ImGui::SetNextItemWidth(item_width);
    }
    
    inline void PushItemWidth(float item_width) {
        ImGui::PushItemWidth(item_width);
    }
    
    inline void PopItemWidth() {
        ImGui::PopItemWidth();
    }
    
    inline float CalcItemWidth() {
        return ImGui::CalcItemWidth();
    }
    
    // -------------------------------------------------------------------------
    // STYLE FUNCTIONS - Direct pass-through to ImGui with compatibility
    // -------------------------------------------------------------------------
    
    inline void PushStyleVar(int var, float value) {
        ImGui::PushStyleVar((ImGuiStyleVar)var, value);
    }
    
    inline void PushStyleVar2(int var, float x, float y) {
        ImGui::PushStyleVar((ImGuiStyleVar)var, ImVec2(x, y));
    }
    
    inline void PushStyleColor(int colorId, float r, float g, float b, float a = 1.0f) {
        ImGui::PushStyleColor((ImGuiCol)colorId, ImVec4(r, g, b, a));
    }
    
    inline void PushStyleColor(int colorId, ImGuiColor color) {
        ImGui::PushStyleColor((ImGuiCol)colorId, ImVec4(color.r, color.g, color.b, color.a));
    }
    
    inline void PopStyleVar(int count = 1) {
        ImGui::PopStyleVar(count);
    }
    
    inline void PopStyleColor(int count = 1) {
        ImGui::PopStyleColor(count);
    }
    
    // -------------------------------------------------------------------------
    // LAYOUT HELPERS
    // -------------------------------------------------------------------------
    
    inline void CenterNextItem(float itemWidth) {
        float windowWidth = ImGui::GetWindowSize().x;
        float center = (windowWidth - itemWidth) * 0.5f;
        if (center > 0) {
            ImGui::SetCursorPosX(center);
        }
    }
    
    inline void RightAlignNextItem(float itemWidth) {
        float windowWidth = ImGui::GetWindowSize().x;
        float rightAlign = windowWidth - itemWidth - ImGui::GetStyle().WindowPadding.x;
        if (rightAlign > 0) {
            ImGui::SetCursorPosX(rightAlign);
        }
    }
    
    // =========================================================================
    // USAGE NOTE - COMPLETE COMPATIBILITY:
    // =========================================================================
    // You can now use EITHER syntax with ZERO breaking changes:
    //
    // Option 1 - Existing code works unchanged:
    // using namespace ImGuiDialogs;
    // Text("Hello World");
    // PushStyleColor(IMGUI_COL_BUTTON, 1,0,0,1);  // Old constants work!
    // if (Button("Click Me")) { }
    //
    // Option 2 - New code can use ImGui directly:
    // ImGui::Text("Hello World");
    // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));  // New style
    // if (ImGui::Button("Click Me")) { }
    //
    // Option 3 - Mix both as needed:
    // ImGuiDialogs::Text("Legacy function");
    // ImGui::SliderFloat("New feature", &value, 0.0f, 1.0f);
    //
    // This provides perfect backward compatibility with immediate new features!
    // =========================================================================
}