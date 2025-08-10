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

V56file::~V56file(void)
{
	if (palSCI)
		delete palSCI;

	if (loops)
	{
		for (unsigned short i=0; i< Head.view32.loopCount; i++)
		{
			Loop *texloop = loops[i];
			delete (texloop);		
		}
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

int V56file::addLoops( int base, int amount )
{
	int retVal = 0;

	int loopIndex = 0;

	if (amount >= 0)
	{
		for (int j = 0; j < Head.view32.loopCount; j++)
		loopIndex++;

		Head.view32.loopCount = Head.view32.loopCount + amount;

		for (int j = 0; j < amount; j++)
		{
			loops[loopIndex] = loops[base];

			loops[loopIndex]->Head = loops[base]->Head;
			Head.view32.celCount = Head.view32.celCount + loops[loopIndex]->Head.numCels;

			loopIndex++;
		}
	}
	else
	{
		for (int j = 0; j < Head.view32.loopCount + amount; j++)
			loopIndex++;

		for (int j = Head.view32.loopCount + amount; j < Head.view32.loopCount; j++)
		{
			Head.view32.celCount = Head.view32.celCount - loops[loopIndex]->Head.numCels;
			loopIndex++;
		}

		Head.view32.loopCount = Head.view32.loopCount + amount;		
	}

	retVal = 1;
	return retVal;
}

int V56file::loadCellOffset(void)
{
	int retVal = 0;

	// Initialize variables
	totalImageSize = 0;
	unsigned long tagsTotalSize = 0;
	unsigned long paletteOffset = 0;
	unsigned long paletteSize = 0;
	unsigned long linesTotalSize = 0;

	// Check if palette is present and calculate palette size
	if (palSCI->palData)
	{
		paletteSize = COMPPALSIZE + (palSCI->Head.nColors * (!palSCI->Head.type ? 4 : 3));
	}

	paletteOffset = 2 + Head.view32.viewHeaderSize + Head.view32.loopHeaderSize * Head.view32.loopCount + Head.view32.celHeaderSize * Head.view32.celCount + 6;
	Head.view32.paletteOffset = (palSCI->palData ? paletteOffset : 0);

	// Loop through all loops
	for (int l = 0; l < Head.view32.loopCount; l++)
	{		
		// Loop through all cells in current loop
		for (int i = 0; i < loops[l]->Head.numCels; i++)
		{
			CellImage *bImage = loops[l]->cells[i]->cellImage;

			totalImageSize += bImage->imageSize + bImage->packSize;
			tagsTotalSize += bImage->imageSize;

			if (loops[l]->cells[i]->cellImage->lines)
			{
				linesTotalSize += loops[l]->cells[i]->Head.view.yDim * 4 * 2;
			}
		}
	}

	// Calculate offsets for various data
	unsigned long cellpos = VIEW32_HEADER_LINK_SIZE + LOOPHEADERSIZE * Head.view32.loopCount;
	unsigned long imagepos = paletteOffset + paletteSize + (palSCI->palData ? 6 : 0);
	unsigned long packpos = (Head.view32.splitView ? imagepos + tagsTotalSize : 0);
	unsigned long linespos = imagepos + totalImageSize + 6;
	unsigned long linkspos = linespos + linesTotalSize + 6; 

	// Loop through all loops again
	for (int l = 0; l < Head.view32.loopCount; l++)
	{
		loops[l]->Head.celOffset = cellpos;
		cellpos += Head.view32.celHeaderSize * loops[l]->Head.numCels;

		// Loop through all cells in current loop
		for (int i = 0; i < loops[l]->Head.numCels; i++)
		{
			CelHeaderView *bCell = (CelHeaderView *)&loops[l]->cells[i]->Head;

			CellImage *bImage = loops[l]->cells[i]->cellImage;

			bCell->dataByteCount = (bCell->compressType ? bImage->imageSize + bImage->packSize : bImage->imageSize);
			bCell->controlByteCount = (bCell->compressType ? bImage->imageSize : 0); // TODO CHECK DUNNO IF IT'S 0 or 6... in GK2 it's 0
	
			// memset(&(loops[l]->cells[i]->Head.view.unknown), 0, 10);
			bCell->controlOffset = imagepos;
			bCell->colorOffset = packpos;

			// Update offsets
			imagepos += (bCell->compressType ? bCell->controlByteCount : bCell->dataByteCount);
			packpos += (Head.view32.splitView ? bCell->dataByteCount - bCell->controlByteCount : 0);

			if (bCell->compressType)
			{	
				bCell->rowTableOffset = (loops[l]->cells[i]->cellImage->lines ? linespos : 0);
				linespos += bCell->yDim * 4 * 2;
			}

			if (bCell->linkTableCount)
			{
				bCell->linkTableOffset = linkspos;

				linkspos += sizeof(LinkPoint) * bCell->linkTableCount;
			}
		}
	}

	retVal = 1;
	return retVal;
}

int V56file::addCells( int loop, int base, int amount)
{
	int retVal = 0;
	
	for (int j = 0; j < Head.view32.loopCount; j++)
	{
		int cellHeaderIndex = 0;
		
		if (j == loop)
		{
			int loopCellIndex = 0;

			loops[j]->Head.numCels = loops[j]->Head.numCels + amount;

			Head.view32.celCount = Head.view32.celCount + amount;

			for (int i = 0; i < loops[j]->Head.numCels - amount; i++)
			{				
				cellHeaderIndex++;
				loopCellIndex++;			
			}

			for (int i = 0; i < amount; i++)
			{
				loops[j]->cells[loopCellIndex] = loops[j]->cells[base];
				cellHeaderIndex++;
				loopCellIndex++;
			}

		}
		else
		{
			for (int i = 0; i < loops[j]->Head.numCels; i++)		
				cellHeaderIndex++;
		}
	}
	
	retVal = 1;
	return retVal;
}

int V56file::writeFileHeader(FILE *cfilebuf)
{
	int retVal = 0;

	unsigned long patchID = 0;
	patchID += V56PATCH;
	fwrite(&patchID, 4, 1, cfilebuf);

	unsigned short tshort = 320;
	fwrite(&tshort, 2, 1, cfilebuf);
	tshort = 200;
	fwrite(&tshort, 2, 1, cfilebuf);
	tshort = 5;
	fwrite(&tshort, 2, 1, cfilebuf);
	tshort = 6;
	fwrite(&tshort, 2, 1, cfilebuf);
	tshort = 256;
	fwrite(&tshort, 2, 1, cfilebuf);
	tshort = 0;
	for (int i = 0; i < 6; i++)
		fwrite(&tshort, 2, 1, cfilebuf);

	retVal = 1;

	return retVal;
}

int V56file::writeViewHeader(FILE *cfilebuf)
{
	int retVal = 0;

	ViewHeaderLinks *bView = (ViewHeaderLinks *)&Head;

	if (bView->version)
		fwrite(&Head, VIEW32_HEADER_LINK_SIZE, 1, cfilebuf);
	else
		fwrite(&Head, VIEW32_HEADER_SIZE, 1, cfilebuf);

	retVal = 1;

	return retVal;
	
}

int V56file::writeLoopHeaders(FILE *cfilebuf)
{
	int retVal = 0;
	
	for (int i = 0; i < Head.view32.loopCount; i++)
		fwrite(&loops[i]->Head, LOOPHEADERSIZE, 1, cfilebuf);		

	retVal = 1;

	return retVal;
	
}

int V56file::writeCellHeaders(FILE *cfilebuf)
{
	int retVal = 0;
		
	for (int j = 0; j < Head.view32.loopCount; j++)
		for (int i = 0; i < loops[j]->Head.numCels; i++)
			fwrite(&loops[j]->cells[i]->Head.view, CELHEADERVIEWSIZE, 1, cfilebuf);

	retVal = 1;

	return retVal;	
}

int V56file::writeImages(FILE *cfilebuf)
{
	int retVal = 0;

	// writing images now:
	if (Head.view32.celCount)
	{
		unsigned short ttag = VIEW32_IMAGE_POS;
		fwrite(&ttag, 2, 1, cfilebuf);
		fwrite(&totalImageSize, 4, 1, cfilebuf);

		// write current cell images
		for (int l = 0; l < Head.view32.loopCount; l++)
		{
			for (int i = 0; i < loops[l]->Head.numCels; i++)
			{
				(*loops[l]->cells[i]).WriteImage(cfilebuf);
			}
		}

		if (Head.view32.splitView)
		{
			// write current cell compression
			for (int l = 0; l < Head.view32.loopCount; l++)
			{
				for (int i = 0; i < loops[l]->Head.numCels; i++)
				{
					(*loops[l]->cells[i]).WritePack(cfilebuf);
				}
			}
		}
	}

	retVal = 1;
	return retVal;
}

int V56file::writeScanLines(FILE *cfilebuf)
{
	int retVal = 0;

	if (Head.view32.splitView) // writing scan lines
	{

		unsigned short ttag = VIEW32_LINES_POS;
		fwrite(&ttag, 2, 1, cfilebuf);
		unsigned long tzero = 0;
		fwrite(&tzero, 4, 1, cfilebuf);

		// write current scan lines
		for (int j = 0; j < Head.view32.loopCount; j++)
		{
			for (int i = 0; i < loops[j]->Head.numCels; i++)
			{
				(*loops[j]->cells[i]).WriteScanLines(cfilebuf);
			}
		}
	}

	retVal = 1;

	return retVal;
}

int V56file::writeLinks(FILE *cfilebuf)
{
	int retVal = 0;

	// first there is an unknown 00 06 00 00 00 00 in all files:
	LinkPoint tlp;
	memset(&tlp, 0, sizeof(tlp));
	tlp.x = VIEW32_LINKS_POS;
	fwrite(&tlp, sizeof(tlp), 1, cfilebuf);

	// write current link points
	for (int l = 0; l < Head.view32.loopCount; l++)
	{
		for (int i = 0; i < loops[l]->Head.numCels; i++)
		{
			CelHeaderView *bCell = bCell = (CelHeaderView *)&loops[l]->cells[i]->Head;

			if (bCell->linkTableCount)
				(*loops[l]->cells[i]).WriteLinks(cfilebuf);
		}
	}

	retVal = 1;
	return retVal;
}

bool V56file::SaveFile (HWND hwnd, LPSTR szFileName)
{
	int retVal = 0;

	FILE *cfilebuf = fopen(szFileName, "wb");
	if (cfilebuf)
	{
		loadCellOffset();	
		writeFileHeader(cfilebuf);
		writeViewHeader(cfilebuf);
		writeLoopHeaders(cfilebuf);
		writeCellHeaders(cfilebuf);

		palSCI->WritePalette(cfilebuf, false);

		writeImages(cfilebuf);

		writeScanLines(cfilebuf);

		writeLinks(cfilebuf);

		fclose(cfilebuf);

		retVal = 1;
	}

	return retVal;
}
