/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Simplified CLUT Generator Dialog - Real-time Updates Only
 *
 */

#include "stdafx.h"
#include "FotoSCIhop.h"
#include "ClutGenerator.h"
// Add these includes at the top of dlgClutGen.cpp after existing includes
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>

// Enhanced preset management data structures
struct PresetData {
    std::vector<std::string> categories;
    std::vector<std::string> names;
    std::vector<std::vector<ColorRemapEntry>> remaps;
    std::vector<std::string> comments;
    std::vector<int> originalIndices;
};

// Current work state for saving
struct WorkingPreset {
    std::string name;
    std::string category;
    std::string comment;
    bool hasUnsavedChanges;
    
    WorkingPreset() : hasUnsavedChanges(false) {}
};

// Forward declarations
bool ParseFullCOLORTBLTable(const std::string& tableData, PresetData& presets);
std::string GenerateFullCOLORTBLTable(const PresetData& presets);

// ============================================================================
// PERSISTENT PRESET STORAGE FUNCTIONS
// ============================================================================

// Get the path to the presets file in the application directory
char* GetPresetsFilePath() {
    static char presetsPath[MAX_PATH];
    
    // Get the application directory
    GetModuleFileName(NULL, presetsPath, MAX_PATH);
    char* lastBackslash = strrchr(presetsPath, '\\');
    if (lastBackslash) {
        *lastBackslash = '\0';
    }
    
    // Append the filename
    strcat(presetsPath, "\\colortbl.txt");
    return presetsPath;
}

// Save presets to file
bool SavePresetsToFile(const PresetData& presets) {
    if (presets.names.empty()) {
        return true; // Nothing to save, but not an error
    }
    
    char* filePath = GetPresetsFilePath();
    FILE* file = fopen(filePath, "w");
    if (!file) {
        return false;
    }
    
    // Generate the full table format and write it
    std::string fullTable = GenerateFullCOLORTBLTable(presets);
    if (!fullTable.empty()) {
        fputs(fullTable.c_str(), file);
    }
    
    fclose(file);
    return true;
}

// Load presets from file
bool LoadPresetsFromFile(PresetData& presets) {
    char* filePath = GetPresetsFilePath();
    FILE* file = fopen(filePath, "r");
    if (!file) {
        // File doesn't exist yet, not an error
        return true;
    }
    
    // Read the entire file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        fclose(file);
        return true;
    }
    
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    
    // Parse the table
    std::string tableData(buffer);
    bool result = ParseFullCOLORTBLTable(tableData, presets);
    
    free(buffer);
    return result;
}

// Auto-save presets (called when presets are modified)
void AutoSavePresets(const PresetData& presets) {
    SavePresetsToFile(presets);
}

// Helper functions for preset management
bool ParseFullCOLORTBLTable(const std::string& tableData, PresetData& presets) {
    presets.categories.clear();
    presets.names.clear();
    presets.remaps.clear();
    presets.comments.clear();
    presets.originalIndices.clear();
    
    std::istringstream stream(tableData);
    std::string line;
    std::string currentCategory = "Uncategorized";
    int lineIndex = 0;
    
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        
        // Check for category markers
        if (line.find(";_______") != std::string::npos || 
            line.find("; ______") != std::string::npos) {
            continue;
        }
        
        if (line.find(";") == 0 && line.find(";;") == std::string::npos) {
            // Category header line
            size_t start = line.find_first_not_of("; \t");
            if (start != std::string::npos) {
                currentCategory = line.substr(start);
                size_t end = currentCategory.find("colors");
                if (end != std::string::npos) {
                    currentCategory = currentCategory.substr(0, end + 6);
                }
            }
            continue;
        }
        
        if (line.find(";;") == std::string::npos) continue;
        
        size_t commentPos = line.find(";;");
        std::string dataLine = line.substr(0, commentPos);
        std::string comment = line.substr(commentPos + 2);
        
        size_t commentStart = comment.find_first_not_of(" \t");
        if (commentStart != std::string::npos) {
            comment = comment.substr(commentStart);
        }
        
        std::string name = comment;
        int originalIndex = lineIndex;
        size_t spacePos = comment.find(' ');
        if (spacePos != std::string::npos) {
            std::string indexStr = comment.substr(0, spacePos);
            originalIndex = atoi(indexStr.c_str());
            name = comment.substr(spacePos + 1);
        }
        
        std::vector<ColorRemapEntry> remaps;
        std::istringstream dataStream(dataLine);
        std::vector<int> numbers;
        int num;
        
        while (dataStream >> num) {
            numbers.push_back(num);
        }
        
        for (size_t i = 0; i < numbers.size() - 1; i += 2) {
            int from = numbers[i];
            int to = numbers[i + 1];
            if (from != -1 && to != -1) {
                remaps.push_back(ColorRemapEntry(from, to));
            }
        }
        
        if (!remaps.empty()) {
            presets.categories.push_back(currentCategory);
            presets.names.push_back(name);
            presets.remaps.push_back(remaps);
            presets.comments.push_back(comment);
            presets.originalIndices.push_back(originalIndex);
        }
        
        lineIndex++;
    }
    
    return !presets.names.empty();
}

