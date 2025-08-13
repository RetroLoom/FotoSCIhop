/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a V56 file
 *
 */

#include "stdafx.h"

//#include "Immagina.h"
#include "v56files.h"

#define _CRT_SECURE_NO_WARNINGS

V56file::V56file() : 
    palSCI(nullptr),
    totalImageSize(0)
{
    initializeMembers();
}

V56file::~V56file()
{
    cleanup();
}

void V56file::initializeMembers()
{
    palSCI = nullptr;
    totalImageSize = 0;
    
    // Initialize Head structure
    memset(&Head, 0, sizeof(Head));
    
    // Initialize loops array
    for (int i = 0; i < MAX_LOOPS; i++) {
        loops[i] = nullptr;
    }
}

void V56file::cleanup()
{
    // Simple cleanup - just delete what we know about
    delete palSCI;
    palSCI = nullptr;
    
    // Note: Loop cleanup can be added later if needed
    // For now, just set pointers to null for safety
    for (int i = 0; i < MAX_LOOPS; i++) {
        loops[i] = nullptr;
    }
}

int V56file::LoadFile(HWND hwnd, LPSTR pszFileName)
{
    FILE* cfilebuf = fopen(pszFileName, "rb");
    if (!cfilebuf) {
        return ID_CANTOPENFILE;
    }
    
    // Determine file offset by checking for patch header
    unsigned char offset = 0;
    unsigned long patchID = 0;
    
    if (fread(&patchID, 3, 1, cfilebuf) != 1) {
        fclose(cfilebuf);
        return ID_CANTOPENFILE;
    }
    
    if (patchID == V56PATCH84 || patchID == V56PATCH) {
        if (fread(&offset, 1, 1, cfilebuf) != 1) {
            fclose(cfilebuf);
            return ID_CANTOPENFILE;
        }
        offset += 26;
    } else if (((patchID & 0xFFFF) == 16) || ((patchID & 0xFFFF) == 18)) {
        offset = 0; // Typical offset used
    } else {
        fclose(cfilebuf);
        return ID_WRONGHEADER;
    }
    
    // Validate loop record size
    fseek(cfilebuf, offset + 12, SEEK_SET);
    unsigned char tlooprecsize = 0;
    if (fread(&tlooprecsize, 1, 1, cfilebuf) != 1 || tlooprecsize != 0x10) {
        fclose(cfilebuf);
        return ID_WRONGLOOPRECSIZE;
    }
    
    // Validate cell record size
    fseek(cfilebuf, offset + 13, SEEK_SET);
    unsigned char tcellrecsize = 0;
    if (fread(&tcellrecsize, 1, 1, cfilebuf) != 1) {
        fclose(cfilebuf);
        return ID_WRONGCELLRECSIZE;
    }
    
    if (tcellrecsize != 0x24 && tcellrecsize != 0x34) {
        fclose(cfilebuf);
        return ID_WRONGCELLRECSIZE;
    }
    
    // Load main header
    fseek(cfilebuf, offset, SEEK_SET);
    if (fread(&Head, VIEW32_HEADER_LINK_SIZE, 1, cfilebuf) != 1) {
        fclose(cfilebuf);
        return ID_CANTOPENFILE;
    }
    
    // Load palette if present
    if (Head.view32.paletteOffset) {
        fseek(cfilebuf, offset + Head.view32.paletteOffset - 6, SEEK_SET);
        
        int ttag = 0;
        if (fread(&ttag, 2, 1, cfilebuf) != 1 || ttag != PALETTE_POS) {
            fclose(cfilebuf);
            return ID_WRONGPALETTELOC;
        }
        
        unsigned long tpalsize = 0;
        if (fread(&tpalsize, 4, 1, cfilebuf) != 1) {
            fclose(cfilebuf);
            return ID_CANTOPENFILE;
        }
        
        palSCI = new Palette;
        palSCI->loadPalette(cfilebuf, tpalsize);
    } else {
        palSCI = new Palette;
        palSCI->noPalette();
    }
    
    // Load loops and cells
    fseek(cfilebuf, offset + Head.view32.viewHeaderSize + 2, SEEK_SET); // 2 additional bytes for counter
    
    // Pre-allocate and batch read all loop headers
    LoopHeader loopHeaders[1024];
    if (fread(loopHeaders, LOOPHEADERSIZE, Head.view32.loopCount, cfilebuf) != Head.view32.loopCount) {
        delete palSCI;
        fclose(cfilebuf);
        return ID_CANTOPENFILE;
    }
    
    // Process each loop
    for (int z = 0; z < Head.view32.loopCount; z++) {
        loops[z] = new Loop;
        loops[z]->Head = loopHeaders[z];
        
        // Load all cell headers for this loop at once
        fseek(cfilebuf, offset + loops[z]->Head.celOffset, SEEK_SET);
        
        for (unsigned short i = 0; i < loops[z]->Head.numCels; i++) {
            loops[z]->cells[i] = new Cell;
            
            if (fread(&(loops[z]->cells[i]->Head.view), CELHEADERVIEWSIZE, 1, cfilebuf) != 1) {
                // Cleanup on error
                for (int cleanup_z = 0; cleanup_z <= z; cleanup_z++) {
                    for (unsigned short cleanup_i = 0; cleanup_i < ((cleanup_z == z) ? i : loops[cleanup_z]->Head.numCels); cleanup_i++) {
                        delete loops[cleanup_z]->cells[cleanup_i];
                    }
                    delete loops[cleanup_z];
                }
                delete palSCI;
                fclose(cfilebuf);
                return ID_CANTOPENFILE;
            }
            
            // Skip remaining header data
            fseek(cfilebuf, Head.view32.celHeaderSize - CELHEADERVIEWSIZE, SEEK_CUR);
        }
        
        // Load image data and links for all cells in this loop
        for (int i = 0; i < loops[z]->Head.numCels; i++) {
            loops[z]->cells[i]->setPalette(&palSCI);
            loops[z]->cells[i]->loadImage(cfilebuf, offset);
            
            const CelHeaderView* bCell = reinterpret_cast<const CelHeaderView*>(&loops[z]->cells[i]->Head);
            
            if (bCell->linkTableCount) {
                fseek(cfilebuf, offset + bCell->linkTableOffset, SEEK_SET);
                loops[z]->cells[i]->ReadLinks(cfilebuf);
            }
        }
    }
    
    Head.view32.celHeaderSize = CELHEADERVIEWSIZE;
    fclose(cfilebuf);
    
    return ID_NOERROR;
}

