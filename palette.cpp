/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a SCI palette
 *
 *  Palette on-disk format (at paletteOffset in the resource header):
 *
 *    The game client's HunkPalette::Init() receives a pointer directly at
 *    paletteOffset and interprets it as:
 *
 *      PalHeader (13 bytes, game-client layout, NO palID field):
 *        [0]      hdSize      (1 byte)
 *        [1..9]   palName     (9 bytes)
 *        [10]     palCount    (1 byte)
 *        [11..12] reserved    (2 bytes)
 *
 *      CompPal (22 bytes, at PalHeader + 13 + 2*palCount):
 *        [0..9]   title       (10 bytes)
 *        [10]     startOffset (1 byte)
 *        [11]     nCycles     (1 byte)
 *        [12..13] fe          (2 bytes)
 *        [14..15] nColors     (2 bytes)
 *        [16]     def         (1 byte)
 *        [17]     type        (1 byte)
 *        [18..21] valid       (4 bytes)
 *
 *      Color entries (nColors * 4 bytes if type==0, or nColors * 3 bytes if type==1)
 *
 *  FotoSCIhop's CompPal struct extends PalHeader which includes palID(2) at the
 *  front. We skip palID when writing/reading so the on-disk layout matches the
 *  game client exactly.
 *
 *  The 6-byte section tag+size prefix (PALETTE_POS + uint32 size) is written
 *  immediately before the PalHeader. paletteOffset points PAST this prefix,
 *  directly at PalHeader (hdSize byte), matching the game client's expectation.
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
}

void Palette::initializeMembers()
{
    memset(&Head, 0, sizeof(Head));
    initializeDefaultPalette();
}

