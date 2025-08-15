/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a P56 file
 *
 */
 
#include "stdafx.h"

//#include "Immagina.h"
#include "p56files.h"

#include "english.h" // Dhel

// Add this to the top of your p56files.cpp file
#define _CRT_SECURE_NO_WARNINGS

P56file32::P56file32() : 
    palSCI(nullptr),
    vector(nullptr), 
    _unkShort1(0),
    _unkShort2(0),
    format(-1),
    imageAllSize(0)
{
    // Initialize Head union to zero
    memset(&Head, 0, sizeof(Head));
    
    // Initialize cells array to null pointers
    for (int i = 0; i < MAX_CELLS; i++) {
        cells[i] = nullptr;
    }
}

P56file32::~P56file32()
{
    // Clean up all allocated resources
    cleanup();
}

void P56file32::cleanup()
{
    // Delete all cells
    for (int i = 0; i < MAX_CELLS; i++) {
        if (cells[i]) {
            delete cells[i];
            cells[i] = nullptr;
        }
    }
    
    // Delete palette
    if (palSCI) {
        delete palSCI;
        palSCI = nullptr;
    }
    
    // Delete vector data
    if (vector) {
        delete[] vector;
        vector = nullptr;
    }
}

void P56file32::initializeMembers()
{
    palSCI = nullptr;
    vector = nullptr;
    _unkShort1 = 0;
    _unkShort2 = 0;
    format = -1;
    imageAllSize = 0;
    
    // Initialize Head union
    memset(&Head, 0, sizeof(Head));
    
    // Initialize cells array
    for (int i = 0; i < MAX_CELLS; i++) {
        cells[i] = nullptr;
    }
}

int P56file32::LoadFile(HWND hwnd, LPSTR pszFileName)
{
    FILE* cfilebuf = fopen(pszFileName, "rb");
    if (!cfilebuf) {
        return ID_CANTOPENFILE;
    }
    
    // Initialize palette pointer
    palSCI = nullptr;
    
    // Check for patch header
    unsigned char offset = 0;
    unsigned long patchID = 0;
    
    if (fread(&patchID, 4, 1, cfilebuf) != 1) {
        fclose(cfilebuf);
        return ID_CANTOPENFILE;
    }
    
    if (patchID == P56PATCH80 || patchID == P56PATCH) {
        offset = 4;
    } else if (patchID == P56PATCHOLD) {
        offset = 26;
    }
    
    // Read header size to determine format
    fseek(cfilebuf, offset, SEEK_SET);
    unsigned short hsize = 0;
    if (fread(&hsize, 2, 1, cfilebuf) != 1) {
        fclose(cfilebuf);
        return ID_CANTOPENFILE;
    }
    
    int result;
    switch (hsize) {
        case 14: // PIC_32 format
            format = _PIC_32;
            result = LoadPic32(cfilebuf, offset);
            break;
        case 38: // PIC_11 format
            format = _PIC_11;
            result = LoadPic11(cfilebuf, offset);
            break;
        default:
            fclose(cfilebuf);
            return ID_WRONGHEADER;
    }
    
    fclose(cfilebuf);
    return (result == ID_NOERROR) ? ID_NOERROR : result;
}

int P56file32::LoadPic32(FILE* cfilebuf, unsigned char offset)
{
    if (!cfilebuf) {
        return ID_CANTOPENFILE;
    }
    
    // Validate cell record size
    fseek(cfilebuf, offset + 4, SEEK_SET);
    int tcellrecsize = 0;
    if (fread(&tcellrecsize, 2, 1, cfilebuf) != 1 || tcellrecsize != 0x2a) {
        return ID_WRONGCELLRECSIZE;
    }
    
    // Load main header
    fseek(cfilebuf, offset, SEEK_SET);
    if (fread(&Head, PICHEADER32SIZE, 1, cfilebuf) != 1) {
        return ID_CANTOPENFILE;
    }
    
    const PicHeader32* bPic = reinterpret_cast<const PicHeader32*>(&Head);
    
    // Load palette
    fseek(cfilebuf, offset - 6 + bPic->paletteOffset, SEEK_SET);
    
    int ttag = 0;
    if (fread(&ttag, 2, 1, cfilebuf) != 1 || ttag != PALETTE_POS) {
        return ID_WRONGPALETTELOC;
    }
    
    unsigned long tpalsize = 0;
    if (fread(&tpalsize, 4, 1, cfilebuf) != 1) {
        return ID_CANTOPENFILE;
    }
    
    palSCI = new Palette;
    palSCI->loadPalette(cfilebuf, tpalsize);
    
    // Load cells if present
    if (bPic->celCount > 0) {
        fseek(cfilebuf, offset + bPic->picHeaderSize, SEEK_SET);
        
        // Load all cell headers first
        for (int i = 0; i < bPic->celCount; i++) {
            cells[i] = new Cell;
            if (fread(&(cells[i]->Head), CELHEADERPICSIZE, 1, cfilebuf) != 1) {
                // Cleanup on error
                for (int cleanup = 0; cleanup <= i; cleanup++) {
                    delete cells[cleanup];
                }
                delete palSCI;
                return ID_CANTOPENFILE;
            }
        }
        
        // Load all cell image data
        for (int i = 0; i < bPic->celCount; i++) {
            cells[i]->setPalette(&palSCI);
            cells[i]->loadImage(cfilebuf, offset);
        }
    }
    
    return ID_NOERROR;
}

