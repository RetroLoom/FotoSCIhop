/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a P56 file 
 *
 */
 
#ifndef P56FILES_H
#define P56FILES_H

#include "list.h"
#include "scicell.h"
#include "palette.h"
#include "language.h"

#pragma pack(1)

// P56 file format identifiers
#define P56PATCH      0x00000101   // Legacy identifier (rarely used)
#define P56PATCH80    0x00008181   // Standard P56 format identifier
#define P56PATCHOLD   0x00008081   // Older P56 format identifier

struct PicCellRangeInfo {
    int totalCells;
    int totalImageSize;
    int totalPackSize;
    int compressedCount;
    int uncompressedCount;
    bool hasLinks;
    
    PicCellRangeInfo() : totalCells(0), totalImageSize(0), totalPackSize(0), 
                     compressedCount(0), uncompressedCount(0), hasLinks(false) {}
};

struct PicStats {
    int totalCells;
    int totalImageSize;
    int totalPackSize;
    int compressedCells;
    int uncompressedCells;
    int emptyCells;
    int largestCellSize;
    int smallestCellSize;
    
    PicStats() : totalCells(0), totalImageSize(0), totalPackSize(0),
                 compressedCells(0), uncompressedCells(0), emptyCells(0),
                 largestCellSize(0), smallestCellSize(2147483647) {}
};

// Union for different P56 header formats
union P56HEAD {
    PicHeader32 pic32;  // SCI32 format header
    PicHeader11 pic11;  // SCI1.1 format header
};

// Enumeration for picture format types
enum PicFormat {
    _PIC_11,  // SCI1.1 format (older)
    _PIC_32   // SCI32 format (newer)
};

#pragma pack()

/**
 * @brief P56file32 class for handling Sierra P56 picture files
 * 
 * This class manages both SCI1.1 and SCI32 format P56 files, providing
 * functionality to load, manipulate, and save picture resources with
 * cells, palettes, and vector data.
 */
class P56file32
{
public:
    // Constructor with proper initialization
    P56file32();
    
    // Destructor with proper cleanup
    ~P56file32();
    
    // Copy constructor and assignment operator (deleted to prevent shallow copying)
    P56file32(const P56file32&) = delete;
    P56file32& operator=(const P56file32&) = delete;
    
    // ============================================================================
    // BACKWARD COMPATIBILITY - Keep existing interface
    // ============================================================================
    
    // Keep the old CellsCount method for backward compatibility
    unsigned char CellsCount() const { return getCellsCount(); }
    void CellsCount(unsigned char value) { setCellsCount(value); }
    
    // Make format public for backward compatibility
    int format = -1;
    
    // ============================================================================
    // PUBLIC ACCESSORS
    // ============================================================================
    
    /**
     * @brief Get the number of cells in the current picture
     * @return Number of cells
     */
    unsigned char getCellsCount() const;
    
    /**
     * @brief Set the number of cells in the current picture
     * @param value New cell count
     */
    void setCellsCount(unsigned char value);
    
    /**
     * @brief Get the current picture format
     * @return PicFormat enum value
     */
    PicFormat getFormat() const { return static_cast<PicFormat>(format); }
    
    /**
     * @brief Check if the file has been loaded successfully
     * @return true if file is loaded and valid
     */
    bool isLoaded() const { return (format >= 0 && palSCI != nullptr); }
    
    /**
     * @brief Validate cell index (backward compatibility)
     * @param index Cell index to validate
     * @return true if valid
     */
    bool IsValidIndex(int index) const { return isValidCellIndex(index); }
    
    // ============================================================================
    // FILE I/O OPERATIONS
    // ============================================================================
    
    /**
     * @brief Load a P56 file from disk
     * @param hwnd Window handle for error messages
     * @param pszFileName Path to the file to load
     * @return Error code (ID_NOERROR on success)
     */
    int LoadFile(HWND hwnd, LPSTR pszFileName);
    
    /**
     * @brief Save the current picture to a P56 file
     * @param hwnd Window handle for error messages
     * @param szFileName Path where to save the file
     * @return true on success, false on failure
     */
    bool SavePic(HWND hwnd, LPSTR szFileName);
    
    // ============================================================================
    // CELL MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Get a pointer to a specific cell
     * @param index Cell index
     * @return Pointer to cell or nullptr if invalid
     */
    Cell* getCell(int index) const;
    
    /**
     * @brief Get the total size of all image data
     * @return Total image size in bytes
     */
    unsigned long getImageAllSize() const { return imageAllSize; }

	// ============================================================================
    // P56 CELL UTILITY FUNCTIONS (SCI32 & SCI11)
    // ============================================================================
    
    // === BASIC CELL OPERATIONS ===
    int modifyCells(int base, int delta);
    int addCell(int baseIndex, int position = -1);
    int addCells(int base, int amount);  // Now uses modifyCells internally
    int deleteCell(int position);
    int deleteCells(int start, int count);
    
    // === STREAMLINED CONVENIENCE FUNCTIONS ===
    int appendCell(int baseIndex);
    int appendCells(int baseIndex, int count);
    int removeLastCell();
    int removeLastCells(int count);
    