std::string GenerateFullCOLORTBLTable(const PresetData& presets) {
    std::string fullTable = "";
    std::string lastCategory = "";
    
    for (size_t i = 0; i < presets.names.size(); i++) {
        if (presets.categories[i] != lastCategory) {
            if (!fullTable.empty()) fullTable += "\n";
            fullTable += ";_______________________\n";
            fullTable += ";\n";
            fullTable += ";\t" + presets.categories[i] + "\n";
            fullTable += ";_______________________\n";
            fullTable += ";\n";
            lastCategory = presets.categories[i];
        }
        
        std::ostringstream line;
        line << "\t";
        
        int pairCount = 0;
        for (size_t j = 0; j < presets.remaps[i].size() && pairCount < 12; j++) {
            if (presets.remaps[i][j].active) {
                if (pairCount > 0) line << "  ";
                line << std::setw(3) << presets.remaps[i][j].fromColor << " " 
                     << std::setw(3) << presets.remaps[i][j].toColor;
                pairCount++;
            }
        }
        
        while (pairCount < 12) {
            if (pairCount > 0) line << "  ";
            line << " -1  -1";
            pairCount++;
        }
        
        line << " ;; " << presets.originalIndices[i] << " " << presets.names[i] << "\n";
        fullTable += line.str();
    }
    
    return fullTable;
}

void RenderPresetCard(const PresetData& presets, int index, int& selectedPreset, 
                     std::string& statusMessage, bool& showStatus, int& selectedRemapIndex, bool& editingMode, int& deleteIndex) {
    using namespace FotoSCIhopStyles;
    
    bool isSelected = (selectedPreset == index);
    ImVec4 cardColor = isSelected ? ImVec4(0.25f, 0.35f, 0.15f, 0.9f) : ImVec4(0.15f, 0.15f, 0.2f, 0.8f);
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, cardColor);
    if (ImGui::BeginChild(("PresetCard" + std::to_string(index)).c_str(), ImVec2(180, 100), true, 
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        
        // Header with index and category
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.6f, 1.0f));
        ImGui::Text("[%d] %s", presets.originalIndices[index], presets.categories[index].c_str());
        ImGui::PopStyleColor();
        
        // Name (larger text)
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font but we can make it bold-ish by using color
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        // Truncate long names
        std::string displayName = presets.names[index];
        if (displayName.length() > 18) {
            displayName = displayName.substr(0, 15) + "...";
        }
        ImGui::Text("%s", displayName.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        
        // Remap info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
        ImGui::Text("%d remaps", (int)presets.remaps[index].size());
        ImGui::PopStyleColor();
        
        // Load button
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.8f, 0.4f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
        if (ImGui::Button("Load", ImVec2(80, 22))) {
            g_clutGenerator->ClearAllRemaps();
            for (const ColorRemapEntry& remap : presets.remaps[index]) {
                g_clutGenerator->AddRemap(remap.fromColor, remap.toColor);
            }
            selectedPreset = index;
            selectedRemapIndex = -1;
            editingMode = false;
            statusMessage = "Loaded: " + presets.names[index];
            showStatus = true;
        }
        ImGui::PopStyleColor(3);
        
        ImGui::SameLine();
        
        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.4f, 0.4f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.2f, 0.2f, 0.8f));
        if (ImGui::Button("Del", ImVec2(35, 22))) {
            deleteIndex = index;
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    // Click card to select
    if (ImGui::IsItemClicked()) {
        selectedPreset = index;
    }
    
    // Hover tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Category: %s\nOriginal Index: %d\nComment: %s\nClick to select, Load to apply", 
                        presets.categories[index].c_str(), 
                        presets.originalIndices[index],
                        presets.comments[index].c_str());
    }
}

