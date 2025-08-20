/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This file defines the entry point for the application, GUI events, etc.
 *
 *  FotoSCIhop is a tool to modify .P56 and .V56 image files from Sierra SCI games
 *
 *  This program is part of the TraduSCI package
 *
 */
 
#include "stdafx.h"
#include "FotoSCIhop.h"
#include "ClutGenerator.h"
#define MAX_LOADSTRING 100
#include "imgui_integration.h"
#include "imgui.h"
#include "fotoscihop_styles.h"
#include "librealmpal.h"
#include <set> 




// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_PATH+20];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
char szFileName[MAX_PATH] = "";
char szNextFileName[MAX_PATH] = "";



// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// ============================================================================
// COMMAND LINE AND CONFIGURATION
// ============================================================================
char *argv[MAX_ARG];
char propstr[10240] = "";
char gAppPath[MAX_PATH];

// Configuration file and settings
char gConfigIni[_MAX_PATH];
int gAppResX = 700;
int gAppResY = 500;
int zScale = 100;
int gPosCells = 0;
int gCliMode = 0;
int gBaseMagnify = 100;
int gCliEnabled = 0;

// ============================================================================
// REFERENCE IMAGE SETTINGS
// ============================================================================
HWND hReferenceDialog;
float gReferenceScaleX = 100;
float gReferenceScaleY = 100;
char gReferenceBM[_MAX_PATH] = "reference.bmp";
int gReferenceXHot = 0;
int gReferenceYHot = 0;
int gReferenceLinkPoint = 0;
int gReferenceLinkPointX = 0;
int gReferenceLinkPointY = 0;
int gReferencePriority = 0;
int gReferenceTransparentIndex = 255;

// ============================================================================
// GLOBAL APPLICATION STATE
// ============================================================================

// Main data objects
P56file32 *globalPicture = NULL;
V56file *globalView = NULL;
bool isPicture = true;

// Current selection state
Cell **curCell = 0;
Loop **curLoop = 0;
int curCellIndex = 0;
int curLoopIndex = 0;

// Application state flags
bool datasaved = true;
bool showpbars = false;

// ============================================================================
// MAGIC WAND AND CLUT GENERATOR STATE
// ============================================================================

// Global state for magic wand tool
bool g_magicWandEnabled = false;
std::set<int> g_usedColorIndices;

// ============================================================================
// DISPLAY AND UI STATE
// ============================================================================

// Display settings
int MagnifyFactor = gBaseMagnify;
int picX = 0;
int picY = 30;
int tableX = 0;

// UI elements and drawing
RECT rc;
HWND hWndTopBar;
HFONT hfDefault;
RGBQUAD skipColor;

// Image import settings
int colorLimit = 255;
int tolerance = 50;

void ShowLoopCell(unsigned char newloop, unsigned char newcell)
{
	// Validate loop index first
	if (!globalView || newloop >= globalView->Head.view32.loopCount) {
		return; // Invalid loop index
	}
	
	// Validate that the loop exists
	if (!globalView->loops[newloop]) {
		return; // Loop is null
	}
	
	// Validate cell index for this specific loop
	if (newcell >= globalView->loops[newloop]->Head.numCels) {
		// If cell index is too high, use the last cell in this loop
		if (globalView->loops[newloop]->Head.numCels > 0) {
			newcell = globalView->loops[newloop]->Head.numCels - 1;
		} else {
			newcell = 0; // Loop has no cells, use 0
		}
	}
	
	curLoopIndex = newloop;
	
	curLoop = &globalView->loops[newloop];
	if (curLoop)
	{		
		// Additional safety check before accessing cells
		if (newcell < globalView->loops[newloop]->Head.numCels && globalView->loops[newloop]->cells[newcell]) {
			curCell = &globalView->loops[newloop]->cells[newcell];
		} else {
			curCell = nullptr; // Set to null if cell doesn't exist
		}
		
		if (curCell || (*curLoop)->Head.flags)
		{					
			if (curCell && !(*curLoop)->Head.flags)
				curCellIndex = newcell;

			HMENU menu = GetMenu(hWnd); 
		
			EnableMenuItem(menu, ID_IMPORTABMP, ((*curLoop)->Head.flags ?MF_GRAYED :MF_ENABLED));
			EnableMenuItem(menu, ID_ESPORTABMP, ((*curLoop)->Head.flags ?MF_GRAYED :MF_ENABLED));
			EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_ENABLED);
			EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_ENABLED);
			if (newloop == globalView->Head.view32.loopCount - 1)
				EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_GRAYED);
		
			if (newloop == 0)
				EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_GRAYED);

			EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
			EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
			if ((newcell == globalView->loops[newloop]->Head.numCels -1) || (globalView->loops[newloop]->Head.numCels==0) )
				EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
		
			if (newcell == 0)
				EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);


			InvalidateRgn(hWnd, NULL, true);
		}
	}
}

void SetMagnify(int value)
{
	MagnifyFactor=value;

	HMENU menu = GetMenu(hWnd);
	CheckMenuItem(menu, ID_INGRANDIMENTO_NORMALE, MF_UNCHECKED);
	CheckMenuItem(menu, ID_INGRANDIMENTO_X2, MF_UNCHECKED);
	CheckMenuItem(menu, ID_INGRANDIMENTO_X3, MF_UNCHECKED);
	CheckMenuItem(menu, ID_INGRANDIMENTO_X4, MF_UNCHECKED);

	long tID;
	switch (value)
	{
		case 1:
			tID = ID_INGRANDIMENTO_NORMALE;
			break;
		case 2:
			tID = ID_INGRANDIMENTO_X2;
			break;
		case 3:
			tID = ID_INGRANDIMENTO_X3;
			break;
		case 4:
			tID = ID_INGRANDIMENTO_X4;
			break;
		default:
			tID = ID_INGRANDIMENTO_NORMALE;

	}

	CheckMenuItem(menu, tID, MF_CHECKED);

	InvalidateRgn(hWnd,NULL,true);

}

void ShowCell(unsigned char newcell)
{
	// Validate that we have a picture loaded
	if (!globalPicture) {
		return;
	}
	
	// Validate cell index
	int totalCells = globalPicture->CellsCount();
	if (newcell >= totalCells) {
		// If cell index is too high, use the last cell
		if (totalCells > 0) {
			newcell = totalCells - 1;
		} else {
			newcell = 0; // No cells, use 0
		}
	}
	
	// Additional bounds check
	if (newcell < 0) {
		newcell = 0;
	}
	
	curCellIndex = newcell;
	
	// Validate that the cell exists before accessing it
	if (newcell < globalPicture->CellsCount() && globalPicture->cells[newcell]) {
		curCell = &globalPicture->cells[curCellIndex];
	} else {
		curCell = nullptr; // Set to null if cell doesn't exist
		return; // Exit early if cell is invalid
	}
	
	if (curCell)
	{	
		HMENU menu = GetMenu(hWnd); 

		EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
		EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
		if (curCellIndex == globalPicture->CellsCount() - 1)
			EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
		
		if (curCellIndex == 0)
			EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

		InvalidateRgn(hWnd, NULL, true);
	}
}

