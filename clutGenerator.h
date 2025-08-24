#ifndef CLUT_GENERATOR_H
#define CLUT_GENERATOR_H

#include "stdafx.h"
#include "FotoSCIhop.h"

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
    bool ImportFromSCITableEntry(const std::string& sciLine);
    
    // UI State
    bool IsPreviewEnabled() const { return m_previewEnabled; }
    void RevertToOriginal();
    
    // Magic wand and usage analysis
    void SetMagicWandEnabled(bool enabled) { m_magicWandEnabled = enabled; }
    bool IsMagicWandEnabled() const { return m_magicWandEnabled; }
    void AnalyzeImageColorUsage();
    const std::set<int>& GetUsedColorIndices() const { return m_usedColorIndices; }
    bool IsColorUsedInImage(int colorIndex) const;
    
    // Getters for UI
    const std::vector<ColorRemapEntry>& GetCurrentRemaps() const { return m_currentRemaps; }
    Palette* GetSourcePalette() const { return m_sourcePalette; }
    
    // UI helpers - WITH PREVIEW UPDATES
    int GetSelectedFromColor() const { return m_selectedFromColor; }
    int GetSelectedToColor() const { return m_selectedToColor; }
    void SetSelectedFromColor(int color) { 
        m_selectedFromColor = color; 
        ApplyPreviewRemap();
    }
    void SetSelectedToColor(int color) { 
        m_selectedToColor = color; 
        ApplyPreviewRemap();
    }
    
    bool IsActive() const { return m_isActive; }
    
    // Get original palette colors for GUI display
    bool GetOriginalPaletteEntry(int colorIndex, PalEntry& entry) const;
    
private:
    // Internal state
    bool m_isActive;
    bool m_previewEnabled;
    bool m_magicWandEnabled;
    
    // Palette management
    Palette* m_sourcePalette;          // Reference to main image palette
    PaletteBackup m_backupPalette;     // Backup of original entries
    
    // Current editing state
    std::vector<ColorRemapEntry> m_currentRemaps;
    int m_selectedFromColor;
    int m_selectedToColor;
    
    // Preview state
    bool m_hasPreviewRemap;
    int m_previewFromColor;
    int m_previewToColor;
    
    // Color usage analysis
    std::set<int> m_usedColorIndices;
    
    // Helper functions
    bool ValidateColorIndex(int colorIndex) const;
    void AnalyzeCellImageUsage(unsigned char* imageData, int width, int height);
    
    // Preview management
    void ApplyPreviewRemap();
    void ClearPreviewRemap();
};

// Global instance
extern ClutGenerator* g_clutGenerator;

#endif // CLUT_GENERATOR_H