int P56file32::LoadPic11(FILE* cfilebuf, unsigned char offset)
{
    if (!cfilebuf) {
        return ID_CANTOPENFILE;
    }
    
    // Load main header
    fseek(cfilebuf, offset, SEEK_SET);
    if (fread(&Head, PICHEADER11SIZE, 1, cfilebuf) != 1) {
        return ID_CANTOPENFILE;
    }
    
    const PicHeader11* bPic = reinterpret_cast<const PicHeader11*>(&Head);
    
    // Show warnings for unsupported features
    if (bPic->priCelOffset) {
        MessageBox(hWnd, "priCelOffset is defined. This file might not be fully supported by FotoSCIhop!", 
                   WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);
    }
    
    if (bPic->controlCelOffset) {
        MessageBox(hWnd, "controlCelOffset is defined. This file might not be fully supported by FotoSCIhop!", 
                   WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);
    }
    
    if (bPic->polygonOffset) {
        MessageBox(hWnd, "polygonOffset is defined. This file might not be fully supported by FotoSCIhop!", 
                   WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);
    }
    
    // Load vector data
    vector = new unsigned char[bPic->vectorSize];
    fseek(cfilebuf, offset + bPic->vectorOffset, SEEK_SET);
    if (fread(vector, bPic->vectorSize, 1, cfilebuf) != 1) {
        delete[] vector;
        return ID_CANTOPENFILE;
    }
    
    // Load palette
    fseek(cfilebuf, offset - 6 + bPic->paletteOffset, SEEK_SET);
    
    int ttag = 0;
    if (fread(&ttag, 2, 1, cfilebuf) != 1 || ttag != PALETTE_POS) {
        delete[] vector;
        return ID_WRONGPALETTELOC;
    }
    
    unsigned long tpalsize = 0;
    if (fread(&tpalsize, 4, 1, cfilebuf) != 1) {
        delete[] vector;
        return ID_CANTOPENFILE;
    }
    
    palSCI = new Palette;
    palSCI->loadPalette(cfilebuf, tpalsize);
    
    // Read unknown shorts
    fseek(cfilebuf, offset + PICHEADER11SIZE, SEEK_SET);
    short tshort = 0;
    
    if (fread(&tshort, 2, 1, cfilebuf) != 1) {
        delete[] vector;
        delete palSCI;
        return ID_CANTOPENFILE;
    }
    _unkShort1 = tshort;
    
    if (fread(&tshort, 2, 1, cfilebuf) != 1) {
        delete[] vector;
        delete palSCI;
        return ID_CANTOPENFILE;
    }
    _unkShort2 = tshort;
    
    // Load cells if present
    if (bPic->celCount > 0) {
        fseek(cfilebuf, offset + bPic->visualHeaderOffset, SEEK_SET);
        
        // Load all cell headers first
        for (int i = 0; i < bPic->celCount; i++) {
            cells[i] = new Cell;
            if (fread(&(cells[i]->Head.pic), CELHEADER11SIZE, 1, cfilebuf) != 1) {
                // Cleanup on error
                for (int cleanup = 0; cleanup <= i; cleanup++) {
                    delete cells[cleanup];
                }
                delete[] vector;
                delete palSCI;
                return ID_CANTOPENFILE;
            }
        }
        
        // Load all cell image data
        for (int i = 0; i < bPic->celCount; i++) {
            cells[i]->setPalette(&palSCI);
            cells[i]->loadImage(cfilebuf, offset);
        }
    }
    
    return ID_NOERROR;
}