int V56file::loadCellOffset(void)
{
    if (!palSCI) {
        return 0; // Error: no palette loaded
    }
    
    // Initialize variables
    totalImageSize = 0;
    unsigned long tagsTotalSize = 0;
    unsigned long linesTotalSize = 0;
    
    // Calculate palette size if palette data exists
    unsigned long paletteSize = 0;
    if (palSCI->palData) {
        paletteSize = COMPPALSIZE + (palSCI->Head.nColors * (palSCI->Head.type ? 3 : 4));
    }
    
    // Calculate palette offset
    const unsigned long paletteOffset = 2 + Head.view32.viewHeaderSize + 
        Head.view32.loopHeaderSize * Head.view32.loopCount + 
        Head.view32.celHeaderSize * Head.view32.celCount + 6;
    
    Head.view32.paletteOffset = palSCI->palData ? paletteOffset : 0;
    
    // First pass: calculate total sizes for all cells
    for (int l = 0; l < Head.view32.loopCount; l++) {
        if (!loops[l]) continue; // Skip invalid loops
        
        for (int i = 0; i < loops[l]->Head.numCels; i++) {
            if (!loops[l]->cells[i] || !loops[l]->cells[i]->cellImage) {
                continue; // Skip invalid cells
            }
            
            const CellImage* bImage = loops[l]->cells[i]->cellImage;
            totalImageSize += bImage->imageSize + bImage->packSize;
            tagsTotalSize += bImage->imageSize;
            
            if (bImage->lines) {
                linesTotalSize += loops[l]->cells[i]->Head.view.yDim * 8; // 4 * 2 = 8
            }
        }
    }
    
    // Calculate base offsets for various data sections
    const unsigned long cellpos_base = VIEW32_HEADER_LINK_SIZE + LOOPHEADERSIZE * Head.view32.loopCount;
    const unsigned long imagepos_base = paletteOffset + paletteSize + (palSCI->palData ? 6 : 0);
    const unsigned long packpos_base = Head.view32.splitView ? (imagepos_base + tagsTotalSize) : 0;
    const unsigned long linespos_base = imagepos_base + totalImageSize + 6;
    const unsigned long linkspos_base = linespos_base + linesTotalSize + 6;
    
    // Working offsets (will be updated as we process cells)
    unsigned long cellpos = cellpos_base;
    unsigned long imagepos = imagepos_base;
    unsigned long packpos = packpos_base;
    unsigned long linespos = linespos_base;
    unsigned long linkspos = linkspos_base;
    
    // Second pass: assign offsets to all cells
    for (int l = 0; l < Head.view32.loopCount; l++) {
        if (!loops[l]) continue; // Skip invalid loops
        
        loops[l]->Head.celOffset = cellpos;
        cellpos += Head.view32.celHeaderSize * loops[l]->Head.numCels;
        
        for (int i = 0; i < loops[l]->Head.numCels; i++) {
            if (!loops[l]->cells[i] || !loops[l]->cells[i]->cellImage) {
                continue; // Skip invalid cells
            }
            
            CelHeaderView* bCell = reinterpret_cast<CelHeaderView*>(&loops[l]->cells[i]->Head);
            const CellImage* bImage = loops[l]->cells[i]->cellImage;
            
            // Calculate data sizes
            bCell->dataByteCount = bImage->imageSize + (bCell->compressType ? bImage->packSize : 0);
            bCell->controlByteCount = bCell->compressType ? bImage->imageSize : 0;
            
            // Set file offsets
            bCell->controlOffset = imagepos;
            bCell->colorOffset = packpos;
            
            // Update positions for next cell
            imagepos += (bCell->compressType ? bCell->controlByteCount : bCell->dataByteCount);
            if (Head.view32.splitView) {
                packpos += (bCell->dataByteCount - bCell->controlByteCount);
            }
            
            // Set row table offset for compressed cells
            if (bCell->compressType) {
                bCell->rowTableOffset = bImage->lines ? linespos : 0;
                linespos += bCell->yDim * 8; // 4 * 2 = 8 bytes per line
            }
            
            // Set link table offset if links exist
            if (bCell->linkTableCount > 0) {
                bCell->linkTableOffset = linkspos;
                linkspos += sizeof(LinkPoint) * bCell->linkTableCount;
            }
        }
    }
    
    return 1; // Success
}