void RenderPresetLibrary(const PresetData& presets, int& selectedPreset, const std::string& currentCategory,
                        std::string& statusMessage, bool& showStatus, int& selectedRemapIndex, bool& editingMode, int& presetVersion) {
    using namespace FotoSCIhopStyles;
    
    static int deleteIndex = -1;
    static bool showDeleteConfirm = false;
    
    if (presets.names.empty()) {
        ImGui::Spacing();
        ImGui::Spacing();
        InfoText("Import a COLORTBL.SC table to see presets here");
        return;
    }
    
    HeaderText("Preset Library");
    ImGui::Text("%d presets available", (int)presets.names.size());
    ImGui::Separator();
    ImGui::Spacing();
    
    // Delete confirmation dialog
    if (showDeleteConfirm && deleteIndex >= 0 && deleteIndex < presets.names.size()) {
        ImGui::OpenPopup("Delete Preset?");
    }
    
    if (ImGui::BeginPopupModal("Delete Preset?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete this preset?");
        ImGui::Spacing();
        if (deleteIndex >= 0 && deleteIndex < presets.names.size()) {
            ImGui::Text("Name: %s", presets.names[deleteIndex].c_str());
            ImGui::Text("Category: %s", presets.categories[deleteIndex].c_str());
        }
        ImGui::Spacing();
        
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            // Remove the preset from all vectors
            if (deleteIndex >= 0 && deleteIndex < presets.names.size()) {
                PresetData& mutablePresets = const_cast<PresetData&>(presets);
                mutablePresets.names.erase(mutablePresets.names.begin() + deleteIndex);
                mutablePresets.categories.erase(mutablePresets.categories.begin() + deleteIndex);
                mutablePresets.remaps.erase(mutablePresets.remaps.begin() + deleteIndex);
                mutablePresets.comments.erase(mutablePresets.comments.begin() + deleteIndex);
                mutablePresets.originalIndices.erase(mutablePresets.originalIndices.begin() + deleteIndex);
                
                // Auto-save after deletion
                AutoSavePresets(presets);
                presetVersion++; // Refresh category filter
                
                // Update selection if needed
                if (selectedPreset == deleteIndex) {
                    selectedPreset = -1;
                } else if (selectedPreset > deleteIndex) {
                    selectedPreset--;
                }
                
                statusMessage = "Preset deleted";
                showStatus = true;
            }
            
            showDeleteConfirm = false;
            deleteIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            showDeleteConfirm = false;
            deleteIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Grid layout for preset cards
    float cardWidth = 180.0f;
    float spacing = 10.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    int cardsPerRow = (int)((availableWidth + spacing) / (cardWidth + spacing));
    if (cardsPerRow < 1) cardsPerRow = 1;
    
    int cardCount = 0;
    int visibleCards = 0;
    
    for (int i = 0; i < static_cast<int>(presets.names.size()); i++) {
        if (currentCategory != "All" && presets.categories[i] != currentCategory) {
            continue;
        }
        
        if (visibleCards > 0 && (visibleCards % cardsPerRow) != 0) {
            ImGui::SameLine();
        }
        
        RenderPresetCard(presets, i, selectedPreset, statusMessage, showStatus, selectedRemapIndex, editingMode, deleteIndex);
        
        // Check if delete was requested
        if (deleteIndex == i && !showDeleteConfirm) {
            showDeleteConfirm = true;
        }
        
        visibleCards++;
    }
}

void RenderWorkingArea(PresetData& presets, WorkingPreset& working, std::string& statusMessage, bool& showStatus, int& presetVersion) {
    using namespace FotoSCIhopStyles;
    
    HeaderText("Current Work");
    ImGui::Separator();
    ImGui::Spacing();
    
    // Show current remap count
    const std::vector<ColorRemapEntry>& currentRemaps = g_clutGenerator->GetCurrentRemaps();
    int activeCount = 0;
    for (const auto& remap : currentRemaps) {
        if (remap.active) activeCount++;
    }
    
    if (activeCount > 0) {
        char remapText[64];
        sprintf(remapText, "Active remaps: %d", activeCount);
        SuccessText(remapText);
        working.hasUnsavedChanges = true;
    } else {
        DisabledText("No active remaps");
        working.hasUnsavedChanges = false;
    }
    
    ImGui::Spacing();
    
    // Save current work as new preset
    if (working.hasUnsavedChanges) {
        ImGui::Text("Save current remaps as preset:");
        
        ImGui::Text("Name:");
        static char nameBuffer[256] = "";
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##preset_name", "Enter preset name...", nameBuffer, sizeof(nameBuffer));
        
        ImGui::Text("Category:");
        static char categoryBuffer[256] = "Custom";
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##preset_category", "Enter category...", categoryBuffer, sizeof(categoryBuffer));
        
        ImGui::Text("Comment:");
        static char commentBuffer[256] = "";
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##preset_comment", "Optional comment...", commentBuffer, sizeof(commentBuffer));
        
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
        if (ImGui::Button("Save as New Preset", ImVec2(200, 30))) {
            if (strlen(nameBuffer) > 0) {
                std::vector<ColorRemapEntry> activeRemaps;
                for (const ColorRemapEntry& remap : currentRemaps) {
                    if (remap.active) {
                        activeRemaps.push_back(remap);
                    }
                }
                
                if (!activeRemaps.empty()) {
                    int newIndex = presets.originalIndices.empty() ? 0 : *std::max_element(presets.originalIndices.begin(), presets.originalIndices.end()) + 1;
                    
                    presets.names.push_back(std::string(nameBuffer));
                    presets.categories.push_back(std::string(categoryBuffer));
                    presets.remaps.push_back(activeRemaps);
                    presets.comments.push_back(std::string(commentBuffer));
                    presets.originalIndices.push_back(newIndex);
                    
                    // Auto-save to file
                    AutoSavePresets(presets);
                    
                    presetVersion++; // Increment version to refresh category filter
                    statusMessage = "Saved preset: " + std::string(nameBuffer);
                    showStatus = true;
                    
                    // Clear inputs
                    nameBuffer[0] = '\0';
                    strcpy(categoryBuffer, "Custom");
                    commentBuffer[0] = '\0';
                }
            } else {
                statusMessage = "Please enter a name for the preset";
                showStatus = true;
            }
        }
        ImGui::PopStyleColor(3);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Export options
    HeaderText("Export");
    
    if (ImGui::Button("Export Current as Line", ImVec2(200, 25))) {
        if (activeCount > 0) {
            std::string exportLine = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop");
            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, exportLine.length() + 1);
                if (hClipboardData) {
                    char* pchData = (char*)GlobalLock(hClipboardData);
                    if (pchData) {
                        strcpy(pchData, exportLine.c_str());
                        GlobalUnlock(hClipboardData);
                        SetClipboardData(CF_TEXT, hClipboardData);
                        statusMessage = "Copied line to clipboard";
                        showStatus = true;
                    }
                }
                CloseClipboard();
            }
        }
    }
    
    if (ImGui::Button("Export Full Table", ImVec2(200, 25))) {
        if (!presets.names.empty()) {
            std::string fullTable = GenerateFullCOLORTBLTable(presets);
            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, fullTable.length() + 1);
                if (hClipboardData) {
                    char* pchData = (char*)GlobalLock(hClipboardData);
                    if (pchData) {
                        strcpy(pchData, fullTable.c_str());
                        GlobalUnlock(hClipboardData);
                        SetClipboardData(CF_TEXT, hClipboardData);
                        statusMessage = "Copied full table to clipboard";
                        showStatus = true;
                    }
                }
                CloseClipboard();
            }
        }
    }
    
    ImGui::Spacing();
    
    // Manual save button
    if (ImGui::Button("Save Presets to File", ImVec2(200, 25))) {
        if (SavePresetsToFile(presets)) {
            statusMessage = "Presets saved to colortbl.txt";
            showStatus = true;
        } else {
            statusMessage = "Failed to save presets";
            showStatus = true;
        }
    }
}

