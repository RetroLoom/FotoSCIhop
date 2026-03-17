/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  File I/O functions implementation
 *
 */

#include "stdafx.h"
#include "fileio.h"
#include "FotoSCIhop.h"
#include "display.h"
#include "palette.h"
#include "p56files.h"
#include "v56files.h"
#include "scicell.h"
#include "sciloop.h"
#include "language.h"
#include <cstdio>

// ============================================================================
// STATIC HELPER FUNCTIONS (INTERNAL TO FILEIO MODULE)
// ============================================================================

// Tiny cross-compiler safe copy
static void copy_path(char* dst, const char* src, size_t cap) {
#ifdef _MSC_VER
    strncpy_s(dst, cap, src ? src : "", _TRUNCATE);
#else
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
#endif
}

// Common utility to validate a bitmap file header
bool ValidateBitmapHeader(FILE* file, BITMAPFILEHEADER& fileHeader, BITMAPINFOHEADER& infoHeader) {
    fread(&fileHeader, sizeof(fileHeader), 1, file);
    fread(&infoHeader, sizeof(infoHeader), 1, file);

    if (fileHeader.bfType != 'MB' || infoHeader.biBitCount != 8 || infoHeader.biCompression != BI_RGB)
        return false;

    return true;
}

// ============================================================================
// CORE FILE OPERATIONS
// ============================================================================

BOOL DoFileOpen(HWND hwnd, const char *filename, const char *ext)
{
   OPENFILENAME ofn;
   
   bool proceed = false;

   ZeroMemory(&ofn, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = INTERFACE_OPENFILEFILTER;
   ofn.lpstrFile = szFileName;
   ofn.nMaxFile = MAX_PATH;
   ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
   
   proceed = (GetOpenFileName(&ofn) != 0);

   if(proceed)
   {
	   if (filename)
	   {
		   strcpy(szFileName, filename);
	   }

	  strcpy(szNextFileName, szFileName);
      
	  if (!_stricmp((ext==NULL?szFileName+ofn.nFileExtension:ext), "v56"))
		isPicture=false;
	  else   //default is .p56 when extension in unknown
		isPicture=true; 

	  int result;
	  
	  curCell=0;

	  if (globalPicture)
	  {
		  delete globalPicture;
		  globalPicture=0;
	  }
	  if (globalView)
	  {
		  delete globalView;
		  globalView=0;
	  }

	  if (isPicture)
	  {
			P56file32 *newPicture = new P56file32;
			result = newPicture->LoadFile(hwnd, szFileName);
			if ( result != ID_NOERROR )
			{
				delete newPicture;
				newPicture = 0;

				const char *emsg;
				switch (result)
				{
					case ID_CANTOPENFILE:
						emsg = ERR_CANTLOADFILE;
						break;
					case ID_WRONGHEADER:
						emsg = ERR_WRONGHEADER;
						break;
					case ID_WRONGCELLRECSIZE:
						emsg = ERR_WRONGCELLRECSIZE;
						break;
					case ID_WRONGPALETTELOC:
						emsg = ERR_WRONGPALETTELOC;
						break;
					default:
						emsg = ERR_CANTLOADFILE;
				}
				MessageBox(hwnd, emsg, ERR_TITLE,
							MB_OK | MB_ICONSTOP);
				
			}
			else
			{	
				globalPicture = newPicture;
				ShowCell(0);
            }

	  }
	  else //isView
	  {
			V56file *newView = new V56file;
			result = newView->LoadFile(hwnd, szFileName);
			if (result!=ID_NOERROR)
			{
				delete newView;
				newView = 0;

				const char *emsg = nullptr;
				switch (result)
				{
					case ID_CANTOPENFILE:
						emsg = ERR_CANTLOADFILE;
						break;
					case ID_WRONGHEADER:
						emsg = ERR_WRONGHEADER;
						break;
					case ID_WRONGLOOPRECSIZE:
						emsg = ERR_WRONGLOOPRECSIZE;
						break;
					case ID_WRONGCELLRECSIZE:
						emsg = ERR_WRONGCELLRECSIZE;
						break;
					case ID_WRONGPALETTELOC:
						emsg = ERR_WRONGPALETTELOC;
						break;
					default:
						emsg = ERR_CANTLOADFILE;
				}
			
				MessageBox(hwnd, emsg, ERR_TITLE, MB_OK | MB_ICONSTOP);
			
			}
            else
            {
                globalView = newView;
                ShowLoopCell(0, 0);
            }
      }

	  HMENU menu = GetMenu(hwnd); 
		
	  EnableMenuItem(menu, ID_IMPORTABMP, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, ID_ESPORTABMP, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));	  
	  datasaved = true;
	  EnableMenuItem(menu, ID_FILE_NEXTFILE, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, ID_SALVACOME, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, ID_IMPORTABMP, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, ID_ESPORTABMP, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, IDM_PROPERTIES, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED)); 
	  EnableMenuItem(menu, ID_PRIORITYBARS, (((result==ID_NOERROR)&&(isPicture)) ?((globalPicture)->format == _PIC_11 ?MF_ENABLED:MF_ENABLED):MF_GRAYED));

	  EnableMenuItem(menu, ID_PALETTE, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, ID_COLORI_IMPORTACOLORI, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
 	  EnableMenuItem(menu, ID_COLORI_ESPORTACOLORI, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
   
	  if (result!=ID_NOERROR)
	  {
			EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_GRAYED);
			EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_GRAYED);
			EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);
			EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
	  }

	  InvalidateRgn(hwnd, NULL, true);
	  // Reset scroll and zoom on new file load
	  zScale = 100;
	  picX = isPicture ? 0 : 220;
	  picY = 30;
	  UpdateScrollBars(hwnd);

      char wname[MAX_PATH + 15] = "FotoSCIhop";
      if (result==ID_NOERROR)
	  {
		  GetWindowRect(hWnd, &rc);
		  unsigned long maxstrlen=(rc.right-rc.left)/8 -12;
		  if (maxstrlen<0)
			  maxstrlen = 10;
		  strcat(wname, " - ");
		  if (strlen(szFileName)<maxstrlen)
			strcat(wname, szFileName);
		  else
		  {
			int pos = 2;
			for (int i=strlen(szFileName); i>strlen(szFileName)-maxstrlen+3; i--) 
				if (szFileName[i] == '\\')
					pos = i;
			
			strcat(wname, "...");
			strcat(wname, (char *)(((long)szFileName)+pos));
		  }
      }
      SetWindowText(hwnd, wname);

      // Simple window refresh after loading
      InvalidateRect(hWnd, NULL, FALSE);

      return (result==ID_NOERROR);
   }
   return FALSE;
}