// Drop-in replacement for addLoops - fixes the shallow copy bug
int V56file::addLoops(int base, int amount)
{
    // Validate input parameters
    if (base < 0 || base >= Head.view32.loopCount || amount == 0) {
        return 0;
    }
    
    if (amount > 0) {
        // Adding loops
        const int oldLoopCount = Head.view32.loopCount;
        const int newLoopCount = oldLoopCount + amount;
        
        // Validate base loop exists
        if (!loops[base]) {
            return 0;
        }
        
        // Add new loops by creating deep copies of the base loop
        for (int j = 0; j < amount; j++) {
            const int newIndex = oldLoopCount + j;
            
            // Create new Loop object (deep copy)
            loops[newIndex] = new Loop;
            loops[newIndex]->Head = loops[base]->Head;
            
            // Deep copy all cells in this loop
            for (int i = 0; i < loops[base]->Head.numCels; i++) {
                if (loops[base]->cells[i]) {
                    // Create new Cell object (deep copy)
                    loops[newIndex]->cells[i] = new Cell;
                    loops[newIndex]->cells[i]->Head = loops[base]->cells[i]->Head;
                    
                    // Deep copy the cell image data
                    if (loops[base]->cells[i]->cellImage) {
                        loops[newIndex]->cells[i]->cellImage = new CellImage;
                        CellImage* srcImg = loops[base]->cells[i]->cellImage;
                        CellImage* dstImg = loops[newIndex]->cells[i]->cellImage;
                        
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
                        loops[newIndex]->cells[i]->cellImage = nullptr;
                    }
                    
                    // Copy link points (element by element since it's a fixed array)
                    if (loops[base]->cells[i]->Head.view.linkTableCount > 0) {
                        const int linkCount = loops[base]->cells[i]->Head.view.linkTableCount;
                        for (int linkIdx = 0; linkIdx < linkCount && linkIdx < 10; linkIdx++) {
                            loops[newIndex]->cells[i]->linkPoints[linkIdx] = loops[base]->cells[i]->linkPoints[linkIdx];
                        }
                    }
                    
                    // Set palette reference
                    loops[newIndex]->cells[i]->setPalette(&palSCI);
                }
            }
            
            // Update total cell count
            Head.view32.celCount += loops[newIndex]->Head.numCels;
        }
        
        // Update loop count
        Head.view32.loopCount = newLoopCount;
    } else {
        // Removing loops (amount is negative)
        const int oldLoopCount = Head.view32.loopCount;
        const int newLoopCount = oldLoopCount + amount; // amount is negative
        
        // Validate we won't go below zero
        if (newLoopCount < 0) {
            return 0;
        }
        
        // Subtract cell counts from loops being removed and clean up
        for (int j = newLoopCount; j < oldLoopCount; j++) {
            if (loops[j]) {
                Head.view32.celCount -= loops[j]->Head.numCels;
                
                // Delete all cells in this loop
                for (int i = 0; i < loops[j]->Head.numCels; i++) {
                    if (loops[j]->cells[i]) {
                        // Delete cell image data
                        if (loops[j]->cells[i]->cellImage) {
                            delete[] loops[j]->cells[i]->cellImage->image;
                            delete[] loops[j]->cells[i]->cellImage->pack;
                            delete[] loops[j]->cells[i]->cellImage->lines;
                            delete loops[j]->cells[i]->cellImage;
                        }
                        
                        // Delete cell
                        delete loops[j]->cells[i];
                        loops[j]->cells[i] = nullptr;
                    }
                }
                
                // Delete loop
                delete loops[j];
                loops[j] = nullptr;
            }
        }
        
        // Update loop count
        Head.view32.loopCount = newLoopCount;
    }
    
    return 1;
}

int V56file::writeFileHeader(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    // Write patch ID
    const unsigned long patchID = V56PATCH;
    if (fwrite(&patchID, 4, 1, cfilebuf) != 1) {
        return 0;
    }
    
    // Write standard header values efficiently
    const unsigned short headerValues[] = {320, 200, 5, 6, 256, 0, 0, 0, 0, 0, 0};
    const size_t numValues = sizeof(headerValues) / sizeof(headerValues[0]);
    
    if (fwrite(headerValues, sizeof(unsigned short), numValues, cfilebuf) != numValues) {
        return 0;
    }
    
    return 1;
}

int V56file::writeViewHeader(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    const ViewHeaderLinks* bView = reinterpret_cast<const ViewHeaderLinks*>(&Head);
    const size_t headerSize = bView->version ? VIEW32_HEADER_LINK_SIZE : VIEW32_HEADER_SIZE;
    
    if (fwrite(&Head, headerSize, 1, cfilebuf) != 1) {
        return 0;
    }
    
    return 1;
}

int V56file::writeLoopHeaders(FILE* cfilebuf)
{
    if (!cfilebuf || Head.view32.loopCount <= 0) {
        return 0;
    }
    
    for (int i = 0; i < Head.view32.loopCount; i++) {
        if (!loops[i]) {
            return 0; // Invalid loop
        }
        
        if (fwrite(&loops[i]->Head, LOOPHEADERSIZE, 1, cfilebuf) != 1) {
            return 0;
        }
    }
    
    return 1;
}

int V56file::writeCellHeaders(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    for (int j = 0; j < Head.view32.loopCount; j++) {
        if (!loops[j]) {
            return 0; // Invalid loop
        }
        
        for (int i = 0; i < loops[j]->Head.numCels; i++) {
            if (!loops[j]->cells[i]) {
                return 0; // Invalid cell
            }
            
            if (fwrite(&loops[j]->cells[i]->Head.view, CELHEADERVIEWSIZE, 1, cfilebuf) != 1) {
                return 0;
            }
        }
    }
    
    return 1;
}

int V56file::writeImages(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    // Only write if there are cells to write
    if (Head.view32.celCount <= 0) {
        return 1; // Success - nothing to write
    }
    
    // Write image section header
    const unsigned short ttag = VIEW32_IMAGE_POS;
    if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
        return 0;
    }
    
    if (fwrite(&totalImageSize, 4, 1, cfilebuf) != 1) {
        return 0;
    }
    
    // Write all cell images
    for (int l = 0; l < Head.view32.loopCount; l++) {
        if (!loops[l]) {
            return 0; // Invalid loop
        }
        
        for (int i = 0; i < loops[l]->Head.numCels; i++) {
            if (!loops[l]->cells[i]) {
                return 0; // Invalid cell
            }
            
            loops[l]->cells[i]->WriteImage(cfilebuf);
        }
    }
    
    // Write pack data if split view format
    if (Head.view32.splitView) {
        for (int l = 0; l < Head.view32.loopCount; l++) {
            if (!loops[l]) {
                return 0; // Invalid loop
            }
            
            for (int i = 0; i < loops[l]->Head.numCels; i++) {
                if (!loops[l]->cells[i]) {
                    return 0; // Invalid cell
                }
                
                loops[l]->cells[i]->WritePack(cfilebuf);
            }
        }
    }
    
    return 1;
}

