/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a single Cell from a SCI P56/V56 file
 *
 */

#ifndef SCICELL_H
#define SCICELL_H

#include "stdafx.h"
#include "palette.h"

#pragma pack(1)

#define MAX_CELLS 512
#define MAX_LOOPS 512

struct CelBase
{
	UInt16 xDim;
	UInt16 yDim;
	Int16 xHot; //  0
	Int16 yHot; //  0
	uchar skip;
	uchar compressType; //  Uncompressed
	UInt16 dataFlags;
	Int32 dataByteCount;	//  0
	Int32 controlByteCount; //  0
	Int32 paletteOffset;	//  Use later (0)
	Int32 controlOffset;	//  0
	Int32 colorOffset;		//  sizeof(CelHeader)
	Int32 rowTableOffset;	//  0
};

struct CelHeader : public CelBase
{
	UInt16   xRes;
	UInt16	yRes;
	Int32    linkTableOffset;
	UInt16   linkNumber;
};

struct CelHeaderPic : public CelBase
{
	Int16 priority;
	Int16 xpos;
	Int16 ypos;
};

struct CelHeaderView : public CelBase
{
	Int32 linkTableOffset;
	Int16 linkTableCount;
	uchar padding[10];
};

struct LoopHeader
{
	char 		altLoop;
	uchar 	flags;
	uchar 	numCels;
	char		contLoop;
	char		startCel;
	char		endCel;
	uchar 	repeatCount;
	uchar 	stepSize;
	Int32		paletteOffset;
	Int32		celOffset;
};
const int LOOPHEADERSIZE = sizeof(LoopHeader);

const int CELHEADERSIZE = sizeof(CelHeader);
const int CELHEADERPICSIZE = sizeof(CelHeaderPic);
const int CELHEADERVIEWSIZE = sizeof(CelHeaderView);
const int CELHEADER11SIZE = sizeof(CelHeaderView) - 16;


const uchar REPEATC = 0x80;
const uchar REPSKIP = 0x40;
const uchar PIC11CELLRECSIZE = 0x24;

#define CELRECSIZE_NOLINKS 0x24
#define CELRECSIZE_LINKS 0x34

#define PIC11_IMAGE_POS 0x0401
#define PIC11_VECTOR_POS 0x0500;

#define PIC32_IMAGE_POS 0x0400;
#define PIC32_LINES_POS 0x0500;

#define VIEW32_IMAGE_POS 0x0400;
#define VIEW32_LINES_POS 0x0500;
#define VIEW32_LINKS_POS 0x0600;


union CellHeader
{
	CelHeaderPic pic;
	CelHeaderView view;
};

struct LinkPoint
{
	Int16 x;
	Int16 y;
	uchar positionType;
	char priority;
};

struct CellImage
{
	unsigned char *image;
	unsigned long imageSize;
	unsigned char *pack;
	unsigned long packSize;
	unsigned char *lines;
	unsigned long lineSize;
};

struct PicHeader11 {
	UInt16	picHeaderSize; //+2 must be added!!!
    uchar nPriorities;
    uchar priLineCount;
    uchar    celCount;	
    uchar	dummy;			//SCI source: dummy
    UInt16    vanishX; //0x00A0				//constant 160         SCI source: vanishX  ??
    UInt16	 vanishY;	//0xd8f0		//constant -10000                  SCI source: vanishY  ??
    UInt16	viewAngle;	//0x0046				//constant 70          SCI source: viewAngle  ??
    UInt32    vectorSize;	
    UInt32	 vectorOffset;		//Offset to the EoF character 0xFF
  	UInt32	priCelOffset; //UnkLong1;		//00000000                    SCI source: priCelOffset
   	UInt32	controlCelOffset; //UnkLong2;		//00000000                SCI source: controlCelOffset
    UInt32	paletteOffset;	//another size but smaller this time
    UInt32  visualHeaderOffset;		//start of cell rec(44 or 48)         SCI source: visualHeaderOffset
    UInt32	polygonOffset; //UnkLong3;	//00000000                        SCI source: polygonOffset
	UInt16 priLines[14]; //always 14 bars
};
const int PICHEADER11SIZE = sizeof(PicHeader11);

struct PicHeader32
{
	UInt16	picHeaderSize;
	uchar		celCount;
	uchar		splitFlag;
	UInt16	celHeaderSize;
	UInt32	paletteOffset;
	UInt16	resX;	//if Height==0 : 0-320x200, 1-640x480, 2-640x400
	UInt16	resY;
};
const int PICHEADER32SIZE = sizeof(PicHeader32);