int P56file32::loadCellOffset()
{
    if (!palSCI) {
        return 0; // Error: no palette loaded
    }
    
    // Calculate palette size
    const unsigned long paletteSize = COMPPALSIZE + 
        (palSCI->Head.nColors * (palSCI->Head.type ? 3 : 4));
    
    unsigned long imagepos = 0;
    unsigned long tagsTotalSize = 0;
    int cellCount = 0;
    
    // Calculate total image sizes and determine format-specific offsets
    switch (format) {
        case _PIC_32:
        {
            PicHeader32* bPic32 = reinterpret_cast<PicHeader32*>(&Head);
            cellCount = bPic32->celCount;
            
            // Calculate total sizes for all cells
            for (int i = 0; i < cellCount; i++) {
                if (cells[i] && cells[i]->cellImage) {
                    const CellImage* cellImg = cells[i]->cellImage;
                    imageAllSize += cellImg->imageSize + cellImg->packSize;
                    tagsTotalSize += cellImg->imageSize;
                }
            }
            
            // Set format-specific offsets
            bPic32->paletteOffset = bPic32->picHeaderSize + CELHEADERPICSIZE * cellCount + 6;
            imagepos = bPic32->paletteOffset + paletteSize + 6;
            break;
        }
        
        case _PIC_11:
        {
            PicHeader11* bPic11 = reinterpret_cast<PicHeader11*>(&Head);
            cellCount = bPic11->celCount;
            
            // Calculate total sizes for all cells
            for (int i = 0; i < cellCount; i++) {
                if (cells[i] && cells[i]->cellImage) {
                    const CellImage* cellImg = cells[i]->cellImage;
                    imageAllSize += cellImg->imageSize + cellImg->packSize;
                    tagsTotalSize += cellImg->imageSize;
                }
            }
            
            // Set PIC_11 specific header values
            bPic11->picHeaderSize = 38;
            bPic11->visualHeaderOffset = 68;
            if (cellCount > 0) {
                bPic11->visualHeaderOffset += 4;
            }
            
            bPic11->paletteOffset = bPic11->visualHeaderOffset + CELHEADER11SIZE * cellCount + 6;
            if (imageAllSize > 0) {
                bPic11->paletteOffset += imageAllSize + 6;
            }
            
            bPic11->vectorOffset = bPic11->paletteOffset + paletteSize + 6;
            imagepos = bPic11->visualHeaderOffset + cellCount * PIC11CELLRECSIZE + 6;
            break;
        }
        
        default:
            return 0; // Error: unknown format
    }
    
    // Calculate cell-specific offsets
    unsigned long packpos = 0;
    unsigned long linespos = 0;
    
    // Find first compressed cell to determine if we need pack data
    bool hasCompressedCells = false;
    for (int i = 0; i < cellCount; i++) {
        if (cells[i]) {
            const CelBase* bCell = reinterpret_cast<const CelBase*>(&cells[i]->Head);
            if (bCell->compressType) {
                hasCompressedCells = true;
                break;
            }
        }
    }
    
    packpos = hasCompressedCells ? (imagepos + tagsTotalSize) : 0;
    linespos = imagepos + imageAllSize + 6;
    
    // Set offsets for each cell
    for (int i = 0; i < cellCount; i++) {
        if (!cells[i] || !cells[i]->cellImage) {
            continue; // Skip invalid cells
        }
        
        CelBase* bCell = reinterpret_cast<CelBase*>(&cells[i]->Head);
        const CellImage* bImage = cells[i]->cellImage;
        
        // Calculate data sizes
        bCell->dataByteCount = bImage->imageSize + (bCell->compressType ? bImage->packSize : 0);
        bCell->controlByteCount = bCell->compressType ? bImage->imageSize : 0;
        
        // Set file offsets
        bCell->controlOffset = imagepos;
        bCell->colorOffset = packpos;
        
        // Update positions for next cell
        imagepos += (bCell->compressType ? bCell->controlByteCount : bCell->dataByteCount);
        if (bCell->compressType) {
            packpos += (bCell->dataByteCount - bCell->controlByteCount);
        }
        
        // Set row table offset for compressed PIC_32 cells
        if (format != _PIC_11 && bCell->compressType) {
            bCell->rowTableOffset = linespos;
            linespos += bCell->yDim * 8; // 4 * 2 = 8 bytes per line
        }
        
        // Set palette offset for PIC_11 format
        if (format == _PIC_11) {
            const PicHeader11* bPic11 = reinterpret_cast<const PicHeader11*>(&Head);
            bCell->paletteOffset = bPic11->paletteOffset;
        } else {
            bCell->paletteOffset = 0;
        }
    }
    
    return 1; // Success
}

int P56file32::writeFileHeader(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    unsigned long patchID = 0;
    
    if (format != _PIC_11) {
        // PIC_32 format
        patchID = P56PATCH80;
        if (fwrite(&patchID, 4, 1, cfilebuf) != 1) {
            return 0;
        }
    } else {
        // PIC_11 format - write extended header
        patchID = P56PATCHOLD;
        if (fwrite(&patchID, 4, 1, cfilebuf) != 1) {
            return 0;
        }
        
        // Write standard header values efficiently
        const unsigned short headerValues[] = {320, 200, 5, 6, 256, 0, 0, 0, 0, 0, 0};
        const size_t numValues = sizeof(headerValues) / sizeof(headerValues[0]);
        
        if (fwrite(headerValues, sizeof(unsigned short), numValues, cfilebuf) != numValues) {
            return 0;
        }
    }
    
    return 1;
}

int P56file32::writePicHeader(FILE* cfilebuf, int cellCount)
{
    if (!cfilebuf) {
        return 0;
    }
    
    // Write main header
    const size_t headerSize = (format == _PIC_11) ? PICHEADER11SIZE : PICHEADER32SIZE;
    if (fwrite(&Head, headerSize, 1, cfilebuf) != 1) {
        return 0;
    }
    
    // Write additional data for PIC_11 format if cells exist
    if (format == _PIC_11 && cellCount > 0) {
        if (fwrite(&_unkShort1, 2, 1, cfilebuf) != 1) {
            return 0;
        }
        if (fwrite(&_unkShort2, 2, 1, cfilebuf) != 1) {
            return 0;
        }
    }
    
    return 1;
}

