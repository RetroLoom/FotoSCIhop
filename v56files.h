/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a V56 file
 *
 */

#ifndef V56FILES_H
#define V56FILES_H

#include "list.h"
#include "sciloop.h"
#include "palette.h"
#include "language.h"

#pragma pack(1)

// ============================================================================
// V56 FILE FORMAT CONSTANTS
// ============================================================================

// V56 file format identifiers
// Format: 0x808000 or 0x808400 identifies a V56 file
// Structure:
//   +00 Pascal String (& NULL terminated)
//   +xx short 320
//   +x2 short 200
//   +x4 short 5
//   +x6 short 6
//   +x8 short 256
//   +xa short[6] unknown values (TODO: determine purpose)
#define V56PATCH      0x008080   // Standard V56 format identifier
#define V56PATCH84    0x008480   // Extended V56 format identifier

#pragma pack()

// ============================================================================
// UTILITY STRUCTURES
// ============================================================================

/**
 * @brief Information about a range of cells
 */
struct CellRangeInfo {
    int totalCells;
    int totalImageSize;
    int totalPackSize;
    int compressedCount;
    int uncompressedCount;
    bool hasLinks;
    
    CellRangeInfo() : totalCells(0), totalImageSize(0), totalPackSize(0), 
                     compressedCount(0), uncompressedCount(0), hasLinks(false) {}
};

/**
 * @brief Information about a range of loops
 */
struct LoopRangeInfo {
    int totalLoops;
    int totalCells;
    int totalImageSize;
    int largestLoop;        // Loop with most cells
    int smallestLoop;       // Loop with fewest cells
    int emptyLoops;         // Loops with no cells
    
    LoopRangeInfo() : totalLoops(0), totalCells(0), totalImageSize(0), 
                     largestLoop(0), smallestLoop(2147483647), emptyLoops(0) {}
};

/**
 * @brief Complete view statistics
 */
struct ViewStats {
    int totalLoops;
    int totalCells;
    int totalImageSize;
    int totalPackSize;
    int compressedCells;
    int uncompressedCells;
    int cellsWithLinks;
    int emptyCells;
    int largestCellSize;
    int smallestCellSize;
    
    ViewStats() : totalLoops(0), totalCells(0), totalImageSize(0), totalPackSize(0),
                 compressedCells(0), uncompressedCells(0), cellsWithLinks(0), emptyCells(0),
                 largestCellSize(0), smallestCellSize(2147483647) {}
};

// ============================================================================
// V56FILE CLASS
// ============================================================================

/**
 * @brief V56file class for handling Sierra V56 view files
 * 
 * This class manages SCI1.1/SCI32 format V56 files, providing functionality
 * to load, manipulate, and save view resources containing loops, cells,
 * palettes, and image data.
 */
class V56file
{
public:
    // ============================================================================
    // CONSTRUCTORS & DESTRUCTORS
    // ============================================================================
    
    /**
     * @brief Constructor with proper initialization
     */
    V56file();
    
    /**
     * @brief Destructor with proper cleanup
     */
    ~V56file();
    
    // Copy constructor and assignment operator (deleted to prevent shallow copying)
    V56file(const V56file&) = delete;
    V56file& operator=(const V56file&) = delete;
    
    // ============================================================================
    // CORE FILE I/O OPERATIONS
    // ============================================================================
    
    /**
     * @brief Load a V56 file from disk
     * @param hwnd Window handle for error messages
     * @param pszFileName Path to the file to load
     * @return Error code (ID_NOERROR on success)
     */
    int LoadFile(HWND hwnd, LPSTR pszFileName);
    
    /**
     * @brief Save the current view to a V56 file
     * @param hwnd Window handle for error messages
     * @param szFileName Path where to save the file
     * @return true on success, false on failure
     */
    bool SaveFile(HWND hwnd, LPSTR szFileName);
    
    /**
     * @brief Calculate and set file offsets for all data
     * @return 1 on success, 0 on failure
     */
    int loadCellOffset();
    
    // ============================================================================
    // CORE WRITE METHODS
    // ============================================================================
    
    int writeFileHeader(FILE* cfilebuf);
    int writeViewHeader(FILE* cfilebuf);
    int writeLoopHeaders(FILE* cfilebuf);
    int writeCellHeaders(FILE* cfilebuf);
    int writeImages(FILE* cfilebuf);
    int writeScanLines(FILE* cfilebuf);
    int writeLinks(FILE* cfilebuf);
    
    // ============================================================================
    // CELL UTILITY FUNCTIONS
    // ============================================================================
    
    // === BASIC CELL OPERATIONS ===
    int modifyCells(int loop, int base, int delta);
    int addCell(int loop, int baseIndex, int position = -1);
    int addCells(int loop, int baseIndex, int amount);
    int deleteCell(int loop, int position);
    int deleteCells(int loop, int start, int count);
    
    // === STREAMLINED CONVENIENCE FUNCTIONS ===
    int appendCell(int loop, int baseIndex);
    int appendCells(int loop, int baseIndex, int count);
    int removeLastCell(int loop);
    int removeLastCells(int loop, int count);
    
