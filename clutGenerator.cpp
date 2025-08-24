#include "stdafx.h"
#include "ClutGenerator.h"
#include "FotoSCIhop.h"
#include "display.h"
#include <sstream>
#include <iomanip>

// Global instance
ClutGenerator* g_clutGenerator = nullptr;

ClutGenerator::ClutGenerator() 
    : m_isActive(false)
    , m_previewEnabled(false)
    , m_magicWandEnabled(false)
    , m_sourcePalette(nullptr)
    , m_selectedFromColor(0)
    , m_selectedToColor(0)
    , m_hasPreviewRemap(false)
    , m_previewFromColor(-1)
    , m_previewToColor(-1)
{
}

ClutGenerator::~ClutGenerator() {
    Shutdown();
}

bool ClutGenerator::Initialize(Palette* sourcePalette) {
    if (!sourcePalette) return false;
    
    Shutdown(); // Clean up any existing state
    
    m_sourcePalette = sourcePalette;
    
    // Backup the original palette
    BackupOriginalPalette();
    
    m_isActive = true;
    m_currentRemaps.clear();
    m_usedColorIndices.clear();
    
    // Always enable magic wand when CLUT generator is active
    SetMagicWandEnabled(true);
    
    return true;
}

void ClutGenerator::Shutdown() {
    if (m_isActive && m_backupPalette.hasValidData && m_sourcePalette) {
        // ALWAYS restore original palette when shutting down
        RestoreOriginalPalette();
    }
    
    // Always disable magic wand when shutting down
    SetMagicWandEnabled(false);
    
    m_sourcePalette = nullptr;
    m_isActive = false;
    m_backupPalette.hasValidData = false;
    m_currentRemaps.clear();
    m_usedColorIndices.clear();
    m_hasPreviewRemap = false;
}

void ClutGenerator::BackupOriginalPalette() {
    if (!m_sourcePalette) return;
    
    // Backup all 256 palette entries
    for (int i = 0; i < 256; i++) {
        PalEntry* sourceEntry = m_sourcePalette->GetPalEntry(i);
        if (sourceEntry) {
            m_backupPalette.entries[i] = *sourceEntry;
        }
    }
    
    m_backupPalette.hasValidData = true;
}

void ClutGenerator::RestoreOriginalPalette() {
    if (!m_backupPalette.hasValidData || !m_sourcePalette) return;
    
    // Restore all palette entries
    for (int i = 0; i < 256; i++) {
        m_sourcePalette->SetPalEntry(m_backupPalette.entries[i], i);
    }
    
    // Force display refresh
    ForceImageRefresh();
}

void ClutGenerator::ApplyCurrentRemaps() {
    if (!m_isActive || !m_sourcePalette || !m_backupPalette.hasValidData) {
        return;
    }
    
    // STEP 1: Restore all colors to original first
    for (int i = 0; i < 256; i++) {
        m_sourcePalette->SetPalEntry(m_backupPalette.entries[i], i);
    }
    
    // STEP 2: Apply only ACTIVE remaps
    for (size_t i = 0; i < m_currentRemaps.size(); i++) {
        const ColorRemapEntry& remap = m_currentRemaps[i];
        if (!remap.active) continue; // Skip inactive remaps
        
        if (ValidateColorIndex(remap.fromColor) && ValidateColorIndex(remap.toColor)) {
            // Get the ORIGINAL color that we want to map TO (from backup)
            PalEntry sourceEntry = m_backupPalette.entries[remap.toColor];
            
            // Apply it to the FROM color position
            m_sourcePalette->SetPalEntry(sourceEntry, remap.fromColor);
        }
    }
    
    // Force display refresh
    ForceImageRefresh();
}

void ClutGenerator::ApplyPreviewRemap() {
    if (!m_isActive || !m_sourcePalette || !m_backupPalette.hasValidData) {
        return;
    }
    
    // First, restore all colors to original and apply existing remaps
    ApplyCurrentRemaps();
    
    // Then apply the preview remap if both from and to colors are valid and different
    if (ValidateColorIndex(m_selectedFromColor) && 
        ValidateColorIndex(m_selectedToColor) && 
        m_selectedFromColor != m_selectedToColor) {
        
        // Check if this would conflict with an existing active remap
        bool hasConflict = false;
        for (size_t i = 0; i < m_currentRemaps.size(); i++) {
            if (m_currentRemaps[i].active && m_currentRemaps[i].fromColor == m_selectedFromColor) {
                hasConflict = true;
                break;
            }
        }
        
        // Only apply preview if no conflict
        if (!hasConflict) {
            PalEntry sourceEntry = m_backupPalette.entries[m_selectedToColor];
            m_sourcePalette->SetPalEntry(sourceEntry, m_selectedFromColor);
            m_hasPreviewRemap = true;
            m_previewFromColor = m_selectedFromColor;
            m_previewToColor = m_selectedToColor;
        }
    }
    
    // Force display refresh
    ForceImageRefresh();
}