int P56file32::writeCellHeaders(FILE* cfilebuf, int cellCount)
{
    if (!cfilebuf) {
        return 0;
    }
    
    if (cellCount <= 0) {
        return 1; // Success - nothing to write
    }
    
    const size_t cellHeaderSize = (format == _PIC_11) ? CELHEADER11SIZE : CELHEADERPICSIZE;
    
    for (int i = 0; i < cellCount; i++) {
        if (!cells[i]) {
            return 0; // Invalid cell
        }
        
        if (fwrite(&cells[i]->Head, cellHeaderSize, 1, cfilebuf) != 1) {
            return 0;
        }
    }
    
    return 1;
}

int P56file32::writeImages(FILE* cfilebuf, int cellCount)
{
    if (!cfilebuf) {
        return 0;
    }
    
    switch (format) {
        case _PIC_32:
            return writePic32Images(cfilebuf, cellCount);
        case _PIC_11:
            return writePic11Images(cfilebuf, cellCount);
        default:
            return 0; // Unknown format
    }
}

int P56file32::writePic32Images(FILE* cfilebuf, int cellCount)
{
    const PicHeader32* bPic32 = reinterpret_cast<const PicHeader32*>(&Head);
    
    // Write palette first for PIC_32
    if (palSCI) {
        palSCI->WritePalette(cfilebuf, false);
    }
    
    // Write images if cells exist
    if (bPic32->celCount > 0 && cellCount > 0) {
        // Write image section header
        const unsigned short ttag = PIC32_IMAGE_POS;
        if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
            return 0;
        }
        if (fwrite(&imageAllSize, 4, 1, cfilebuf) != 1) {
            return 0;
        }
        
        // Write all cell images
        for (int i = 0; i < cellCount; i++) {
            if (!cells[i]) {
                return 0; // Invalid cell
            }
            cells[i]->WriteImage(cfilebuf);
        }
        
        // Write all pack data
        for (int i = 0; i < cellCount; i++) {
            if (!cells[i]) {
                return 0; // Invalid cell
            }
            cells[i]->WritePack(cfilebuf);
        }
    }
    
    // Write scan lines if split flag is set
    if (bPic32->splitFlag) {
        const unsigned short ttag = PIC32_LINES_POS;
        if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
            return 0;
        }
        
        const unsigned long tzero = 0;
        if (fwrite(&tzero, 4, 1, cfilebuf) != 1) {
            return 0;
        }
        
        for (int i = 0; i < cellCount; i++) {
            if (!cells[i]) {
                return 0; // Invalid cell
            }
            cells[i]->WriteScanLines(cfilebuf);
        }
    }
    
    return 1;
}

int P56file32::writePic11Images(FILE* cfilebuf, int cellCount)
{
    const PicHeader11* bPic11 = reinterpret_cast<const PicHeader11*>(&Head);
    
    // Write images first for PIC_11
    if (cellCount > 0) {
        // Write image section header
        const unsigned short ttag = PIC11_IMAGE_POS;
        if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
            return 0;
        }
        if (fwrite(&imageAllSize, 4, 1, cfilebuf) != 1) {
            return 0;
        }
        
        // Write all cell images
        for (int i = 0; i < cellCount; i++) {
            if (!cells[i]) {
                return 0; // Invalid cell
            }
            cells[i]->WriteImage(cfilebuf);
        }
        
        // Write all pack data
        for (int i = 0; i < cellCount; i++) {
            if (!cells[i]) {
                return 0; // Invalid cell
            }
            cells[i]->WritePack(cfilebuf);
        }
    }
    
    // Write palette after images for PIC_11
    if (palSCI) {
        palSCI->WritePalette(cfilebuf, false);
    }
    
    // Write vector data
    const unsigned short ttag = PIC11_VECTOR_POS;
    if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
        return 0;
    }
    if (fwrite(&bPic11->vectorSize, 4, 1, cfilebuf) != 1) {
        return 0;
    }
    
    if (vector && bPic11->vectorSize > 0) {
        if (fwrite(vector, bPic11->vectorSize, 1, cfilebuf) != 1) {
            return 0;
        }
    }
    
    return 1;
}

bool P56file32::SavePic(HWND hwnd, LPSTR szFileName)
{
    if (!szFileName) {
        return false;
    }
    
    FILE* cfilebuf = fopen(szFileName, "wb");
    if (!cfilebuf) {
        return false;
    }
    
    // Reset image size counter
    imageAllSize = 0;
    
    // Determine cell count based on format
    int cellCount = 0;
    switch (format) {
        case _PIC_32:
        {
            const PicHeader32* bPic32 = reinterpret_cast<const PicHeader32*>(&Head);
            cellCount = bPic32->celCount;
            break;
        }
        case _PIC_11:
        {
            const PicHeader11* bPic11 = reinterpret_cast<const PicHeader11*>(&Head);
            cellCount = bPic11->celCount;
            break;
        }
        default:
            fclose(cfilebuf);
            return false; // Unknown format
    }
    
    // Calculate offsets before writing
    if (!loadCellOffset()) {
        fclose(cfilebuf);
        return false;
    }
    
    // Write all file sections in order
    bool success = true;
    success &= (writeFileHeader(cfilebuf) != 0);
    success &= (writePicHeader(cfilebuf, cellCount) != 0);
    success &= (writeCellHeaders(cfilebuf, cellCount) != 0);
    success &= (writeImages(cfilebuf, cellCount) != 0);
    
    fclose(cfilebuf);
    
    // If writing failed, optionally delete the partial file
    if (!success) {
        remove(szFileName);
    }
    
    return success;
}

