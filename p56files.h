/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a P56 file 
 *
 */
 
#ifndef P56FILES_H
#define P56FILES_H

#include "list.h"
#include "scicell.h"
#include "palette.h"
#include "language.h"

#pragma pack(1)
/* 0x8181 identifies a P56 file.*/                                  
#define P56PATCH      0x00000101   ///does it exists????
#define P56PATCH80    0x00008181
#define P56PATCHOLD	  0x00008081

union P56HEAD {
	PicHeader32 pic32;
	PicHeader11 pic11;
};


enum PicFormat{
	_PIC_11,
	_PIC_32
};

#pragma pack()

class P56file32
{

public:
	P56file32(void) : vector(0) {}
			
	~P56file32(void);

	unsigned char	 *vector;		//Offset to the EoF character 0xFF

	//these two exist only when cellcount > 0
   	short	_unkShort1;			//usually FFFF
    short	_unkShort2;   		//usually FFFF

	unsigned char CellsCount() const { return format == _PIC_11	? Head.pic11.celCount : Head.pic32.celCount; }
	void CellsCount(unsigned char value) { format == _PIC_11	? Head.pic11.celCount : Head.pic32.celCount = value; }
   
	int LoadFile(HWND hwnd, LPSTR pszFileName);
	int LoadPic32 ( FILE *cfilebuf, unsigned char offset );
	int LoadPic11 ( FILE *cfilebuf, unsigned char offset );

	Palette *palSCI;
	P56HEAD Head;

	Cell *cells[MAX_CELLS];
	
	int format = -1;

	unsigned long imageAllSize = 0;
	
	int loadCellOffset();
	int addCells(int base, int amount);

	int writeFileHeader(FILE *cfilebuf);
	int writePicHeader(FILE *cfilebuf, int cellCount = 0);
	int writeCellHeaders(FILE *cfilebuf, int cellCount = 0);
	int writeImages(FILE *cfilebuf, int cellCount = 0);

	bool SavePic(HWND hwnd, LPSTR szFileName);

	int writePic32Images(FILE* cfilebuf, int cellCount);
    int writePic11Images(FILE* cfilebuf, int cellCount);
};

#endif