void ClutGenerator::ClearPreviewRemap() {
    if (m_hasPreviewRemap) {
        m_hasPreviewRemap = false;
        // Restore to current remaps without preview
        ApplyCurrentRemaps();
    }
}

void ClutGenerator::RevertToOriginal() {
    // Clear all remaps and restore original palette
    m_currentRemaps.clear();
    m_hasPreviewRemap = false;
    RestoreOriginalPalette();
}

void ClutGenerator::ClearAllRemaps() {
    m_currentRemaps.clear();
    m_hasPreviewRemap = false;
    // Apply (which will restore to original since no remaps exist)
    ApplyCurrentRemaps();
}

void ClutGenerator::AddRemap(int fromColor, int toColor) {
    if (!ValidateColorIndex(fromColor) || !ValidateColorIndex(toColor)) return;
    
    // Clear any preview first
    m_hasPreviewRemap = false;
    
    // Remove existing remap for this fromColor
    RemoveRemap(fromColor);
    
    // Add new remap
    m_currentRemaps.push_back(ColorRemapEntry(fromColor, toColor));
    
    // Apply immediately for real-time preview
    ApplyCurrentRemaps();
}

void ClutGenerator::RemoveRemap(int fromColor) {
    // Find and remove remap with matching fromColor
    for (std::vector<ColorRemapEntry>::iterator it = m_currentRemaps.begin(); it != m_currentRemaps.end(); ) {
        if (it->fromColor == fromColor) {
            it = m_currentRemaps.erase(it);
        } else {
            ++it;
        }
    }
    
    // Apply immediately
    ApplyCurrentRemaps();
}

void ClutGenerator::ClearRemap(int index) {
    if (index >= 0 && index < static_cast<int>(m_currentRemaps.size())) {
        m_currentRemaps.erase(m_currentRemaps.begin() + index);
        // Apply immediately
        ApplyCurrentRemaps();
    }
}

void ClutGenerator::ToggleRemapActive(int index) {
    if (index >= 0 && index < static_cast<int>(m_currentRemaps.size())) {
        m_currentRemaps[index].active = !m_currentRemaps[index].active;
        // Apply immediately
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

// Get original palette entry for GUI display
bool ClutGenerator::GetOriginalPaletteEntry(int colorIndex, PalEntry& entry) const {
    if (!ValidateColorIndex(colorIndex) || !m_backupPalette.hasValidData) {
        return false;
    }
    
    entry = m_backupPalette.entries[colorIndex];
    return true;
}

void ClutGenerator::AnalyzeImageColorUsage() {
    m_usedColorIndices.clear();
    
    if (!m_isActive) return;
    
    if (globalView && curCell && (*curCell)) {
        // Analyze current view cell
        if (!(*curCell)->bmImage || !(*curCell)->bmInfo) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
            int width = (*curCell)->bmInfo->bmiHeader.biWidth;
            int height = abs((*curCell)->bmInfo->bmiHeader.biHeight);
            AnalyzeCellImageUsage((*curCell)->bmImage, width, height);
        }
    }
    else if (globalPicture && curCellIndex >= 0 && curCellIndex < globalPicture->CellsCount()) {
        // Analyze current picture cell
        Cell* cell = globalPicture->cells[curCellIndex];
        if (cell) {
            if (!cell->bmImage || !cell->bmInfo) {
                cell->GetImage(&cell->bmInfo, &cell->bmImage);
            }
            
            if (cell->bmImage && cell->bmInfo) {
                int width = cell->bmInfo->bmiHeader.biWidth;
                int height = abs(cell->bmInfo->bmiHeader.biHeight);
                AnalyzeCellImageUsage(cell->bmImage, width, height);
            }
        }
    }
}

void ClutGenerator::AnalyzeCellImageUsage(unsigned char* imageData, int width, int height) {
    if (!imageData || width <= 0 || height <= 0) return;
    
    // Calculate row width (may include padding)
    int rowWidth = ((width + 3) & ~3); // Round up to multiple of 4 for alignment
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixelIndex = y * rowWidth + x;
            unsigned char colorIndex = imageData[pixelIndex];
            m_usedColorIndices.insert(colorIndex);
        }
    }
}

bool ClutGenerator::IsColorUsedInImage(int colorIndex) const {
    return m_usedColorIndices.find(colorIndex) != m_usedColorIndices.end();
}