    // === ESSENTIAL COPY/MOVE OPERATIONS ===
    int copyCells(int srcStart, int count, int dstPos);
    int moveCells(int srcStart, int count, int dstPos);
    int duplicateCells(int start, int count);
    
    // === ESSENTIAL REORDER OPERATIONS ===
    int shiftCells(int start, int count, int newPos);
    int swapCells(int index1, int index2);
    int reverseCells(int start, int count);
    
    // === ADVANCED OPERATIONS ===
    int insertEmptyCells(int position, int count);
    
    // === BATCH OPERATIONS ===
    int batchDeleteCells(const int* indices, int indexCount);
    
    // === SEARCH OPERATIONS ===
    int findEmptyCells(int* results, int maxResults);
    
    // === MAINTENANCE OPERATIONS ===
    int optimizeCells();
        
    // === P56 UTILITY FUNCTIONS ===
    PicCellRangeInfo getCellRangeInfo(int start, int count);
    PicStats getPicStats();
    
    // === HELPER FUNCTIONS FOR DUAL FORMAT SUPPORT ===
    int getCellWidth(int index) const;
    int getCellHeight(int index) const;
    int getCellCompressType(int index) const;
    
    // ============================================================================
    // PUBLIC MEMBER DATA
    // ============================================================================
    
    Palette* palSCI;                    // Palette data
    P56HEAD Head;                       // File header (format-specific)
    Cell* cells[MAX_CELLS];            // Array of cell pointers
    unsigned char* vector;              // Vector data (PIC_11 format only)
    
    // Unknown shorts (PIC_11 format only, usually 0xFFFF)
    short _unkShort1;
    short _unkShort2;
    
    // Total size of all image data (public for backward compatibility)
    unsigned long imageAllSize = 0;

private:
    // ============================================================================
    // PRIVATE LOADING METHODS
    // ============================================================================
    
    /**
     * @brief Load SCI32 format picture data
     * @param cfilebuf File buffer
     * @param offset File offset
     * @return Error code
     */
    int LoadPic32(FILE* cfilebuf, unsigned char offset);
    
    /**
     * @brief Load SCI1.1 format picture data
     * @param cfilebuf File buffer
     * @param offset File offset
     * @return Error code
     */
    int LoadPic11(FILE* cfilebuf, unsigned char offset);
    
    // ============================================================================
    // PRIVATE SAVING METHODS
    // ============================================================================
    
    /**
     * @brief Calculate and set file offsets for all data
     * @return 1 on success, 0 on failure
     */
    int loadCellOffset();
    
    /**
     * @brief Write file header based on current format
     * @param cfilebuf File buffer
     * @return 1 on success, 0 on failure
     */
    int writeFileHeader(FILE* cfilebuf);
    
    /**
     * @brief Write picture header
     * @param cfilebuf File buffer
     * @param cellCount Number of cells to write
     * @return 1 on success, 0 on failure
     */
    int writePicHeader(FILE* cfilebuf, int cellCount = 0);
    
    /**
     * @brief Write all cell headers
     * @param cfilebuf File buffer
     * @param cellCount Number of cells to write
     * @return 1 on success, 0 on failure
     */
    int writeCellHeaders(FILE* cfilebuf, int cellCount = 0);
    
    /**
     * @brief Write image data (format-specific)
     * @param cfilebuf File buffer
     * @param cellCount Number of cells to write
     * @return 1 on success, 0 on failure
     */
    int writeImages(FILE* cfilebuf, int cellCount = 0);
    
    /**
     * @brief Write SCI32 format image data
     * @param cfilebuf File buffer
     * @param cellCount Number of cells to write
     * @return 1 on success, 0 on failure
     */
    int writePic32Images(FILE* cfilebuf, int cellCount);
    
    /**
     * @brief Write SCI1.1 format image data
     * @param cfilebuf File buffer
     * @param cellCount Number of cells to write
     * @return 1 on success, 0 on failure
     */
    int writePic11Images(FILE* cfilebuf, int cellCount);
    
    // ============================================================================
    // PRIVATE UTILITY METHODS
    // ============================================================================
    
    /**
     * @brief Initialize all member variables to safe defaults
     */
    void initializeMembers();
    
    /**
     * @brief Clean up all allocated resources
     */
    void cleanup();
    
    /**
     * @brief Validate cell index
     * @param index Cell index to validate
     * @return true if valid
     */
    bool isValidCellIndex(int index) const;
};

// ============================================================================
// INLINE IMPLEMENTATIONS
// ============================================================================

inline unsigned char P56file32::getCellsCount() const
{
    return (format == _PIC_11) ? Head.pic11.celCount : Head.pic32.celCount;
}

inline void P56file32::setCellsCount(unsigned char value)
{
    if (format == _PIC_11) {
        Head.pic11.celCount = value;
    } else {
        Head.pic32.celCount = value;
    }
}

inline Cell* P56file32::getCell(int index) const
{
    return isValidCellIndex(index) ? cells[index] : nullptr;
}

inline bool P56file32::isValidCellIndex(int index) const
{
    return (index >= 0 && index < getCellsCount() && cells[index] != nullptr);
}

#endif // P56FILES_H