void RenderModernPresetManager(float availableWidth, std::string& statusMessage, bool& showStatus,
                              int& selectedRemapIndex, bool& editingMode) {
    using namespace FotoSCIhopStyles;
    
    static PresetData presets;
    static WorkingPreset working;
    static int selectedPreset = -1;
    static std::string currentCategory = "All";
    static char fullTableBuffer[65536] = "";
    static char singleLineBuffer[1024] = "";
    static bool showImportArea = false;
    static bool presetsLoaded = false;
    static int presetVersion = 0; // Track when presets change for category refresh
    
    // Load presets from file on first run
    if (!presetsLoaded) {
        if (LoadPresetsFromFile(presets)) {
            presetsLoaded = true;
            presetVersion++; // Increment version to refresh categories
            if (!presets.names.empty()) {
                statusMessage = "Loaded " + std::to_string(presets.names.size()) + " presets from colortbl.txt";
                showStatus = true;
            }
        } else {
            presetsLoaded = true; // Don't keep trying if it fails
        }
    }
    
    // Modern header with main actions
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.18f, 0.9f));
    if (ImGui::BeginChild("PresetHeader", ImVec2(0, 45), true, ImGuiWindowFlags_NoScrollbar)) {
        
        ImGui::BeginGroup();
        {
            // Import actions
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 0.9f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.4f, 0.7f, 0.8f));
            if (ImGui::Button("Import Table", ImVec2(100, 30))) {
                showImportArea = !showImportArea;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::Button("Import Line", ImVec2(100, 30))) {
                if (strlen(singleLineBuffer) > 0) {
                    g_clutGenerator->ClearAllRemaps();
                    selectedRemapIndex = -1;
                    editingMode = false;
                    std::string importLine(singleLineBuffer);
                    bool success = g_clutGenerator->ImportFromSCITableEntry(importLine);
                    
                    if (success) {
                        const std::vector<ColorRemapEntry>& newRemaps = g_clutGenerator->GetCurrentRemaps();
                        statusMessage = "Imported " + std::to_string(newRemaps.size()) + " remaps";
                        showStatus = true;
                        singleLineBuffer[0] = '\0';
                    }
                }
            }
            
            // Category filter
            if (!presets.names.empty()) {
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(30, 0));
                ImGui::SameLine();
                
                ImGui::Text("Filter:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                
                // Use presetVersion in combo ID to force refresh when presets change
                char comboId[32];
                sprintf(comboId, "##category%d", presetVersion);
                
                if (ImGui::BeginCombo(comboId, currentCategory.c_str())) {
                    if (ImGui::Selectable("All", currentCategory == "All")) {
                        currentCategory = "All";
                    }
                    
                    std::set<std::string> uniqueCategories;
                    for (const std::string& cat : presets.categories) {
                        uniqueCategories.insert(cat);
                    }
                    
                    for (const std::string& cat : uniqueCategories) {
                        bool isSelected = (currentCategory == cat);
                        if (ImGui::Selectable(cat.c_str(), isSelected)) {
                            currentCategory = cat;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            
            // Single line import
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(300);
            ImGui::InputTextWithHint("##single_line", "Quick import: paste SCI line here...", singleLineBuffer, sizeof(singleLineBuffer));
        }
        ImGui::EndGroup();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    // Import area (collapsible)
    if (showImportArea) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.08f, 0.8f));
        if (ImGui::BeginChild("ImportArea", ImVec2(0, 120), true)) {
            ImGui::Text("Paste your complete COLORTBL.SC table:");
            ImGui::PushItemWidth(-80);
            ImGui::InputTextMultiline("##full_table", fullTableBuffer, sizeof(fullTableBuffer), 
                                    ImVec2(0, 60), ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::Button("Parse\nTable", ImVec2(70, 60))) {
                if (strlen(fullTableBuffer) > 0) {
                    std::string tableData(fullTableBuffer);
                    if (ParseFullCOLORTBLTable(tableData, presets)) {
                        // Auto-save after importing
                        AutoSavePresets(presets);
                        
                        presetVersion++; // Increment version to refresh category filter
                        statusMessage = "Loaded " + std::to_string(presets.names.size()) + " presets";
                        showStatus = true;
                        selectedPreset = -1;
                        fullTableBuffer[0] = '\0';
                        showImportArea = false;
                    } else {
                        statusMessage = "Failed to parse table";
                        showStatus = true;
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
    }
    
    // Main content area - two columns
    if (ImGui::BeginChild("PresetContent", ImVec2(0, 0), false)) {
        
        // Left column - Preset library
        if (ImGui::BeginChild("LeftPresets", ImVec2(availableWidth * 0.7f, 0), true)) {
            RenderPresetLibrary(presets, selectedPreset, currentCategory, statusMessage, showStatus, selectedRemapIndex, editingMode, presetVersion);
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right column - Working area
        if (ImGui::BeginChild("RightWork", ImVec2(0, 0), true)) {
            RenderWorkingArea(presets, working, statusMessage, showStatus, presetVersion);
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();
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
    static std::string statusMessage = "";
    static bool showStatus = false;
    
    // Enhanced state for better editing
    static int selectedRemapIndex = -1;
    static int hoveredRemapIndex = -1;
    static bool editingMode = false;
    static std::string editModeStatus = "";
    static bool editingFromColor = true;
    
    // Layout controls
    static int activeTab = 0; // 0=Editor, 1=Presets, 2=Import/Export
    static bool rightPanelOpen = true;
    
    bool open = true;
    SetNextWindowSize(1200, 700);  // Wider, better proportioned
    
    if (!BeginDialog("CLUT Generator - Real-time Preview", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open || shouldClose) {
        shouldClose = false;
        selectedRemapIndex = -1;
        editingMode = false;
        
        if (g_clutGenerator) {
            g_clutGenerator->Shutdown();
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
            g_clutGenerator->AnalyzeImageColorUsage();
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
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // COMPACT HEADER - Essential info only
    // =========================================================================
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    
    if (ImGui::BeginChild("CompactHeader", ImVec2(0, 60), true)) {
        
        // Row 1: Status and current selection
        ImGui::BeginGroup();
        {
            SuccessText("Magic Wand Active");
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            
            // Current FROM/TO selection - compact
            int fromColor = g_clutGenerator->GetSelectedFromColor();
            int toColor = g_clutGenerator->GetSelectedToColor();
            PalEntry fromEntry, toEntry;
            
            if (g_clutGenerator->GetOriginalPaletteEntry(fromColor, fromEntry)) {
                char fromLabel[24];
                sprintf(fromLabel, "F:%d", fromColor);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                if (ImGui::Button(fromLabel, ImVec2(45, 18))) {
                    g_clutGenerator->SetSelectedFromColor(0);
                    statusMessage = "Cleared FROM";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::SameLine();
            ImGui::Text(">");
            ImGui::SameLine();
            
            if (g_clutGenerator->GetOriginalPaletteEntry(toColor, toEntry)) {
                char toLabel[24];
                sprintf(toLabel, "T:%d", toColor);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                if (ImGui::Button(toLabel, ImVec2(45, 18))) {
                    g_clutGenerator->SetSelectedToColor(0);
                    statusMessage = "Cleared TO";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
            }
            
            // Auto-add remap when ready
            if (fromColor != toColor && fromColor != 0 && toColor != 0) {
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(10, 0));
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.36f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.24f, 1.0f));
                if (ImGui::Button("Add", ImVec2(60, 18))) {
                    g_clutGenerator->AddRemap(fromColor, toColor);
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    selectedRemapIndex = remaps.size() - 1;
                    statusMessage = "Added remap";
                    showStatus = true;
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            ImGui::SameLine();
            
            // Tab buttons
            if (ImGui::Button("Editor", ImVec2(80, 20))) activeTab = 0;
            ImGui::SameLine();
            if (ImGui::Button("Presets", ImVec2(80, 20))) activeTab = 1;
            ImGui::SameLine();
            if (ImGui::Button("Import/Export", ImVec2(100, 20))) activeTab = 2;
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));
            ImGui::SameLine();
            
            // Main actions
            if (ImGui::Button("Analyze", ImVec2(70, 20))) {
                g_clutGenerator->AnalyzeImageColorUsage();
                statusMessage = "Analysis complete";
                showStatus = true;
            }
            
            ImGui::SameLine();
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            char usageText[32];
            sprintf(usageText, "(%d)", (int)usedColors.size());
            InfoText(usageText);
            
            ImGui::SameLine();
            if (ImGui::Button("Clear All", ImVec2(70, 20))) {
                g_clutGenerator->ClearAllRemaps();
                selectedRemapIndex = -1;
                editingMode = false;
                statusMessage = "Cleared";
                showStatus = true;
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            if (FotoSCIhopStyles::CloseButton("Close")) {
                shouldClose = true;
            }
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        
        // Row 2: Edit mode or instructions
        if (editingMode && selectedRemapIndex >= 0) {
            WarningText("EDIT MODE:");
            ImGui::SameLine();
            SuccessText(editModeStatus.c_str());
            ImGui::SameLine();
            
            const char* editModeText = editingFromColor ? "[FROM]" : "[TO]";
            if (ImGui::Button(editModeText, ImVec2(60, 16))) {
                editingFromColor = !editingFromColor;
                editModeStatus = editingFromColor ? "Edit FROM" : "Edit TO";
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Apply", ImVec2(50, 16))) {
                if (selectedRemapIndex >= 0) {
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    if (selectedRemapIndex < remaps.size()) {
                        g_clutGenerator->RemoveRemap(remaps[selectedRemapIndex].fromColor);
                        g_clutGenerator->AddRemap(g_clutGenerator->GetSelectedFromColor(), 
                                                g_clutGenerator->GetSelectedToColor());
                        statusMessage = "Applied changes";
                        showStatus = true;
                    }
                }
                editingMode = false;
                selectedRemapIndex = -1;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(50, 16))) {
                editingMode = false;
                selectedRemapIndex = -1;
                editModeStatus = "";
            }
        } else {
            InfoText("Left-click = FROM, Right-click = TO, Double-click palette to clear");
        }
        
    }
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // =========================================================================
    // MAIN TABBED CONTENT AREA
    // =========================================================================
    
    if (activeTab == 0) {
        // EDITOR TAB - Palette and Remaps
        if (ImGui::BeginChild("EditorTab", ImVec2(0, 0), false)) {
            
            // Two-column layout
            if (ImGui::BeginChild("PaletteColumn", ImVec2(availableWidth * 0.6f, 0), true)) {
                
                HeaderText("Palette Grid");
                
                // Very compact legend
                const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
                if (usedColors.size() > 0) {
                    ImGui::SameLine();
                    ImGui::Dummy(ImVec2(20, 0));
                    ImGui::SameLine();
                    
                    // Inline legend
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    
                    drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                    drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(100, 150, 255, 255), 0.0f, 0, 1.0f);
                    ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                    ImGui::Text("Used");
                    
                    ImGui::SameLine();
                    pos = ImGui::GetCursorScreenPos();
                    drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                    drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(255, 80, 80, 255), 0.0f, 0, 2.0f);
                    ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                    ImGui::Text("FROM");
                    
                    ImGui::SameLine();
                    pos = ImGui::GetCursorScreenPos();
                    drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                    drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(80, 255, 80, 255), 0.0f, 0, 2.0f);
                    ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                    ImGui::Text("TO");
                    
                    ImGui::SameLine();
                    pos = ImGui::GetCursorScreenPos();
                    drawList->AddRectFilled(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(128, 128, 128, 255));
                    drawList->AddRect(pos, ImVec2(pos.x + 6, pos.y + 6), IM_COL32(255, 255, 80, 255), 0.0f, 0, 2.0f);
                    ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y - 1));
                    ImGui::Text("Remap");
                }
                
                ImGui::Separator();
                ImGui::Spacing();
                
                // Larger palette grid
                if (g_clutGenerator && g_clutGenerator->IsActive()) {
                    
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.8f));
                    if (ImGui::BeginChild("PaletteGrid", ImVec2(0, 0), true)) {
                        
                        const int COLORS_PER_ROW = 16;
                        const float BUTTON_SIZE = 22.0f;  // Larger buttons
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
                                    
                                    int fromColor = g_clutGenerator->GetSelectedFromColor();
                                    int toColor = g_clutGenerator->GetSelectedToColor();
                                    bool isFromColor = (colorIndex == fromColor);
                                    bool isToColor = (colorIndex == toColor);
                                    bool hasRemap = g_clutGenerator->HasRemap(colorIndex);
                                    bool isUsedInImage = g_clutGenerator->IsColorUsedInImage(colorIndex);
                                    bool isSelectedRemapFrom = (colorIndex == highlightFromColor);
                                    bool isSelectedRemapTo = (colorIndex == highlightToColor);
                                    
                                    float r = originalEntry.red / 255.0f;
                                    float g = originalEntry.green / 255.0f;
                                    float b = originalEntry.blue / 255.0f;
                                    
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.0f));
                                    
                                    ImVec2 buttonPos = ImGui::GetCursorScreenPos();
                                    
                                    if (ImGui::Button(buttonId, ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
                                        // Handle clicks
                                    }
                                    
                                    ImGui::PopStyleColor(3);
                                    
                                    // Draw borders
                                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                                    ImVec2 buttonMin = buttonPos;
                                    ImVec2 buttonMax = ImVec2(buttonPos.x + BUTTON_SIZE, buttonPos.y + BUTTON_SIZE);
                                    
                                    if (isSelectedRemapFrom) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                                    } else if (isSelectedRemapTo) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 255, 255), 0.0f, 0, 3.0f);
                                    } else if (isFromColor) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                                    } else if (isToColor) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                                    } else if (hasRemap) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 80, 200), 0.0f, 0, 2.0f);
                                    } else if (isUsedInImage) {
                                        drawList->AddRect(buttonMin, buttonMax, IM_COL32(100, 150, 255, 150), 0.0f, 0, 1.0f);
                                    }
                                    
                                    // Click handling
                                    if (ImGui::IsItemClicked(0)) {
                                        if (editingMode) {
                                            if (editingFromColor) {
                                                g_clutGenerator->SetSelectedFromColor(colorIndex);
                                                editModeStatus = "FROM updated";
                                            } else {
                                                g_clutGenerator->SetSelectedToColor(colorIndex);
                                                editModeStatus = "TO updated";
                                            }
                                        } else {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                        }
                                    }
                                    if (ImGui::IsItemClicked(1)) {
                                        if (editingMode) {
                                            if (editingFromColor) {
                                                g_clutGenerator->SetSelectedToColor(colorIndex);
                                                editModeStatus = "TO updated";
                                            } else {
                                                g_clutGenerator->SetSelectedFromColor(colorIndex);
                                                editModeStatus = "FROM updated";
                                            }
                                        } else {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                        }
                                    }
                                    
                                    // Double-click to clear
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                        g_clutGenerator->SetSelectedFromColor(0);
                                        g_clutGenerator->SetSelectedToColor(0);
                                        statusMessage = "Cleared selections";
                                        showStatus = true;
                                    }
                                    
                                    // Enhanced tooltip
                                    if (ImGui::IsItemHovered()) {
                                        char tooltipText[512];
                                        std::string roleText = "";
                                        
                                        if (isSelectedRemapFrom) roleText += " [SELECTED FROM]";
                                        if (isSelectedRemapTo) roleText += " [SELECTED TO]";
                                        if (isFromColor) roleText += " [CURRENT FROM]";
                                        if (isToColor) roleText += " [CURRENT TO]";
                                        if (hasRemap) roleText += " [REMAPPED]";
                                        if (isUsedInImage) roleText += " [USED IN IMAGE]";
                                        
                                        sprintf(tooltipText, 
                                            "Color Index: %d\n"
                                            "RGB: (%d, %d, %d)\n"
                                            "Hex: #%02X%02X%02X%s\n"
                                            "Left-click = FROM, Right-click = TO\n"
                                            "Double-click to clear selections%s",
                                            colorIndex, 
                                            originalEntry.red, originalEntry.green, originalEntry.blue,
                                            originalEntry.red, originalEntry.green, originalEntry.blue,
                                            roleText.c_str(),
                                            editingMode ? " (EDIT MODE)" : ""
                                        );
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
            
            // Right column - Active remaps (more compact)
            if (ImGui::BeginChild("RemapColumn", ImVec2(0, 0), true)) {
                
                HeaderText("Active Remaps");
                ImGui::Separator();
                
                const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                
                int activeRemaps = 0;
                for (size_t i = 0; i < remaps.size(); i++) {
                    if (remaps[i].active) activeRemaps++;
                }
                
                char statusText[64];
                sprintf(statusText, "%d Active (max 12)", activeRemaps);
                if (activeRemaps > 12) {
                    WarningText(statusText);
                } else if (activeRemaps > 0) {
                    SuccessText(statusText);
                } else {
                    DisabledText(statusText);
                }
                
                ImGui::Spacing();
                
                if (remaps.empty()) {
                    InfoText("No remaps yet");
                    ImGui::Spacing();
                    InfoText("1. Click colors in palette/image");
                    InfoText("2. Use 'Add' button in header");
                } else {
                    // Compact remap list
                    hoveredRemapIndex = -1;
                    
                    for (int i = 0; i < static_cast<int>(remaps.size()); i++) {
                        const ColorRemapEntry& remap = remaps[i];
                        
                        bool isSelected = (i == selectedRemapIndex);
                        if (isSelected) {
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 28);
                            ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                                                                    IM_COL32(76, 51, 25, 180));
                        }
                        
                        ImGui::BeginGroup();
                        
                        PalEntry fromEntry, toEntry;
                        if (g_clutGenerator->GetOriginalPaletteEntry(remap.fromColor, fromEntry) &&
                            g_clutGenerator->GetOriginalPaletteEntry(remap.toColor, toEntry)) {
                            
                            // FROM color
                            char fromId[32];
                            sprintf(fromId, "##from%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                            if (ImGui::Button(fromId, ImVec2(22, 22))) {
                                if (selectedRemapIndex == i && !editingMode) {
                                    editingMode = true;
                                    editingFromColor = true;
                                    g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                    g_clutGenerator->SetSelectedToColor(remap.toColor);
                                    editModeStatus = "Edit FROM";
                                }
                            }
                            ImGui::PopStyleColor(3);
                            
                            ImGui::SameLine();
                            char fromText[16];
                            sprintf(fromText, "%d", remap.fromColor);
                            ImGui::Text("%s", fromText);
                            
                            ImGui::SameLine();
                            ImGui::Text(">");
                            
                            ImGui::SameLine();
                            // TO color
                            char toId[32];
                            sprintf(toId, "##to%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                            if (ImGui::Button(toId, ImVec2(22, 22))) {
                                if (selectedRemapIndex == i && !editingMode) {
                                    editingMode = true;
                                    editingFromColor = false;
                                    g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                    g_clutGenerator->SetSelectedToColor(remap.toColor);
                                    editModeStatus = "Edit TO";
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
                            if (ImGui::Button(toggleId, ImVec2(30, 22))) {
                                g_clutGenerator->ToggleRemapActive(i);
                            }
                            ImGui::PopStyleColor(3);
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
                            if (ImGui::Button(toggleId, ImVec2(30, 22))) {
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
                        if (ImGui::Button(deleteId, ImVec2(22, 22))) {
                            g_clutGenerator->ClearRemap(i);
                            if (selectedRemapIndex == i) {
                                selectedRemapIndex = -1;
                                editingMode = false;
                            } else if (selectedRemapIndex > i) {
                                selectedRemapIndex--;
                            }
                            statusMessage = "Deleted remap";
                            showStatus = true;
                        }
                        ImGui::PopStyleColor(3);
                        
                        ImGui::EndGroup();
                        
                        // Click to select
                        if (ImGui::IsItemClicked()) {
                            selectedRemapIndex = i;
                            g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                            g_clutGenerator->SetSelectedToColor(remap.toColor);
                            editingMode = false;
                        }
                        
                        // Track hover
                        if (ImGui::IsItemHovered()) {
                            hoveredRemapIndex = i;
                            ImGui::SetTooltip("Click to select - Click FROM/TO to edit");
                        }
                        
                        if (i < static_cast<int>(remaps.size()) - 1) {
                            ImGui::Spacing();
                        }
                    }
                }
                
            }
            ImGui::EndChild();
            
        }
        ImGui::EndChild();
        
    } else if (activeTab == 1) {
        // PRESETS TAB
        if (ImGui::BeginChild("PresetsTab", ImVec2(0, 0), false)) {
            RenderModernPresetManager(availableWidth, statusMessage, showStatus, selectedRemapIndex, editingMode);
        }
        ImGui::EndChild();
        
    } else if (activeTab == 2) {
        // IMPORT/EXPORT TAB
        if (ImGui::BeginChild("ImportExportTab", ImVec2(0, 0), true)) {
            
            HeaderText("Import / Export");
            ImGui::Separator();
            ImGui::Spacing();
            
            // Quick import
            ImGui::Text("Quick Import:");
            static char importBuffer[1024] = "";
            ImGui::PushItemWidth(availableWidth * 0.7f);
            ImGui::InputTextWithHint("##quick_import", "Paste COLORTBL.SC line here...", importBuffer, sizeof(importBuffer));
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::Button("Import")) {
                if (strlen(importBuffer) > 0) {
                    g_clutGenerator->ClearAllRemaps();
                    selectedRemapIndex = -1;
                    editingMode = false;
                    std::string importLine(importBuffer);
                    bool success = g_clutGenerator->ImportFromSCITableEntry(importLine);
                    
                    if (success) {
                        const std::vector<ColorRemapEntry>& newRemaps = g_clutGenerator->GetCurrentRemaps();
                        statusMessage = "Imported " + std::to_string(newRemaps.size()) + " remaps";
                        showStatus = true;
                        importBuffer[0] = '\0';
                        activeTab = 0; // Switch to editor
                    }
                }
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Quick export
            ImGui::Text("Quick Export:");
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            int activeCount = 0;
            for (const auto& remap : remaps) {
                if (remap.active) activeCount++;
            }
            
            if (activeCount > 0) {
                char countText[64];
                sprintf(countText, "Ready to export %d active remaps", activeCount);
                SuccessText(countText);
                
                ImGui::Spacing();
                
                if (ImGui::Button("Copy Line to Clipboard", ImVec2(200, 30))) {
                    std::string exportLine = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop");
                    if (OpenClipboard(hWnd)) {
                        EmptyClipboard();
                        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, exportLine.length() + 1);
                        if (hClipboardData) {
                            char* pchData = (char*)GlobalLock(hClipboardData);
                            if (pchData) {
                                strcpy(pchData, exportLine.c_str());
                                GlobalUnlock(hClipboardData);
                                SetClipboardData(CF_TEXT, hClipboardData);
                                statusMessage = "Copied to clipboard";
                                showStatus = true;
                            }
                        }
                        CloseClipboard();
                    }
                }
                
                ImGui::Spacing();
                
                // Show generated code
                std::string previewLine = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop");
                ImGui::Text("Preview:");
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
                
                char previewBuffer[1024];
                strncpy(previewBuffer, previewLine.c_str(), sizeof(previewBuffer) - 1);
                previewBuffer[sizeof(previewBuffer) - 1] = '\0';
                
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##preview", previewBuffer, sizeof(previewBuffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
                
                ImGui::PopStyleColor(2);
                
            } else {
                DisabledText("No active remaps to export");
                ImGui::Spacing();
                InfoText("Create some remaps first, then return to this tab to export them.");
            }
            
        }
        ImGui::EndChild();
    }
    
    // Compact status message at bottom
    if (showStatus && !statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        
        if (statusMessage.find("Success") != std::string::npos || 
            statusMessage.find("complete") != std::string::npos || 
            statusMessage.find("Imported") != std::string::npos ||
            statusMessage.find("Loaded") != std::string::npos ||
            statusMessage.find("Saved") != std::string::npos) {
            SuccessText(statusMessage.c_str());
        } else if (statusMessage.find("Failed") != std::string::npos) {
            ErrorText(statusMessage.c_str());
        } else {
            InfoText(statusMessage.c_str());
        }
        
        // Auto-hide status
        static int statusCounter = 0;
        statusCounter++;
        if (statusCounter > 180) {
            showStatus = false;
            statusMessage = "";
            statusCounter = 0;
        }
    }
    
    EndDialog();
}