// ============================================================================
// P56 CELL UTILITY FUNCTIONS 
// ============================================================================

// === HELPER FUNCTIONS FOR DUAL FORMAT SUPPORT ===

int P56file32::getCellWidth(int index) const
{
    if (!isValidCellIndex(index)) return 0;
    
    // Both formats use the same .pic structure, just different data interpretation
    return cells[index]->Head.pic.xDim;
}

int P56file32::getCellHeight(int index) const
{
    if (!isValidCellIndex(index)) return 0;
    
    // Both formats use the same .pic structure, just different data interpretation
    return cells[index]->Head.pic.yDim;
}

int P56file32::getCellCompressType(int index) const
{
    if (!isValidCellIndex(index)) return 0;
    
    // Both formats use the same .pic structure, just different data interpretation
    return cells[index]->Head.pic.compressType;
}

// === BASIC CELL OPERATIONS ===

int P56file32::modifyCells(int base, int delta)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    // Handle no-op case
    if (delta == 0) {
        return 1;
    }
    
    const int currentCellCount = getCellsCount();
    const int newCellCount = currentCellCount + delta;
    
    // Validate new cell count
    if (newCellCount < 1 || newCellCount > MAX_CELLS) {
        return 0; // Don't allow zero cells or exceed maximum
    }
    
    if (delta > 0) {
        // ADDING CELLS
        
        // Validate base cell exists for copying
        if (!isValidCellIndex(base)) {
            return 0;
        }
        
        // Add cells at the end by creating deep copies of the base cell
        for (int i = 0; i < delta; i++) {
            const int newIndex = currentCellCount + i;
            
            // Create new Cell object (deep copy of base cell)
            cells[newIndex] = new Cell;
            cells[newIndex]->Head = cells[base]->Head;
            cells[newIndex]->isClone = true; // Maintain P56 behavior
            
            // Deep copy the cell image data
            if (cells[base]->cellImage) {
                cells[newIndex]->cellImage = new CellImage;
                CellImage* srcImg = cells[base]->cellImage;
                CellImage* dstImg = cells[newIndex]->cellImage;
                
                // Copy image data
                dstImg->imageSize = srcImg->imageSize;
                if (srcImg->image && srcImg->imageSize > 0) {
                    dstImg->image = new unsigned char[srcImg->imageSize];
                    memcpy(dstImg->image, srcImg->image, srcImg->imageSize);
                } else {
                    dstImg->image = nullptr;
                }
                
                // Copy pack data
                dstImg->packSize = srcImg->packSize;
                if (srcImg->pack && srcImg->packSize > 0) {
                    dstImg->pack = new unsigned char[srcImg->packSize];
                    memcpy(dstImg->pack, srcImg->pack, srcImg->packSize);
                } else {
                    dstImg->pack = nullptr;
                }
                
                // Copy lines data
                dstImg->lineSize = srcImg->lineSize;
                if (srcImg->lines && srcImg->lineSize > 0) {
                    dstImg->lines = new unsigned char[srcImg->lineSize];
                    memcpy(dstImg->lines, srcImg->lines, srcImg->lineSize);
                } else {
                    dstImg->lines = nullptr;
                }
            } else {
                cells[newIndex]->cellImage = nullptr;
            }
            
            // Set palette reference
            cells[newIndex]->setPalette(&palSCI);
        }
    } else {
        // REMOVING CELLS (delta is negative)
        
        const int removeCount = -delta;
        const int startRemoveIndex = newCellCount; // Start removing from this index
        
        // Clean up cells being removed
        for (int i = startRemoveIndex; i < currentCellCount; i++) {
            if (cells[i]) {
                if (cells[i]->cellImage) {
                    CellImage* img = cells[i]->cellImage;
                    delete[] img->image;
                    delete[] img->pack;
                    delete[] img->lines;
                    delete img;
                }
                delete cells[i];
                cells[i] = nullptr;
            }
        }
    }
    
    // Update cell count
    setCellsCount(newCellCount);
    
    return 1;
}

