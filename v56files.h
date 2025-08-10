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
    // Constructor with proper initialization
    V56file();
    
    // Destructor with proper cleanup
    ~V56file();
    
    // Copy constructor and assignment operator (deleted to prevent shallow copying)
    V56file(const V56file&) = delete;
    V56file& operator=(const V56file&) = delete;
    
    // ============================================================================
    // FILE I/O OPERATIONS (EXISTING INTERFACE)
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
    
    // ============================================================================
    // EXISTING LOOP/CELL MANAGEMENT (BACKWARD COMPATIBILITY)
    // ============================================================================
    
    /**
     * @brief Add loops by duplicating an existing loop
     * @param base Index of the loop to copy
     * @param amount Number of copies to create
     * @return 1 on success, 0 on failure
     */
    int addLoops(int base, int amount);
    
    /**
     * @brief Add cells by duplicating an existing cell
     * @param loop Index of the loop containing the cell
     * @param base Index of the cell to copy
     * @param amount Number of copies to create
     * @return 1 on success, 0 on failure
     */
    int addCells(int loop, int base, int amount);
    
    /**
     * @brief Calculate and set file offsets for all data
     * @return 1 on success, 0 on failure
     */
    int loadCellOffset();
    
    // ============================================================================
    // EXISTING WRITE METHODS (BACKWARD COMPATIBILITY)
    // ============================================================================
    
    int writeFileHeader(FILE* cfilebuf);
    int writeViewHeader(FILE* cfilebuf);
    int writeLoopHeaders(FILE* cfilebuf);
    int writeCellHeaders(FILE* cfilebuf);
    int writeImages(FILE* cfilebuf);
    int writeScanLines(FILE* cfilebuf);
    int writeLinks(FILE* cfilebuf);
    
    // ============================================================================
    // PUBLIC MEMBER DATA (EXISTING INTERFACE)
    // ============================================================================
    
    Palette* palSCI;                    // Palette data
    ViewHeader Head;                    // View file header
    Loop* loops[MAX_LOOPS];            // Array of loop pointers
    
    // Make totalImageSize public for backward compatibility
    unsigned long totalImageSize = 0;

private:
    // ============================================================================
    // PRIVATE UTILITY METHODS (MINIMAL SET)
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