void Palette::initializeDefaultPalette()
{
    for (int i = 0; i < 256; i++)
    {
        if (i > 224)
        {
            palData[i].blue  = i;
            palData[i].green = i;
        }
        else
        {
            palData[i].blue  = 224;
            palData[i].green = 224;
        }
        palData[i].red   = 0;
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
    if (which >= 256)
        return false;

    palData[which] = value;

    if (which < (unsigned short)Head.startOffset)
    {
        Head.nColors += (Head.startOffset - which);
        Head.startOffset = which;
    }
    else if (which > (unsigned short)(Head.startOffset + Head.nColors))
    {
        Head.nColors = which - Head.startOffset;
    }

    return true;
}

// ============================================================================
// PALETTE MANAGEMENT
// ============================================================================

void Palette::noPalette()
{
    hasPalette = false;
    for (int i = 0; i < 256; i++)
    {
        palData[i].blue  = 255 - i;
        palData[i].green = 255 - i;
        palData[i].red   = 255 - i;
        palData[i].remap = 0;
    }
}

// ----------------------------------------------------------------------------
// loadPalette
//
// Reads the palette from the current file position.  The caller has already
// seeked to paletteOffset (i.e. the first byte of the on-disk PalHeader,
// which is hdSize).  We skip the palID field that FotoSCIhop's CompPal struct
// has but the game client's PalHeader does not.
//
// On-disk layout at the current file position:
//   hdSize(1) palName(9) palCount(1) reserved(2)   <- game-client PalHeader
//   title(10) startOffset(1) nCycles(1) fe(2) nColors(2) def(1) type(1) valid(4)
//   color entries...
//
// We reconstruct FotoSCIhop's CompPal (which has palID prepended) by reading
// the fields individually.
// ----------------------------------------------------------------------------
bool Palette::loadPalette(FILE* cfilebuf, unsigned long /*palsize*/)
{
    if (!cfilebuf)
        return false;

    // Read game-client PalHeader (13 bytes, no palID)
    unsigned char  hdSize   = 0;
    char           palName[9];
    unsigned char  palCount = 0;
    short          reserved = 0;

    if (fread(&hdSize,   1,  1, cfilebuf) != 1) return false;
    if (fread(palName,   9,  1, cfilebuf) != 1) return false;
    if (fread(&palCount, 1,  1, cfilebuf) != 1) return false;
    if (fread(&reserved, 2,  1, cfilebuf) != 1) return false;

    // Skip 2*palCount bytes (palette index table, always 1 entry = 2 bytes)
    if (palCount > 0)
        fseek(cfilebuf, 2 * palCount, SEEK_CUR);

    // Read CompPal (22 bytes)
    char           title[10];
    unsigned char  startOffset = 0;
    unsigned char  nCycles     = 0;
    unsigned short fe          = 0;
    unsigned short nColors     = 0;
    unsigned char  def         = 0;
    unsigned char  type        = 0;
    unsigned int   valid       = 0;

    if (fread(title,        10, 1, cfilebuf) != 1) return false;
    if (fread(&startOffset,  1, 1, cfilebuf) != 1) return false;
    if (fread(&nCycles,      1, 1, cfilebuf) != 1) return false;
    if (fread(&fe,           2, 1, cfilebuf) != 1) return false;
    if (fread(&nColors,      2, 1, cfilebuf) != 1) return false;
    if (fread(&def,          1, 1, cfilebuf) != 1) return false;
    if (fread(&type,         1, 1, cfilebuf) != 1) return false;
    if (fread(&valid,        4, 1, cfilebuf) != 1) return false;

    // Populate FotoSCIhop's CompPal (palID is not stored on disk; set to 0)
    Head.palID       = 0;
    Head.hdSize      = (char)hdSize;
    memcpy(Head.palName, palName, 9);
    Head.palCount    = (char)palCount;
    Head.reserved    = reserved;
    memcpy(Head.title, title, 10);
    Head.startOffset = startOffset;
    Head.nCycles     = nCycles;
    Head.fe          = fe;
    Head.nColors     = nColors;
    Head.def         = def;
    Head.type        = type;
    Head.valid       = valid;

    if (nColors == 0 || nColors > 256)
        return false;

    // Read color entries
    if (!type)    {
        // New format: each entry has remap(1) + red(1) + green(1) + blue(1)
        for (int i = 0; i < nColors; i++)
        {
            if (fread(&palData[i + startOffset], PalEntrySIZE, 1, cfilebuf) != 1)
                return false;
        }
    }
    else
    {
        // Old format: each entry has red(1) + green(1) + blue(1), no remap byte
        PalEntryOld tmp;
        for (int i = 0; i < nColors; i++)
        {
            if (fread(&tmp, PalEntryOldSIZE, 1, cfilebuf) != 1)
                return false;
            palData[i + startOffset].red   = tmp.red;
            palData[i + startOffset].green = tmp.green;
            palData[i + startOffset].blue  = tmp.blue;
            palData[i + startOffset].remap = def;
        }
    }

    hasPalette = true;
    return true;
}

// ----------------------------------------------------------------------------
// WritePalette
//
// Writes the palette to the file.  The caller is responsible for seeking to
// the correct position before calling this.
//
// When writesciheader == false (normal resource embedding):
//   Writes: PALETTE_POS(2) + size(4) + PalHeader(13) + CompPal(22) + entries
//   paletteOffset in the resource header must point to the PalHeader (i.e.
//   6 bytes after the start of this block, at the hdSize byte).
//
// When writesciheader == true (standalone .pal patch file):
//   Writes: PALPATCH80(2) + PalHeader(13) + CompPal(22) + entries
//   (no size field; the 2-byte tag is the patch file identifier)
//
// In both cases the game client reads from paletteOffset which points at hdSize.
// We write hdSize first (skipping palID) so the layout matches exactly.
// ----------------------------------------------------------------------------
void Palette::WritePalette(FILE* cfb, bool writesciheader)
{
    if (!cfb)
        return;

    // Size of the palette data block (PalHeader + CompPal + entries), no tag/size prefix
    const unsigned long entryBytes = Head.nColors * (!Head.type ? PalEntrySIZE : PalEntryOldSIZE);
    const unsigned long blockSize  = PAL_HEADER_GAME_SIZE + PAL_COMPPAL_GAME_SIZE + entryBytes;

    if (writesciheader)
    {
        // Standalone patch file: 2-byte tag, then PalHeader directly
        unsigned short tag = PALPATCH80;
        fwrite(&tag, 2, 1, cfb);
    }
    else
    {
        // Embedded in resource: 2-byte section tag + 4-byte size, then PalHeader
        unsigned short tag = PALETTE_POS;
        fwrite(&tag,       2, 1, cfb);
        fwrite(&blockSize, 4, 1, cfb);
    }

    // Write game-client PalHeader (13 bytes, NO palID field)
    unsigned char  hdSize   = (unsigned char)Head.hdSize;
    unsigned char  palCount = (unsigned char)Head.palCount;
    fwrite(&hdSize,        1,  1, cfb);
    fwrite(Head.palName,   9,  1, cfb);
    fwrite(&palCount,      1,  1, cfb);
    fwrite(&Head.reserved, 2,  1, cfb);

    // Write 2*palCount palette index bytes (one entry = 2 bytes, value = 0)
    if (palCount > 0)
    {
        unsigned short idx = 0;
        for (int i = 0; i < palCount; i++)
            fwrite(&idx, 2, 1, cfb);
    }

    // Write CompPal (22 bytes)
    fwrite(Head.title,        10, 1, cfb);
    fwrite(&Head.startOffset,  1, 1, cfb);
    fwrite(&Head.nCycles,      1, 1, cfb);
    fwrite(&Head.fe,           2, 1, cfb);
    fwrite(&Head.nColors,      2, 1, cfb);
    fwrite(&Head.def,          1, 1, cfb);
    fwrite(&Head.type,         1, 1, cfb);
    fwrite(&Head.valid,        4, 1, cfb);

    // Write color entries
    for (int i = 0; i < Head.nColors; i++)
    {
        if (!Head.type)
        {
            fwrite(&palData[i + Head.startOffset], PalEntrySIZE, 1, cfb);
        }
        else
        {
            // Old format: skip remap byte, write only RGB
            fwrite(((char*)&palData[i + Head.startOffset]) + 1, PalEntryOldSIZE, 1, cfb);
        }
    }
}

// ----------------------------------------------------------------------------
// PaletteBlockSize
//
// Returns the total number of bytes written by WritePalette (including the
// 6-byte tag+size prefix when writesciheader==false, or 2-byte tag when true).
// Used by loadCellOffset() to calculate subsequent section offsets.
// ----------------------------------------------------------------------------
unsigned long Palette::PaletteBlockSize(bool writesciheader) const
{
    const unsigned long entryBytes = Head.nColors * (!Head.type ? PalEntrySIZE : PalEntryOldSIZE);
    const unsigned long dataSize   = PAL_HEADER_GAME_SIZE
                                   + (2 * Head.palCount)   // palette index table
                                   + PAL_COMPPAL_GAME_SIZE
                                   + entryBytes;
    return (writesciheader ? 2 : 6) + dataSize;
}
