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
    // LEGACY LOOP MANAGEMENT (BACKWARD COMPATIBILITY)
    // ============================================================================
    
    /**
     * @brief Add loops by duplicating an existing loop
     * @param base Index of the loop to copy
     * @param amount Number of copies to create
     * @return 1 on success, 0 on failure
     */
    int addLoops(int base, int amount);
    
    // ============================================================================
    // BASIC CELL OPERATIONS
    // ============================================================================
    
    /**
     * @brief Add a cell by duplicating an existing cell
     * @param loop Loop index
     * @param baseIndex Index of cell to copy
     * @param position Position to insert new cell (use -1 to append at end)
     * @return 1 on success, 0 on failure
     */
    int addCell(int loop, int baseIndex, int position = -1);
    
    /**
     * @brief Add multiple cells by duplicating an existing cell
     * @param loop Loop index
     * @param baseIndex Index of cell to copy
     * @param amount Number of cells to add
     * @return 1 on success, 0 on failure
     */
    int addCells(int loop, int baseIndex, int amount);
    
    /**
     * @brief Delete a cell at a specific position
     * @param loop Loop index
     * @param position Position of cell to delete
     * @return 1 on success, 0 on failure
     */
    int deleteCell(int loop, int position);
    
    /**
     * @brief Delete a range of cells from a loop
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to delete
     * @return 1 on success, 0 on failure
     */
    int deleteCells(int loop, int start, int count);
    
    // ============================================================================
    // CONVENIENCE FUNCTIONS
    // ============================================================================
    
    /**
     * @brief Insert a copy of a cell at a specific position
     * @param loop Loop index
     * @param baseIndex Index of cell to copy
     * @param position Position to insert at
     * @return 1 on success, 0 on failure
     */
    int insertCell(int loop, int baseIndex, int position);
    
    /**
     * @brief Append a copy of a cell at the end of a loop
     * @param loop Loop index
     * @param baseIndex Index of cell to copy
     * @return 1 on success, 0 on failure
     */
    int appendCell(int loop, int baseIndex);
    
    // ============================================================================
    // COPY OPERATIONS
    // ============================================================================
    
    /**
     * @brief Copy cells from one position to another
     * @param srcLoop Source loop index
     * @param srcStart Starting cell index in source
     * @param count Number of cells to copy
     * @param dstLoop Destination loop index
     * @param dstPos Position to insert copied cells
     * @return 1 on success, 0 on failure
     */
    int copyCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos);
    
    /**
     * @brief Copy a single cell
     * @param srcLoop Source loop index
     * @param srcIndex Source cell index
     * @param dstLoop Destination loop index
     * @param dstPos Position to insert copied cell
     * @return 1 on success, 0 on failure
     */
    int copyCell(int srcLoop, int srcIndex, int dstLoop, int dstPos);
    
    // ============================================================================
    // MOVE OPERATIONS
    // ============================================================================
    
    /**
     * @brief Move cells from one position to another
     * @param srcLoop Source loop index
     * @param srcStart Starting cell index in source
     * @param count Number of cells to move
     * @param dstLoop Destination loop index
     * @param dstPos Position to move cells to
     * @return 1 on success, 0 on failure
     */
    int moveCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos);
    
    /**
     * @brief Move a single cell
     * @param srcLoop Source loop index
     * @param srcIndex Source cell index
     * @param dstLoop Destination loop index
     * @param dstPos Position to move cell to
     * @return 1 on success, 0 on failure
     */
    int moveCell(int srcLoop, int srcIndex, int dstLoop, int dstPos);
    
    // ============================================================================
    // REORDER OPERATIONS
    // ============================================================================
    
    /**
     * @brief Shift cells within the same loop (reorder)
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to move
     * @param newPos New position for the cells
     * @return 1 on success, 0 on failure
     */
    int shiftCells(int loop, int start, int count, int newPos);
    
    /**
     * @brief Duplicate cells (insert copies right after originals)
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to duplicate
     * @return 1 on success, 0 on failure
     */
    int duplicateCells(int loop, int start, int count);
    
    /**
     * @brief Duplicate a single cell
     * @param loop Loop index
     * @param index Cell index to duplicate
     * @return 1 on success, 0 on failure
     */
    int duplicateCell(int loop, int index);
    
    /**
     * @brief Swap two cells
     * @param loop1 First loop index
     * @param index1 First cell index
     * @param loop2 Second loop index
     * @param index2 Second cell index
     * @return 1 on success, 0 on failure
     */
    int swapCells(int loop1, int index1, int loop2, int index2);
    
    // ============================================================================
    // ADVANCED CELL OPERATIONS
    // ============================================================================
    
    /**
     * @brief Insert empty cells at a specific position
     * @param loop Loop index
     * @param position Position to insert at
     * @param count Number of empty cells to insert
     * @return 1 on success, 0 on failure
     */
    int insertEmptyCells(int loop, int position, int count);
    
    /**
     * @brief Replace cells in one loop with cells from another
     * @param dstLoop Destination loop
     * @param dstStart Starting position in destination
     * @param srcLoop Source loop
     * @param srcStart Starting position in source
     * @param count Number of cells to replace
     * @return 1 on success, 0 on failure
     */
    int replaceCells(int dstLoop, int dstStart, int srcLoop, int srcStart, int count);
    
    /**
     * @brief Reverse the order of cells in a range
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to reverse
     * @return 1 on success, 0 on failure
     */
    int reverseCells(int loop, int start, int count);
    
    /**
     * @brief Sort cells in a loop by given criteria
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to sort
     * @param sortBy Sort criteria (0=width, 1=height, 2=size)
     * @param ascending true for ascending order
     * @return 1 on success, 0 on failure
     */
    int sortCells(int loop, int start, int count, int sortBy, bool ascending);
    
    // ============================================================================
    // BATCH OPERATIONS
    // ============================================================================
    
    /**
     * @brief Delete multiple non-contiguous cells
     * @param loop Loop index
     * @param indices Array of cell indices to delete
     * @param indexCount Number of indices in the array
     * @return 1 on success, 0 on failure
     */
    int batchDeleteCells(int loop, const int* indices, int indexCount);
    
    // ============================================================================
    // SEARCH AND FIND OPERATIONS
    // ============================================================================
    
    /**
     * @brief Find cells with specific dimensions
     * @param loop Loop index to search in (-1 for all loops)
     * @param width Width to search for (0 for any width)
     * @param height Height to search for (0 for any height)
     * @param results Array to store found cell positions (loop, cell pairs)
     * @param maxResults Maximum number of results to return
     * @return Number of cells found
     */
    int findCellsBySize(int loop, int width, int height, int* results, int maxResults);
    
    /**
     * @brief Find empty or invalid cells
     * @param loop Loop index to search in (-1 for all loops)
     * @param results Array to store found cell positions (loop, cell pairs)
     * @param maxResults Maximum number of results to return
     * @return Number of empty cells found
     */
    int findEmptyCells(int loop, int* results, int maxResults);
    
    // ============================================================================
    // GUI UTILITY FUNCTIONS
    // ============================================================================
    
    /**
     * @brief Get information about a cell range
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells in range
     * @return Information structure with statistics
     */
    CellRangeInfo getCellRangeInfo(int loop, int start, int count);
    
    /**
     * @brief Get information about a loop range
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
    // MAINTENANCE OPERATIONS
    // ============================================================================
    
    /**
     * @brief Cut cells (copy then delete)
     * @param loop Loop index
     * @param start Starting cell index
     * @param count Number of cells to cut
     * @param clipboard Array to store copied cells
     * @return 1 on success, 0 on failure
     */
    int cutCells(int loop, int start, int count, Cell** clipboard);
    
    /**
     * @brief Optimize a loop by removing empty cells and compacting
     * @param loop Loop index
     * @return Number of cells removed
     */
    int optimizeLoop(int loop);
    
    // ============================================================================
    // PUBLIC MEMBER DATA (EXISTING INTERFACE)
    // ============================================================================
    
    Palette* palSCI;                    // Palette data
    ViewHeader Head;                    // View file header
    Loop* loops[MAX_LOOPS];            // Array of loop pointers
    unsigned long totalImageSize;      // Total size of all image data

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