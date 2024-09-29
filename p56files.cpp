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
	//_myHwnd = hwnd;
	FILE *cfilebuf = fopen(pszFileName, "rb");
	if (cfilebuf)
	{
		// dispose old file structs from memory:
		/*
		if (_cells)
			delete _cells;
		_cells = 0;
		*/

		//if (palSCI)
		//	delete palSCI;
		palSCI = 0;

		// first check if the Patches header is there :
		unsigned char offset = 0;
		unsigned long patchID;
		unsigned short hsize = 0;

		fread((void *)&patchID, 4, 1, cfilebuf);
		if (patchID == P56PATCH80 || patchID == P56PATCH)
			offset = 4;
		if (patchID == P56PATCHOLD)
			offset = 26;

		fseek(cfilebuf, offset, SEEK_SET);

		fread((void *)&hsize, 2, 1, cfilebuf);

		switch (hsize)
		{
		case 14: // typical offset used
			format = _PIC_32;
			LoadPic32(cfilebuf, offset);
			break;
		case 38:
			format = _PIC_11;
			LoadPic11(cfilebuf, offset);
			break;
		default:
			return ID_WRONGHEADER;
		}

		fclose(cfilebuf);

		return ID_NOERROR;
	}

	return ID_CANTOPENFILE;
}

int P56file32::LoadPic32( FILE *cfilebuf, unsigned char offset )
{
	//_myHwnd = hwnd;
	
	if (cfilebuf)
	{

		fseek(cfilebuf, offset + 4, SEEK_SET);
		int tcellrecsize = 0;
		fread(&tcellrecsize, 2, 1, cfilebuf);
		if (tcellrecsize != 0x2a)
			return ID_WRONGCELLRECSIZE;

		fseek(cfilebuf, offset, SEEK_SET);

		fread(&Head, PICHEADER32SIZE, 1, cfilebuf);

		PicHeader32 *bPic = new PicHeader32;
		bPic = (PicHeader32 *)&Head;

		fseek(cfilebuf, offset - 6 + bPic->paletteOffset, SEEK_SET);

		int ttag = 0;
		fread(&ttag, 2, 1, cfilebuf);
		if (ttag != PALETTE_POS)
			return ID_WRONGPALETTELOC;

		unsigned long tpalsize = 0;
		fread(&tpalsize, 4, 1, cfilebuf);

		palSCI = new Palette;

		palSCI->loadPalette(cfilebuf, tpalsize);

		// palette loading completed, now read cells:
		if (bPic->celCount)
		{
			fseek(cfilebuf, offset + bPic->picHeaderSize, SEEK_SET);

			for (int i = 0; i < bPic->celCount; i++)
			{
				cells[i] = new Cell;
				fread(&(cells[i]->Head), CELHEADERPICSIZE, 1, cfilebuf);
			}

			for (int i = 0; i < bPic->celCount; i++)
			{			
				cells[i]->setPalette(&palSCI);	
				cells[i]->loadImage( cfilebuf, offset );
			}
		}

		fclose(cfilebuf);

		return ID_NOERROR;
	}

	return ID_CANTOPENFILE;
}

int P56file32::LoadPic11( FILE *cfilebuf, unsigned char offset )
{
	//_myHwnd = hwnd;
	
	if (cfilebuf)
	{
		
		fseek(cfilebuf, offset, SEEK_SET);

		fread(&Head, PICHEADER11SIZE, 1, cfilebuf);

		PicHeader11 *bPic = new PicHeader11;
		bPic = (PicHeader11 *)&Head;

		if (bPic->priCelOffset)
			MessageBox(hWnd, "priCelOffset is defined. This file might not be fully supported by FotoSCIhop!", WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);

		if (bPic->controlCelOffset)
			MessageBox(hWnd, "controlCelOffset is defined. This file might not be fully supported by FotoSCIhop!", WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);

		if (bPic->polygonOffset)
			MessageBox(hWnd, "polygonOffset is defined. This file might not be fully supported by FotoSCIhop!", WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);

		vector = (unsigned char *)new char[bPic->vectorSize];
		fseek(cfilebuf, offset + bPic->vectorOffset, SEEK_SET);
		fread(vector, bPic->vectorSize, 1, cfilebuf);

		// if (Head.oldhead.VectorDataLenght != Head.oldhead.VectorDataLenght)
		//	MessageBox(hwnd, "Vector Size block has a different size than expected!", WARN_ATTENTION, MB_OK | MB_ICONEXCLAMATION);

		fseek(cfilebuf, offset - 6 + bPic->paletteOffset, SEEK_SET);

		int ttag = 0;
		fread(&ttag, 2, 1, cfilebuf);
		if (ttag != PALETTE_POS)
			return ID_WRONGPALETTELOC;

		unsigned long tpalsize = 0;
		fread(&tpalsize, 4, 1, cfilebuf);

		palSCI = new Palette;

		palSCI->loadPalette(cfilebuf, tpalsize);

		// palette loading completed, now read cells:
		short tshort;
		fseek(cfilebuf, offset + PICHEADER11SIZE, SEEK_SET);
		fread(&tshort, 2, 1, cfilebuf);
		_unkShort1 = tshort;
		fread(&tshort, 2, 1, cfilebuf);
		_unkShort2 = tshort;

		if (bPic->celCount)
		{
			fseek(cfilebuf, offset + bPic->visualHeaderOffset, SEEK_SET);

			for (int i = 0; i < bPic->celCount; i++)
			{
				cells[i] = new Cell;
				fread(&(cells[i]->Head.pic), CELHEADER11SIZE, 1, cfilebuf);
			}

			for (int i = 0; i < bPic->celCount; i++)
			{
				cells[i]->setPalette(&palSCI);
				cells[i]->loadImage( cfilebuf, offset );
			}
		}

		fclose(cfilebuf);

		return ID_NOERROR;
	}

	return ID_CANTOPENFILE;
}