BOOL DoFileOpen(HWND hwnd, const char *filename, const char *ext)
{
   OPENFILENAME ofn;
   
   bool proceed = false;

   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   // szFileName[0] = 0;

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = INTERFACE_OPENFILEFILTER;
   ofn.lpstrFile = szFileName;
   ofn.nMaxFile = MAX_PATH;

   ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
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

				//for (int i=0; i<globalPicture->CellsCount(); i++)
				//{
				//	globalPicture->cells[i]->GetImage(&globalPicture->cells[i]->bmInfo, &globalPicture->cells[i]->bmImage);
				//}
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
				//globalView->loadView(); // Dhel - view object load
				ShowLoopCell(0,0);
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
   
   /* Dhel - removed for CLI. will rather 
   int btn;   

   btn = MessageBox (hwnd, WARN_OVERWRITE, WARN_ATTENTION,
                              MB_APPLMODAL | MB_ICONQUESTION | MB_OKCANCEL);
   if (btn == IDCANCEL)
         return FALSE; 
	*/
  
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

BOOL DoAddCells(int loop, int base, int amount)
{
     
   if(!(isPicture ?globalPicture->addCells(base, amount):globalView->addCells(loop, base, amount)))
   { 
       //MessageBox(hwnd, ERR_CANTSAVECHANGES, ERR_TITLE,
       //           MB_OK | MB_ICONSTOP);
      // return FALSE;
   } else {
       //datasaved = false;
   }

  // InvalidateRect(hwnd, NULL, true); 

   return TRUE;
}

BOOL DoAddLoops(int base, int amount)
{
	
   if (FILE *tempf = fopen(szFileName, "rb"))
      fclose(tempf);
   else {
     // MessageBox(hwnd, ERR_FILEMOVED, ERR_TITLE, MB_OK | MB_ICONSTOP);
      return FALSE;
   }
   
   /* Dhel - removed for CLI. will rather 
   int btn;   

   btn = MessageBox (hwnd, WARN_OVERWRITE, WARN_ATTENTION,
                              MB_APPLMODAL | MB_ICONQUESTION | MB_OKCANCEL);
   if (btn == IDCANCEL)
         return FALSE; 
	*/
  
   if(!(isPicture ? 0:globalView->addLoops(base, amount)))
   { 
       //MessageBox(hwnd, ERR_CANTSAVECHANGES, ERR_TITLE,
       //           MB_OK | MB_ICONSTOP);
       return FALSE;
   } else {
       //datasaved = true;
	   //HMENU menu = GetMenu(hwnd); 
       //EnableMenuItem(menu, ID_SALVA, MF_GRAYED);
   }

   //InvalidateRect(hwnd, NULL, true); 

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


BOOL DoFileSaveAs(HWND hwnd)
{
   OPENFILENAME ofn;
   char szSaveFileName[MAX_PATH] = "";

   ZeroMemory(&ofn, sizeof(ofn));
   //szSaveFileName[0] = 0;

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = (isPicture ?INTERFACE_SAVEFILEFILTERP56 :INTERFACE_SAVEFILEFILTERV56);
   //ofn.nFilterIndex = 2;
   ofn.lpstrFile = szSaveFileName;
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = (isPicture ?"p56" :"v56"); 

   ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
               OFN_OVERWRITEPROMPT;
   
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
       //FIX is this the best solution? by doing this, the source folder is always changed!
   }

   InvalidateRect(hwnd, NULL, true);

   return TRUE;
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

// Common utility to validate a bitmap file header
static bool ValidateBitmapHeader(FILE* file, BITMAPFILEHEADER& fileHeader, BITMAPINFOHEADER& infoHeader) {
    fread(&fileHeader, sizeof(fileHeader), 1, file);
    fread(&infoHeader, sizeof(infoHeader), 1, file);

    if (fileHeader.bfType != 'MB' || infoHeader.biBitCount != 8 || infoHeader.biCompression != BI_RGB)
        return false;

    return true;
}

bool ExportCurrentCellBMP(const char* filename) {
    if (!filename || !curCell || !(*curCell)) return false;

    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    const BITMAPINFO* info = (*curCell)->bmInfo;
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
        ofn.Flags        = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
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
        ofn.Flags        = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
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
        ofn.Flags        = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
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
        ofn.Flags        = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
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
		//globalPicture->MaxWidth(vanishX);
		//globalPicture->MaxHeight(viewAngle);

		globalPicture->Head.pic32.resX = vanishX;
		globalPicture->Head.pic32.resY = viewAngle;
	}

	retVal = 1;

	return retVal;
}

// =============================================================================
// DISPLAY - HELPER FUNCTIONS
// =============================================================================

void ForceImageRefresh() {
    // Force cached image data to be regenerated with new palette
    if (globalView && curCell && (*curCell)) {
        // Clear cached view cell data
        if ((*curCell)->bmImage) {
            delete (*curCell)->bmImage;
            (*curCell)->bmImage = nullptr;
        }
        if ((*curCell)->bmInfo) {
            delete (*curCell)->bmInfo;
            (*curCell)->bmInfo = nullptr;
        }
    }
    
    if (globalPicture) {
        // Clear cached picture cell data
        for (int i = 0; i < globalPicture->CellsCount(); i++) {
            if (globalPicture->cells[i]) {
                if (globalPicture->cells[i]->bmImage) {
                    delete globalPicture->cells[i]->bmImage;
                    globalPicture->cells[i]->bmImage = nullptr;
                }
                if (globalPicture->cells[i]->bmInfo) {
                    delete globalPicture->cells[i]->bmInfo;
                    globalPicture->cells[i]->bmInfo = nullptr;
                }
            }
        }
    }
    
    // Force window repaint
    InvalidateRgn(hWnd, NULL, true);
}

// Fast integer scaling with bounds checking
static int ScaleCoordinate(int value, int magnifyFactor) {
    if (magnifyFactor <= 0) return value; // Safety check
    return (value * magnifyFactor) / 100;
}

// Cached origin calculation to avoid repeated arithmetic
static POINT GetDisplayOrigin() {
    static int lastPicX = -1, lastPicY = -1, lastTableX = -1;
    static POINT cachedOrigin = {0, 0};
    
    // Only recalculate if values have changed
    if (picX != lastPicX || picY != lastPicY || tableX != lastTableX) {
        cachedOrigin.x = UI_LEFT_MARGIN + picX + tableX;
        cachedOrigin.y = UI_TOP_MARGIN + picY;
        lastPicX = picX;
        lastPicY = picY;
        lastTableX = tableX;
    }
    
    return cachedOrigin;
}

// Color conversion for performance
static COLORREF RGBQUADToColorRef(const RGBQUAD& quad) {
    return RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
}

// Safe GDI object deletion with null checking
static void SafeDeleteGDIObject(HGDIOBJ obj) {
    if (obj && obj != GetStockObject(NULL_PEN) && obj != GetStockObject(NULL_BRUSH)) {
        DeleteObject(obj);
    }
}

// Text drawing with consistent formatting
static void DrawTextInRect(HDC hdc, const char* text, int left, int top, int right, int bottom) {
    if (!text || !*text) return; // Early exit for empty strings
    
    RECT textRect = {left, top, right, bottom};
    DrawText(hdc, text, -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
}

// Point drawing helper
static void DrawPoint(HDC hdc, int x, int y, HPEN pen) {
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y);
    SelectObject(hdc, oldPen);
}

// Enhanced bounds checking for arrays
static bool IsValidIndex(int index, int maxSize) {
    return index >= 0 && index < maxSize;
}

// Color interpolation for smooth gradients
static COLORREF InterpolateColor(int current, int total, COLORREF startColor, COLORREF endColor) {
    if (total <= 0) return startColor;
    
    double ratio = (double)current / (double)total;
    int r1 = GetRValue(startColor), g1 = GetGValue(startColor), b1 = GetBValue(startColor);
    int r2 = GetRValue(endColor), g2 = GetGValue(endColor), b2 = GetBValue(endColor);
    
    int r = (int)(r1 + ratio * (r2 - r1));
    int g = (int)(g1 + ratio * (g2 - g1));
    int b = (int)(b1 + ratio * (b2 - b1));
    
    return RGB(r, g, b);
}

// Helper function for skip color information
static void DrawSkipColorInfo(HDC hdc, CelBase* bCell, char* textBuffer) {
    if (!bCell || !textBuffer || !curCell || !(*curCell)) return;
    
    int result = sprintf(textBuffer, INTERFACE_SKIPCOLORSTR, bCell->skip);
    if (result > 0) {
        DrawTextInRect(hdc, textBuffer, 250, 0, 390, UI_INFO_HEIGHT);
    }

    // Draw color swatch with improved error handling
    if ((*curCell)->bmInfo && IsValidIndex(bCell->skip, PALETTE_TOTAL_COLORS)) {
        RGBQUAD skipColorQuad = (*curCell)->bmInfo->bmiColors[bCell->skip];
        HBRUSH colorBrush = CreateSolidBrush(RGB(skipColorQuad.rgbRed, skipColorQuad.rgbGreen, skipColorQuad.rgbBlue));
        HPEN outline = CreatePen(PS_SOLID, 1, COLOR_BLACK);
        
        HPEN oldPen = (HPEN)SelectObject(hdc, outline);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, colorBrush);
        
        Rectangle(hdc, COLOR_SWATCH_LEFT, COLOR_SWATCH_TOP, COLOR_SWATCH_RIGHT, COLOR_SWATCH_BOTTOM);
        
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        
        SafeDeleteGDIObject(colorBrush);
        SafeDeleteGDIObject(outline);
    }
}