int P56file32::addCell(int baseIndex, int position)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    // Validate parameters
    if (!isValidCellIndex(baseIndex)) {
        return 0;
    }
    
    const int cellCount = getCellsCount();
    
    // If position is -1 or at end, use optimized delta function
    if (position == -1 || position == cellCount) {
        return modifyCells(baseIndex, 1);
    }
    
    // Validate position for insertion
    if (position < 0 || position > cellCount || cellCount >= MAX_CELLS) {
        return 0;
    }
    
    // For middle insertion, we need to do the complex shifting
    // Shift existing cells to make room
    for (int i = cellCount - 1; i >= position; i--) {
        cells[i + 1] = cells[i];
    }
    
    // Create new Cell object (deep copy of base cell)
    cells[position] = new Cell;
    cells[position]->Head = cells[baseIndex]->Head;
    cells[position]->isClone = true; // Maintain P56 behavior
    
    // Deep copy the cell image data
    if (cells[baseIndex]->cellImage) {
        cells[position]->cellImage = new CellImage;
        CellImage* srcImg = cells[baseIndex]->cellImage;
        CellImage* dstImg = cells[position]->cellImage;
        
        // Copy image data
        dstImg->imageSize = srcImg->imageSize;
        if (srcImg->image && srcImg->imageSize > 0) {
            dstImg->image = new unsigned char[srcImg->imageSize];
            memcpy(dstImg->image, srcImg->image, srcImg->imageSize);
        } else {
            dstImg->image = nullptr;
        }
        
        // Copy pack data
        dstImg->packSize = srcImg->packSize;
        if (srcImg->pack && srcImg->packSize > 0) {
            dstImg->pack = new unsigned char[srcImg->packSize];
            memcpy(dstImg->pack, srcImg->pack, srcImg->packSize);
        } else {
            dstImg->pack = nullptr;
        }
        
        // Copy lines data
        dstImg->lineSize = srcImg->lineSize;
        if (srcImg->lines && srcImg->lineSize > 0) {
            dstImg->lines = new unsigned char[srcImg->lineSize];
            memcpy(dstImg->lines, srcImg->lines, srcImg->lineSize);
        } else {
            dstImg->lines = nullptr;
        }
    } else {
        cells[position]->cellImage = nullptr;
    }
    
    // Set palette reference
    cells[position]->setPalette(&palSCI);
    
    // Update count using helper function
    setCellsCount(cellCount + 1);
    
    return 1;
}

int P56file32::addCells(int base, int amount)
{
    // Use optimized delta function for multiple additions
    return modifyCells(base, amount);
}

int P56file32::deleteCell(int position)
{
    // Use the more robust deleteCells function
    return deleteCells(position, 1);
}

int P56file32::deleteCells(int start, int count)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (count <= 0) {
        return 0;
    }
    
    const int cellCount = getCellsCount();
    
    // Validate range
    if (start < 0 || start >= cellCount || start + count > cellCount) {
        return 0;
    }
    
    // Don't allow deleting all cells
    if (count >= cellCount) {
        return 0;
    }
    
    // If deleting from the end, use optimized delta function
    if (start + count == cellCount) {
        return modifyCells(0, -count);
    }
    
    // For middle deletion, we need complex shifting
    // Clean up memory for cells being deleted
    for (int i = start; i < start + count; i++) {
        if (cells[i]) {
            if (cells[i]->cellImage) {
                CellImage* img = cells[i]->cellImage;
                delete[] img->image;
                delete[] img->pack;
                delete[] img->lines;
                delete img;
            }
            delete cells[i];
            cells[i] = nullptr;
        }
    }
    
    // Shift remaining cells down
    const int remainingCells = cellCount - start - count;
    for (int i = 0; i < remainingCells; i++) {
        cells[start + i] = cells[start + count + i];
    }
    
    // Clear pointers at the end
    for (int i = cellCount - count; i < cellCount; i++) {
        cells[i] = nullptr;
    }
    
    // Update count using helper function
    setCellsCount(cellCount - count);
    
    return 1;
}

// === STREAMLINED CONVENIENCE FUNCTIONS ===

int P56file32::appendCell(int baseIndex)
{
    // Direct call to optimized delta function
    return modifyCells(baseIndex, 1);
}

int P56file32::appendCells(int baseIndex, int count)
{
    // Direct call to optimized delta function
    return modifyCells(baseIndex, count);
}

int P56file32::removeLastCell()
{
    // Direct call to optimized delta function
    return modifyCells(0, -1);
}

int P56file32::removeLastCells(int count)
{
    // Direct call to optimized delta function
    return modifyCells(0, -count);
}

// === ESSENTIAL COPY/MOVE OPERATIONS (KEPT AS-IS BUT OPTIMIZED) ===

