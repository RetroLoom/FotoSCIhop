#pragma once
#include "stdafx.h"
#include "palette.h"
#include <vector>
#include <string>

// Simple color remap entry
struct ColorRemapEntry {
    int fromColor;
    int toColor;
    bool active;
    
    ColorRemapEntry() : fromColor(0), toColor(0), active(false) {}
    ColorRemapEntry(int from, int to) : fromColor(from), toColor(to), active(true) {}
};

// Structure to store original palette entries for backup
struct PaletteBackup {
    PalEntry entries[256];
    bool hasValidData;
    
    PaletteBackup() : hasValidData(false) {
        memset(entries, 0, sizeof(entries));
    }
};

class ClutGenerator {
public:
    ClutGenerator();
    ~ClutGenerator();
    
    // Core functionality
    bool Initialize(Palette* sourcePalette);
    void Shutdown();
    
    // Palette management
    void BackupOriginalPalette();
    void RestoreOriginalPalette();
    void ApplyCurrentRemaps();
    void ClearAllRemaps();
    
    // Remap management
    void AddRemap(int fromColor, int toColor);
    void RemoveRemap(int fromColor);
    void ClearRemap(int index);
    void ToggleRemapActive(int index);
    bool HasRemap(int fromColor) const;
    
    // Generate SCI table output
    std::string GenerateSCITableEntry(const std::string& comment) const;
    
    // UI State
    bool IsPreviewEnabled() const { return m_previewEnabled; }
    void SetPreviewEnabled(bool enabled);
    
    // Getters for UI
    const std::vector<ColorRemapEntry>& GetCurrentRemaps() const { return m_currentRemaps; }
    Palette* GetSourcePalette() const { return m_sourcePalette; }
    
    // UI helpers
    int GetSelectedFromColor() const { return m_selectedFromColor; }
    int GetSelectedToColor() const { return m_selectedToColor; }
    void SetSelectedFromColor(int color) { m_selectedFromColor = color; }
    void SetSelectedToColor(int color) { m_selectedToColor = color; }
    
    bool IsActive() const { return m_isActive; }
    
private:
    // Internal state
    bool m_isActive;
    bool m_previewEnabled;
    
    // Palette management
    Palette* m_sourcePalette;          // Reference to main image palette
    PaletteBackup m_backupPalette;     // Backup of original entries
    
    // Current editing state
    std::vector<ColorRemapEntry> m_currentRemaps;
    int m_selectedFromColor;
    int m_selectedToColor;
    
    // Helper functions
    void ApplyRemapsToMainPalette();
    bool ValidateColorIndex(int colorIndex) const;
    void CopyPaletteEntry(const PalEntry* source, PalEntry* dest) const;
    void BackupPaletteEntry(int colorIndex);
    void RestorePaletteEntry(int colorIndex);
};

// Global instance
extern ClutGenerator* g_clutGenerator;