// Helper function for changed indicator
static void DrawChangedIndicator(HDC hdc) {
    COLORREF oldTextColor = SetTextColor(hdc, COLOR_RED);
    DrawTextInRect(hdc, INTERFACE_CHANGEDSTR, 480, 0, 530, UI_INFO_HEIGHT);
    SetTextColor(hdc, oldTextColor); // Restore original color
}

// Helper function for palette status indicators
static void DrawPaletteStatusIndicators(HDC hdc, Palette* tpalette) {
    if (!tpalette->palData) {
        DrawTextInRect(hdc, INTERFACE_MISSINGPALETTE, 30, 300, 190, 320);
        return;
    }

    // Missing colors indicator
    HBRUSH cyanBrush = CreateSolidBrush(COLOR_CYAN);
    HPEN redPen = CreatePen(PS_SOLID, 1, COLOR_RED);
    
    RECT indicatorRect = {20, 300, 30, 310};
    FillRect(hdc, &indicatorRect, cyanBrush);

    HPEN oldPen = (HPEN)SelectObject(hdc, redPen);
    MoveToEx(hdc, 19, 299, NULL);
    LineTo(hdc, 31, 311);
    MoveToEx(hdc, 19, 310, NULL);
    LineTo(hdc, 31, 298);
    SelectObject(hdc, oldPen);

    DrawTextInRect(hdc, INTERFACE_MISSINGCOLORSSTR, 40, 295, 190, 315);

    // Locked colors indicator (only for certain palette types)
    if (!tpalette->Head.type) {
        RECT lockRect = {20, 320, 30, 330};
        FillRect(hdc, &lockRect, cyanBrush);

        oldPen = (HPEN)SelectObject(hdc, redPen);
        for (int i = 1; i <= 2; i++) {
            int yLine = lockRect.bottom + i;
            MoveToEx(hdc, lockRect.left, yLine, NULL);
            LineTo(hdc, lockRect.right, yLine);
        }

        SelectObject(hdc, GetStockObject(WHITE_PEN));
        MoveToEx(hdc, lockRect.left, lockRect.bottom, NULL);
        LineTo(hdc, lockRect.right, lockRect.bottom);
        SelectObject(hdc, oldPen);

        DrawTextInRect(hdc, INTERFACE_LOCKEDCOLORSSTR, 40, 316, 190, 336);
    }
    
    SafeDeleteGDIObject(cyanBrush);
    SafeDeleteGDIObject(redPen);
}

RGBQUAD ExtractPaletteIndexFromBM(char *image, int index) {
    // Early validation
    if (!image || index < 0 || index >= PALETTE_TOTAL_COLORS) {
        RGBQUAD defaultColor = {0, 0, 0, 0};
        return defaultColor;
    }

    char bmPath[_MAX_PATH];
    int result = sprintf(bmPath, "%s\\%s", gAppPath ? gAppPath : "", image);
    if (result <= 0 || result >= _MAX_PATH) {
        RGBQUAD errorColor = {0, 0, 0, 0};
        return errorColor;
    }

    static RGBQUAD rgbQuad[PALETTE_TOTAL_COLORS];
    static char lastImagePath[_MAX_PATH] = "";
    
    // Cache optimization - only reload if different image
    if (strcmp(lastImagePath, bmPath) != 0) {
        memset(rgbQuad, 0, sizeof(rgbQuad));
        
        FILE *tempfile = fopen(bmPath, "rb");
        if (tempfile) {
            BITMAPFILEHEADER tfh;
            BITMAPINFOHEADER tbih;
            
            // Read headers with error checking
            if (fread(&tfh, sizeof(BITMAPFILEHEADER), 1, tempfile) == 1 &&
                fread(&tbih, sizeof(BITMAPINFOHEADER), 1, tempfile) == 1) {
                
                // Validate bitmap format
                if (tfh.bfType == 0x4D42) { // "BM" signature
                    fread(rgbQuad, sizeof(RGBQUAD), PALETTE_TOTAL_COLORS, tempfile);
                    strncpy(lastImagePath, bmPath, _MAX_PATH - 1);
                    lastImagePath[_MAX_PATH - 1] = '\0';
                }
            }
            fclose(tempfile);
        }
    }
    
    return rgbQuad[index];
}

void DisplayReferenceImage(HDC hdc) {
    if (!curCell || !(*curCell) || !gReferenceBM) return;
    
    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
    HBITMAP hbm = (HBITMAP)LoadImage(NULL, gReferenceBM, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if (!hbm) return;

    BITMAP bm;
    GetObject(hbm, sizeof(BITMAP), &bm);

    HDC memdc = CreateCompatibleDC(hdc);
    if (!memdc) {
        DeleteObject(hbm);
        return;
    }
    
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memdc, hbm);

    // Calculate scaling with overflow protection
    int baseScaleX = (bm.bmWidth * gReferenceScaleX) / 10000;
    int baseScaleY = (bm.bmHeight * gReferenceScaleY) / 10000;
    
    // Prevent zero or negative scaling
    if (baseScaleX < 1) baseScaleX = 1;
    if (baseScaleY < 1) baseScaleY = 1;
    
    int scaleX = ScaleCoordinate(baseScaleX, MagnifyFactor);
    int scaleY = ScaleCoordinate(baseScaleY, MagnifyFactor);

    int xOrigin = (scaleX >> 1) - ScaleCoordinate(gReferenceXHot, MagnifyFactor);
    int yOrigin = scaleY - ScaleCoordinate(gReferenceYHot, MagnifyFactor);

    int xHot = ScaleCoordinate(bCell->xHot, MagnifyFactor);
    int yHot = ScaleCoordinate(bCell->yHot, MagnifyFactor);

    // Calculate position with bounds checking
    int xPos = gReferenceLinkPointX - xOrigin + xHot;
    int yPos = gReferenceLinkPointY - yOrigin + yHot;

    // Safe link point access
    if (IsValidIndex(gReferenceLinkPoint - 1, MAX_LINK_POINTS) && gReferenceLinkPoint > 0) {
        int linkIndex = gReferenceLinkPoint - 1;
        xPos = ScaleCoordinate((*curCell)->linkPoints[linkIndex].x, MagnifyFactor) - xOrigin + xHot;
        yPos = ScaleCoordinate((*curCell)->linkPoints[linkIndex].y, MagnifyFactor) - yOrigin + yHot;
    }

    POINT origin = GetDisplayOrigin();
    int posX = origin.x + xPos;
    int posY = origin.y + yPos;

    RGBQUAD refSkip = ExtractPaletteIndexFromBM(gReferenceBM, gReferenceTransparentIndex);

    TransparentBlt(hdc, posX, posY, scaleX, scaleY, memdc, 0, 0, bm.bmWidth, bm.bmHeight,
                   RGBQUADToColorRef(refSkip));

    // Cleanup
    SelectObject(memdc, oldBitmap);
    DeleteDC(memdc);
    DeleteObject(hbm);
}