int P56file32::copyCells(int srcStart, int count, int dstPos)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (count <= 0) {
        return 0;
    }
    
    const int cellCount = getCellsCount();
    
    if (srcStart < 0 || srcStart >= cellCount || srcStart + count > cellCount ||
        dstPos < 0 || dstPos > cellCount || cellCount + count > MAX_CELLS) {
        return 0;
    }
    
    // Special optimization: if copying single cell to end, use delta
    if (count == 1 && dstPos == cellCount) {
        return modifyCells(srcStart, 1);
    }
    
    // Make room for new cells by shifting existing ones
    for (int i = cellCount - 1; i >= dstPos; i--) {
        cells[i + count] = cells[i];
    }
    
    // Copy cells with error handling
    for (int i = 0; i < count; i++) {
        const int srcIndex = srcStart + i;
        const int dstIndex = dstPos + i;
        
        if (!cells[srcIndex]) {
            cells[dstIndex] = nullptr;
            continue;
        }
        
        // Create new Cell object (deep copy)
        cells[dstIndex] = new Cell;
        cells[dstIndex]->Head = cells[srcIndex]->Head;
        cells[dstIndex]->isClone = true;
        
        // Deep copy the cell image data
        if (cells[srcIndex]->cellImage) {
            cells[dstIndex]->cellImage = new CellImage;
            CellImage* srcImg = cells[srcIndex]->cellImage;
            CellImage* dstImg = cells[dstIndex]->cellImage;
            
            dstImg->imageSize = srcImg->imageSize;
            dstImg->packSize = srcImg->packSize;
            dstImg->lineSize = srcImg->lineSize;
            dstImg->image = nullptr;
            dstImg->pack = nullptr;
            dstImg->lines = nullptr;
            
            if (srcImg->image && srcImg->imageSize > 0) {
                dstImg->image = new unsigned char[srcImg->imageSize];
                memcpy(dstImg->image, srcImg->image, srcImg->imageSize);
            }
            
            if (srcImg->pack && srcImg->packSize > 0) {
                dstImg->pack = new unsigned char[srcImg->packSize];
                memcpy(dstImg->pack, srcImg->pack, srcImg->packSize);
            }
            
            if (srcImg->lines && srcImg->lineSize > 0) {
                dstImg->lines = new unsigned char[srcImg->lineSize];
                memcpy(dstImg->lines, srcImg->lines, srcImg->lineSize);
            }
        } else {
            cells[dstIndex]->cellImage = nullptr;
        }
        
        cells[dstIndex]->setPalette(&palSCI);
    }
    
    // Update count using helper function
    setCellsCount(cellCount + count);
    
    return 1;
}

int P56file32::moveCells(int srcStart, int count, int dstPos)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    // Special case: moving within the same area
    if (srcStart == dstPos) {
        return 1; // No-op
    }
        
    // Use shiftCells for efficiency when moving within same array
    return shiftCells(srcStart, count, dstPos);
}

int P56file32::duplicateCells(int start, int count)
{
    return copyCells(start, count, start + count);
}

// === ESSENTIAL REORDER OPERATIONS (KEPT AS-IS) ===

int P56file32::shiftCells(int start, int count, int newPos)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (count <= 0) {
        return 0;
    }
    
    const int cellCount = getCellsCount();
    if (start < 0 || start >= cellCount || start + count > cellCount ||
        newPos < 0 || newPos > cellCount - count || start == newPos) {
        return start == newPos ? 1 : 0; // No-op if same position
    }
    
    Cell** tempCells = new Cell*[count];
    for (int i = 0; i < count; i++) {
        tempCells[i] = cells[start + i];
    }
    
    if (newPos < start) {
        for (int i = start - 1; i >= newPos; i--) {
            cells[i + count] = cells[i];
        }
    } else {
        for (int i = start + count; i < newPos + count; i++) {
            cells[i - count] = cells[i];
        }
        newPos -= count;
    }
    
    for (int i = 0; i < count; i++) {
        cells[newPos + i] = tempCells[i];
    }
    
    delete[] tempCells;
    return 1;
}

int P56file32::swapCells(int index1, int index2)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (!isValidCellIndex(index1) || !isValidCellIndex(index2)) {
        return 0;
    }
    
    Cell* temp = cells[index1];
    cells[index1] = cells[index2];
    cells[index2] = temp;
    
    return 1;
}

int P56file32::reverseCells(int start, int count)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (count <= 1) {
        return 1;
    }
    
    const int cellCount = getCellsCount();
    if (start < 0 || start >= cellCount || start + count > cellCount) {
        return 0;
    }
    
    for (int i = 0; i < count / 2; i++) {
        int leftIndex = start + i;
        int rightIndex = start + count - 1 - i;
        
        Cell* temp = cells[leftIndex];
        cells[leftIndex] = cells[rightIndex];
        cells[rightIndex] = temp;
    }
    
    return 1;
}

// === ADVANCED OPERATIONS (STREAMLINED) ===

int P56file32::insertEmptyCells(int position, int count)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (count <= 0) {
        return 0;
    }
    
    const int cellCount = getCellsCount();
    if (position < 0 || position > cellCount || cellCount + count > MAX_CELLS) {
        return 0;
    }
    
    // Shift existing cells to make room
    for (int i = cellCount - 1; i >= position; i--) {
        cells[i + count] = cells[i];
    }
    
    // Create empty cells
    for (int i = 0; i < count; i++) {
        cells[position + i] = new Cell;
        memset(&cells[position + i]->Head, 0, sizeof(CelHeader));
        cells[position + i]->cellImage = nullptr;
        cells[position + i]->isClone = false;
        cells[position + i]->setPalette(&palSCI);
    }
    
    setCellsCount(cellCount + count);
    
    return 1;
}