int P56file32::loadCellOffset()
{
	int retVal = 0;

	unsigned long paletteSize = COMPPALSIZE + (palSCI->Head.nColors * (!palSCI->Head.type ? 4 : 3));
	unsigned long imagepos;

	unsigned long tagsTotalSize = 0;

	
	CelBase *bCell = new CelBase;

	PicHeader32 *bPic32 = new PicHeader32;
	PicHeader11 *bPic11 = new PicHeader11;

	int cellCount = 0;

	switch (format)
	{

	case _PIC_32:

		bPic32 = (PicHeader32 *)&Head;

		cellCount = bPic32->celCount;

		for (int i = 0; i < cellCount; i++)
		{
			bCell = (CelBase *)&cells[i]->Head;

			imageAllSize += cells[i]->cellImage->imageSize + cells[i]->cellImage->packSize;
			tagsTotalSize += cells[i]->cellImage->imageSize;
		}

		bPic32->paletteOffset = bPic32->picHeaderSize + CELHEADERPICSIZE * cellCount + 6;

		imagepos = bPic32->paletteOffset + paletteSize + 6;

		break;

	case _PIC_11:

		bPic11 = (PicHeader11 *)&Head;

		cellCount = bPic11->celCount;

		for (int i = 0; i < cellCount; i++)
		{

			imageAllSize += cells[i]->cellImage->imageSize + cells[i]->cellImage->packSize;
			tagsTotalSize += cells[i]->cellImage->imageSize;
		}

		bPic11->picHeaderSize = 38;

		bPic11->visualHeaderOffset = 68;
		if (cellCount)
			bPic11->visualHeaderOffset += 4;

		bPic11->paletteOffset = bPic11->visualHeaderOffset + CELHEADER11SIZE * cellCount + 6;

		if (imageAllSize)
			bPic11->paletteOffset += imageAllSize + 6;

		bPic11->vectorOffset = bPic11->paletteOffset + paletteSize + 6;

		imagepos = bPic11->visualHeaderOffset + cellCount * PIC11CELLRECSIZE + 6;

		break;
	default:
		return ID_WRONGHEADER;
	}

	// loading cells now:
	unsigned long packpos = (bCell->compressType ? imagepos + tagsTotalSize : 0); // TODO check 0!!
	unsigned long linespos = imagepos + imageAllSize + 6;

	for (int i = 0; i < cellCount; i++)
	{
		bCell = (CelBase *)&cells[i]->Head;

		CellImage *bImage = cells[i]->cellImage;
		bCell->dataByteCount = (bCell->compressType ? bImage->imageSize + bImage->packSize : bImage->imageSize);
		bCell->controlByteCount = (bCell->compressType ? bImage->imageSize : 0); // TODO CHECK DUNNO IF IT'S 0 or 6... in GK2 it's 0

		bCell->controlOffset = imagepos;
		bCell->colorOffset = packpos;
		
		imagepos += (bCell->compressType ? bCell->controlByteCount : bCell->dataByteCount);
		packpos += (bCell->compressType ? bCell->dataByteCount - bCell->controlByteCount : 0);

		if (format != _PIC_11 && bCell->compressType)
		{
			bCell->rowTableOffset = linespos;
			linespos += bCell->yDim * 4 * 2;
		}

		bCell->paletteOffset = (format == _PIC_11 ? bPic11->paletteOffset : 0);
	}

	retVal = 1;

	return retVal;
}