void DisplayImage(HDC hdc, unsigned char *bmImage, BITMAPINFO *bmInfo, int xPos, int yPos) {
    if (!bmImage || !bmInfo) return;

    POINT origin = GetDisplayOrigin();
    
    int scaledX = origin.x + ScaleCoordinate(xPos, MagnifyFactor);
    int scaledY = origin.y + ScaleCoordinate(yPos, MagnifyFactor);

    int bmWidth = bmInfo->bmiHeader.biWidth;
    int bmHeight = bmInfo->bmiHeader.biHeight;

    HBITMAP hbm = CreateCompatibleBitmap(hdc, bmWidth, -bmHeight);
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memdc, hbm);

    SetDIBitsToDevice(memdc, 0, 0, bmWidth, -bmHeight, 0, 0, 0, -bmHeight,
                      bmImage, bmInfo, DIB_RGB_COLORS);

    int scaledWidth = ScaleCoordinate(bmWidth, MagnifyFactor);
    int scaledHeight = ScaleCoordinate(-bmHeight, MagnifyFactor);

    TransparentBlt(hdc, scaledX, scaledY, scaledWidth, scaledHeight, 
                   memdc, 0, 0, bmWidth, -bmHeight, 
                   RGBQUADToColorRef(skipColor));

    // Cleanup
    SelectObject(memdc, oldBitmap);
    DeleteObject(hbm);
    DeleteDC(memdc);
}

void DisplayCell(HDC hdc, int index) {
    if (!globalPicture || index < 0 || index >= globalPicture->CellsCount()) return;

    // Using C-style cast to avoid auto keyword issues
    void* cellPtr = globalPicture->cells[index];
    if (!cellPtr) return;

    // Refresh bitmap data if needed
    if (globalPicture->cells[index]->cellImage->image != globalPicture->cells[index]->bmImage) {
        delete globalPicture->cells[index]->bmImage;
        if (globalPicture->cells[index]->bmInfo) {
            delete globalPicture->cells[index]->bmInfo;
        }
        globalPicture->cells[index]->bmInfo = 0;
        globalPicture->cells[index]->bmImage = 0;
    }

    if (!globalPicture->cells[index]->bmInfo || !globalPicture->cells[index]->bmImage) {
        globalPicture->cells[index]->GetImage(&globalPicture->cells[index]->bmInfo, &globalPicture->cells[index]->bmImage);
    }

    if (!globalPicture->cells[index]->bmInfo) return;

    CelHeaderPic *bCell = (CelHeaderPic *)&globalPicture->cells[index]->Head;
    skipColor = (*curCell)->bmInfo->bmiColors[bCell->skip];

    DisplayImage(hdc, globalPicture->cells[index]->bmImage, globalPicture->cells[index]->bmInfo, bCell->xpos, bCell->ypos);
}

void DisplayCurrentView(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    // Refresh bitmap data if needed
    if ((*curCell)->cellImage->image != (*curCell)->bmImage) {
        delete (*curCell)->bmImage;
        if ((*curCell)->bmInfo) {
            delete (*curCell)->bmInfo;
        }
        (*curCell)->bmInfo = 0;
        (*curCell)->bmImage = 0;
    }

    if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
        (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
    }

    if (!(*curCell)->bmInfo) return;

    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
    skipColor = (*curCell)->bmInfo->bmiColors[bCell->skip];

    DisplayImage(hdc, (*curCell)->bmImage, (*curCell)->bmInfo, bCell->xHot, bCell->yHot);
}

void DisplayLinkPoints(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
    if (bCell->linkTableCount <= 0) return;

    POINT origin = GetDisplayOrigin();
    
    int xHot = ScaleCoordinate(bCell->xHot, MagnifyFactor);
    int yHot = ScaleCoordinate(bCell->yHot, MagnifyFactor);
    int pointSize = ScaleCoordinate(LINK_POINT_BASE_SIZE, MagnifyFactor);

    // Create base pens
    HPEN accentPen = CreatePen(PS_SOLID, pointSize + LINK_POINT_ACCENT_THICKNESS, COLOR_WHITE);
    
    // Calculate and draw last link point with accent
    int lastIndex = (bCell->linkTableCount - 1 < MAX_LINK_POINTS - 1) ? bCell->linkTableCount - 1 : MAX_LINK_POINTS - 1;
    int linkX = ScaleCoordinate((*curCell)->linkPoints[lastIndex].x, MagnifyFactor);
    int linkY = ScaleCoordinate((*curCell)->linkPoints[lastIndex].y, MagnifyFactor);
    int xPos = origin.x + xHot + linkX;
    int yPos = origin.y + yHot + linkY;

    HPEN lastPointPen = CreatePen(PS_SOLID, pointSize, COLOR_RED);
    DrawPoint(hdc, xPos, yPos, accentPen);
    DrawPoint(hdc, xPos, yPos, lastPointPen);

    // Early exit if only one point
    if (bCell->linkTableCount <= 1) {
        SafeDeleteGDIObject(accentPen);
        SafeDeleteGDIObject(lastPointPen);
        return;
    }

    // Set up for line drawing
    HPEN oldPen = (HPEN)SelectObject(hdc, accentPen);

    // Draw trail through all link points with improved color interpolation
    int maxPoints = (bCell->linkTableCount < MAX_LINK_POINTS) ? bCell->linkTableCount : MAX_LINK_POINTS;
    for (int i = 0; i < maxPoints; i++) {
        // Calculate coordinates for current link point
        linkX = ScaleCoordinate((*curCell)->linkPoints[i].x, MagnifyFactor);
        linkY = ScaleCoordinate((*curCell)->linkPoints[i].y, MagnifyFactor);
        xPos = origin.x + xHot + linkX;
        yPos = origin.y + yHot + linkY;

        // Use smooth color interpolation instead of stepped
        COLORREF pointColor = InterpolateColor(i, bCell->linkTableCount - 1, COLOR_RED, RGB(0, 0, 255));
        
        // Create colored pens for this point
        HPEN coloredDottedPen = CreatePen(PS_DOT, DOTTED_LINE_THICKNESS, pointColor);
        HPEN coloredSolidPen = CreatePen(PS_SOLID, pointSize, pointColor);

        // Draw dotted line to current point
        SelectObject(hdc, coloredDottedPen);
        LineTo(hdc, xPos, yPos);

        // Draw accent and colored point
        DrawPoint(hdc, xPos, yPos, accentPen);
        DrawPoint(hdc, xPos, yPos, coloredSolidPen);
        
        // Cleanup colored pens
        SafeDeleteGDIObject(coloredDottedPen);
        SafeDeleteGDIObject(coloredSolidPen);
    }

    SelectObject(hdc, oldPen);
    SafeDeleteGDIObject(accentPen);
    SafeDeleteGDIObject(lastPointPen);
}