int V56file::writeScanLines(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    // Only write scan lines for split view format
    if (!Head.view32.splitView) {
        return 1; // Success - nothing to write
    }
    
    // Write scan lines section header
    const unsigned short ttag = VIEW32_LINES_POS;
    if (fwrite(&ttag, 2, 1, cfilebuf) != 1) {
        return 0;
    }
    
    const unsigned long tzero = 0;
    if (fwrite(&tzero, 4, 1, cfilebuf) != 1) {
        return 0;
    }
    
    // Write scan lines for all cells
    for (int j = 0; j < Head.view32.loopCount; j++) {
        if (!loops[j]) {
            return 0; // Invalid loop
        }
        
        for (int i = 0; i < loops[j]->Head.numCels; i++) {
            if (!loops[j]->cells[i]) {
                return 0; // Invalid cell
            }
            
            loops[j]->cells[i]->WriteScanLines(cfilebuf);
        }
    }
    
    return 1;
}

int V56file::writeLinks(FILE* cfilebuf)
{
    if (!cfilebuf) {
        return 0;
    }
    
    // Write the standard link header (unknown 00 06 00 00 00 00 pattern)
    LinkPoint tlp = {0}; // Initialize all fields to zero
    tlp.x = VIEW32_LINKS_POS;
    
    if (fwrite(&tlp, sizeof(tlp), 1, cfilebuf) != 1) {
        return 0;
    }
    
    // Write link points for all cells that have them
    for (int l = 0; l < Head.view32.loopCount; l++) {
        if (!loops[l]) {
            return 0; // Invalid loop
        }
        
        for (int i = 0; i < loops[l]->Head.numCels; i++) {
            if (!loops[l]->cells[i]) {
                return 0; // Invalid cell
            }
            
            const CelHeaderView* bCell = reinterpret_cast<const CelHeaderView*>(&loops[l]->cells[i]->Head);
            
            if (bCell->linkTableCount > 0) {
                loops[l]->cells[i]->WriteLinks(cfilebuf);
            }
        }
    }
    
    return 1;
}

bool V56file::SaveFile(HWND hwnd, LPSTR szFileName)
{
    if (!szFileName) {
        return false;
    }
    
    FILE* cfilebuf = fopen(szFileName, "wb");
    if (!cfilebuf) {
        return false;
    }
    
    // Calculate offsets before writing
    if (!loadCellOffset()) {
        fclose(cfilebuf);
        return false;
    }
    
    // Write all file sections in order
    bool success = true;
    
    success &= (writeFileHeader(cfilebuf) != 0);
    success &= (writeViewHeader(cfilebuf) != 0);
    success &= (writeLoopHeaders(cfilebuf) != 0);
    success &= (writeCellHeaders(cfilebuf) != 0);
    
    // Write palette
    if (success && palSCI) {
        palSCI->WritePalette(cfilebuf, false);
        // Note: WritePalette returns void, so we assume success
        // You could add error checking inside WritePalette if needed
    } else {
        success = false;
    }
    
    success &= (writeImages(cfilebuf) != 0);
    success &= (writeScanLines(cfilebuf) != 0);
    success &= (writeLinks(cfilebuf) != 0);
    
    fclose(cfilebuf);
    
    // If writing failed, optionally delete the partial file
    if (!success) {
        remove(szFileName);
    }
    
    return success;
}

// ============================================================================
// CELL UTILITY FUNCTIONS IMPLEMENTATION
// ============================================================================

// === ACCESSOR FUNCTIONS ===

int V56file::getLoopCount() const 
{ 
    return Head.view32.loopCount; 
}

int V56file::getCellCount(int loop) const 
{
    if (loop < 0 || loop >= Head.view32.loopCount || !loops[loop]) {
        return 0;
    }
    return loops[loop]->Head.numCels;
}

int V56file::getTotalCellCount() const 
{ 
    return Head.view32.celCount; 
}

bool V56file::isValidLoop(int loop) const 
{
    return loop >= 0 && loop < Head.view32.loopCount && loops[loop] != nullptr;
}

bool V56file::isValidCell(int loop, int cell) const 
{
    return isValidLoop(loop) && cell >= 0 && cell < loops[loop]->Head.numCels;
}

// === CORE CELL MANIPULATION FUNCTIONS ===

