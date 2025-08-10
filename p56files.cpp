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

P56file32::~P56file32(void)
{
	if (palSCI)
		delete palSCI;

 	if (vector)
		delete vector;

	if (cells)
	{
		PicHeader32 *bPic32;
		PicHeader11 *bPic11;

		int cellCount = 0;

		switch (format)
		{

		case _PIC_32:

			bPic32 = (PicHeader32 *)&Head;
			cellCount = bPic32->celCount;
			break;

		case _PIC_11:

			bPic11 = (PicHeader11 *)&Head;
			cellCount = bPic11->celCount;
			break;
		}

		for (unsigned short i = 0; i < cellCount; i++)
		{
			if (cells[i])
			{
				if (!cells[i]->isClone)
				{
					Cell *texcell = cells[i];
					delete (texcell);
				}
			}
		}
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

// Drop-in replacement for P56file32::addCells - fixes the shallow copy bug
int P56file32::addCells(int base, int amount)
{
    // Validate input parameters
    if (base < 0 || amount <= 0) {
        return 0;
    }
    
    // Check if base cell exists
    if (base >= Head.pic32.celCount || !cells[base]) {
        return 0;
    }
    
    int cellIndex = Head.pic32.celCount;
    Head.pic32.celCount += amount;
    
    for (int i = 0; i < amount; i++) {
        // Create new Cell object (deep copy) instead of shallow copy
        Cell* newCell = new Cell;
        newCell->Head = cells[base]->Head;
        newCell->isClone = true; // Maintain original behavior
        
        // Deep copy the cell image data
        if (cells[base]->cellImage) {
            newCell->cellImage = new CellImage;
            CellImage* srcImg = cells[base]->cellImage;
            CellImage* dstImg = newCell->cellImage;
            
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
            newCell->cellImage = nullptr;
        }
        
        // Set palette reference (shallow copy is OK for palette)
        newCell->setPalette(&palSCI);
        
        // Add to cells array
        cells[cellIndex++] = newCell;
    }
    
    return 1;
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