void DisplayCurrentPic(HDC hdc) {
    if (!globalPicture) return;

    if (curCellIndex == 0) {
        // Display all cells
        for (int i = 0; i < globalPicture->CellsCount(); i++) {
            DisplayCell(hdc, i);
        }
    } else {
        // Display specific cell
        DisplayCell(hdc, curCellIndex);
    }
}

void DisplayPriorityBars(HDC hdc) {
    if (!globalPicture || !curCell || !(*curCell)) return;

    int xOrigin = UI_PRIORITY_MARGIN + picX + tableX;
    int yOrigin = UI_TOP_MARGIN + picY;

    HPEN redpen = CreatePen(PS_SOLID, ScaleCoordinate(1, MagnifyFactor), COLOR_RED);
    HPEN oldPen = (HPEN)SelectObject(hdc, redpen);

    if (globalPicture->format == _PIC_11) {
        // SCI 1.1 priority lines - validate cell index
        if (!IsValidIndex(curCellIndex, globalPicture->CellsCount())) {
            SelectObject(hdc, oldPen);
            SafeDeleteGDIObject(redpen);
            return;
        }
        
        CelBase *bCell = (CelBase *)&globalPicture->cells[curCellIndex]->Head;
        int xSpan = xOrigin + ScaleCoordinate(bCell->xDim, MagnifyFactor);
        
        for (int i = 0; i < MAX_PRIORITY_LINES; i++) {
            int yPos = yOrigin + ScaleCoordinate(globalPicture->Head.pic11.priLines[i], MagnifyFactor);
            
            MoveToEx(hdc, xOrigin, yPos, NULL);
            LineTo(hdc, xSpan, yPos);
        }
    } else {
        // SCI32 priority lines - optimized loop
        int cellCount = globalPicture->CellsCount();
        
        for (int i = 1; i < cellCount; i++) {
            // Skip if not displaying this cell
            if (curCellIndex != i && curCellIndex != 0) continue;
            
            CelHeaderPic *bCell = (CelHeaderPic *)&globalPicture->cells[i]->Head;

            int xPos = xOrigin + ScaleCoordinate(bCell->xpos, MagnifyFactor);
            int xSpan = xPos + ScaleCoordinate(bCell->xDim, MagnifyFactor);
            
            // Prevent division by zero
            int priorityScale = (zScale > 0) ? zScale : 100;
            int zOffset = bCell->ypos + bCell->yDim - (bCell->priority * priorityScale / 100);
            int zDepth = bCell->ypos + bCell->yDim - zOffset;
            int yPos = yOrigin + ScaleCoordinate(zDepth, MagnifyFactor);
            
            MoveToEx(hdc, xPos, yPos, NULL);
            LineTo(hdc, xSpan, yPos);
        }
    }

    SelectObject(hdc, oldPen);
    SafeDeleteGDIObject(redpen);
}

void DrawPaletteTable(HDC hdc) {
    HPEN redpen = CreatePen(PS_SOLID, 1, COLOR_RED);
    
    Palette *tpalette = (isPicture ? globalPicture->palSCI : globalView->palSCI);
    
    if (!tpalette) {
        DrawTextInRect(hdc, INTERFACE_MISSINGPALETTE, 30, 300, 190, 320);
        SafeDeleteGDIObject(redpen);
        return;
    }

    // Draw palette grid with optimized drawing
    for (int i = 0; i < PALETTE_COLORS_PER_ROW; i++) {
        for (int j = 0; j < PALETTE_COLORS_PER_ROW; j++) {
            int colorIndex = i * PALETTE_COLORS_PER_ROW + j;
            PalEntry *tentry = tpalette->GetPalEntry(colorIndex);
            
            if (!tentry) continue; // Safety check

            // Calculate cell rectangle once
            RECT cellRect = {
                UI_LEFT_MARGIN + (j * PALETTE_CELL_WIDTH), 
                UI_TOP_MARGIN + (i * PALETTE_CELL_HEIGHT), 
                UI_LEFT_MARGIN + (j * PALETTE_CELL_WIDTH) + PALETTE_CELL_DISPLAY_SIZE, 
                UI_TOP_MARGIN + (i * PALETTE_CELL_HEIGHT) + PALETTE_CELL_DISPLAY_SIZE
            };

            // Fill color cell
            HBRUSH tbrush = CreateSolidBrush(RGB(tentry->red, tentry->green, tentry->blue));
            FillRect(hdc, &cellRect, tbrush);
            SafeDeleteGDIObject(tbrush);

            // Draw remap indicator with optimized pen management
            if (tentry->remap == 1) {
                HPEN remapRedPen = CreatePen(PS_SOLID, 1, COLOR_RED);
                HPEN oldPen = (HPEN)SelectObject(hdc, remapRedPen);
                
                // Red indicator lines
                for (int lineOffset = 1; lineOffset <= 2; lineOffset++) {
                    int yLine = cellRect.bottom + lineOffset;
                    MoveToEx(hdc, cellRect.left, yLine, NULL);
                    LineTo(hdc, cellRect.right, yLine);
                }
                
                // White line
                SelectObject(hdc, GetStockObject(WHITE_PEN));
                MoveToEx(hdc, cellRect.left, cellRect.bottom - 1, NULL);
                LineTo(hdc, cellRect.right, cellRect.bottom - 1);
                
                SelectObject(hdc, oldPen);
                SafeDeleteGDIObject(remapRedPen);
            }

            // Draw invalid color indicator
            bool isOutOfRange = (colorIndex < tpalette->Head.startOffset) ||
                               (colorIndex >= tpalette->Head.startOffset + tpalette->Head.nColors);
                               
            if (isOutOfRange) {
                HPEN oldPen = (HPEN)SelectObject(hdc, redpen);
                
                // Draw X pattern with extended bounds for visibility
                MoveToEx(hdc, cellRect.left - 1, cellRect.top - 1, NULL);
                LineTo(hdc, cellRect.right + 1, cellRect.bottom + 1);
                MoveToEx(hdc, cellRect.left - 1, cellRect.bottom, NULL);
                LineTo(hdc, cellRect.right + 1, cellRect.top - 2);
                
                SelectObject(hdc, oldPen);
            }
        }
    }

    // Draw palette status indicators
    DrawPaletteStatusIndicators(hdc, tpalette);
    SafeDeleteGDIObject(redpen);
}