    // === ESSENTIAL COPY/MOVE OPERATIONS ===
    int copyCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos);
    int moveCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos);
    int duplicateCells(int loop, int start, int count);
    
    // === ESSENTIAL REORDER OPERATIONS ===
    int shiftCells(int loop, int start, int count, int newPos);
    int swapCells(int loop1, int index1, int loop2, int index2);
    int reverseCells(int loop, int start, int count);
    
    // === ADVANCED OPERATIONS ===
    int insertEmptyCells(int loop, int position, int count);
    
    // === BATCH OPERATIONS ===
    int batchDeleteCells(int loop, const int* indices, int indexCount);
    
    // === SEARCH OPERATIONS ===
    int findEmptyCells(int loop, int* results, int maxResults);

	// ============================================================================
    // LOOP UTILITY FUNCTIONS
    // ============================================================================
    
    // === BASIC LOOP OPERATIONS ===
    int modifyLoops(int base, int delta);
    int addLoop(int baseIndex, int position = -1);
    int addLoops(int base, int amount);  // Now uses modifyLoops internally
    int deleteLoop(int position);
    int deleteLoops(int start, int count);
    
    // === STREAMLINED CONVENIENCE FUNCTIONS ===
    int appendLoop(int baseIndex);
    int appendLoops(int baseIndex, int count);
    int removeLastLoop();
    int removeLastLoops(int count);
    
    // === ESSENTIAL COPY/MOVE OPERATIONS ===
    int copyLoops(int srcStart, int count, int dstPos);
    int moveLoops(int srcStart, int count, int dstPos);
    int duplicateLoops(int start, int count);
    
    // === ESSENTIAL REORDER OPERATIONS ===
    int shiftLoops(int start, int count, int newPos);
    int swapLoops(int index1, int index2);
    int reverseLoops(int start, int count);
    
    // === ADVANCED OPERATIONS ===
    int insertEmptyLoops(int position, int count);
    
    // === BATCH OPERATIONS ===
    int batchDeleteLoops(const int* indices, int indexCount);
    
    // === SEARCH OPERATIONS ===
    int findEmptyLoops(int* results, int maxResults);
    
    // === VALIDATION FUNCTIONS ===
    bool canDeleteLoops(int start, int count);
    bool canInsertLoops(int position, int count);
    bool canMoveLoops(int srcStart, int count, int dstPos);

	// ============================================================================
    // ACCESSOR FUNCTIONS
    // ============================================================================
    
    /**
     * @brief Get the number of loops in this view
     * @return Number of loops
     */
    int getLoopCount() const;
    
    /**
     * @brief Get the number of cells in a specific loop
     * @param loop Loop index
     * @return Number of cells, or 0 if loop is invalid
     */
    int getCellCount(int loop) const;
    
    /**
     * @brief Get the total number of cells across all loops
     * @return Total cell count
     */
    int getTotalCellCount() const;
    
    /**
     * @brief Check if a loop index is valid
     * @param loop Loop index to check
     * @return true if valid
     */
    bool isValidLoop(int loop) const;
    
    /**
     * @brief Check if a cell index is valid for a given loop
     * @param loop Loop index
     * @param cell Cell index
     * @return true if valid
     */
    bool isValidCell(int loop, int cell) const;
    
    // ============================================================================
    // VALIDATION FUNCTIONS
    // ============================================================================
    
    /**
     * @brief Check if deleting cells would be valid
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to delete
     * @return true if operation is valid
     */
    bool canDeleteCells(int loop, int start, int count);
    
    /**
     * @brief Check if inserting cells would be valid
     * @param loop Loop index
     * @param position Position to insert at
     * @param count Number of cells to insert
     * @return true if operation is valid
     */
    bool canInsertCells(int loop, int position, int count);
    
    /**
     * @brief Check if moving cells would be valid
     * @param srcLoop Source loop index
     * @param srcStart Starting cell index in source
     * @param count Number of cells to move
     * @param dstLoop Destination loop index
     * @param dstPos Position to move cells to
     * @return true if operation is valid
     */
    bool canMoveCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos);
    
    /**
     * @brief Check if a range of cells can be safely modified
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells
     * @return true if range is valid and safe to modify
     */
    bool canModifyCellRange(int loop, int start, int count);
    
    // ============================================================================
    // GUI UTILITY FUNCTIONS
    // ============================================================================
    
    /**
     * @brief Get information about a range of cells
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells in range
     * @return Information structure with statistics
     */
    CellRangeInfo getCellRangeInfo(int loop, int start, int count);
    
    /**
     * @brief Get information about a range of loops
     * @param start Starting loop index
     * @param count Number of loops in range
     * @return Information structure with statistics
     */
    LoopRangeInfo getLoopRangeInfo(int start, int count);
    
    /**
     * @brief Get detailed statistics about the entire view
     * @return Extended information structure
     */
    ViewStats getViewStats();
    
    // ============================================================================
    // PUBLIC MEMBER DATA (EXISTING INTERFACE)
    // ============================================================================
    
    Palette* palSCI;                    // Palette data
    ViewHeader Head;                    // View file header
    Loop* loops[MAX_LOOPS];            // Array of loop pointers
    unsigned long totalImageSize;      // Total size of all image data (image + pack bytes)
    unsigned long tagsTotalSize;       // Total size of image/tag bytes only (no pack), for split-view section header
    bool hasLinkVersion;               // True if this file uses the extended ViewHeaderLinks format (version >= 0x84)

private:
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
};

#endif // V56FILES_H