int V56file::addCell(int loop, int baseIndex, int position)
{
    // Validate parameters
    if (!isValidLoop(loop) || !isValidCell(loop, baseIndex)) {
        return 0;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    
    // If position is -1, append at end
    if (position == -1) {
        position = celCount;
    }
    
    // Validate position
    if (position < 0 || position > celCount || celCount >= MAX_LOOPS) {
        return 0;
    }
    
    // Shift existing cells to make room (if inserting in middle)
    for (int i = celCount - 1; i >= position; i--) {
        loops[loop]->cells[i + 1] = loops[loop]->cells[i];
    }
    
    // Create new Cell object (deep copy of base cell)
    loops[loop]->cells[position] = new Cell;
    loops[loop]->cells[position]->Head = loops[loop]->cells[baseIndex]->Head;
    
    // Deep copy the cell image data
    if (loops[loop]->cells[baseIndex]->cellImage) {
        loops[loop]->cells[position]->cellImage = new CellImage;
        CellImage* srcImg = loops[loop]->cells[baseIndex]->cellImage;
        CellImage* dstImg = loops[loop]->cells[position]->cellImage;
        
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
        loops[loop]->cells[position]->cellImage = nullptr;
    }
    
    // Copy link points
    if (loops[loop]->cells[baseIndex]->Head.view.linkTableCount > 0) {
        const int linkCount = loops[loop]->cells[baseIndex]->Head.view.linkTableCount;
        for (int linkIdx = 0; linkIdx < linkCount && linkIdx < 10; linkIdx++) {
            loops[loop]->cells[position]->linkPoints[linkIdx] = loops[loop]->cells[baseIndex]->linkPoints[linkIdx];
        }
    }
    
    // Set palette reference
    loops[loop]->cells[position]->setPalette(&palSCI);
    
    // Update counts
    loops[loop]->Head.numCels++;
    Head.view32.celCount++;
    
    return 1;
}

int V56file::addCells(int loop, int baseIndex, int amount)
{
    // Simple wrapper that adds multiple cells at the end using the new addCell function
    if (amount <= 0) {
        return 0;
    }
    
    // Add cells one by one at the end
    for (int i = 0; i < amount; i++) {
        if (!addCell(loop, baseIndex, -1)) {
            return 0; // Failed to add cell
        }
    }
    
    return 1;
}

int V56file::insertCell(int loop, int baseIndex, int position)
{
    return addCell(loop, baseIndex, position);
}

int V56file::appendCell(int loop, int baseIndex)
{
    return addCell(loop, baseIndex, -1);
}

int V56file::deleteCell(int loop, int position)
{
    // Validate parameters
    if (!isValidLoop(loop) || !isValidCell(loop, position)) {
        return 0;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    
    // Don't allow deleting the last cell
    if (celCount <= 1) {
        return 0;
    }
    
    // Clean up memory for the cell being deleted
    if (loops[loop]->cells[position]) {
        if (loops[loop]->cells[position]->cellImage) {
            CellImage* img = loops[loop]->cells[position]->cellImage;
            delete[] img->image;
            delete[] img->pack;
            delete[] img->lines;
            delete img;
        }
        delete loops[loop]->cells[position];
    }
    
    // Shift remaining cells down
    for (int i = position; i < celCount - 1; i++) {
        loops[loop]->cells[i] = loops[loop]->cells[i + 1];
    }
    
    // Clear the last pointer
    loops[loop]->cells[celCount - 1] = nullptr;
    
    // Update counts
    loops[loop]->Head.numCels--;
    Head.view32.celCount--;
    
    return 1;
}

int V56file::deleteCells(int loop, int start, int count)
{
    if (!isValidLoop(loop) || count <= 0) {
        return 0;
    }
    
    const int oldCelCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= oldCelCount || start + count > oldCelCount) {
        return 0;
    }
    
    if (count >= oldCelCount) {
        return 0; // Don't allow deleting all cells
    }
    
    // Clean up memory for cells being deleted
    for (int i = start; i < start + count; i++) {
        if (loops[loop]->cells[i]) {
            if (loops[loop]->cells[i]->cellImage) {
                CellImage* img = loops[loop]->cells[i]->cellImage;
                delete[] img->image;
                delete[] img->pack;
                delete[] img->lines;
                delete img;
            }
            delete loops[loop]->cells[i];
        }
    }
    
    // Shift remaining cells down
    for (int i = start; i < oldCelCount - count; i++) {
        loops[loop]->cells[i] = loops[loop]->cells[i + count];
    }
    
    // Clear pointers at the end
    for (int i = oldCelCount - count; i < oldCelCount; i++) {
        loops[loop]->cells[i] = nullptr;
    }
    
    // Update counts
    loops[loop]->Head.numCels = oldCelCount - count;
    Head.view32.celCount -= count;
    
    return 1;
}



int V56file::copyCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos)
{
    if (!isValidLoop(srcLoop) || !isValidLoop(dstLoop) || count <= 0) {
        return 0;
    }
    
    const int srcCelCount = loops[srcLoop]->Head.numCels;
    const int dstCelCount = loops[dstLoop]->Head.numCels;
    
    if (srcStart < 0 || srcStart >= srcCelCount || srcStart + count > srcCelCount ||
        dstPos < 0 || dstPos > dstCelCount || dstCelCount + count > MAX_LOOPS) {
        return 0;
    }
    
    // Make room for new cells by shifting existing ones
    for (int i = dstCelCount - 1; i >= dstPos; i--) {
        loops[dstLoop]->cells[i + count] = loops[dstLoop]->cells[i];
    }
    
    // Copy cells
    for (int i = 0; i < count; i++) {
        const int srcIndex = srcStart + i;
        const int dstIndex = dstPos + i;
        
        if (!loops[srcLoop]->cells[srcIndex]) {
            continue;
        }
        
        // Create new Cell object (deep copy)
        loops[dstLoop]->cells[dstIndex] = new Cell;
        loops[dstLoop]->cells[dstIndex]->Head = loops[srcLoop]->cells[srcIndex]->Head;
        
        // Deep copy the cell image data
        if (loops[srcLoop]->cells[srcIndex]->cellImage) {
            loops[dstLoop]->cells[dstIndex]->cellImage = new CellImage;
            CellImage* srcImg = loops[srcLoop]->cells[srcIndex]->cellImage;
            CellImage* dstImg = loops[dstLoop]->cells[dstIndex]->cellImage;
            
            dstImg->imageSize = srcImg->imageSize;
            if (srcImg->image && srcImg->imageSize > 0) {
                dstImg->image = new unsigned char[srcImg->imageSize];
                memcpy(dstImg->image, srcImg->image, srcImg->imageSize);
            } else {
                dstImg->image = nullptr;
            }
            
            dstImg->packSize = srcImg->packSize;
            if (srcImg->pack && srcImg->packSize > 0) {
                dstImg->pack = new unsigned char[srcImg->packSize];
                memcpy(dstImg->pack, srcImg->pack, srcImg->packSize);
            } else {
                dstImg->pack = nullptr;
            }
            
            dstImg->lineSize = srcImg->lineSize;
            if (srcImg->lines && srcImg->lineSize > 0) {
                dstImg->lines = new unsigned char[srcImg->lineSize];
                memcpy(dstImg->lines, srcImg->lines, srcImg->lineSize);
            } else {
                dstImg->lines = nullptr;
            }
        } else {
            loops[dstLoop]->cells[dstIndex]->cellImage = nullptr;
        }
        
        // Copy link points
        if (loops[srcLoop]->cells[srcIndex]->Head.view.linkTableCount > 0) {
            const int linkCount = loops[srcLoop]->cells[srcIndex]->Head.view.linkTableCount;
            for (int linkIdx = 0; linkIdx < linkCount && linkIdx < 10; linkIdx++) {
                loops[dstLoop]->cells[dstIndex]->linkPoints[linkIdx] = loops[srcLoop]->cells[srcIndex]->linkPoints[linkIdx];
            }
        }
        
        loops[dstLoop]->cells[dstIndex]->setPalette(&palSCI);
    }
    
    // Update counts
    loops[dstLoop]->Head.numCels += count;
    if (dstLoop != srcLoop) {
        Head.view32.celCount += count;
    }
    
    return 1;
}

