/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a SCI palette
 *
 */
 
#include "stdafx.h"
#include "palette.h"

// ============================================================================
// CONSTRUCTORS AND DESTRUCTOR
// ============================================================================

Palette::Palette(void)
{
    initializeMembers();
}

Palette::~Palette(void)
{
    // No dynamic memory to clean up - palData is a fixed array
    // CompPal Head is a simple struct
}

void Palette::initializeMembers()
{
    // Initialize Head structure
    memset(&Head, 0, sizeof(Head));
    
    // Initialize default palette
    initializeDefaultPalette();
}

void Palette::initializeDefaultPalette()
{
    // Initialize palette with original default colors
    for (int i = 0; i < 256; i++)
    {
        if (i > 224)
        {
            palData[i].blue = i;
            palData[i].green = i;
        } 
        else 
        {
            palData[i].blue = 224;
            palData[i].green = 224;
        }
        
        palData[i].red = 0;      // red=255 green=0 => violet
        palData[i].remap = 0;
    }
}

// ============================================================================
// PALETTE DATA ACCESS
// ============================================================================

PalEntry* Palette::GetPalEntry(unsigned short which)
{
    if (which < 256)
        return &palData[which];

    return nullptr;
}

bool Palette::SetPalEntry(PalEntry value, unsigned short which)
{
    if (which < 256)
    {
        palData[which] = value;

        // Update header information based on the entry position
        if (which < Head.startOffset)
        {
            Head.nColors += (Head.startOffset - which);
            Head.startOffset = which;
        } 
        else if (which > Head.startOffset + Head.nColors)
        {
            Head.nColors = which - Head.startOffset;
        }

        return true;
    }

    return false;
}

// ============================================================================
// PALETTE MANAGEMENT
// ============================================================================

void Palette::noPalette() 
{ 
    // Set grayscale palette
    for (int i = 0; i < 256; i++)
    {
        palData[i].blue = 255 - i;
        palData[i].green = 255 - i;
        palData[i].red = 255 - i;	
        palData[i].remap = 0;
    }
}

bool Palette::loadPalette(FILE* cfilebuf, unsigned long palsize)
{
    // Check if the file buffer is valid
    if (!cfilebuf)
        return false;
        
    // Read the first 2 bytes of the file buffer
    unsigned short tcheck = 0;
    fread(&tcheck, 2, 1, cfilebuf);     
    
    // Check the value of tcheck, if it is not PALPATCH80 or PALPATCH
    // then set the file pointer back by 2 bytes and read the header
    if ((tcheck != PALPATCH80) && (tcheck != PALPATCH))
    {
        fseek(cfilebuf, -2, SEEK_CUR);
        fread(&Head, COMPPALSIZE, 1, cfilebuf);
        
        // Check if the data length plus 15 is equal to the size of the palette
        // If not, return false
        // Dhel - not sure what this is used for, but using it now breaks p56 importing of different size images.
        // if ((Head.reserved + 15) != palsize)    
        //    return false;             
    }
    else
    {
        fread(&Head, COMPPALSIZE, 1, cfilebuf);
    }
    
    // Allocate memory for the data based on the type value in the header
    void* data = nullptr;
    if (Head.type)
    {
        data = new PalEntryOld[Head.nColors];
        fread(data, PalEntryOldSIZE, Head.nColors, cfilebuf);
    }
    else
    {
        data = new PalEntry[Head.nColors];
        fread(data, PalEntrySIZE, Head.nColors, cfilebuf);
    }

    // Copy the data to the palData array based on the value of Head.type
    if (!Head.type)
    {
        // New format - direct copy
        for (int i = 0; i < Head.nColors; i++)
            palData[i + Head.startOffset] = ((PalEntry*)data)[i];
    }
    else
    {
        // Old format - convert to new format
        PalEntryOld tpalold;
        for (int i = 0; i < Head.nColors; i++)
        {
            tpalold = ((PalEntryOld*)data)[i];
            palData[i + Head.startOffset].red = tpalold.red;
            palData[i + Head.startOffset].green = tpalold.green;
            palData[i + Head.startOffset].blue = tpalold.blue;
            palData[i + Head.startOffset].remap = 0;
        }
    }

    // Clean up allocated memory
    if (Head.type)
        delete[] static_cast<PalEntryOld*>(data);
    else
        delete[] static_cast<PalEntry*>(data);

    // Return true indicating that the function has executed successfully
    return true;
}

void Palette::WritePalette(FILE* cfb, bool writesciheader)
{
    if (!cfb || !palData)
        return;
            
    unsigned long tsize = COMPPALSIZE + (Head.nColors * (!Head.type ? 4 : 3));

    unsigned short ttag = PALPATCH80;
    if (writesciheader)
    {
        fwrite(&ttag, 2, 1, cfb);
    }
    else
    {
        ttag = PALETTE_POS;
        fwrite(&ttag, 2, 1, cfb);
        fwrite(&tsize, 4, 1, cfb);
    }

    fwrite(&Head, COMPPALSIZE, 1, cfb);
    
    // Write palette entries based on type
    for (int i = 0; i < Head.nColors; i++)
    {
        if (!Head.type)
        {
            // New format - write full PalEntry
            fwrite(&(palData[i + Head.startOffset]), PalEntrySIZE, 1, cfb);
        }
        else
        {
            // Old format - write only RGB (skip remap byte)
            fwrite(((char*)&(palData[i + Head.startOffset])) + 1, PalEntryOldSIZE, 1, cfb);
        }
    }
}