BOOL DoFileSave(HWND hwnd)
{
   if (FILE *tempf = fopen(szFileName, "rb"))
      fclose(tempf);
   else {
      MessageBox(hwnd, ERR_FILEMOVED, ERR_TITLE, MB_OK | MB_ICONSTOP);
      return FALSE;
   }
  
   if(!(isPicture ?globalPicture->SavePic(hwnd, szFileName):globalView->SaveFile(hwnd, szFileName)))
   { 
       MessageBox(hwnd, ERR_CANTSAVECHANGES, ERR_TITLE,
                  MB_OK | MB_ICONSTOP);
       return FALSE;
   } else {
       datasaved = true;
	   HMENU menu = GetMenu(hwnd); 
       EnableMenuItem(menu, ID_SALVA, MF_GRAYED);
   }

   InvalidateRect(hwnd, NULL, true); 

   return TRUE;
}

BOOL DoFileSaveAs(HWND hwnd)
{
   OPENFILENAME ofn;
   char szSaveFileName[MAX_PATH] = "";

   ZeroMemory(&ofn, sizeof(ofn));

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = (isPicture ?INTERFACE_SAVEFILEFILTERP56 :INTERFACE_SAVEFILEFILTERV56);
   ofn.lpstrFile = szSaveFileName;
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = (isPicture ?"p56" :"v56"); 
   ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
   
   if(GetSaveFileName(&ofn))
   {
        if(!(isPicture ?globalPicture->SavePic(hwnd, szSaveFileName):globalView->SaveFile(hwnd, szSaveFileName)))
		{ 
			MessageBox(hwnd, ERR_CANTSAVE, ERR_TITLE,
                  MB_OK | MB_ICONSTOP);
			return FALSE;
		} else {
			datasaved = true;
			HMENU menu = GetMenu(hwnd); 
			EnableMenuItem(menu, ID_SALVA, MF_GRAYED);
		}

		char wname[MAX_PATH + 15] = "FotoSCIhop";
        strcat(wname, " - ");
        strcat(wname, szSaveFileName);
		
		SetWindowText(hwnd, wname); 

       memcpy(szFileName, szSaveFileName, MAX_PATH); 
   }

   InvalidateRect(hwnd, NULL, true);

   return TRUE;
}

