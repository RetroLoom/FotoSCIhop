/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This class represents a SCI Cell from a P56/V56 file
 *
 */
 
#include "StdAfx.h"
#include "scicell.h"
#include "p56files.h"
//#include "palette.h"

// Dhel
#include "v56files.h"

void Cell::makeSCI()
{
    if (!bmImage || !bmInfo) {
        return;
    }
    
    // Clean up existing data
    delete cellImage->image;
    cellImage->image = nullptr;
    
    delete cellImage->pack;
    cellImage->pack = nullptr;
    
    const bool hasLines = (cellImage->lines != nullptr);
    delete cellImage->lines;
    cellImage->lines = nullptr;
    
    // Extract dimensions
    const unsigned short width = static_cast<unsigned short>(bmInfo->bmiHeader.biWidth);
    const unsigned short height = static_cast<unsigned short>(-bmInfo->bmiHeader.biHeight);
    
    // Setup cell base
    CelBase* bCell = new CelBase;
    bCell = reinterpret_cast<CelBase*>(&Head);
    bCell->xDim = width;
    bCell->yDim = height;
    
    const unsigned long totalPixels = width * height;
    cellImage->imageSize = totalPixels;
    cellImage->packSize = 0;
    cellImage->image = new unsigned char[totalPixels];
    
    // Calculate bitmap padding (BMP requires DWORD alignment)
    const unsigned short bmpPadding = (4 - (width % 4)) % 4;
    const unsigned short bmpwidth = width + bmpPadding;
    
    if (!bCell->compressType) {
        // Uncompressed - direct copy with optimized loop
        if (bmpPadding == 0) {
            // No padding - single memcpy for entire image
            memcpy(cellImage->image, bmImage, totalPixels);
        } else {
            // Has padding - copy row by row
            const unsigned char* srcRow = bmImage;
            unsigned char* dstRow = cellImage->image;
            
            for (unsigned short i = 0; i < height; i++) {
                memcpy(dstRow, srcRow, width);
                srcRow += bmpwidth;
                dstRow += width;
            }
        }
    } else {
        // Compressed - RLE encoding
        cellImage->pack = new unsigned char[totalPixels];
        if (hasLines) {
            cellImage->lines = new unsigned char[height << 3]; // height * 8 (bit shift optimization)
        }
        
        unsigned char* ppack = cellImage->pack;
        unsigned char* pimage = cellImage->image;
        unsigned long* ptaglines = nullptr;
        unsigned long* pdatalines = nullptr;
        
        if (hasLines) {
            ptaglines = reinterpret_cast<unsigned long*>(cellImage->lines);
            pdatalines = ptaglines + height;
        }
        
        // Process each scanline
        for (unsigned short i = 0; i < height; i++) {
            unsigned short j = 0;
            const unsigned char* pcached = bmImage + (i * bmpwidth);
            const unsigned char* const rowEnd = pcached + width; // Cache row end
            
            if (hasLines) {
                ptaglines[i] = static_cast<unsigned long>(pimage - cellImage->image);
                pdatalines[i] = static_cast<unsigned long>(ppack - cellImage->pack);
            }
            
            while (j < width) {
                const unsigned short remainingPixels = width - j;
                
                if (remainingPixels > 2) {
                    const unsigned char b1 = pcached[0];
                    const unsigned char b2 = pcached[1];
                    const unsigned char b3 = pcached[2];
                    
                    if ((b1 == b2) || (b1 == 255)) {
                        // Run-length encode identical bytes or transparency
                        const unsigned char runValue = b1;
                        pcached++;
                        j++;
                        unsigned char cont = 1;
                        
                        // Optimized run detection - check 4 bytes at a time when possible
                        while ((j < width) && (cont < 0x3F)) {
                            if (*pcached != runValue) break;
                            
                            // Check if we can process 4 bytes at once
                            if ((j + 3 < width) && (cont <= 0x3C) && 
                                (pcached[0] == runValue) && (pcached[1] == runValue) && 
                                (pcached[2] == runValue) && (pcached[3] == runValue)) {
                                cont += 4;
                                pcached += 4;
                                j += 4;
                            } else {
                                cont++;
                                pcached++;
                                j++;
                            }
                        }
                        
                        if (runValue == 255) {
                            *pimage++ = 0xC0 + cont; // Transparency run
                        } else {
                            *pimage++ = 0x80 + cont; // Color run
                            *ppack++ = runValue;
                        }
                    } else {
                        // Encode literal sequence
                        unsigned char cont = 1;
                        const unsigned char* literalStart = pcached;
                        
                        // Find literal sequence length (max 0x3F = 63)
                        while ((j + cont < width - 2) && (cont < 0x3F)) {
                            const unsigned char next1 = pcached[cont];
                            const unsigned char next2 = pcached[cont + 1];
                            const unsigned char next3 = pcached[cont + 2];
                            
                            if ((next1 == next2) && ((next2 == next3) || (next2 == 255))) {
                                break; // Found start of a run
                            }
                            cont++;
                        }
                        
                        // Handle edge case at end of scanline
                        if ((j + cont == width - 2) && (cont < 0x3E) && (pcached[cont + 1] != pcached[cont])) {
                            cont += 2;
                        }
                        
                        *pimage++ = cont; // Literal count
                        
                        // Copy literal bytes efficiently
                        memcpy(ppack, literalStart, cont);
                        ppack += cont;
                        pcached += cont;
                        j += cont;
                    }
                } else {
                    // Handle remaining 1-2 pixels at end of scanline
                    const unsigned char cont = static_cast<unsigned char>(remainingPixels);
                    const unsigned char b1 = pcached[0];
                    const unsigned char b2 = (cont == 2) ? pcached[1] : b1;
                    
                    if ((b1 == b2) || (cont == 1)) {
                        // Single byte or identical pair
                        if (b1 == 255) {
                            *pimage++ = 0xC0 + cont; // Transparency
                        } else {
                            *pimage++ = 0x80 + cont; // Color run
                            *ppack++ = b1;
                        }
                    } else {
                        // Two different bytes - optimized assignment
                        *pimage++ = cont;
                        *ppack++ = b1;
                        *ppack++ = b2;
                    }
                    
                    pcached += cont;
                    j += cont;
                }
            }
        }
        
        // Update final sizes
        cellImage->packSize = static_cast<unsigned long>(ppack - cellImage->pack);
        cellImage->imageSize = static_cast<unsigned long>(pimage - cellImage->image);
    }
}