void DrawCellInfo(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    CelBase *bCell = (CelBase *)&(*curCell)->Head;
    
    // Optimized text buffer to reduce sprintf calls
    char textBuffer[128];

    // Draw view-specific information
    if (globalView) {
        // Loop count information
        int result = sprintf(textBuffer, INTERFACE_LOOPSSTR, curLoopIndex + 1, globalView->Head.view32.loopCount);
        if (result > 0) {
            DrawTextInRect(hdc, textBuffer, 25, 0, 125, UI_INFO_HEIGHT);
        }

        if (curLoop && (*curLoop)) {
            if ((*curLoop)->Head.flags) {
                // Mirrored loop information
                result = sprintf(textBuffer, INTERFACE_MIRROREDSTR, (*curLoop)->Head.altLoop + 1);
                if (result > 0) {
                    DrawTextInRect(hdc, textBuffer, 125, 0, 325, UI_INFO_HEIGHT);
                }
            } else {
                // Cell count information
                result = sprintf(textBuffer, INTERFACE_CELLSSTR, curCellIndex + 1, (*curLoop)->Head.numCels);
                if (result > 0) {
                    DrawTextInRect(hdc, textBuffer, 125, 0, 225, UI_INFO_HEIGHT);
                }
            }
        }

        // Skip color info for non-mirrored loops
        if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
            DrawSkipColorInfo(hdc, bCell, textBuffer);
            
            if ((*curCell)->changed) {
                DrawChangedIndicator(hdc);
            }
        }
    }

    // Draw picture-specific information
    if (globalPicture) {
        DrawSkipColorInfo(hdc, bCell, textBuffer);

        // Version information
        const char* versionStr = (globalPicture->format == _PIC_11) ? "SCI1.1" : "SCI32";
        DrawTextInRect(hdc, versionStr, 25, 0, 100, UI_INFO_HEIGHT);

        // Cell count for pictures
        int result = sprintf(textBuffer, INTERFACE_CELLSSTR, curCellIndex + 1, globalPicture->CellsCount());
        if (result > 0) {
            DrawTextInRect(hdc, textBuffer, 125, 0, 250, UI_INFO_HEIGHT);
        }

        if ((*curCell)->changed) {
            DrawChangedIndicator(hdc);
        }
    }
}

void LoadConfig ()
{
	// get ini settings
	sprintf(gConfigIni, "%s\\config.ini", gAppPath);

	gAppResX = GetPrivateProfileInt("main", "resX", gAppResX, gConfigIni);
	gAppResY = GetPrivateProfileInt("main", "resY", gAppResY, gConfigIni);
	zScale = GetPrivateProfileInt("main", "zScale", zScale, gConfigIni);
	gPosCells = GetPrivateProfileInt("main", "posCells", gPosCells, gConfigIni);
	gBaseMagnify = GetPrivateProfileInt("main", "magScale", gBaseMagnify, gConfigIni);
	gCliEnabled = GetPrivateProfileInt("main", "cliStartup", gCliEnabled, gConfigIni);

	// image references
	gReferenceScaleX = GetPrivateProfileInt("reference", "referenceScaleX", gReferenceScaleX, gConfigIni);
	gReferenceScaleY = GetPrivateProfileInt("reference", "referenceScaleY", gReferenceScaleY, gConfigIni);
	GetPrivateProfileString("reference", "referenceBM", gReferenceBM, gReferenceBM, _MAX_PATH, gConfigIni);
	gReferenceXHot = GetPrivateProfileInt("reference", "referenceXHot", gReferenceXHot, gConfigIni);
	gReferenceYHot = GetPrivateProfileInt("reference", "referenceYHot", gReferenceYHot, gConfigIni);
	gReferenceLinkPoint = GetPrivateProfileInt("reference", "referenceLinkPoint", gReferenceLinkPoint, gConfigIni);
	gReferenceLinkPointX = GetPrivateProfileInt("reference", "referenceLinkPointX", gReferenceLinkPointX, gConfigIni);
	gReferenceLinkPointY = GetPrivateProfileInt("reference", "referenceLinkPointY", gReferenceLinkPointY, gConfigIni);
	gReferencePriority = GetPrivateProfileInt("reference", "referencePriority", gReferencePriority, gConfigIni);

	MagnifyFactor = gBaseMagnify;
}

#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecation warnings for legacy functions

typedef BOOL (WINAPI*Func)(HWND, const char*, unsigned char, const char*, char*);
Func ExtractFromVolume;

void ParseAppPath(void)
{
    GetModuleFileName(NULL, gAppPath, MAX_PATH);
    char* lastBackslash = strrchr(gAppPath, '\\');
    if (lastBackslash)
        *lastBackslash = '\0';
}

typedef void (*CliHandler)(int argc, char** argv);

typedef struct {
    const char* name;
    int minArgs;
    CliHandler handler;
    const char* description;
} CliCommand;

// === Command Handlers ===

void HandleExport(int argc, char** argv) {
    if (!ExportCurrentCellBMP(argv[2]))
        fprintf(stderr, "[export] Failed to export to: %s\n", argv[2]);
}

void HandleImport(int argc, char** argv) {
    if (!ImportBMPToCurrentCell(argv[2], true)) {
        fprintf(stderr, "[import] Failed to import BMP: %s\n", argv[2]);
        return;
    }

    if (argc >= 5)
        cliScale(atoi(argv[3]), atoi(argv[4]));

    if (argc >= 7)
        cliSetHeader(atoi(argv[5]), atoi(argv[6]));

    DoFileSave(hWnd);
}