// === BATCH OPERATIONS (ESSENTIAL ONLY) ===

int P56file32::batchDeleteCells(const int* indices, int indexCount)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    if (!indices || indexCount <= 0) {
        return 0;
    }
    
    // Sort indices in descending order to avoid index shifting issues
    int* sortedIndices = new int[indexCount];
    for (int i = 0; i < indexCount; i++) {
        sortedIndices[i] = indices[i];
    }
    
    // Simple bubble sort (descending)
    for (int i = 0; i < indexCount - 1; i++) {
        for (int j = 0; j < indexCount - i - 1; j++) {
            if (sortedIndices[j] < sortedIndices[j + 1]) {
                int temp = sortedIndices[j];
                sortedIndices[j] = sortedIndices[j + 1];
                sortedIndices[j + 1] = temp;
            }
        }
    }
    
    int deletedCount = 0;
    for (int i = 0; i < indexCount; i++) {
        if (deleteCell(sortedIndices[i])) {
            deletedCount++;
        }
    }
    
    delete[] sortedIndices;
    return deletedCount > 0 ? 1 : 0;
}

// === SEARCH OPERATIONS (ESSENTIAL ONLY) ===

int P56file32::findEmptyCells(int* results, int maxResults)
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    int foundCount = 0;
    const int cellCount = getCellsCount();
    
    for (int c = 0; c < cellCount && foundCount < maxResults; c++) {
        bool isEmpty = !cells[c] || 
                      !cells[c]->cellImage ||
                      cells[c]->cellImage->imageSize == 0;
        
        if (isEmpty) {
            results[foundCount] = c;
            foundCount++;
        }
    }
    
    return foundCount;
}

// === MAINTENANCE OPERATIONS (STREAMLINED) ===

int P56file32::optimizeCells()
{
    // Support both formats
    if (format != _PIC_32 && format != _PIC_11) {
        return 0;
    }
    
    int removed = 0;
    int writePos = 0;
    const int originalCount = getCellsCount();
    
    // Compact array by moving valid cells to the front
    for (int readPos = 0; readPos < originalCount; readPos++) {
        bool isEmpty = !cells[readPos] || 
                      !cells[readPos]->cellImage ||
                      cells[readPos]->cellImage->imageSize == 0;
        
        if (isEmpty) {
            // Clean up empty cell
            if (cells[readPos]) {
                if (cells[readPos]->cellImage) {
                    CellImage* img = cells[readPos]->cellImage;
                    delete[] img->image;
                    delete[] img->pack;
                    delete[] img->lines;
                    delete img;
                }
                delete cells[readPos];
            }
            removed++;
        } else {
            // Move valid cell to write position
            if (writePos != readPos) {
                cells[writePos] = cells[readPos];
            }
            writePos++;
        }
    }
    
    // Clear remaining pointers
    for (int i = writePos; i < originalCount; i++) {
        cells[i] = nullptr;
    }
    
    // Update count using helper function
    setCellsCount(writePos);
    
    return removed;
}

// === P56 UTILITY FUNCTIONS ===

PicCellRangeInfo P56file32::getCellRangeInfo(int start, int count)
{
    PicCellRangeInfo info;
    
    if (format != _PIC_32 && format != _PIC_11) {
        return info;
    }
    
    const int cellCount = getCellsCount();
    if (start < 0 || start >= cellCount) {
        return info;
    }
    
    const int endPos = (start + count > cellCount) ? cellCount : start + count;
    
    for (int i = start; i < endPos; i++) {
        if (!cells[i]) continue;
        
        info.totalCells++;
        
        if (cells[i]->cellImage) {
            CellImage* img = cells[i]->cellImage;
            info.totalImageSize += img->imageSize;
            info.totalPackSize += img->packSize;
        }
        
        if (getCellCompressType(i)) {
            info.compressedCount++;
        } else {
            info.uncompressedCount++;
        }
        
        // P56 doesn't have links, so hasLinks stays false
    }
    
    return info;
}

PicStats P56file32::getPicStats()
{
    PicStats stats;
    
    if (format != _PIC_32 && format != _PIC_11) {
        return stats;
    }
    
    stats.totalCells = getCellsCount();
    
    for (int c = 0; c < stats.totalCells; c++) {
        if (!cells[c]) {
            stats.emptyCells++;
            continue;
        }
        
        if (cells[c]->cellImage) {
            CellImage* img = cells[c]->cellImage;
            stats.totalImageSize += img->imageSize;
            stats.totalPackSize += img->packSize;
            
            if (img->imageSize > stats.largestCellSize) {
                stats.largestCellSize = img->imageSize;
            }
            if (img->imageSize < stats.smallestCellSize && img->imageSize > 0) {
                stats.smallestCellSize = img->imageSize;
            }
        } else {
            stats.emptyCells++;
        }
        
        if (getCellCompressType(c)) {
            stats.compressedCells++;
        } else {
            stats.uncompressedCells++;
        }
    }
    
    if (stats.smallestCellSize == 2147483647) { // INT_MAX
        stats.smallestCellSize = 0;
    }
    
    return stats;
}
