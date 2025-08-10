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

// Drop-in replacement for addCells - fixes the shallow copy bug
int V56file::addCells(int loop, int base, int amount)
{
    // Validate input parameters
    if (loop < 0 || loop >= Head.view32.loopCount || amount <= 0) {
        return 0;
    }
    
    if (!loops[loop]) {
        return 0;
    }
    
    if (base < 0 || base >= loops[loop]->Head.numCels) {
        return 0;
    }
    
    if (!loops[loop]->cells[base]) {
        return 0;
    }
    
    const int oldCelCount = loops[loop]->Head.numCels;
    const int newCelCount = oldCelCount + amount;
    
    // Simple bounds check (assuming reasonable limits)
    if (newCelCount > 256) {
        return 0;
    }
    
    // Append new cells at the end (matching original behavior)
    for (int i = 0; i < amount; i++) {
        const int newIndex = oldCelCount + i;
        
        // Create new Cell object (deep copy)
        loops[loop]->cells[newIndex] = new Cell;
        loops[loop]->cells[newIndex]->Head = loops[loop]->cells[base]->Head;
        
        // Deep copy the cell image data
        if (loops[loop]->cells[base]->cellImage) {
            loops[loop]->cells[newIndex]->cellImage = new CellImage;
            CellImage* srcImg = loops[loop]->cells[base]->cellImage;
            CellImage* dstImg = loops[loop]->cells[newIndex]->cellImage;
            
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
            loops[loop]->cells[newIndex]->cellImage = nullptr;
        }
        
        // Copy link points (element by element since it's a fixed array)
        if (loops[loop]->cells[base]->Head.view.linkTableCount > 0) {
            const int linkCount = loops[loop]->cells[base]->Head.view.linkTableCount;
            for (int linkIdx = 0; linkIdx < linkCount && linkIdx < 10; linkIdx++) {
                loops[loop]->cells[newIndex]->linkPoints[linkIdx] = loops[loop]->cells[base]->linkPoints[linkIdx];
            }
        }
        
        // Set palette reference
        loops[loop]->cells[newIndex]->setPalette(&palSCI);
    }
    
    // Update counts
    loops[loop]->Head.numCels = newCelCount;
    Head.view32.celCount += amount;
    
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