int V56file::copyCell(int srcLoop, int srcIndex, int dstLoop, int dstPos)
{
    return copyCells(srcLoop, srcIndex, 1, dstLoop, dstPos);
}

int V56file::moveCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos)
{
    if (srcLoop == dstLoop) {
        return shiftCells(srcLoop, srcStart, count, dstPos);
    }
    
    if (!copyCells(srcLoop, srcStart, count, dstLoop, dstPos)) {
        return 0;
    }
    
    if (!deleteCells(srcLoop, srcStart, count)) {
        deleteCells(dstLoop, dstPos, count);
        return 0;
    }
    
    return 1;
}

int V56file::moveCell(int srcLoop, int srcIndex, int dstLoop, int dstPos)
{
    return moveCells(srcLoop, srcIndex, 1, dstLoop, dstPos);
}

int V56file::shiftCells(int loop, int start, int count, int newPos)
{
    if (!isValidLoop(loop) || count <= 0) {
        return 0;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount || start + count > celCount ||
        newPos < 0 || newPos > celCount - count || start == newPos) {
        return start == newPos ? 1 : 0; // No-op if same position
    }
    
    Cell** tempCells = new Cell*[count];
    for (int i = 0; i < count; i++) {
        tempCells[i] = loops[loop]->cells[start + i];
    }
    
    if (newPos < start) {
        for (int i = start - 1; i >= newPos; i--) {
            loops[loop]->cells[i + count] = loops[loop]->cells[i];
        }
    } else {
        for (int i = start + count; i < newPos + count; i++) {
            loops[loop]->cells[i - count] = loops[loop]->cells[i];
        }
        newPos -= count;
    }
    
    for (int i = 0; i < count; i++) {
        loops[loop]->cells[newPos + i] = tempCells[i];
    }
    
    delete[] tempCells;
    return 1;
}

int V56file::duplicateCells(int loop, int start, int count)
{
    return copyCells(loop, start, count, loop, start + count);
}

int V56file::duplicateCell(int loop, int index)
{
    return duplicateCells(loop, index, 1);
}

int V56file::swapCells(int loop1, int index1, int loop2, int index2)
{
    if (!isValidCell(loop1, index1) || !isValidCell(loop2, index2)) {
        return 0;
    }
    
    Cell* temp = loops[loop1]->cells[index1];
    loops[loop1]->cells[index1] = loops[loop2]->cells[index2];
    loops[loop2]->cells[index2] = temp;
    
    return 1;
}

// === ADVANCED CELL OPERATIONS ===