long Cell::makeBitmap()
{
	if (bmImage)
		delete bmImage;

	if (bmInfo)
		delete bmInfo;

	unsigned long width;
	unsigned long height;

	CelBase *bCell = new CelBase;
	bCell = (CelBase *)&Head;

	width = bCell->xDim;
	height = bCell->yDim;

	unsigned long imsize = width*height;
	int dwremainder = width%4;
	if (dwremainder)
		imsize+= height*(4-dwremainder); //bmp requires DWORD align for each scanline
	unsigned char *initdata;

	long tsizep = ((sizeof(BITMAPINFO)) + 256*(sizeof(RGBQUAD)));

	BITMAPINFO *binfo = (BITMAPINFO *) new char[tsizep];
	

	binfo->bmiHeader.biBitCount=8;
	binfo->bmiHeader.biClrImportant=256;
	binfo->bmiHeader.biClrUsed=256;//TODO ok?
	binfo->bmiHeader.biCompression=BI_RGB;
	binfo->bmiHeader.biHeight=-height; //so that it will be a top-down DIB
	binfo->bmiHeader.biPlanes=1;
	binfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//???
	binfo->bmiHeader.biSizeImage=imsize;//was 0;
	binfo->bmiHeader.biWidth=width;
	binfo->bmiHeader.biXPelsPerMeter=0; //nu
	binfo->bmiHeader.biYPelsPerMeter=0; //nu

	RGBQUAD tquad;
	for (int i=0; i<256; i++)
	{
		PalEntry *tpal = palette->GetPalEntry(i);
        
		if (tpal != NULL)
		{
			tquad.rgbBlue=tpal->blue;
			tquad.rgbGreen=tpal->green;
			tquad.rgbRed=tpal->red;
			tquad.rgbReserved=0;
		}
		else
		{
			tquad.rgbBlue=0;
			tquad.rgbGreen=0;
			tquad.rgbRed=0;
			tquad.rgbReserved=0;
		}
		binfo->bmiColors[i]= tquad;

	}


	if (!bCell->compressType)
	{
		if (!dwremainder)
			initdata =  cellImage->image;
		else
		{
			initdata = (unsigned char *) (new char[imsize]);
			
			unsigned char *pdata = cellImage->image;

			unsigned char *pinit = initdata;
			for (int i=0; i<height; i++)
			{
				memcpy(pinit, pdata, width);
				pinit += width;
				pdata += width;
				for (int j=0; j<4-dwremainder;j++)
				{
					pinit[0] = 0;
					pinit++;	
				}
			}

		}
	}
	else
	{
		initdata = (unsigned char *) (new char[imsize]);

		unsigned char *ptags = cellImage->image;
		unsigned char *pdata = cellImage->pack;

		unsigned char *pinit = initdata;

        unsigned long *plines = (unsigned long *)cellImage->lines;
	
		//TODO add position verification using scan lines!!
		int i=0;
		do
		{
            
            if (cellImage->lines)
			{
                //HWND thwnd = ((P56file32*)_parent)->MyHWnd();
				if (plines[i] != (unsigned long)(ptags - cellImage->image))
                    MessageBox(hWnd, "Lines tags are wrong!", "Error",
                                MB_OK | MB_ICONEXCLAMATION);
				if (plines[i+height] != (unsigned long)(pdata - cellImage->pack))
                    MessageBox(hWnd, "Lines colors are wrong!", "Error",
                                MB_OK | MB_ICONEXCLAMATION);
		    }
            
			int curwidth=0;
			do
			{
				switch ((*ptags)>>6)
				{
				case 2:	//80
				{
					unsigned char color = *pdata;
					pdata++;
					for (int j=0; j<((*ptags)-0x80); j++)
					{
						pinit[0] = color; 
						pinit++;
						curwidth++;
					}
					break;
				}
				case 3:	//C0
					for (int j=0; j<((*ptags)-0xC0); j++)
					{
						pinit[0] = 255; 
						pinit++;
						curwidth++;
					}
					break;
				default:
					for (int j=0; j<*ptags; j++)
					{
						pinit[0] = *pdata;
						pdata++;
						pinit++;	
						curwidth++;
					}

				}

				ptags++;
			
			} while (curwidth<width);
			//TODO check the line here!!!

			if (dwremainder) //if it can't be divided by 4
				for (int j=0; j<4-dwremainder;j++)
				{
					pinit[0] = 0;
					pinit++;	
				}

			i++;

		} while (i<height);
		
		//HWND thwnd = ((P56file32*)_parent)->MyHWnd();
		if (imsize!=(long)(pinit-initdata))
			MessageBox(hWnd, "The uncompressed length is different than the expected!", "Error",
            MB_OK | MB_ICONEXCLAMATION);
		imsize = imsize-((long)(pinit-initdata));

	}



//if (tbmap)
	bmImage = initdata;
	bmInfo = binfo;

	return imsize;
}

