#include "stdafx.h"
#include "ClutGenerator.h"
#include <sstream>
#include <iomanip>

// Forward declarations from FotoSCIhop
extern HWND hWnd;  // For InvalidateRgn calls
extern void ForceImageRefresh();  // Function to force cached image refresh

// Global instance
ClutGenerator* g_clutGenerator = nullptr;

ClutGenerator::ClutGenerator() 
    : m_isActive(false)
    , m_previewEnabled(true)
    , m_sourcePalette(nullptr)
    , m_selectedFromColor(0)
    , m_selectedToColor(0)
{
}

ClutGenerator::~ClutGenerator() {
    Shutdown();
}

bool ClutGenerator::Initialize(Palette* sourcePalette) {
    if (!sourcePalette) return false;
    
    Shutdown(); // Clean up any existing state
    
    m_sourcePalette = sourcePalette;
    
    // Backup the original palette before any modifications
    BackupOriginalPalette();
    
    m_isActive = true;
    m_currentRemaps.clear();
    m_previewEnabled = true;
    
    return true;
}

void ClutGenerator::Shutdown() {
    if (m_isActive && m_backupPalette.hasValidData && m_sourcePalette) {
        // ALWAYS restore original palette when shutting down
        RestoreOriginalPalette();
    }
    
    m_sourcePalette = nullptr;
    m_isActive = false;
    m_previewEnabled = false;
    m_backupPalette.hasValidData = false;
    m_currentRemaps.clear();
}

void ClutGenerator::BackupOriginalPalette() {
    if (!m_sourcePalette) return;
    
    // Backup all 256 palette entries using GetPalEntry()
    for (int i = 0; i < 256; i++) {
        BackupPaletteEntry(i);
    }
    
    m_backupPalette.hasValidData = true;
}

void ClutGenerator::RestoreOriginalPalette() {
    if (!m_backupPalette.hasValidData || !m_sourcePalette) return;
    
    // Restore all palette entries using SetPalEntry()
    for (int i = 0; i < 256; i++) {
        RestorePaletteEntry(i);
    }
    
    // Force cached image data to refresh with restored palette
    ForceImageRefresh();
}

void ClutGenerator::ApplyCurrentRemaps() {
    if (!m_isActive || !m_previewEnabled || !m_sourcePalette || !m_backupPalette.hasValidData) {
        return;
    }
    
    ApplyRemapsToMainPalette();
    
    // Force cached image data to refresh with new palette
    ForceImageRefresh();
}

void ClutGenerator::ApplyRemapsToMainPalette() {
    if (!m_sourcePalette || !m_backupPalette.hasValidData) return;
    
    // STEP 1: Restore all colors to original first
    for (int i = 0; i < 256; i++) {
        RestorePaletteEntry(i);
    }
    
    // STEP 2: Apply only ACTIVE remaps to the main image palette
    for (size_t i = 0; i < m_currentRemaps.size(); i++) {
        const ColorRemapEntry& remap = m_currentRemaps[i];
        if (!remap.active) continue; // Skip inactive remaps
        
        if (ValidateColorIndex(remap.fromColor) && ValidateColorIndex(remap.toColor)) {
            // Get the ORIGINAL color that we want to map TO (from backup)
            PalEntry sourceEntry = m_backupPalette.entries[remap.toColor];
            
            // Apply it to the FROM color position using SetPalEntry()
            // This directly modifies the main image palette for live preview
            m_sourcePalette->SetPalEntry(sourceEntry, remap.fromColor);
        }
    }
}

void ClutGenerator::ClearAllRemaps() {
    m_currentRemaps.clear();
    // Restore to original state
    RestoreOriginalPalette();
}

void ClutGenerator::AddRemap(int fromColor, int toColor) {
    if (!ValidateColorIndex(fromColor) || !ValidateColorIndex(toColor)) return;
    
    // Remove existing remap for this fromColor
    RemoveRemap(fromColor);
    
    // Add new remap
    m_currentRemaps.push_back(ColorRemapEntry(fromColor, toColor));
    
    // Apply live preview immediately
    ApplyCurrentRemaps();
}