int P56file32::addCells(int base, int amount)
{
	int cellIndex = Head.pic32.celCount;
	Head.pic32.celCount += amount;
	for (int i = 0; i < amount; i++)
	{
		Cell *newCell = cells[base];
		newCell->isClone = true;
		cells[cellIndex++] = newCell;
	}

	return 1;
}

int P56file32::writeFileHeader(FILE *cfilebuf)
{
	int retVal = 0;

	unsigned long offset = 0;
	unsigned long patchID = 0;

	if (format != _PIC_11)
	{
		patchID = P56PATCH80;
		fwrite(&patchID, 4, 1, cfilebuf);

		offset = 4;
	}
	else
	{
		patchID = 0;
		patchID += P56PATCHOLD;
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

		offset = 26;
	}

	retVal = 1;

	return retVal;
}

int P56file32::writePicHeader(FILE *cfilebuf, int cellCount)
{
	int retVal = 0;

	fwrite(&Head, (format == _PIC_11 ? PICHEADER11SIZE : PICHEADER32SIZE), 1, cfilebuf);

	if (format == _PIC_11 && cellCount)
	{
		fwrite(&_unkShort1, 2, 1, cfilebuf);
		fwrite(&_unkShort2, 2, 1, cfilebuf);
	}

	retVal = 1;

	return retVal;
}

int P56file32::writeCellHeaders(FILE *cfilebuf, int cellCount)
{
	int retVal = 0;

	for (int i = 0; i < cellCount; i++)
	{
		fwrite(&cells[i]->Head, (format == _PIC_11 ? CELHEADER11SIZE : CELHEADERPICSIZE), 1, cfilebuf);
	}

	retVal = 1;

	return retVal;

}

int P56file32::writeImages(FILE *cfilebuf, int cellCount)
{
	int retVal = 0;

	PicHeader11 *bPic11 = new PicHeader11;
	PicHeader32 *bPic32 = new PicHeader32;		


	switch (format)
	{
	case _PIC_32:

		palSCI->WritePalette(cfilebuf, false);
		bPic32 = (PicHeader32 *)&Head;

		if (bPic32->celCount)
		{
			unsigned short ttag = PIC32_IMAGE_POS;
			fwrite(&ttag, 2, 1, cfilebuf);
			fwrite(&imageAllSize, 4, 1, cfilebuf);

			for (int i = 0; i < cellCount; i++)
				cells[i]->WriteImage(cfilebuf);
			for (int i = 0; i < cellCount; i++)
				cells[i]->WritePack(cfilebuf);
		}

		if (bPic32->splitFlag)
		{
			unsigned short ttag = PIC32_LINES_POS;
			fwrite(&ttag, 2, 1, cfilebuf);
			unsigned long tzero = 0;
			fwrite(&tzero, 4, 1, cfilebuf);

			for (int i = 0; i < cellCount; i++)
				cells[i]->WriteScanLines(cfilebuf);
		}

		break;

	case _PIC_11:
		bPic11 = (PicHeader11 *)&Head;

		if (cellCount)
		{
			unsigned short ttag = PIC11_IMAGE_POS;
			fwrite(&ttag, 2, 1, cfilebuf);
			fwrite(&imageAllSize, 4, 1, cfilebuf);

			for (int i = 0; i < cellCount; i++)
				cells[i]->WriteImage(cfilebuf);
			for (int i = 0; i < cellCount; i++)
				cells[i]->WritePack(cfilebuf);
		}
		
		palSCI->WritePalette(cfilebuf, false);

		unsigned short ttag = PIC11_VECTOR_POS;
		fwrite(&ttag, 2, 1, cfilebuf);
		fwrite(&bPic11->vectorSize, 4, 1, cfilebuf);
		fwrite(vector, bPic11->vectorSize, 1, cfilebuf);

		break;
	}

	retVal = 1;

	return retVal;
}

bool P56file32::SavePic(HWND hwnd, LPSTR szFileName)
{
	FILE *cfilebuf = fopen(szFileName, "wb");
	if (cfilebuf)
	{
		imageAllSize = 0;
		
		PicHeader32 *bPic32 = new PicHeader32;
		PicHeader11 *bPic11 = new PicHeader11;

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

		loadCellOffset();

		writeFileHeader(cfilebuf);

		writePicHeader(cfilebuf, cellCount);

		writeCellHeaders(cfilebuf, cellCount);

		writeImages(cfilebuf, cellCount);

		fclose(cfilebuf);

		return true;
	}

	return false;
}