void Cell::loadImageOffset()
{
	
}


void Cell::loadImage( FILE *cfilebuf, unsigned char offset )
{
	CellImage *sciImage = new CellImage;

	sciImage->lines = 0;
	sciImage->pack = 0;
	sciImage->lines = 0;
	sciImage->imageSize = 0;

	CelBase *bCell = new CelBase;
	bCell = (CelBase *)&Head;

	if (!bCell->compressType)
		// timsize = cells[i]->Head.pic.imageandPackSize;
		// NOTE in certain files it is wrong, Sierra's tool writes this only on two bytes,
		// but if the image is too big, it gets cut.
		sciImage->imageSize = bCell->xDim * bCell->yDim;
	else
	{
		sciImage->imageSize = bCell->controlByteCount;
	}

	fseek(cfilebuf, offset + bCell->controlOffset, SEEK_SET);

	sciImage->image = (unsigned char *)new char[sciImage->imageSize];
	fread(sciImage->image, sciImage->imageSize, 1, cfilebuf);

	if (bCell->compressType)
	{
		fseek(cfilebuf, offset + bCell->colorOffset, SEEK_SET);
		sciImage->packSize = bCell->dataByteCount - bCell->controlByteCount;
		sciImage->pack = (unsigned char *)new char[sciImage->packSize];
		fread(sciImage->pack, sciImage->packSize, 1, cfilebuf);

		if (bCell->rowTableOffset)
		{
		fseek(cfilebuf, offset + bCell->rowTableOffset, SEEK_SET);
		sciImage->lineSize = bCell->yDim * 4 * 2;
		sciImage->lines = (unsigned char *)new char[sciImage->lineSize];
		fread(sciImage->lines, sciImage->lineSize, 1, cfilebuf);
		}
	}

	if (!bCell->compressType)
	{

		//_imageSize = chead->imageandPackSize;
		// unreliable!!! it's better to calculate it again
		
		sciImage->imageSize = bCell->xDim * bCell->yDim;

		sciImage->packSize = 0;
	}

	cellImage = sciImage;	
}
	
void Cell::WriteImage(FILE *cfb)
{
	if (cfb)
	{
		fwrite(cellImage->image, cellImage->imageSize, 1, cfb);
		changed = false;
	}
}

void Cell::WritePack(FILE *cfb)
{
	CelBase *bCell = new CelBase;
	bCell = (CelBase *)&Head;
	
	if (cfb)
	{
		if (bCell->compressType)
			fwrite(cellImage->pack, cellImage->packSize, 1, cfb);
	}
}

void Cell::WriteScanLines(FILE *cfb)
{
	CelBase *bCell = new CelBase;
	bCell = (CelBase *)&Head;

	if (cfb)
	{
		if (cellImage->lines)
		{

			fwrite(cellImage->lines, bCell->yDim *4*2, 1, cfb);
		}
	}
}

void Cell::ReadLinks(FILE *cfb)
{
	CelHeaderView *bCell = new CelHeaderView;
	bCell = (CelHeaderView *)&Head;

	if (cfb && bCell->linkTableCount)
	{
		for (int i = 0; i < bCell->linkTableCount; ++i)
		{
			fread(&(linkPoints[i]), sizeof(LinkPoint), 1, cfb);
        }	
	}
}

void Cell::WriteLinks(FILE *cfb)
{
	CelHeaderView *bCell = new CelHeaderView;
	bCell = (CelHeaderView *)&Head;

	if (cfb && bCell->linkTableCount)
	{
		for (int i = 0; i < bCell->linkTableCount; ++i)
				fwrite(&(linkPoints[i]), sizeof(LinkPoint), 1, cfb);
	}
}