struct ViewHeader32
{
	UInt16	viewHeaderSize;
	uchar 	loopCount;
	uchar 	stripView;
	uchar 	splitView;
	uchar 	resolution;	//0-320x200, 1-640x480, 2-640x400 
	UInt16 	celCount;
	UInt32	paletteOffset;
	uchar 	loopHeaderSize;
	uchar 	celHeaderSize;
	UInt16	resX;	//if ResX==0 && ResY==0 - look at ViewSize  
	UInt16	resY;
};
const int VIEW32_HEADER_SIZE = sizeof(ViewHeader32);

struct ViewHeaderLinks : public ViewHeader32
{
	uchar	version;
	uchar	futureExpansion;
};
const int VIEW32_HEADER_LINK_SIZE = sizeof(ViewHeaderLinks);

union ViewHeader
{
	ViewHeader32 view32;
	ViewHeaderLinks view32links;
};



#pragma pack()

class Cell // size 2A
{
public:
	Cell(void) : cellImage(new CellImage), 
				bmInfo(new BITMAPINFO), bmImage(new unsigned char),
				changed(false), palette(0)
	{
		cellImage->image = 0;
		cellImage->imageSize = 0;
		cellImage->pack = 0;
		cellImage->packSize = 0;
		cellImage->lines = 0;
		cellImage->lineSize = 0;

	}

	/*
	// copy constructor
	Cell(Cell& other)
	{
									changed = other.changed;
									palette = other.palette;

									bmInfo = new BITMAPINFO;
									memcpy(bmInfo, other.bmInfo, sizeof(BITMAPINFO));

									int imageSize = other.bmInfo->bmiHeader.biSizeImage;
									bmImage = new unsigned char[imageSize];
									memcpy(bmImage, other.bmImage, imageSize);

									cellImage = new CellImage;
									memcpy(cellImage, other.cellImage, sizeof(CellImage));

									if (cellImage->image)
									{
										cellImage->image = new unsigned char;
										memcpy(cellImage->image, other.cellImage->image, sizeof(other.cellImage->image));
									}

									if (cellImage->pack)
									{
										cellImage->pack = new unsigned char;
										memcpy(cellImage->pack, other.cellImage->pack, sizeof(other.cellImage->pack));
									}

									if (cellImage->lines)
									{
										cellImage->lines = new unsigned char;
										memcpy(cellImage->lines, other.cellImage->lines, sizeof(other.cellImage->lines));
									}

									memcpy(&Head, &other.Head, sizeof(CellHeader));
									//memcpy(linkPoints, other.linkPoints, sizeof(linkPoints));
	}
	*/
	

	~Cell(void) { if (cellImage->image) delete[] cellImage->image; if (cellImage->pack) delete[] cellImage->pack;
					if (cellImage->lines) delete[] cellImage->lines; 
					if (bmImage && (cellImage->image != bmImage)) delete[] bmImage;
					if (bmInfo) delete bmInfo;}

	bool changed;
	//if set to true, the new image is the one referenced by bmImage
	//that was the initial Idea, but now it is suddenly converted

	Palette *palette;

	void GetImage(BITMAPINFO **imhd, unsigned char **im) {  if (bmImage==0) makeBitmap();
															*imhd=bmInfo;
															*im=bmImage; }

	void SetImage(BITMAPINFO *imhd, unsigned char *im)
	{
															bmImage = im;
															bmInfo = imhd;
															changed = true;
															makeSCI();
	}

	long makeBitmap();	
	void makeSCI();

	void setPalette(Palette **value) {
									palette = *value;
									}

	void WriteImage(FILE *cfb);
	void WritePack(FILE *cfb);

	void WriteScanLines(FILE *cfb);

    void ReadLinks(FILE *cfb);
    void WriteLinks(FILE *cfb);

	LinkPoint linkPoints[10];
	void loadImage( FILE *cfilebuf, unsigned char offset );
	void loadImageOffset (void);
				
	CellHeader Head;
	CellImage *cellImage;
	
	unsigned char *bmImage;
	BITMAPINFO *bmInfo;

	void shiftPaletteRange (int startIndex, int endIndex, int offset, RGBQUAD *tctab);

	bool isClone = false;
};

#endif