int V56file::insertEmptyCells(int loop, int position, int count)
{
    if (!isValidLoop(loop) || count <= 0) {
        return 0;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (position < 0 || position > celCount || celCount + count > MAX_LOOPS) {
        return 0;
    }
    
    // Shift existing cells to make room
    for (int i = celCount - 1; i >= position; i--) {
        loops[loop]->cells[i + count] = loops[loop]->cells[i];
    }
    
    // Create empty cells
    for (int i = 0; i < count; i++) {
        loops[loop]->cells[position + i] = new Cell;
        memset(&loops[loop]->cells[position + i]->Head, 0, sizeof(CelHeader));
        loops[loop]->cells[position + i]->cellImage = nullptr;
        loops[loop]->cells[position + i]->setPalette(&palSCI);
    }
    
    loops[loop]->Head.numCels += count;
    Head.view32.celCount += count;
    
    return 1;
}

int V56file::replaceCells(int dstLoop, int dstStart, int srcLoop, int srcStart, int count)
{
    if (!isValidLoop(dstLoop) || !isValidLoop(srcLoop) || count <= 0) {
        return 0;
    }
    
    if (dstStart < 0 || dstStart + count > loops[dstLoop]->Head.numCels ||
        srcStart < 0 || srcStart + count > loops[srcLoop]->Head.numCels) {
        return 0;
    }
    
    // Delete existing cells in destination range
    for (int i = 0; i < count; i++) {
        int dstIndex = dstStart + i;
        if (loops[dstLoop]->cells[dstIndex]) {
            if (loops[dstLoop]->cells[dstIndex]->cellImage) {
                CellImage* img = loops[dstLoop]->cells[dstIndex]->cellImage;
                delete[] img->image;
                delete[] img->pack;
                delete[] img->lines;
                delete img;
            }
            delete loops[dstLoop]->cells[dstIndex];
        }
    }
    
    // Copy cells from source (reuse copyCells logic)
    for (int i = 0; i < count; i++) {
        int srcIndex = srcStart + i;
        int dstIndex = dstStart + i;
        
        if (!loops[srcLoop]->cells[srcIndex]) {
            loops[dstLoop]->cells[dstIndex] = nullptr;
            continue;
        }
        
        loops[dstLoop]->cells[dstIndex] = new Cell;
        loops[dstLoop]->cells[dstIndex]->Head = loops[srcLoop]->cells[srcIndex]->Head;
        
        if (loops[srcLoop]->cells[srcIndex]->cellImage) {
            loops[dstLoop]->cells[dstIndex]->cellImage = new CellImage;
            CellImage* srcImg = loops[srcLoop]->cells[srcIndex]->cellImage;
            CellImage* dstImg = loops[dstLoop]->cells[dstIndex]->cellImage;
            
            dstImg->imageSize = srcImg->imageSize;
            if (srcImg->image && srcImg->imageSize > 0) {
                dstImg->image = new unsigned char[srcImg->imageSize];
                memcpy(dstImg->image, srcImg->image, srcImg->imageSize);
            } else {
                dstImg->image = nullptr;
            }
            
            dstImg->packSize = srcImg->packSize;
            if (srcImg->pack && srcImg->packSize > 0) {
                dstImg->pack = new unsigned char[srcImg->packSize];
                memcpy(dstImg->pack, srcImg->pack, srcImg->packSize);
            } else {
                dstImg->pack = nullptr;
            }
            
            dstImg->lineSize = srcImg->lineSize;
            if (srcImg->lines && srcImg->lineSize > 0) {
                dstImg->lines = new unsigned char[srcImg->lineSize];
                memcpy(dstImg->lines, srcImg->lines, srcImg->lineSize);
            } else {
                dstImg->lines = nullptr;
            }
        } else {
            loops[dstLoop]->cells[dstIndex]->cellImage = nullptr;
        }
        
        if (loops[srcLoop]->cells[srcIndex]->Head.view.linkTableCount > 0) {
            const int linkCount = loops[srcLoop]->cells[srcIndex]->Head.view.linkTableCount;
            for (int linkIdx = 0; linkIdx < linkCount && linkIdx < 10; linkIdx++) {
                loops[dstLoop]->cells[dstIndex]->linkPoints[linkIdx] = loops[srcLoop]->cells[srcIndex]->linkPoints[linkIdx];
            }
        }
        
        loops[dstLoop]->cells[dstIndex]->setPalette(&palSCI);
    }
    
    return 1;
}

int V56file::reverseCells(int loop, int start, int count)
{
    if (!isValidLoop(loop) || count <= 1) {
        return 1;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount || start + count > celCount) {
        return 0;
    }
    
    for (int i = 0; i < count / 2; i++) {
        int leftIndex = start + i;
        int rightIndex = start + count - 1 - i;
        
        Cell* temp = loops[loop]->cells[leftIndex];
        loops[loop]->cells[leftIndex] = loops[loop]->cells[rightIndex];
        loops[loop]->cells[rightIndex] = temp;
    }
    
    return 1;
}

int V56file::sortCells(int loop, int start, int count, int sortBy, bool ascending)
{
    if (!isValidLoop(loop) || count <= 1 || sortBy < 0 || sortBy > 2) {
        return count <= 1 ? 1 : 0;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount || start + count > celCount) {
        return 0;
    }
    
    // Simple bubble sort
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            int idx1 = start + j;
            int idx2 = start + j + 1;
            
            if (!loops[loop]->cells[idx1] || !loops[loop]->cells[idx2]) {
                continue;
            }
            
            int val1 = 0, val2 = 0;
            
            switch (sortBy) {
                case 0: // Width
                    val1 = loops[loop]->cells[idx1]->Head.view.xDim;
                    val2 = loops[loop]->cells[idx2]->Head.view.xDim;
                    break;
                case 1: // Height
                    val1 = loops[loop]->cells[idx1]->Head.view.yDim;
                    val2 = loops[loop]->cells[idx2]->Head.view.yDim;
                    break;
                case 2: // Image size
                    val1 = loops[loop]->cells[idx1]->cellImage ? 
                           loops[loop]->cells[idx1]->cellImage->imageSize : 0;
                    val2 = loops[loop]->cells[idx2]->cellImage ? 
                           loops[loop]->cells[idx2]->cellImage->imageSize : 0;
                    break;
            }
            
            bool shouldSwap = ascending ? (val1 > val2) : (val1 < val2);
            
            if (shouldSwap) {
                Cell* temp = loops[loop]->cells[idx1];
                loops[loop]->cells[idx1] = loops[loop]->cells[idx2];
                loops[loop]->cells[idx2] = temp;
            }
        }
    }
    
    return 1;
}

// === BATCH OPERATIONS ===

int V56file::batchDeleteCells(int loop, const int* indices, int indexCount)
{
    if (!isValidLoop(loop) || !indices || indexCount <= 0) {
        return 0;
    }
    
    int* sortedIndices = new int[indexCount];
    for (int i = 0; i < indexCount; i++) {
        sortedIndices[i] = indices[i];
    }
    
    // Sort in descending order
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
        if (deleteCell(loop, sortedIndices[i])) {
            deletedCount++;
        }
    }
    
    delete[] sortedIndices;
    return deletedCount > 0 ? 1 : 0;
}

// === SEARCH AND FIND OPERATIONS ===

int V56file::findCellsBySize(int loop, int width, int height, int* results, int maxResults)
{
    int foundCount = 0;
    int startLoop = (loop >= 0) ? loop : 0;
    int endLoop = (loop >= 0) ? loop : Head.view32.loopCount - 1;
    
    for (int l = startLoop; l <= endLoop && foundCount < maxResults; l++) {
        if (!isValidLoop(l)) continue;
        
        for (int c = 0; c < loops[l]->Head.numCels && foundCount < maxResults; c++) {
            if (!loops[l]->cells[c]) continue;
            
            bool widthMatch = (width == 0) || (loops[l]->cells[c]->Head.view.xDim == width);
            bool heightMatch = (height == 0) || (loops[l]->cells[c]->Head.view.yDim == height);
            
            if (widthMatch && heightMatch) {
                results[foundCount * 2] = l;
                results[foundCount * 2 + 1] = c;
                foundCount++;
            }
        }
    }
    
    return foundCount;
}

int V56file::findEmptyCells(int loop, int* results, int maxResults)
{
    int foundCount = 0;
    int startLoop = (loop >= 0) ? loop : 0;
    int endLoop = (loop >= 0) ? loop : Head.view32.loopCount - 1;
    
    for (int l = startLoop; l <= endLoop && foundCount < maxResults; l++) {
        if (!isValidLoop(l)) continue;
        
        for (int c = 0; c < loops[l]->Head.numCels && foundCount < maxResults; c++) {
            bool isEmpty = !loops[l]->cells[c] || 
                          !loops[l]->cells[c]->cellImage ||
                          loops[l]->cells[c]->cellImage->imageSize == 0;
            
            if (isEmpty) {
                results[foundCount * 2] = l;
                results[foundCount * 2 + 1] = c;
                foundCount++;
            }
        }
    }
    
    return foundCount;
}