std::string ClutGenerator::GenerateSCITableEntry(const std::string& comment) const {
    std::ostringstream ss;
    
    // Start with a tab to match COLORTBL.SC formatting
    ss << "\t";
    
    int pairCount = 0;
    
    // Output only active remaps, limited to 12 pairs (SCI engine limitation)
    for (size_t i = 0; i < m_currentRemaps.size() && pairCount < 12; i++) {
        const ColorRemapEntry& remap = m_currentRemaps[i];
        if (!remap.active) continue;
        
        if (pairCount > 0) ss << "  ";
        
        // Format as "fromColor toColor" with proper spacing
        ss << std::setw(3) << remap.fromColor << " " << std::setw(3) << remap.toColor;
        pairCount++;
    }
    
    // Pad with -1 -1 pairs to fill the standard 12 pairs (24 numbers total)
    while (pairCount < 12) {
        if (pairCount > 0) ss << "  ";
        ss << " -1  -1";
        pairCount++;
    }
    
    // Add comment in SCI format
    ss << " ;; " << (comment.empty() ? "Generated remap" : comment);
    
    return ss.str();
}

bool ClutGenerator::ImportFromSCITableEntry(const std::string& sciLine) {
    if (!m_isActive || sciLine.empty()) return false;
    
    // Find the first semicolon and truncate there
    std::string cleanLine = sciLine;
    size_t semicolonPos = cleanLine.find(';');
    if (semicolonPos != std::string::npos) {
        cleanLine = cleanLine.substr(0, semicolonPos);
    }
    
    // Parse numbers from the line
    std::vector<int> numbers;
    std::istringstream iss(cleanLine);
    int num;
    
    while (iss >> num) {
        numbers.push_back(num);
    }
    
    // Process pairs of numbers
    int importedCount = 0;
    for (size_t i = 0; i < numbers.size() - 1; i += 2) {
        int fromColor = numbers[i];
        int toColor = numbers[i + 1];
        
        // Skip -1 -1 pairs (empty slots)
        if (fromColor == -1 || toColor == -1) continue;
        
        // Validate color indices
        if (!ValidateColorIndex(fromColor) || !ValidateColorIndex(toColor)) continue;
        
        // Add the remap (this will automatically remove any existing remap for fromColor)
        RemoveRemap(fromColor); // Clear existing first
        m_currentRemaps.push_back(ColorRemapEntry(fromColor, toColor));
        importedCount++;
    }
    
    // Apply the imported remaps
    if (importedCount > 0) {
        ApplyCurrentRemaps();
        return true;
    }
    
    return false;
}

// Helper functions
bool ClutGenerator::ValidateColorIndex(int colorIndex) const {
    return colorIndex >= 0 && colorIndex < 256;
}

bool SampleColorAtScreenPosition(int clientX, int clientY, int& colorIndex) {
    if (!g_clutGenerator || !g_clutGenerator->IsActive()) return false;
    
    // Use the same display origin calculation as the original display code
    int displayOriginX = UI_LEFT_MARGIN + picX + tableX;
    int displayOriginY = UI_TOP_MARGIN + picY;
    
    // Calculate relative position within the display area - NO MAGNIFICATION
    int relativeX = clientX - displayOriginX;
    int relativeY = clientY - displayOriginY;
    
    if (globalView && curCell && (*curCell)) {
        // For view files - sample from current view cell
        if (!(*curCell)->bmImage || !(*curCell)->bmInfo) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
            CelHeaderView* header = (CelHeaderView*)&(*curCell)->Head;
            
            // Adjust for hot spot offset (same as DisplayCurrentView function)
            int imageX = relativeX - header->xHot;
            int imageY = relativeY - header->yHot;
            
            int width = (*curCell)->bmInfo->bmiHeader.biWidth;
            int height = abs((*curCell)->bmInfo->bmiHeader.biHeight);
            
            // Check bounds
            if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                // Calculate pixel index (accounting for row padding)
                int rowWidth = ((width + 3) & ~3); // Round up to multiple of 4
                int pixelIndex = imageY * rowWidth + imageX;
                colorIndex = (*curCell)->bmImage[pixelIndex];
                return true;
            }
        }
    }
    else if (globalPicture && curCellIndex >= 0 && curCellIndex < globalPicture->CellsCount()) {
        // For picture files - we need to handle both single cell and all cells display
        if (curCellIndex == 0) {
            // When displaying all cells, we need to check each cell
            for (int i = 0; i < globalPicture->CellsCount(); i++) {
                Cell* cell = globalPicture->cells[i];
                if (!cell) continue;
                
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    // Check if click is within this cell's bounds
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        } else {
            // When displaying specific cell
            Cell* cell = globalPicture->cells[curCellIndex];
            if (cell) {
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}