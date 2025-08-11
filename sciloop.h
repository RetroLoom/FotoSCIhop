/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a loop from a V56 file
 *
 */
 
#ifndef SCILOOP_H
#define SCILOOP_H

#include "list.h"
#include "scicell.h"

#pragma pack(1)
#pragma pack()

/**
 * @brief Loop class for handling SCI loop data from V56 files
 * 
 * This class manages a loop containing multiple cells, including
 * proper memory management and initialization.
 */
class Loop
{
public:
    // ============================================================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ============================================================================
    
    /**
     * @brief Default constructor
     */
    Loop(void);
    
    /**
     * @brief Destructor
     */
    ~Loop(void);
    
    // Prevent copying to avoid shallow copy issues
    Loop(const Loop&) = delete;
    Loop& operator=(const Loop&) = delete;
    
    // ============================================================================
    // PUBLIC MEMBER DATA
    // ============================================================================
    
    Cell* cells[MAX_CELLS];        // Array of cell pointers
    LoopHeader Head;               // Loop header structure

private:
    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================
    
    /**
     * @brief Initialize member variables to safe defaults
     */
    void initializeMembers();
    
    /**
     * @brief Clean up allocated memory
     */
    void cleanup();
};

#endif // SCILOOP_H