// === GUI UTILITY FUNCTIONS ===

CellRangeInfo V56file::getCellRangeInfo(int loop, int start, int count)
{
    CellRangeInfo info;
    
    if (!isValidLoop(loop)) {
        return info;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount) {
        return info;
    }
    
    const int endPos = (start + count > celCount) ? celCount : start + count;
    
    for (int i = start; i < endPos; i++) {
        if (!loops[loop]->cells[i]) continue;
        
        info.totalCells++;
        
        if (loops[loop]->cells[i]->cellImage) {
            CellImage* img = loops[loop]->cells[i]->cellImage;
            info.totalImageSize += img->imageSize;
            info.totalPackSize += img->packSize;
        }
        
        CelHeaderView* header = reinterpret_cast<CelHeaderView*>(&loops[loop]->cells[i]->Head);
        if (header->compressType) {
            info.compressedCount++;
        } else {
            info.uncompressedCount++;
        }
        
        if (header->linkTableCount > 0) {
            info.hasLinks = true;
        }
    }
    
    return info;
}

ViewStats V56file::getViewStats()
{
    ViewStats stats;
    stats.totalLoops = Head.view32.loopCount;
    stats.totalCells = Head.view32.celCount;
    
    for (int l = 0; l < Head.view32.loopCount; l++) {
        if (!isValidLoop(l)) continue;
        
        for (int c = 0; c < loops[l]->Head.numCels; c++) {
            if (!loops[l]->cells[c]) {
                stats.emptyCells++;
                continue;
            }
            
            if (loops[l]->cells[c]->cellImage) {
                CellImage* img = loops[l]->cells[c]->cellImage;
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
            
            CelHeaderView* header = reinterpret_cast<CelHeaderView*>(&loops[l]->cells[c]->Head);
            if (header->compressType) {
                stats.compressedCells++;
            } else {
                stats.uncompressedCells++;
            }
            
            if (header->linkTableCount > 0) {
                stats.cellsWithLinks++;
            }
        }
    }
    
    if (stats.smallestCellSize == 2147483647) { // INT_MAX
        stats.smallestCellSize = 0;
    }
    
    return stats;
}

// === VALIDATION FUNCTIONS ===

bool V56file::canDeleteCells(int loop, int start, int count)
{
    if (!isValidLoop(loop) || count <= 0) {
        return false;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount || start + count > celCount) {
        return false;
    }
    
    return count < celCount; // Don't allow deleting all cells
}

bool V56file::canInsertCells(int loop, int position, int count)
{
    if (!isValidLoop(loop) || count <= 0) {
        return false;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (position < 0 || position > celCount) {
        return false;
    }
    
    return celCount + count <= MAX_LOOPS;
}

bool V56file::canMoveCells(int srcLoop, int srcStart, int count, int dstLoop, int dstPos)
{
    if (!canDeleteCells(srcLoop, srcStart, count)) {
        return false;
    }
    
    if (srcLoop == dstLoop) {
        // Moving within same loop - just check positions
        const int celCount = loops[srcLoop]->Head.numCels;
        return dstPos >= 0 && dstPos <= celCount - count && dstPos != srcStart;
    }
    
    // Moving between loops
    return canInsertCells(dstLoop, dstPos, count);
}

bool V56file::canModifyCellRange(int loop, int start, int count)
{
    if (!isValidLoop(loop) || count <= 0) {
        return false;
    }
    
    const int celCount = loops[loop]->Head.numCels;
    if (start < 0 || start >= celCount || start + count > celCount) {
        return false;
    }
    
    // Check if any cells in range are null
    for (int i = start; i < start + count; i++) {
        if (!loops[loop]->cells[i]) {
            return false;
        }
    }
    
    return true;
}

// === MAINTENANCE OPERATIONS ===

int V56file::cutCells(int loop, int start, int count, Cell** clipboard)
{
    if (!clipboard || !canModifyCellRange(loop, start, count)) {
        return 0;
    }
    
    // Copy cells to clipboard first (simplified - user manages memory)
    for (int i = 0; i < count; i++) {
        clipboard[i] = loops[loop]->cells[start + i];
    }
    
    // Set cell pointers to null to avoid double deletion
    for (int i = start; i < start + count; i++) {
        loops[loop]->cells[i] = nullptr;
    }
    
    // Then delete the cells (but they're already nulled, so just shift)
    return deleteCells(loop, start, count);
}

int V56file::optimizeLoop(int loop)
{
    if (!isValidLoop(loop)) {
        return 0;
    }
    
    int removed = 0;
    int writePos = 0;
    const int originalCount = loops[loop]->Head.numCels;
    
    // Compact array by moving valid cells to the front
    for (int readPos = 0; readPos < originalCount; readPos++) {
        bool isEmpty = !loops[loop]->cells[readPos] || 
                      !loops[loop]->cells[readPos]->cellImage ||
                      loops[loop]->cells[readPos]->cellImage->imageSize == 0;
        
        if (isEmpty) {
            // Clean up empty cell
            if (loops[loop]->cells[readPos]) {
                if (loops[loop]->cells[readPos]->cellImage) {
                    CellImage* img = loops[loop]->cells[readPos]->cellImage;
                    delete[] img->image;
                    delete[] img->pack;
                    delete[] img->lines;
                    delete img;
                }
                delete loops[loop]->cells[readPos];
            }
            removed++;
        } else {
            // Move valid cell to write position
            if (writePos != readPos) {
                loops[loop]->cells[writePos] = loops[loop]->cells[readPos];
            }
            writePos++;
        }
    }
    
    // Clear remaining pointers
    for (int i = writePos; i < originalCount; i++) {
        loops[loop]->cells[i] = nullptr;
    }
    
    // Update counts
    loops[loop]->Head.numCels = writePos;
    Head.view32.celCount -= removed;
    
    return removed;
}
