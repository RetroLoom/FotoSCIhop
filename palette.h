/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a SCI palette
 *
 */
 
#ifndef PALETTE_H
#define PALETTE_H

#pragma pack(1)

// ============================================================================
// PALETTE ENTRY STRUCTURES
// ============================================================================

struct PalEntryOld
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct PalEntry
{
    unsigned char remap;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct BMPColorHead
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char color_space_type;
    unsigned char unused;
};

// ============================================================================
// PALETTE HEADER STRUCTURES
// ============================================================================

struct PalHeader
{
    short palID;
    char hdSize;
    char palName[9];
    char palCount;
    short reserved;
};

struct CompPal : public PalHeader
{
    char title[10];         // 8 chars, 0 terminated
    uchar startOffset;
    uchar nCycles;          // number of cycling ranges following header
    UInt16 fe;              // future expansion (0)
    UInt16 nColors;         // number of "colors" defined in this palette
    uchar def;              // "Default" flag setting (1)
    uchar type;             // (0 = each RGB has flag)
                           // (1 = All RGBs share default flag)
    UInt32 valid;
};

#pragma pack()

// ============================================================================
// SIZE CONSTANTS
// ============================================================================

#define PalEntryOldSIZE (sizeof(PalEntryOld))
#define PalEntrySIZE (sizeof(PalEntry))
#define BMPColorHeadSize (sizeof(BMPColorHead))
#define PALHEADERSIZE (sizeof(PalHeader))
#define COMPPALSIZE (sizeof(CompPal))

// ============================================================================
// FORMAT CONSTANTS
// ============================================================================

#define PALPATCH80    0x008B
#define PALPATCH      0x000B
#define PALETTE_POS   0x0300

/**
 * @brief Palette class for handling SCI palette data
 * 
 * This class manages palette information including loading from files,
 * color management, and writing back to SCI format files.
 */
class Palette
{
public:
    // ============================================================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ============================================================================
    
    /**
     * @brief Default constructor
     */
    Palette(void);
    
    /**
     * @brief Destructor
     */
    ~Palette(void);
    
    // Prevent copying to avoid issues with the large palData array
    Palette(const Palette&) = delete;
    Palette& operator=(const Palette&) = delete;
    
    // ============================================================================
    // PALETTE DATA ACCESS
    // ============================================================================
    
    /**
     * @brief Get a palette entry by index
     * @param which Index of the palette entry (0-255)
     * @return Pointer to palette entry or null if index invalid
     */
    PalEntry* GetPalEntry(unsigned short which);
    
    /**
     * @brief Set a palette entry by index
     * @param value New palette entry value
     * @param which Index to set (0-255)
     * @return True if successful, false if index invalid
     */
    bool SetPalEntry(PalEntry value, unsigned short which);
    
    // ============================================================================
    // PALETTE MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Load palette from file buffer
     * @param cfilebuf File buffer to read from
     * @param palsize Size of palette data
     * @return True if successful, false otherwise
     */
    bool loadPalette(FILE* cfilebuf, unsigned long palsize);
    
    /**
     * @brief Set palette to grayscale default
     */
    void noPalette();
    
    /**
     * @brief Write palette to file
     * @param cfb File buffer to write to
     * @param writesciheader Whether to write SCI header
     */
    void WritePalette(FILE* cfb, bool writesciheader);
    
    // ============================================================================
    // PUBLIC MEMBER DATA
    // ============================================================================
    
    PalEntry palData[256];          // Palette data array
    CompPal Head;                   // Palette header

private:
    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================
    
    /**
     * @brief Initialize member variables to safe defaults
     */
    void initializeMembers();
    
    /**
     * @brief Initialize default palette colors
     */
    void initializeDefaultPalette();
};

#endif // PALETTE_H