BOOL DoNextFile(HWND hwnd)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	char *fname=0;

	int pos = 0;
	char fpath[MAX_PATH];
	for (unsigned int i=0; i<strlen(szNextFileName); i++) 
		if (szNextFileName[i] == '\\')
			pos = i;

	fname = (char *)(((unsigned long) szNextFileName) + pos+1);
	strncpy(fpath, szNextFileName,pos+1);
	fpath[pos+1]=0;

	char searchstr[MAX_PATH];
	sprintf(searchstr, "%s*.?56", fpath); 

	hFind = FindFirstFile(searchstr, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		MessageBox(hwnd, INTERFACE_INVALIDSEARCHHANDLE, INTERFACE_SEARCHTITLE,
                  MB_OK | MB_ICONEXCLAMATION);
		
	} 
	else 
	{
		bool retvalue = true;
		bool passed =false;
		char *extension=0;
		
		if (!_stricmp(szNextFileName, fpath))
			passed = true;
		
		do
		{			
			if (retvalue)
			{
				if (passed)
				{
					extension = (char *)(((unsigned long) FindFileData.cFileName) + strlen(FindFileData.cFileName)-3);
					strcat(fpath, FindFileData.cFileName);				
                    DoFileOpen(hwnd, fpath, extension);
					FindClose(hFind);
                    
					return TRUE;
				}
				if (!_stricmp(FindFileData.cFileName, fname))
					passed = true;
			}
			retvalue = (FindNextFile(hFind, &FindFileData) != 0);
		}
		while (retvalue);
			
		MessageBox(hwnd, INTERFACE_ENDOFFILESSTR, INTERFACE_SEARCHTITLE,
                  MB_OK | MB_ICONINFORMATION);

		strcpy(szNextFileName, fpath);
		
		FindClose(hFind);
	}

	return FALSE;
}

int DoSaveChangesDialog(HWND hwnd)
{
	int btn;

	if (!datasaved)
	{
		btn = MessageBox (hwnd, WARN_UNSAVEDCHANGES, WARN_ATTENTION, 
								MB_APPLMODAL | MB_ICONQUESTION | MB_YESNOCANCEL);
		if (btn == IDYES)
			if (!DoFileSave(hwnd))
				return IDCANCEL;
	}
	else
		btn = IDNO;

	return btn;	
}

// ============================================================================
// BITMAP IMPORT/EXPORT FUNCTIONS
// ============================================================================

bool ExportCurrentCellBMP(const char* filename) {
    if (!filename || !curCell || !(*curCell)) return false;

    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    BITMAPINFO* info = (*curCell)->bmInfo;
    const BITMAPINFOHEADER* hdr = &info->bmiHeader;

    BITMAPFILEHEADER fileHeader = {};
    fileHeader.bfType = 'MB';
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    fileHeader.bfSize = fileHeader.bfOffBits + hdr->biSizeImage;

    fwrite(&fileHeader, sizeof(fileHeader), 1, file);

    BITMAPINFOHEADER localHdr;
    memcpy(&localHdr, hdr, sizeof(BITMAPINFOHEADER));
    localHdr.biHeight = abs(localHdr.biHeight);
    fwrite(&localHdr, sizeof(localHdr), 1, file);
    fwrite(info->bmiColors, sizeof(RGBQUAD), 256, file);

    long rowWidth = hdr->biSizeImage / abs(hdr->biHeight);
    for (int i = abs(hdr->biHeight) - 1; i >= 0; --i)
        fwrite((uint8_t*)(*curCell)->bmImage + i * rowWidth, rowWidth, 1, file);

    fclose(file);
    return true;
}

