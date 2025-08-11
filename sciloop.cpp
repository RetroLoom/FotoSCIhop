/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a SCI loop from a V56 file
 *
 */
 
#include "StdAfx.h"
#include "sciloop.h"

// Constructor and destructor implementations for sciloop.cpp

#include "StdAfx.h"
#include "sciloop.h"

Loop::Loop(void)
{
    initializeMembers();
}

Loop::~Loop(void)
{
    cleanup();
}

void Loop::initializeMembers()
{
    // Initialize Head structure
    memset(&Head, 0, sizeof(Head));
    
    // Initialize cells array to null pointers
    for (int i = 0; i < MAX_CELLS; i++) {
        cells[i] = nullptr;
    }
}

void Loop::cleanup()
{
    // Clean up all cell pointers
    for (int i = 0; i < MAX_CELLS; i++) {
        delete cells[i];
        cells[i] = nullptr;
    }
}