void HandleScale(int argc, char** argv) {
    cliScale(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

void HandleHeader(int argc, char** argv) {
    cliSetHeader(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

void HandleAddCells(int argc, char** argv) {
    DoAddCells(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    DoFileSave(hWnd);
}

void HandleAddLoops(int argc, char** argv) {
    DoAddLoops(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

// === Command Table ===

CliCommand cliCommands[] = {
    { "export",   3, HandleExport,   "Export file to output path" },
    { "import",   3, HandleImport,   "Import BMP with optional scale/header" },
    { "scale",    4, HandleScale,    "Scale then save" },
    { "header",   4, HandleHeader,   "Set header then save" },
    { "addCells", 5, HandleAddCells, "Add animation cells" },
    { "addLoops", 4, HandleAddLoops, "Add animation loops" },
    { NULL, 0, NULL, NULL }
};

bool HandleCliCommands(char* cmdLine)
{
    const int MAX_ARGS = 16;
    char* argv[MAX_ARGS] = {0};
    int argc = 0;

    char* token = strtok(cmdLine, " ");
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc < 1) return false;

    // Parse startup file
    char startupfile[_MAX_PATH] = {0};
    if (argv[0][0] == '"' && argv[0][strlen(argv[0]) - 1] == '"') {
        strncpy(startupfile, argv[0] + 1, strlen(argv[0]) - 2);
        startupfile[strlen(argv[0]) - 2] = '\0';
    } else {
        strncpy(startupfile, argv[0], sizeof(startupfile) - 1);
    }

    if (argc == 1) {
        DoFileOpen(hWnd, startupfile, startupfile + strlen(startupfile) - 3);
        fprintf(stderr, "[CLI] No command given. Opened file only.\n");
        return true;
    }

    DoFileOpen(hWnd, startupfile, startupfile + strlen(startupfile) - 3);

    const char* command = argv[1];
    for (int i = 0; cliCommands[i].name; ++i) {
        if (strcmp(cliCommands[i].name, command) == 0) {
            if (argc < cliCommands[i].minArgs) {
                fprintf(stderr, "[%s] Not enough args (have %d, need %d)\n", command, argc, cliCommands[i].minArgs);
                return true;
            }
            cliCommands[i].handler(argc, argv);
            return true;
        }
    }

    fprintf(stderr, "[CLI Error] Unknown command: %s\n", command);
    return true;
}

void HandleRealmpalFileDialogs() {
    if (g_requestInputDialog) {
        g_requestInputDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Image files (*.png, *.bmp *.jpg)\0*.png;*.bmp;*.jpg\0All files (*.*)\0*.*\0\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalInputFile = fileName;
        }
    }
    
    if (g_requestPaletteDialog) {
        g_requestPaletteDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = INTERFACE_PALINFILTER;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalPaletteFile = fileName;
        }
    }
    
    if (g_requestExtraDialog) {
        g_requestExtraDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = INTERFACE_PALINFILTER;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalExtraFile = fileName;
        }
    }
}

#ifdef __DEVC
int STDCALL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else 
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    MSG msg;
    HACCEL hAccelTable;

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_IMMAGINA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    ParseAppPath();
    LoadConfig();

    if (gCliEnabled && lpCmdLine[0] != '\0')
    {
        if (HandleCliCommands(lpCmdLine))
        {
            return 0;
        }
    }

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_IMMAGINA);

#if defined _M_IX86
    HINSTANCE DLL = LoadLibrary("SCIdump.dll");
    if (!DLL) {
        MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
    } else {
        ExtractFromVolume = (Func)GetProcAddress(DLL, "?ExtractFromVolumeSkel@@YAHPAUHWND__@@PADE11@Z");
        if (!ExtractFromVolume) {
            FreeLibrary(DLL);
            DLL = NULL;
            MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
        }
    }
#endif

    if (lpCmdLine[0] != '\0') {
        char startupfile[_MAX_PATH] = {0};

        if (lpCmdLine[0] == '"') {
            size_t len = strlen(lpCmdLine);
            if (len > 2) {
                strncpy(startupfile, lpCmdLine + 1, len - 2);
                startupfile[len - 2] = '\0';
            }
        } else {
            strncpy(startupfile, lpCmdLine, sizeof(startupfile) - 1);
        }

        size_t len = strlen(startupfile);
        if (len >= 3) {
            DoFileOpen(hWnd, startupfile, startupfile + (len - 3));
        }
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

#if defined _M_IX86
    if (DLL) {
        FreeLibrary(DLL);
    }
#endif

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_IMMAGINA);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = (LPCTSTR)IDC_IMMAGINA;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    // Get the width and height of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Get the width and height of the window
    int windowWidth = gAppResX;
    int windowHeight = gAppResY;

    // Calculate the x and y coordinates to center the window on the screen
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // Create the window
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                       x, y, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

    // If the window couldn't be created, return FALSE
    if (!hWnd)
    {
        return FALSE;
    }

    // Initialize ImGui AFTER window creation
    if (!ImGuiDialogs::Initialize(hWnd))
    {
        MessageBox(hWnd, "Failed to initialize ImGui", "Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    FotoSCIhopStyles::Initialize();

    // Set up dialog callbacks
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PROPERTIES, "Properties", &RenderPropertiesDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_ABOUT, "About FotoSCIhop", &RenderAboutDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR, "CLUT Generator", &RenderClutGeneratorDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_REALMPAL, "Realmpal Converter", &RenderRealmpalDialog);

    SetTimer(hWnd, 1, 16, NULL);

    // Show the window
    ShowWindow(hWnd, nCmdShow);

    // Create a font to use for the window
    hfDefault = CreateFont(16, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE, ANSI_CHARSET, 
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                          VARIABLE_PITCH | FF_SWISS, "Arial");

    // Update the window
    UpdateWindow(hWnd);

    return TRUE;
}

void exit_proc(HWND hwnd)
{
    if (globalPicture) {
        delete globalPicture;
        globalPicture = NULL;  // Prevent double deletion
    }

    if (globalView) {
        delete globalView;
        globalView = NULL;  // Prevent double deletion
    }

    if (hfDefault) {
        DeleteObject(hfDefault);
        hfDefault = NULL;  // Prevent double deletion
    }

    FotoSCIhopStyles::Shutdown();
    ImGuiDialogs::Shutdown();

    // Clean up CLUT generator
    if (g_clutGenerator) {
        g_clutGenerator->Shutdown();
        delete g_clutGenerator;
        g_clutGenerator = nullptr;
    }
    
    DestroyWindow(hwnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;

    // Menu checks
    HMENU menu = GetMenu(hWnd);
    if (menu)  // Safety check
        EnableMenuItem(menu, ID_SALVA, (datasaved == false) ? MF_ENABLED : MF_GRAYED);

    switch (message) 
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_ABOUT);
            break;
            
        case IDM_MANUAL:
            {
                char szAppPath[MAX_PATH];
                GetModuleFileName(NULL, szAppPath, MAX_PATH);
                char* lastBackslash = strrchr(szAppPath, '\\');
                if (lastBackslash) *lastBackslash = '\0';
                ShellExecute(hWnd, "open", MANUAL_PATH, NULL, szAppPath, SW_SHOW);
                break; 
            }
            
        case ID_CARICA:
            if (DoSaveChangesDialog(hWnd) != IDCANCEL) 
                DoFileOpen(hWnd, NULL, NULL);
            break;
            
        case ID_CARICAV56VOL:
            {
                char exFile[MAX_PATH]="";
                if (ExtractFromVolume && ExtractFromVolume(hWnd, NULL, 0x80, "v56", exFile))
                    DoFileOpen(hWnd, exFile, "v56");         
                break;
            }
            
        case ID_CARICAP56VOL:
            {
                char exFile[MAX_PATH]="";
                if (ExtractFromVolume && ExtractFromVolume(hWnd, NULL, 0x81, "p56", exFile))
                    DoFileOpen(hWnd, exFile, "p56");         
                break;
            }
            
        case ID_FILE_NEXTFILE:
            DoNextFile(hWnd);
            RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(200);
            break;
            
        case ID_SALVA:
            DoFileSave(hWnd);
            break;
            
        case ID_SALVACOME:
            DoFileSaveAs(hWnd);
            break;
            
        case ID_IMPORTABMP:
            ImportBitmapUnified(hWnd, NULL, TRUE);
            break;
            
        case ID_ESPORTABMP:
            ExportBitmapUnified(hWnd, NULL);
            break;

        case IDM_CLUTGEN:
            if ((globalView && globalView->palSCI) || (globalPicture && globalPicture->palSCI))
            {
                ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
            }
            else
            {
                MessageBox(hWnd, "Please load a .v56 or .p56 file before using the CLUT Generator.",
                           "No File Loaded", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case IDM_REALMPAL_IMPORT:
            if ((globalView && globalView->palSCI) || (globalPicture && globalPicture->palSCI))
            {
                ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_REALMPAL);
            }
            else
            {
                MessageBox(hWnd, "Please load a .v56 or .p56 file before using the PNG import.",
                           "No File Loaded", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case IDM_PROPERTIES:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_PROPERTIES);
            break;
                
        case ID_PALETTE:
            {
                HMENU menu = GetMenu(hWnd);
                switch (CheckMenuItem(menu, ID_PALETTE, MF_BYCOMMAND))
                {
                case MF_CHECKED:
                    CheckMenuItem(menu, ID_PALETTE, MF_UNCHECKED);
                    tableX = 0;
                    break;

                case MF_UNCHECKED:
                    CheckMenuItem(menu, ID_PALETTE, MF_CHECKED);
                    tableX = 190;
                    break;
                }
                
                InvalidateRgn(hWnd, NULL, true);
                break;
            }
            
        case ID_COLORI_IMPORTACOLORI:
            ImportPaletteUnified(hWnd, NULL);
            break;
            
        case ID_COLORI_ESPORTACOLORI:
            ExportPaletteUnified(hWnd, NULL);
            break;
            
        case ID_INGRANDIMENTO_NORMALE:
            SetMagnify(gBaseMagnify);
            break;
            
        case ID_INGRANDIMENTO_X2:
            SetMagnify(gBaseMagnify * 2);
            break;
            
        case ID_INGRANDIMENTO_X3:
            SetMagnify(gBaseMagnify * 3);
            break;
            
        case ID_INGRANDIMENTO_X4:
            SetMagnify(gBaseMagnify * 4);
            break;
            
        case ID_PRIORITYBARS:
            {
                HMENU menu = GetMenu(hWnd);
                switch (CheckMenuItem(menu, ID_PRIORITYBARS, MF_BYCOMMAND))
                {
                case MF_CHECKED:
                    CheckMenuItem(menu, ID_PRIORITYBARS, MF_UNCHECKED);
                    showpbars = false;
                    break;

                case MF_UNCHECKED:
                    CheckMenuItem(menu, ID_PRIORITYBARS, MF_CHECKED);
                    showpbars = true;
                    break;
                }
                
                InvalidateRgn(hWnd, NULL, true);
                break;
            }
            
        case ID_CICLOPRECEDENTE:
            if (!isPicture)
                ShowLoopCell(curLoopIndex-1, 0);
            break;
            
        case ID_CICLOSUCCESSIVO:
            if (!isPicture)
                ShowLoopCell(curLoopIndex+1, 0);
            break;

        case ID_CELLAPRECEDENTE:
            if (isPicture)
                ShowCell(curCellIndex-1);
            else
            {
                Loop *tloop = globalView->loops[curLoopIndex];
                if (tloop)
                {
                    ShowLoopCell(curLoopIndex, curCellIndex-1);
                }
            }
            break;
            
        case ID_CELLASUCCESSIVA:
            if (isPicture)
                ShowCell(curCellIndex+1);
            else
            {
                Loop *tloop = globalView->loops[curLoopIndex];
                if (tloop)
                {
                    ShowLoopCell(curLoopIndex, curCellIndex+1);
                }
            }
            break;
            
        case IDM_EXIT:
            if (DoSaveChangesDialog(hWnd) != IDCANCEL)   
                exit_proc(hWnd);
            break;
            
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
        
    case WM_PAINT:
        {
            HDC hdc = BeginPaint(hWnd, &ps);
            SelectObject(hdc, hfDefault);
            GetClientRect(hWnd, &rc);
            SetBkMode(hdc, TRANSPARENT);
            GetWindowRect(hWnd, &rc);
            long int twidth = rc.right - rc.left;
            SetRect(&rc, 0, 0, twidth, 20);
            FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

            if (globalView)
                picX = 220;

            if (globalPicture)
                picX = 0;

            // palette will be drawn only if the image exists
            if (tableX > 0)
                DrawPaletteTable(hdc);

            if (globalView && !(*curLoop)->Head.flags)
            {
                if (gReferenceBM && !gReferencePriority)
                    DisplayReferenceImage(hdc);

                DisplayCurrentView(hdc);

                if (gReferenceBM && gReferencePriority)
                    DisplayReferenceImage(hdc);
            
                if ((*curCell)->Head.view.linkTableCount >= 1)
                    DisplayLinkPoints(hdc);
            }

            if (globalPicture)
            {
                DisplayCurrentPic(hdc);

                if (showpbars)
                    DisplayPriorityBars(hdc);
            }

            if (curCell)
                DrawCellInfo(hdc);

            EndPaint(hWnd, &ps);

            break;
        }

    case WM_LBUTTONDOWN:
    {
        // Check if magic wand is enabled first
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            int colorIndex;
            int clientX = LOWORD(lParam);
            int clientY = HIWORD(lParam);
            
            if (SampleColorAtScreenPosition(clientX, clientY, colorIndex)) {
                g_clutGenerator->SetSelectedFromColor(colorIndex);
                
                // Show feedback to user
                char message[256];
                sprintf(message, "FotoSCIhop - Magic Wand: Selected color %d as FROM color", colorIndex);
                SetWindowText(hWnd, message);
                
                // Restore normal title after 3 seconds
                SetTimer(hWnd, 2, 3000, NULL);
                
                // Optional: Also show in console for debugging
                #ifdef _DEBUG
                char debugMsg[128];
                sprintf(debugMsg, "[DEBUG] Magic Wand FROM: Color %d at (%d,%d)\n", colorIndex, clientX, clientY);
                OutputDebugStringA(debugMsg);
                #endif
            } else {
                // Click was outside image area
                SetWindowText(hWnd, "FotoSCIhop - Magic Wand: Click inside the image area");
                SetTimer(hWnd, 2, 2000, NULL);
            }
            return 0; // Consume the message
        }
        // If magic wand not enabled, let default processing handle it
        break;
    }

    case WM_RBUTTONDOWN:
    {
        // Check if magic wand is enabled first
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            int colorIndex;
            int clientX = LOWORD(lParam);
            int clientY = HIWORD(lParam);
            
            if (SampleColorAtScreenPosition(clientX, clientY, colorIndex)) {
                g_clutGenerator->SetSelectedToColor(colorIndex);
                
                // Show feedback to user
                char message[256];
                sprintf(message, "FotoSCIhop - Magic Wand: Selected color %d as TO color", colorIndex);
                SetWindowText(hWnd, message);
                
                // Restore normal title after 3 seconds
                SetTimer(hWnd, 2, 3000, NULL);
                
                // Optional: Also show in console for debugging
                #ifdef _DEBUG
                char debugMsg[128];
                sprintf(debugMsg, "[DEBUG] Magic Wand TO: Color %d at (%d,%d)\n", colorIndex, clientX, clientY);
                OutputDebugStringA(debugMsg);
                #endif
            } else {
                // Click was outside image area
                SetWindowText(hWnd, "FotoSCIhop - Magic Wand: Click inside the image area");
                SetTimer(hWnd, 2, 2000, NULL);
            }
            return 0; // Consume the message
        }
        // If magic wand not enabled, let default processing handle it
        break;
    }


    case WM_TIMER:
    if (wParam == 1) { // ImGui timer
        // Handle file dialogs BEFORE ImGui rendering
        HandleRealmpalFileDialogs();
        
        if (ImGuiDialogs::IsAnyDialogOpen()) {
            ImGuiDialogs::Render();
        }
    }
    else if (wParam == 2) { // Title restore timer
        KillTimer(hWnd, 2);
        
        // Restore normal window title
        char wname[MAX_PATH + 15] = "FotoSCIhop";
        if (strlen(szFileName) > 0) {
            strcat(wname, " - ");
            
            // Extract just the filename from the full path
            char* filename = strrchr(szFileName, '\\');
            if (filename) {
                strcat(wname, filename + 1); // Skip the backslash
            } else {
                strcat(wname, szFileName);
            }
        }
        SetWindowText(hWnd, wname);
    }
    break;

    case WM_SETCURSOR:
    {
        // Only change cursor when magic wand is enabled and mouse is over client area
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            
            // Check if cursor is over the image display area
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            
            if (pt.x >= 0 && pt.x < clientRect.right && pt.y >= 0 && pt.y < clientRect.bottom) {
                SetCursor(LoadCursor(NULL, IDC_CROSS));
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_CLOSE:
        if (DoSaveChangesDialog(hWnd) != IDCANCEL)   
            exit_proc(hWnd);
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma warning(pop)  // Restore warning level