void ClutGenerator::RemoveRemap(int fromColor) {
    // Find and remove remap with matching fromColor using older C++ syntax
    for (std::vector<ColorRemapEntry>::iterator it = m_currentRemaps.begin(); it != m_currentRemaps.end(); ) {
        if (it->fromColor == fromColor) {
            it = m_currentRemaps.erase(it);
        } else {
            ++it;
        }
    }
    
    // Apply live preview (will restore original colors for removed remaps)
    ApplyCurrentRemaps();
}

void ClutGenerator::ClearRemap(int index) {
    if (index >= 0 && index < static_cast<int>(m_currentRemaps.size())) {
        m_currentRemaps.erase(m_currentRemaps.begin() + index);
        // Apply live preview
        ApplyCurrentRemaps();
    }
}

void ClutGenerator::ToggleRemapActive(int index) {
    if (index >= 0 && index < static_cast<int>(m_currentRemaps.size())) {
        m_currentRemaps[index].active = !m_currentRemaps[index].active;
        // Apply live preview immediately
        ApplyCurrentRemaps();
    }
}

bool ClutGenerator::HasRemap(int fromColor) const {
    for (size_t i = 0; i < m_currentRemaps.size(); i++) {
        const ColorRemapEntry& remap = m_currentRemaps[i];
        if (remap.fromColor == fromColor && remap.active) {
            return true;
        }
    }
    return false;
}

void ClutGenerator::SetPreviewEnabled(bool enabled) {
    m_previewEnabled = enabled;
    
    if (enabled) {
        // Apply current remaps for live preview
        ApplyCurrentRemaps();
    } else {
        // Restore original palette
        RestoreOriginalPalette();
    }
}

std::string ClutGenerator::GenerateSCITableEntry(const std::string& comment) const {
    std::ostringstream ss;
    
    if (!comment.empty()) {
        ss << "\t;; " << comment << "\n";
    }
    
    ss << "\t";
    
    int entryCount = 0;
    
    // Output only active remaps, limited to 12 (SCI engine limitation)
    for (size_t i = 0; i < m_currentRemaps.size(); i++) {
        const ColorRemapEntry& remap = m_currentRemaps[i];
        if (!remap.active || entryCount >= 12) continue;
        
        if (entryCount > 0) ss << "  ";
        ss << std::setw(3) << remap.fromColor << " " << std::setw(3) << remap.toColor;
        entryCount++;
    }
    
    // Pad with -1 values to fill the standard 12 pairs
    while (entryCount < 12) {
        if (entryCount > 0) ss << "  ";
        ss << " -1  -1";
        entryCount++;
    }
    
    ss << " ;; " << (comment.empty() ? "Generated remap" : comment);
    
    return ss.str();
}

// Helper functions
bool ClutGenerator::ValidateColorIndex(int colorIndex) const {
    return colorIndex >= 0 && colorIndex < 256;
}

void ClutGenerator::CopyPaletteEntry(const PalEntry* source, PalEntry* dest) const {
    if (!source || !dest) return;
    
    dest->red = source->red;
    dest->green = source->green;
    dest->blue = source->blue;
    dest->remap = source->remap;
}

void ClutGenerator::BackupPaletteEntry(int colorIndex) {
    if (!ValidateColorIndex(colorIndex) || !m_sourcePalette) return;
    
    // Use Palette's GetPalEntry method to get the original entry
    PalEntry* sourceEntry = m_sourcePalette->GetPalEntry(colorIndex);
    if (sourceEntry) {
        CopyPaletteEntry(sourceEntry, &m_backupPalette.entries[colorIndex]);
    }
}

void ClutGenerator::RestorePaletteEntry(int colorIndex) {
    if (!ValidateColorIndex(colorIndex) || !m_sourcePalette || !m_backupPalette.hasValidData) return;
    
    // Use Palette's SetPalEntry method to restore the original entry
    m_sourcePalette->SetPalEntry(m_backupPalette.entries[colorIndex], colorIndex);
}