bool ImportBMPToCurrentCell(const char* filename, bool applyPalette) {
    if (!filename || !curCell || !(*curCell)) return false;

    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    if (!ValidateBitmapHeader(file, fileHeader, infoHeader)) {
        fclose(file);
        return false;
    }

    RGBQUAD palette[256];
    fread(palette, sizeof(RGBQUAD), 256, file);

    unsigned long height = abs(infoHeader.biHeight);
    unsigned long width = infoHeader.biWidth;
    unsigned long rowPadding = (4 - (width % 4)) % 4;
    unsigned long rowSize = width + rowPadding;
    unsigned long imageSize = rowSize * height;

    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    uint8_t* imageData = new uint8_t[imageSize];
    if (infoHeader.biHeight < 0) {
        fread(imageData, imageSize, 1, file);
    } else {
        for (int i = height - 1; i >= 0; --i)
            fread(imageData + i * rowSize, rowSize, 1, file);
    }

    BITMAPINFO* bmpInfo = (BITMAPINFO*)new uint8_t[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    memset(bmpInfo, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    bmpInfo->bmiHeader = infoHeader;
    bmpInfo->bmiHeader.biHeight = -height;
    bmpInfo->bmiHeader.biSizeImage = imageSize;

    if (applyPalette)
        memcpy(bmpInfo->bmiColors, palette, sizeof(palette));
    else
        memcpy(bmpInfo->bmiColors, (*curCell)->bmInfo->bmiColors, sizeof(palette));

    (*curCell)->SetImage(bmpInfo, imageData);
    datasaved = false;

    fclose(file);
    return true;
}

bool ImportPaletteFromBMP(const char* filename, Palette* targetPal) {
    if (!filename || !targetPal) return false;

    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    if (!ValidateBitmapHeader(file, fileHeader, infoHeader)) {
        fclose(file);
        return false;
    }

    RGBQUAD palette[256];
    fread(palette, sizeof(palette), 1, file);

    for (int i = 0; i < 256; ++i) {
        PalEntry entry;
        PalEntry *pe = targetPal->GetPalEntry(i);
        entry.red = palette[i].rgbRed;
        entry.green = palette[i].rgbGreen;
        entry.blue = palette[i].rgbBlue;
        entry.remap = (pe != nullptr) ? pe->remap : 0;
        targetPal->SetPalEntry(entry, i);
    }

    fclose(file);
    return true;
}

// ============================================================================
// UNIFIED IMPORT/EXPORT FUNCTIONS (GUI OR CLI)
// ============================================================================

// GUI or CLI: export current cell BMP.
// If 'path' is null/empty => show Save dialog (GUI). Otherwise, export directly (CLI).
BOOL ExportBitmapUnified(HWND hwnd, const char* path)
{
    char filePath[MAX_PATH] = "";
    if (path && path[0]) {
        copy_path(filePath, path, MAX_PATH);
    } else {
        OPENFILENAME ofn = {0};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hwnd;
        ofn.lpstrFilter  = INTERFACE_BMPFILTER;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrDefExt  = "bmp";
        ofn.Flags        = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (!GetSaveFileName(&ofn)) return FALSE; // cancel
    }

    if (!ExportCurrentCellBMP(filePath)) {
        if (hwnd) MessageBox(hwnd, ERR_CANTEXPORTBMP, ERR_TITLE, MB_OK | MB_ICONSTOP);
        return FALSE;
    }
    return TRUE;
}

// GUI or CLI: import BMP into current cell (applyPalette=true uses file palette).
// If 'path' is null/empty => show Open dialog (GUI). Otherwise, import directly (CLI).
BOOL ImportBitmapUnified(HWND hwnd, const char* path, BOOL applyPalette)
{
    char filePath[MAX_PATH] = "";
    if (path && path[0]) {
        copy_path(filePath, path, MAX_PATH);
    } else {
        OPENFILENAME ofn = {0};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hwnd;
        ofn.lpstrFilter  = INTERFACE_BMPFILTER;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.Flags        = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (!GetOpenFileName(&ofn)) return FALSE; // cancel
    }

    if (!ImportBMPToCurrentCell(filePath, !!applyPalette)) {
        if (hwnd) MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE, MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    datasaved = false;
    if (hwnd) InvalidateRect(hwnd, NULL, TRUE);
    return TRUE;
}

// GUI or CLI: import palette. If BMP path, uses ImportPaletteFromBMP;
// otherwise tries Palette::loadPalette on file.
// If 'path' is null/empty => show Open dialog (GUI). Otherwise, import directly (CLI).
BOOL ImportPaletteUnified(HWND hwnd, const char* path)
{
    char filePath[MAX_PATH] = "";
    if (path && path[0]) {
        copy_path(filePath, path, MAX_PATH);
    } else {
        OPENFILENAME ofn = {0};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hwnd;
        ofn.lpstrFilter  = INTERFACE_PALINFILTER;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.Flags        = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        if (!GetOpenFileName(&ofn)) return FALSE; // cancel
    }

    Palette* pal = isPicture ? globalPicture->palSCI : globalView->palSCI;

    // First try BMP-style palette import
    if (ImportPaletteFromBMP(filePath, pal)) {
        datasaved = false;
        if (hwnd) InvalidateRect(hwnd, NULL, TRUE);
        return TRUE;
    }

    // Otherwise try native palette load
    FILE* f = fopen(filePath, "rb");
    if (!f) {
        if (hwnd) MessageBox(hwnd, ERR_CANTLOADPALETTE, ERR_TITLE, MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    Palette* newPal = new Palette;
    fseek(f, 0, SEEK_END);
    unsigned long sz = (unsigned long)ftell(f);
    fseek(f, 0, SEEK_SET);

    BOOL ok = FALSE;
    if (newPal->loadPalette(f, sz)) {
        if (isPicture) {
            delete globalPicture->palSCI;
            globalPicture->palSCI = newPal;
            pal = globalPicture->palSCI;
        } else {
            delete globalView->palSCI;
            globalView->palSCI = newPal;
            pal = globalView->palSCI;
        }
        ok = TRUE;
    } else {
        delete newPal;
        if (hwnd) MessageBox(hwnd, ERR_CANTLOADPALETTE, ERR_TITLE, MB_OK | MB_ICONSTOP);
    }
    fclose(f);

    if (!ok) return FALSE;

    datasaved = false;
    if (hwnd) InvalidateRect(hwnd, NULL, TRUE);
    return TRUE;
}

// GUI or CLI: export current palette.
// If 'path' is null/empty => show Save dialog (GUI). Otherwise, export directly (CLI).
BOOL ExportPaletteUnified(HWND hwnd, const char* path)
{
    char filePath[MAX_PATH] = "";
    if (path && path[0]) {
        copy_path(filePath, path, MAX_PATH);
    } else {
        OPENFILENAME ofn = {0};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hwnd;
        ofn.lpstrFilter  = INTERFACE_PALFILTER;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrDefExt  = "pal";
        ofn.Flags        = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (!GetSaveFileName(&ofn)) return FALSE; // cancel
    }

    FILE* f = fopen(filePath, "wb");
    if (!f) {
        if (hwnd) MessageBox(hwnd, ERR_CANTEXPORTPALETTE, ERR_TITLE, MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    if (isPicture) globalPicture->palSCI->WritePalette(f, true);
    else           globalView->palSCI->WritePalette(f, true);

    fclose(f);
    return TRUE;
}

// ============================================================================
// CLI FUNCTIONS
// ============================================================================

int cliExport(char* baseName)
{
    if (!baseName) return 0;

    if (globalView) {
        for (int l = 0; l < globalView->Head.view32.loopCount; ++l) {
            Loop* tloop = globalView->loops[l];
            for (int c = 0; c < tloop->Head.numCels; ++c) {
                ShowLoopCell(l, c);
                if (!tloop->Head.flags) {
                    char out[MAX_PATH];
                    sprintf(out, "%s-%d-%d.bmp", baseName, l + 1, c + 1);
                    ExportBitmapUnified(NULL, out);
                }
            }
        }
    }

    if (globalPicture) {
        for (int c = 0; c < globalPicture->CellsCount(); ++c) {
            ShowCell(c);
            char out[MAX_PATH];
            sprintf(out, "%s-%d.bmp", baseName, c + 1);
            ExportBitmapUnified(NULL, out);
        }
    }
    return 1;
}

int cliImport(char* baseName)
{
    if (!baseName) return 0;

    if (globalView) {
        for (int l = 0; l < globalView->Head.view32.loopCount; ++l) {
            Loop* tloop = globalView->loops[l];
            for (int c = 0; c < tloop->Head.numCels; ++c) {
                ShowLoopCell(l, c);
                if (!tloop->Head.flags) {
                    char inPath[MAX_PATH];
                    sprintf(inPath, "%s-%d-%d.bmp", baseName, l + 1, c + 1);
                    ImportPaletteUnified(NULL, inPath);        // keep prior behavior
                    ImportBitmapUnified(NULL, inPath, TRUE);
                }
            }
        }
    }

    if (globalPicture) {
        for (int c = 0; c < globalPicture->CellsCount(); ++c) {
            ShowCell(c);
            char inPath[MAX_PATH];
            sprintf(inPath, "%s-%d.bmp", baseName, c + 1);
            ImportPaletteUnified(NULL, inPath);                // keep prior behavior
            ImportBitmapUnified(NULL, inPath, TRUE);
        }
    }
    return 1;
}

int cliScale(int scaleX, int scaleY)
{
	int retVal = 0;

	if (globalView)
	{
		globalView->Head.view32.resX = (globalView->Head.view32.resX * scaleX) / 100;
		globalView->Head.view32.resY = (globalView->Head.view32.resY * scaleY) / 100;

		for (int j = 0; j < globalView->Head.view32.loopCount; j++)
		{
			Loop *loop = globalView->loops[j];
			for (int i = 0; i < globalView->loops[j]->Head.numCels; i++)
			{
				Cell *cell = loop->cells[i];

				CelHeaderView *bCell = new CelHeaderView;
				bCell = (CelHeaderView*)&cell->Head;
				
				for (int lp = 0; lp < bCell->linkTableCount; lp++)
				{
					globalView->loops[j]->cells[i]->linkPoints[lp].x = (cell->linkPoints[lp].x * scaleX) / 100;
					globalView->loops[j]->cells[i]->linkPoints[lp].y = (cell->linkPoints[lp].y * scaleY) / 100;
				}

				bCell->xHot = (bCell->xHot * scaleX) / 100;
				bCell->yHot = (bCell->yHot * scaleY) / 100;
			}
		}
	}

	if (globalPicture)
	{
		globalPicture->Head.pic32.resX = (globalPicture->Head.pic32.resX * scaleX) / 100;
		globalPicture->Head.pic32.resY = (globalPicture->Head.pic32.resY * scaleY) / 100;

		for (int i = 0; i < globalPicture->CellsCount(); i++)
		{
			CelHeaderPic *bCell = new CelHeaderPic;
			bCell = (CelHeaderPic*)&(*curCell)->Head;

			bCell->xpos = (bCell->xpos * scaleX) / 100; 
			bCell->ypos = (bCell->ypos * scaleY) / 100; 
			bCell->priority = (bCell->priority * scaleY) / 100; 
		}
	}

	retVal = 1;

	return retVal;
}

int cliSetHeader( int vanishX, int viewAngle )
{
	int retVal = 0;

	if (globalView)
	{
		globalView->Head.view32.resX = vanishX;
		globalView->Head.view32.resY = viewAngle;
	}

	if (globalPicture)		
	{
		globalPicture->Head.pic32.resX = vanishX;
		globalPicture->Head.pic32.resY = viewAngle;
	}

	retVal = 1;

	return retVal;
}

// ============================================================================
// CELL/LOOP MANIPULATION FUNCTIONS
// ============================================================================

BOOL DoAddCells(int loop, int base, int amount)
{
   if(!(isPicture ?globalPicture->addCells(base, amount):globalView->addCells(loop, base, amount)))
   { 
       // Error handling could be added here
   } else {
       // Success handling could be added here
   }

   return TRUE;
}

BOOL DoAddLoops(int base, int amount)
{
   if (FILE *tempf = fopen(szFileName, "rb"))
      fclose(tempf);
   else {
      return FALSE;
   }
  
   if(!(isPicture ? 0:globalView->addLoops(base, amount)))
   { 
       return FALSE;
   } else {
       // Success handling
   }

   return TRUE;
}