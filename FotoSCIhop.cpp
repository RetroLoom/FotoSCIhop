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
// DISPLAY - CONFIGURATION CONSTANTS
// =============================================================================

static const int UI_LEFT_MARGIN = 10;
static const int UI_TOP_MARGIN = 30;
static const int UI_PRIORITY_MARGIN = 5;
static const int UI_INFO_HEIGHT = 20;

static const int PALETTE_COLORS_PER_ROW = 16;
static const int PALETTE_TOTAL_COLORS = 256;
static const int PALETTE_CELL_WIDTH = 11;
static const int PALETTE_CELL_HEIGHT = 16;
static const int PALETTE_CELL_DISPLAY_SIZE = 10;

static const int MAX_PRIORITY_LINES = 14;
static const int MAX_LINK_POINTS = 12;

static const int LINK_POINT_BASE_SIZE = 4;
static const int LINK_POINT_ACCENT_THICKNESS = 2;
static const int DOTTED_LINE_THICKNESS = 1;

static const int COLOR_SWATCH_LEFT = 225;
static const int COLOR_SWATCH_TOP = 2;
static const int COLOR_SWATCH_RIGHT = 245;
static const int COLOR_SWATCH_BOTTOM = 18;

// Color constants for better readability
static const COLORREF COLOR_RED = RGB(255, 0, 0);
static const COLORREF COLOR_WHITE = RGB(255, 255, 255);
static const COLORREF COLOR_BLACK = RGB(0, 0, 0);
static const COLORREF COLOR_CYAN = RGB(0, 255, 255);

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

// ==== ImGui Dialog Callbacks ====
void RenderPropertiesDialog() {
    using namespace ImGuiDialogs;
    
    bool open = true;
    if (!BeginDialog("Properties", &open)) {
        EndDialog();
        return;
    }
    
    // If user clicked the X button or pressed Escape, hide this dialog
    if (!open) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PROPERTIES);
        EndDialog();
        return;
    }

    // Calculate responsive widths
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = availableWidth * 0.22f; // 22% for each button

    // =========================================================================
    // SCROLLABLE CONTENT AREA
    // =========================================================================
    
    // Reserve space for the close button at the bottom
    float reservedHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y;
    float contentHeight = ImGui::GetContentRegionAvail().y - reservedHeight;
    
    // Create scrollable child window for all content
    // This allows the dialog content to scroll when sections are expanded beyond window height
    if (ImGui::BeginChild("PropertiesContent", ImVec2(0, contentHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        
        // Auto-scroll functionality - track which sections were just opened
        // When a section is newly opened, automatically scroll to make it visible
        static bool wasFileInfoOpen = false;
        static bool wasResolutionOpen = false;
        static bool wasLoopPropsOpen = false;
        static bool wasCellPropsOpen = false;
        static bool wasAddRemoveOpen = false;
        static bool wasLinkPointsOpen = false;
        static bool wasRefImageOpen = false;

        // =========================================================================
        // FILE INFO SECTION (just adding colors)
        // =========================================================================
            bool fileInfoOpen = ImGui::CollapsingHeader("File Information", ImGuiTreeNodeFlags_DefaultOpen);
        
        // Auto-scroll when section is newly opened
        if (fileInfoOpen && !wasFileInfoOpen) {
            ImGui::SetScrollHereY(0.0f); // Scroll so this section is at the top
        }
        wasFileInfoOpen = fileInfoOpen;
        
        if (fileInfoOpen) {
        char textBuffer[256];
        
        if (globalView) {
            ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "File Type: View File (.v56)");  // Light blue
            sprintf(textBuffer, "Current Loop: %d / %d", curLoopIndex + 1, globalView->Head.view32.loopCount);
            ImGui::Text("%s", textBuffer);
            
            if (curLoop && (*curLoop)) {
                if ((*curLoop)->Head.flags) {
                    sprintf(textBuffer, "Loop Type: Mirror of Loop %d", (*curLoop)->Head.altLoop + 1);
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", textBuffer);  // Orange
                } else {
                    sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, (*curLoop)->Head.numCels);
                    ImGui::Text("%s", textBuffer);
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loop Type: Normal");  // Green
                }
            }
        } else if (globalPicture) {
            const char* version = (globalPicture->format == _PIC_11) ? "SCI1.1 Picture" : "SCI32 Picture";
            sprintf(textBuffer, "File Type: %s (.p56)", version);
            ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", textBuffer);  // Light blue
            sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, globalPicture->CellsCount());
            ImGui::Text("%s", textBuffer);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No file loaded");  // Red
        }
    }
    
    // =========================================================================
    // RESOLUTION SECTION (keeping original logic, adding color to apply button)
    // =========================================================================
    if (ImGui::CollapsingHeader("Resolution Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // Get current data - same logic as before
        static int resX = 320, resY = 200;
        static bool needsResolutionRefresh = true;
        
        // Refresh data when needed
        if (needsResolutionRefresh) {
            if (globalView) {
                resX = globalView->Head.view32.resX;
                resY = globalView->Head.view32.resY;
            } else if (globalPicture) {
                switch (globalPicture->format) {
                case _PIC_11: {
                    PicHeader11 *bPic11 = (PicHeader11 *)&globalPicture->Head;
                    resX = bPic11->vanishX; 
                    resY = bPic11->viewAngle;
                    break;
                }
                case _PIC_32: {
                    PicHeader32 *bPic32 = (PicHeader32 *)&globalPicture->Head;
                    resX = bPic32->resX; 
                    resY = bPic32->resY;
                    break;
                }
                }
            }
            needsResolutionRefresh = false;
        }

        // Single column layout for resolution
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Width", &resX);
        ImGui::InputInt("Height", &resY);
        
        // Apply resolution button (now with green color)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
        if (ImGui::Button("Apply Resolution")) {
            if (globalView) {
                globalView->Head.view32.resX = resX;
                globalView->Head.view32.resY = resY;
            } else if (globalPicture) {
                switch (globalPicture->format) {
                case _PIC_11: {
                    PicHeader11 *bPic11 = (PicHeader11 *)&globalPicture->Head;
                    bPic11->vanishX = resX;
                    bPic11->viewAngle = resY;
                    break;
                }
                case _PIC_32: {
                    PicHeader32 *bPic32 = (PicHeader32 *)&globalPicture->Head;
                    bPic32->resX = resX;
                    bPic32->resY = resY;
                    break;
                }
                }
            }
            datasaved = false;
            needsResolutionRefresh = true;
            InvalidateRgn(hWnd, NULL, true);
        }
        ImGui::PopStyleColor(3);
    }
    
    // =========================================================================
    // LOOP PROPERTIES SECTION (View files only) - just adding colors
    // =========================================================================
    if (globalView && curLoop && (*curLoop)) {
        if (ImGui::CollapsingHeader("Loop Properties")) {
            
            static int loopMirror = 0, loopBase = 0;
            static int loopContinue = -1, loopStartCell = -1, loopEndCell = -1;
            static int loopRepeat = 255, loopStepSize = 3;
            static bool needsLoopRefresh = true;
            
            // Refresh loop data
            if (needsLoopRefresh) {
                int selLoop = curLoopIndex;
                loopMirror = globalView->loops[selLoop]->Head.flags;
                loopBase = globalView->loops[selLoop]->Head.altLoop;
                
                if (!loopMirror) {
                    loopContinue = globalView->loops[selLoop]->Head.contLoop;
                    loopStartCell = globalView->loops[selLoop]->Head.startCel;
                    loopEndCell = globalView->loops[selLoop]->Head.endCel;
                    loopRepeat = globalView->loops[selLoop]->Head.repeatCount;
                    loopStepSize = globalView->loops[selLoop]->Head.stepSize;
                } else {
                    loopContinue = -1; loopStartCell = -1; loopEndCell = -1;
                    loopRepeat = 255; loopStepSize = 3;
                }
                needsLoopRefresh = false;
            }

            // Single column layout for loop properties
            bool mirror = (loopMirror != 0);
            ImGui::Checkbox("Mirror Loop", &mirror);
            loopMirror = mirror ? 1 : 0;
            
            ImGui::InputInt("Base Loop", &loopBase);
            
            if (!loopMirror) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Animation Settings:");  // Light blue header
                ImGui::InputInt("Continue Loop", &loopContinue);
                ImGui::InputInt("Start Cell", &loopStartCell);
                ImGui::InputInt("End Cell", &loopEndCell);
                ImGui::InputInt("Repeat Count", &loopRepeat);
                ImGui::InputInt("Step Size", &loopStepSize);
            }
            
            // Apply loop properties button (now with green color)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
            if (ImGui::Button("Apply Loop Properties")) {
                int selLoop = curLoopIndex;
                globalView->loops[selLoop]->Head.flags = loopMirror;
                globalView->loops[selLoop]->Head.altLoop = loopBase;
                
                if (!loopMirror) {
                    globalView->loops[selLoop]->Head.contLoop = loopContinue;
                    globalView->loops[selLoop]->Head.startCel = loopStartCell;
                    globalView->loops[selLoop]->Head.endCel = loopEndCell;
                    globalView->loops[selLoop]->Head.repeatCount = loopRepeat;
                    globalView->loops[selLoop]->Head.stepSize = loopStepSize;
                } else {
                    globalView->loops[selLoop]->Head.contLoop = -1;
                    globalView->loops[selLoop]->Head.startCel = -1;
                    globalView->loops[selLoop]->Head.endCel = -1;
                    globalView->loops[selLoop]->Head.repeatCount = 255;
                    globalView->loops[selLoop]->Head.stepSize = 3;
                }
                
                ShowLoopCell(curLoopIndex, curCellIndex);
                datasaved = false;
                needsLoopRefresh = true;
            }
            ImGui::PopStyleColor(3);
        }
    }
    
    // =========================================================================
    // CELL PROPERTIES SECTION WITH AUTO-APPLY (keeping original logic)
    // =========================================================================
    if (curCell && (*curCell)) {
        if (ImGui::CollapsingHeader("Cell Properties")) {
            
            // Current values
            static int cellX = 0, cellY = 0, cellPriority = 0;
            // Original values for reset/cancel
            static int originalCellX = 0, originalCellY = 0, originalCellPriority = 0;
            // Previous values for change detection
            static int prevCellX = 0, prevCellY = 0, prevCellPriority = 0;
            static bool needsCellRefresh = true;
            static bool cellEditingStarted = false;
            static bool cellHasChanges = false;
            
            // Refresh cell data
            if (needsCellRefresh) {
                if (globalView) {
                    if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                        CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                        cellX = originalCellX = prevCellX = bCell->xHot;
                        cellY = originalCellY = prevCellY = bCell->yHot;
                        cellPriority = originalCellPriority = prevCellPriority = 0;
                    } else {
                        cellX = originalCellX = prevCellX = 0; 
                        cellY = originalCellY = prevCellY = 0;
                        cellPriority = originalCellPriority = prevCellPriority = 0;
                    }
                } else if (globalPicture) {
                    CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                    cellX = originalCellX = prevCellX = bCell->xpos; 
                    cellY = originalCellY = prevCellY = bCell->ypos; 
                    cellPriority = originalCellPriority = prevCellPriority = bCell->priority;
                }
                needsCellRefresh = false;
                cellEditingStarted = false;
                cellHasChanges = false;
            }

            // Single column layout for cell properties
            if (globalView) {
                if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Hot Spot:");  // Light blue header
                    
                    if (ImGui::InputInt("X Hot", &cellX)) cellEditingStarted = true;
                    if (ImGui::InputInt("Y Hot", &cellY)) cellEditingStarted = true;
                    
                } else {
                    ImGui::TextDisabled("Cell properties not available for mirror loops");
                }
            } else if (globalPicture) {
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Position:");  // Light blue header
                
                if (ImGui::InputInt("X Position", &cellX)) cellEditingStarted = true;
                if (ImGui::InputInt("Y Position", &cellY)) cellEditingStarted = true;
                if (ImGui::InputInt("Priority", &cellPriority)) cellEditingStarted = true;
            }
            
            // Auto-apply changes when values change
            if (cellEditingStarted && (cellX != prevCellX || cellY != prevCellY || cellPriority != prevCellPriority)) {
                
                // Apply changes immediately
                if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                    bCell->xHot = cellX;
                    bCell->yHot = cellY;
                    ShowLoopCell(curLoopIndex, curCellIndex);
                } else if (globalPicture) {
                    CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                    bCell->xpos = cellX;
                    bCell->ypos = cellY;
                    bCell->priority = cellPriority;
                    ShowCell(curCellIndex);
                }
                
                // Update tracking variables
                prevCellX = cellX;
                prevCellY = cellY;
                prevCellPriority = cellPriority;
                
                // Check if we have changes from original
                cellHasChanges = (cellX != originalCellX || cellY != originalCellY || cellPriority != originalCellPriority);
                
                if (cellHasChanges) {
                    datasaved = false;
                }
            }
            
            // Reset and Cancel buttons (only show if we have changes or are editing)
            if (cellEditingStarted) {
                ImGui::Separator();
                
                // Show changed indicator with colors
                if (cellHasChanges) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* Values have been modified *");  // Orange
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No changes");  // Gray
                }
                
                if (cellHasChanges) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.36f, 0.36f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
                    if (ImGui::Button("Reset to Original")) {  // Red button
                        cellX = originalCellX;
                        cellY = originalCellY;
                        cellPriority = originalCellPriority;
                        
                        // Apply the reset values
                        if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
                            CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                            bCell->xHot = cellX;
                            bCell->yHot = cellY;
                            ShowLoopCell(curLoopIndex, curCellIndex);
                        } else if (globalPicture) {
                            CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
                            bCell->xpos = cellX;
                            bCell->ypos = cellY;
                            bCell->priority = cellPriority;
                            ShowCell(curCellIndex);
                        }
                        
                        prevCellX = cellX;
                        prevCellY = cellY;
                        prevCellPriority = cellPriority;
                        cellHasChanges = false;
                        cellEditingStarted = false;
                    }
                    ImGui::PopStyleColor(3);
                }
                
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
                if (ImGui::Button("Done Editing")) {  // Green button
                    cellEditingStarted = false;
                    cellHasChanges = false;
                    // Keep current values as new originals
                    originalCellX = cellX;
                    originalCellY = cellY;
                    originalCellPriority = cellPriority;
                }
                ImGui::PopStyleColor(3);
            }
        }
    }
    
    // =========================================================================
    // ADD / REMOVE SECTION
    // =========================================================================
    if (FotoSCIhopStyles::BeginManagementSection("Add / Remove"))
    {

        // Determine what we're working with
        bool hasLoops = (globalView != nullptr);
        bool hasCells = (globalView != nullptr || globalPicture != nullptr);

        if (!hasCells)
        {
            FotoSCIhopStyles::ErrorText("No file loaded");
            FotoSCIhopStyles::InfoText("Load a .v56 or .p56 file to begin editing");
            FotoSCIhopStyles::EndSection();
            return;
        }

        // Display current file info
        char fileInfo[256];
        if (globalView)
        {
            sprintf(fileInfo, "View File: %d loops, current loop %d (%d cells)",
                    globalView->Head.view32.loopCount, curLoopIndex + 1,
                    (curLoop && (*curLoop)) ? (*curLoop)->Head.numCels : 0);
        }
        else if (globalPicture)
        {
            sprintf(fileInfo, "Picture File: %d cells, current cell %d",
                    globalPicture->CellsCount(), curCellIndex + 1);
        }
        FotoSCIhopStyles::InfoText(fileInfo);
        ImGui::Separator();

        // =====================================================================
        // QUICK OPERATIONS
        // =====================================================================
        if (ImGui::CollapsingHeader("Quick Operations", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Loop operations (V56 only)
            if (hasLoops)
            {
                FotoSCIhopStyles::HeaderText("Loop Operations:");

                ImGui::BeginGroup();
                if (ImGui::Button("Add Loop", ImVec2(buttonWidth, 0)))
                {
                    if (globalView->addLoop(curLoopIndex))
                    {
                        ShowLoopCell(curLoopIndex, curCellIndex);
                        datasaved = false;
                        FotoSCIhopStyles::SuccessText("Loop added successfully");
                    }
                    else
                    {
                        FotoSCIhopStyles::ErrorText("Failed to add loop");
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Add a new loop after the current loop");
                }

                ImGui::SameLine();
                if (globalView->Head.view32.loopCount > 1)
                {
                    if (FotoSCIhopStyles::RemoveButton("Remove Loop"))
                    {
                        if (globalView->deleteLoop(curLoopIndex))
                        {
                            // Adjust current loop index if needed
                            if (curLoopIndex >= globalView->Head.view32.loopCount && globalView->Head.view32.loopCount > 0)
                            {
                                curLoopIndex = globalView->Head.view32.loopCount - 1;
                            }
                            ShowLoopCell(curLoopIndex, curCellIndex);
                            datasaved = false;
                            FotoSCIhopStyles::SuccessText("Loop removed successfully");
                        }
                        else
                        {
                            FotoSCIhopStyles::ErrorText("Failed to remove loop");
                        }
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Remove the current loop");
                    }
                }
                else
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    ImGui::Button("Remove Loop", ImVec2(buttonWidth, 0));
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Cannot remove the last loop");
                    }
                }
                ImGui::EndGroup();

                ImGui::Separator();
            }

            // Cell operations (both P56 and V56)
            FotoSCIhopStyles::HeaderText("Cell Operations:");

            ImGui::BeginGroup();
            if (ImGui::Button("Add Cell", ImVec2(buttonWidth, 0)))
            {
                bool success = false;
                if (globalView)
                {
                    success = globalView->addCell(curLoopIndex, curCellIndex);
                    if (success)
                        ShowLoopCell(curLoopIndex, curCellIndex);
                }
                else if (globalPicture)
                {
                    success = globalPicture->addCell(curCellIndex);
                    if (success)
                        ShowCell(curCellIndex);
                }
                if (success)
                {
                    datasaved = false;
                    FotoSCIhopStyles::SuccessText("Cell added successfully");
                }
                else
                {
                    FotoSCIhopStyles::ErrorText("Failed to add cell");
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Add a new empty cell after the current cell");
            }

            ImGui::SameLine();
            bool canRemoveCell = false;
            if (globalView && curLoop && (*curLoop))
            {
                canRemoveCell = ((*curLoop)->Head.numCels > 1);
            }
            else if (globalPicture)
            {
                canRemoveCell = (globalPicture->CellsCount() > 1);
            }

            if (canRemoveCell)
            {
                if (FotoSCIhopStyles::RemoveButton("Remove Cell"))
                {
                    bool success = false;

                    if (globalView)
                    {
                        // Store current counts before deletion
                        int oldCellCount = (*curLoop)->Head.numCels;

                        success = globalView->deleteCell(curLoopIndex, curCellIndex);

                        if (success)
                        {
                            int newCellCount = (*curLoop)->Head.numCels;

                            // Handle index adjustment more safely
                            if (newCellCount > 0)
                            {
                                // If we deleted the last cell, move to the new last cell
                                if (curCellIndex >= newCellCount)
                                {
                                    curCellIndex = newCellCount - 1;
                                }
                                // Ensure index is still valid
                                if (curCellIndex < 0)
                                {
                                    curCellIndex = 0;
                                }

                                // Only show if we have a valid cell to show
                                ShowLoopCell(curLoopIndex, curCellIndex);
                            }
                            else
                            {
                                // No cells left - set invalid index and handle accordingly
                                curCellIndex = -1;
                                // Don't call ShowLoopCell - maybe show empty state instead
                                // ShowEmptyLoop(curLoopIndex); // If you have such a function
                            }
                        }
                    }
                    else if (globalPicture)
                    {
                        // Store current count before deletion
                        int oldCellCount = globalPicture->CellsCount();

                        success = globalPicture->deleteCell(curCellIndex);

                        if (success)
                        {
                            int newCellCount = globalPicture->CellsCount();

                            // Handle index adjustment more safely
                            if (newCellCount > 0)
                            {
                                // If we deleted the last cell, move to the new last cell
                                if (curCellIndex >= newCellCount)
                                {
                                    curCellIndex = newCellCount - 1;
                                }
                                // Ensure index is still valid
                                if (curCellIndex < 0)
                                {
                                    curCellIndex = 0;
                                }

                                // Only show if we have a valid cell to show
                                ShowCell(curCellIndex);
                            }
                            else
                            {
                                // No cells left - set invalid index
                                curCellIndex = -1;
                                // Don't call ShowCell - handle empty state
                                // This should never happen due to our "don't delete last cell" check
                            }
                        }
                    }

                    if (success)
                    {
                        datasaved = false;
                        FotoSCIhopStyles::SuccessText("Cell removed successfully");
                    }
                    else
                    {
                        FotoSCIhopStyles::ErrorText("Failed to remove cell");
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Remove the current cell");
                }
            }
            else
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::Button("Remove Cell", ImVec2(buttonWidth, 0));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Cannot remove the last cell");
                }
            }
            ImGui::EndGroup();
        }
        FotoSCIhopStyles::EndSection();
    }
    
    // =========================================================================
    // LINK POINTS SECTION WITH AUTO-APPLY
    // =========================================================================

    // Only show Link Points section for valid view files
    bool canShowLinkPoints = false;
    if (globalView && curCell && (*curCell) && curLoop && (*curLoop)) {
        if (!(*curLoop)->Head.flags) {  // Not a mirrored loop
            canShowLinkPoints = true;
        }
    }
    
    if (canShowLinkPoints) {
        if (ImGui::CollapsingHeader("Link Points")) {
            
            // Current link point data
            static int linkCount = 0;
            static int linkX[10] = {0};
            static int linkY[10] = {0};
            static int linkPri[10] = {0};
            static int linkType[10] = {0};
            
            // Original values for reset/cancel
            static int originalLinkCount = 0;
            static int originalLinkX[10] = {0};
            static int originalLinkY[10] = {0};
            static int originalLinkPri[10] = {0};
            static int originalLinkType[10] = {0};
            
            // Previous values for change detection
            static int prevLinkCount = 0;
            static int prevLinkX[10] = {0};
            static int prevLinkY[10] = {0};
            static int prevLinkPri[10] = {0};
            static int prevLinkType[10] = {0};
            
            static bool linkNeedsRefresh = true;
            static bool linkEditingStarted = false;
            static bool linkHasChanges = false;
            
            // Refresh link points data
            if (linkNeedsRefresh) {
                CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                linkCount = originalLinkCount = prevLinkCount = bCell->linkTableCount;
                
                // Clear all arrays first
                for (int i = 0; i < 10; i++) {
                    linkX[i] = originalLinkX[i] = prevLinkX[i] = 0;
                    linkY[i] = originalLinkY[i] = prevLinkY[i] = 0;
                    linkPri[i] = originalLinkPri[i] = prevLinkPri[i] = 0;
                    linkType[i] = originalLinkType[i] = prevLinkType[i] = 0;
                }
                
                // Fill in the actual link points
                for (int i = 0; i < linkCount && i < 10; i++) {
                    linkX[i] = originalLinkX[i] = prevLinkX[i] = (*curCell)->linkPoints[i].x;
                    linkY[i] = originalLinkY[i] = prevLinkY[i] = (*curCell)->linkPoints[i].y;
                    linkPri[i] = originalLinkPri[i] = prevLinkPri[i] = (*curCell)->linkPoints[i].priority;
                    linkType[i] = originalLinkType[i] = prevLinkType[i] = (*curCell)->linkPoints[i].positionType;
                }
                
                linkNeedsRefresh = false;
                linkEditingStarted = false;
                linkHasChanges = false;
            }
            
            // Link Count control
            int oldLinkCount = linkCount;
            if (ImGui::InputInt("Number of Link Points", &linkCount)) {
                linkEditingStarted = true;
            }
            if (linkCount < 0) linkCount = 0;
            if (linkCount > 10) linkCount = 10;
            
            if (linkCount > 0) {
                ImGui::Separator();
                ImGui::Text("Link Point Coordinates:");
                
                // Show link points in single column layout
                for (int i = 0; i < linkCount; i++) {
                    char headerLabel[32];
                    sprintf(headerLabel, "Link Point %d", i + 1);
                    
                    if (ImGui::CollapsingHeader(headerLabel)) {
                        char label[32];
                        
                        sprintf(label, "X##%d", i);
                        if (ImGui::InputInt(label, &linkX[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Y##%d", i);
                        if (ImGui::InputInt(label, &linkY[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Priority##%d", i);
                        if (ImGui::InputInt(label, &linkPri[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Type##%d", i);
                        if (ImGui::InputInt(label, &linkType[i])) linkEditingStarted = true;
                    }
                }
            }
            
            // Check for changes and auto-apply
            bool valuesChanged = (linkCount != prevLinkCount);
            if (!valuesChanged) {
                for (int i = 0; i < linkCount && i < 10; i++) {
                    if (linkX[i] != prevLinkX[i] || linkY[i] != prevLinkY[i] || 
                        linkPri[i] != prevLinkPri[i] || linkType[i] != prevLinkType[i]) {
                        valuesChanged = true;
                        break;
                    }
                }
            }
            
            if (linkEditingStarted && valuesChanged) {
                // Auto-apply changes
                if (globalView && curCell) {
                    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                    
                    bCell->linkTableCount = linkCount;
                    
                    for (int i = 0; i < bCell->linkTableCount && i < 10; i++) {
                        (*curCell)->linkPoints[i].x = linkX[i];
                        (*curCell)->linkPoints[i].y = linkY[i];
                        (*curCell)->linkPoints[i].priority = linkPri[i];
                        (*curCell)->linkPoints[i].positionType = linkType[i];
                    }
                    
                    ShowLoopCell(curLoopIndex, curCellIndex); // refresh screen
                    datasaved = false;
                }
                
                // Update previous values
                prevLinkCount = linkCount;
                for (int i = 0; i < 10; i++) {
                    prevLinkX[i] = linkX[i];
                    prevLinkY[i] = linkY[i];
                    prevLinkPri[i] = linkPri[i];
                    prevLinkType[i] = linkType[i];
                }
                
                // Check if we have changes from original
                linkHasChanges = (linkCount != originalLinkCount);
                if (!linkHasChanges) {
                    for (int i = 0; i < linkCount && i < 10; i++) {
                        if (linkX[i] != originalLinkX[i] || linkY[i] != originalLinkY[i] || 
                            linkPri[i] != originalLinkPri[i] || linkType[i] != originalLinkType[i]) {
                            linkHasChanges = true;
                            break;
                        }
                    }
                }
            }
            
            // Reset and Cancel buttons (only show if we have changes or are editing)
            if (linkEditingStarted) {
                ImGui::Separator();
                
                // Show changed indicator here to prevent shifting
                if (linkHasChanges) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
                    ImGui::Text("* Link points have been modified *");
                    ImGui::PopStyleVar();
                }
                
                if (linkHasChanges && ImGui::Button("Reset Link Points")) {
                    linkCount = originalLinkCount;
                    for (int i = 0; i < 10; i++) {
                        linkX[i] = originalLinkX[i];
                        linkY[i] = originalLinkY[i];
                        linkPri[i] = originalLinkPri[i];
                        linkType[i] = originalLinkType[i];
                    }
                    
                    // Apply the reset values
                    if (globalView && curCell) {
                        CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
                        bCell->linkTableCount = linkCount;
                        
                        for (int i = 0; i < bCell->linkTableCount && i < 10; i++) {
                            (*curCell)->linkPoints[i].x = linkX[i];
                            (*curCell)->linkPoints[i].y = linkY[i];
                            (*curCell)->linkPoints[i].priority = linkPri[i];
                            (*curCell)->linkPoints[i].positionType = linkType[i];
                        }
                        ShowLoopCell(curLoopIndex, curCellIndex);
                    }
                    
                    // Update tracking
                    prevLinkCount = linkCount;
                    for (int i = 0; i < 10; i++) {
                        prevLinkX[i] = linkX[i];
                        prevLinkY[i] = linkY[i];
                        prevLinkPri[i] = linkPri[i];
                        prevLinkType[i] = linkType[i];
                    }
                    linkHasChanges = false;
                    linkEditingStarted = false;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Done with Link Points")) {
                    linkEditingStarted = false;
                    linkHasChanges = false;
                    // Keep current values as new originals
                    originalLinkCount = linkCount;
                    for (int i = 0; i < 10; i++) {
                        originalLinkX[i] = linkX[i];
                        originalLinkY[i] = linkY[i];
                        originalLinkPri[i] = linkPri[i];
                        originalLinkType[i] = linkType[i];
                    }
                }
            }
        }
    } else {
        // Show grayed out section when link points aren't available
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
        if (ImGui::CollapsingHeader("Link Points (Not Available)")) {
            ImGui::Text("Link points are only available for:");
            ImGui::Text("- View files (.v56)");
            ImGui::Text("- Non-mirrored loops");
            ImGui::Text("- When a loop and cell are selected");
        }
        ImGui::PopStyleVar();
    }

    // =========================================================================
    // REFERENCE IMAGE SECTION WITH AUTO-APPLY
    // =========================================================================
    if (ImGui::CollapsingHeader("Reference Image")) {
        
        // Current reference image data
        static float refScaleX = 100.0f, refScaleY = 100.0f;
        static char refBitmapName[_MAX_PATH] = "";
        static int refXHot = 0, refYHot = 0;
        static int refLinkPoint = 0, refLinkPointX = 0, refLinkPointY = 0;
        static bool refPriority = false;
        
        // Original values for reset/cancel
        static float originalRefScaleX = 100.0f, originalRefScaleY = 100.0f;
        static char originalRefBitmapName[_MAX_PATH] = "";
        static int originalRefXHot = 0, originalRefYHot = 0;
        static int originalRefLinkPoint = 0, originalRefLinkPointX = 0, originalRefLinkPointY = 0;
        static bool originalRefPriority = false;
        
        // Previous values for change detection
        static float prevRefScaleX = 100.0f, prevRefScaleY = 100.0f;
        static char prevRefBitmapName[_MAX_PATH] = "";
        static int prevRefXHot = 0, prevRefYHot = 0;
        static int prevRefLinkPoint = 0, prevRefLinkPointX = 0, prevRefLinkPointY = 0;
        static bool prevRefPriority = false;
        
        static bool refNeedsRefresh = true;
        static bool refEditingStarted = false;
        static bool refHasChanges = false;
        
        // Refresh reference image data
        if (refNeedsRefresh) {
            refScaleX = originalRefScaleX = prevRefScaleX = gReferenceScaleX;
            refScaleY = originalRefScaleY = prevRefScaleY = gReferenceScaleY;
            strncpy(refBitmapName, gReferenceBM, _MAX_PATH - 1);
            strncpy(originalRefBitmapName, gReferenceBM, _MAX_PATH - 1);
            strncpy(prevRefBitmapName, gReferenceBM, _MAX_PATH - 1);
            refBitmapName[_MAX_PATH - 1] = '\0';
            originalRefBitmapName[_MAX_PATH - 1] = '\0';
            prevRefBitmapName[_MAX_PATH - 1] = '\0';
            
            refXHot = originalRefXHot = prevRefXHot = gReferenceXHot;
            refYHot = originalRefYHot = prevRefYHot = gReferenceYHot;
            refLinkPoint = originalRefLinkPoint = prevRefLinkPoint = gReferenceLinkPoint;
            refLinkPointX = originalRefLinkPointX = prevRefLinkPointX = gReferenceLinkPointX;
            refLinkPointY = originalRefLinkPointY = prevRefLinkPointY = gReferenceLinkPointY;
            refPriority = originalRefPriority = prevRefPriority = gReferencePriority;
            
            refNeedsRefresh = false;
            refEditingStarted = false;
            refHasChanges = false;
        }

        // Scale Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Scale Settings:");  // Light blue header
        
        // Limit scale input to 3 digits (like original dialog)
        ImGui::PushItemWidth(120);
        if (ImGui::InputFloat("Scale X (%)", &refScaleX, 0.0f, 0.0f, "%.1f")) refEditingStarted = true;
        if (refScaleX < 0) refScaleX = 0;
        if (refScaleX > 999) refScaleX = 999;
        
        if (ImGui::InputFloat("Scale Y (%)", &refScaleY, 0.0f, 0.0f, "%.1f")) refEditingStarted = true;
        if (refScaleY < 0) refScaleY = 0;
        if (refScaleY > 999) refScaleY = 999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Bitmap File
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Reference Bitmap:");  // Light blue header
        if (ImGui::InputText("Bitmap File", refBitmapName, _MAX_PATH)) refEditingStarted = true;
        
        ImGui::Separator();
        
        // Hot Spot Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Hot Spot:");  // Light blue header
        
        ImGui::PushItemWidth(120);
        // Limit hot spot values to 4 digits (like original dialog)
        if (ImGui::InputInt("X Hot", &refXHot)) refEditingStarted = true;
        if (refXHot < -9999) refXHot = -9999;
        if (refXHot > 9999) refXHot = 9999;
        
        if (ImGui::InputInt("Y Hot", &refYHot)) refEditingStarted = true;
        if (refYHot < -9999) refYHot = -9999;
        if (refYHot > 9999) refYHot = 9999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Link Point Settings
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Link Point:");  // Light blue header
        
        ImGui::PushItemWidth(120);
        // Limit link point to 2 digits (like original dialog)
        if (ImGui::InputInt("Link Point Index", &refLinkPoint)) refEditingStarted = true;
        if (refLinkPoint < 0) refLinkPoint = 0;
        if (refLinkPoint > 99) refLinkPoint = 99;
        
        // Limit link point coordinates to 4 digits (like original dialog)
        if (ImGui::InputInt("Link Point X", &refLinkPointX)) refEditingStarted = true;
        if (refLinkPointX < -9999) refLinkPointX = -9999;
        if (refLinkPointX > 9999) refLinkPointX = 9999;
        
        if (ImGui::InputInt("Link Point Y", &refLinkPointY)) refEditingStarted = true;
        if (refLinkPointY < -9999) refLinkPointY = -9999;
        if (refLinkPointY > 9999) refLinkPointY = 9999;
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        
        // Priority Setting
        if (ImGui::Checkbox("Priority", &refPriority)) refEditingStarted = true;
        
        // Auto-apply changes when values change
        if (refEditingStarted && (
            refScaleX != prevRefScaleX || refScaleY != prevRefScaleY ||
            strcmp(refBitmapName, prevRefBitmapName) != 0 ||
            refXHot != prevRefXHot || refYHot != prevRefYHot ||
            refLinkPoint != prevRefLinkPoint || 
            refLinkPointX != prevRefLinkPointX || refLinkPointY != prevRefLinkPointY ||
            refPriority != prevRefPriority)) {
            
            // Apply changes immediately to global variables
            gReferenceScaleX = refScaleX;
            gReferenceScaleY = refScaleY;
            strncpy(gReferenceBM, refBitmapName, _MAX_PATH - 1);
            gReferenceBM[_MAX_PATH - 1] = '\0';
            gReferenceXHot = refXHot;
            gReferenceYHot = refYHot;
            gReferenceLinkPoint = refLinkPoint;
            gReferenceLinkPointX = refLinkPointX;
            gReferenceLinkPointY = refLinkPointY;
            gReferencePriority = refPriority;
            
            // Write to INI file (same as original dialog)
            char buffer[16];
            WritePrivateProfileStringA("reference", "referenceBM", (LPCSTR)gReferenceBM, gConfigIni);
            
            sprintf(buffer, "%d", gReferenceXHot);
            WritePrivateProfileStringA("reference", "referenceXHot", buffer, gConfigIni);
            
            sprintf(buffer, "%d", gReferenceYHot);
            WritePrivateProfileStringA("reference", "referenceYHot", buffer, gConfigIni);

            sprintf(buffer, "%f", gReferenceScaleX);
            WritePrivateProfileStringA("reference", "referenceScaleX", buffer, gConfigIni);

            sprintf(buffer, "%f", gReferenceScaleY);
            WritePrivateProfileStringA("reference", "referenceScaleY", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferenceLinkPoint);
            WritePrivateProfileStringA("reference", "referenceLinkPoint", buffer, gConfigIni);	

            sprintf(buffer, "%d", gReferenceLinkPointX);
            WritePrivateProfileStringA("reference", "referenceLinkPointX", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferenceLinkPointY);
            WritePrivateProfileStringA("reference", "referenceLinkPointY", buffer, gConfigIni);

            sprintf(buffer, "%d", gReferencePriority);
            WritePrivateProfileStringA("reference", "referencePriority", buffer, gConfigIni);
            
            // Invalidate main window (same as original dialog)
            InvalidateRgn(hWnd, NULL, true);
            
            // Update tracking variables
            prevRefScaleX = refScaleX;
            prevRefScaleY = refScaleY;
            strncpy(prevRefBitmapName, refBitmapName, _MAX_PATH - 1);
            prevRefBitmapName[_MAX_PATH - 1] = '\0';
            prevRefXHot = refXHot;
            prevRefYHot = refYHot;
            prevRefLinkPoint = refLinkPoint;
            prevRefLinkPointX = refLinkPointX;
            prevRefLinkPointY = refLinkPointY;
            prevRefPriority = refPriority;
            
            // Check if we have changes from original
            refHasChanges = (refScaleX != originalRefScaleX || refScaleY != originalRefScaleY ||
                           strcmp(refBitmapName, originalRefBitmapName) != 0 ||
                           refXHot != originalRefXHot || refYHot != originalRefYHot ||
                           refLinkPoint != originalRefLinkPoint || 
                           refLinkPointX != originalRefLinkPointX || refLinkPointY != originalRefLinkPointY ||
                           refPriority != originalRefPriority);
            
            if (refHasChanges) {
                datasaved = false;
            }
        }
        
        // Reset and Cancel buttons (only show if we have changes or are editing)
        if (refEditingStarted) {
            ImGui::Separator();
            
            // Show changed indicator with colors
            if (refHasChanges) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* Reference image settings have been modified *");  // Orange
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No changes");  // Gray
            }
            
            if (refHasChanges) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.36f, 0.36f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
                if (ImGui::Button("Reset to Original")) {  // Red button
                    refScaleX = originalRefScaleX;
                    refScaleY = originalRefScaleY;
                    strncpy(refBitmapName, originalRefBitmapName, _MAX_PATH - 1);
                    refBitmapName[_MAX_PATH - 1] = '\0';
                    refXHot = originalRefXHot;
                    refYHot = originalRefYHot;
                    refLinkPoint = originalRefLinkPoint;
                    refLinkPointX = originalRefLinkPointX;
                    refLinkPointY = originalRefLinkPointY;
                    refPriority = originalRefPriority;
                    
                    // Apply the reset values
                    gReferenceScaleX = refScaleX;
                    gReferenceScaleY = refScaleY;
                    strncpy(gReferenceBM, refBitmapName, _MAX_PATH - 1);
                    gReferenceBM[_MAX_PATH - 1] = '\0';
                    gReferenceXHot = refXHot;
                    gReferenceYHot = refYHot;
                    gReferenceLinkPoint = refLinkPoint;
                    gReferenceLinkPointX = refLinkPointX;
                    gReferenceLinkPointY = refLinkPointY;
                    gReferencePriority = refPriority;
                    
                    // Write reset values to INI
                    char buffer[16];
                    WritePrivateProfileStringA("reference", "referenceBM", (LPCSTR)gReferenceBM, gConfigIni);
                    sprintf(buffer, "%d", gReferenceXHot);
                    WritePrivateProfileStringA("reference", "referenceXHot", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceYHot);
                    WritePrivateProfileStringA("reference", "referenceYHot", buffer, gConfigIni);
                    sprintf(buffer, "%f", gReferenceScaleX);
                    WritePrivateProfileStringA("reference", "referenceScaleX", buffer, gConfigIni);
                    sprintf(buffer, "%f", gReferenceScaleY);
                    WritePrivateProfileStringA("reference", "referenceScaleY", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceLinkPoint);
                    WritePrivateProfileStringA("reference", "referenceLinkPoint", buffer, gConfigIni);	
                    sprintf(buffer, "%d", gReferenceLinkPointX);
                    WritePrivateProfileStringA("reference", "referenceLinkPointX", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferenceLinkPointY);
                    WritePrivateProfileStringA("reference", "referenceLinkPointY", buffer, gConfigIni);
                    sprintf(buffer, "%d", gReferencePriority);
                    WritePrivateProfileStringA("reference", "referencePriority", buffer, gConfigIni);
                    
                    InvalidateRgn(hWnd, NULL, true);
                    
                    // Update tracking
                    prevRefScaleX = refScaleX;
                    prevRefScaleY = refScaleY;
                    strncpy(prevRefBitmapName, refBitmapName, _MAX_PATH - 1);
                    prevRefBitmapName[_MAX_PATH - 1] = '\0';
                    prevRefXHot = refXHot;
                    prevRefYHot = refYHot;
                    prevRefLinkPoint = refLinkPoint;
                    prevRefLinkPointX = refLinkPointX;
                    prevRefLinkPointY = refLinkPointY;
                    prevRefPriority = refPriority;
                    refHasChanges = false;
                    refEditingStarted = false;
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.84f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.56f, 0.16f, 1.0f));
            if (ImGui::Button("Done Editing")) {  // Green button
                refEditingStarted = false;
                refHasChanges = false;
                // Keep current values as new originals
                originalRefScaleX = refScaleX;
                originalRefScaleY = refScaleY;
                strncpy(originalRefBitmapName, refBitmapName, _MAX_PATH - 1);
                originalRefBitmapName[_MAX_PATH - 1] = '\0';
                originalRefXHot = refXHot;
                originalRefYHot = refYHot;
                originalRefLinkPoint = refLinkPoint;
                originalRefLinkPointX = refLinkPointX;
                originalRefLinkPointY = refLinkPointY;
                originalRefPriority = refPriority;
            }
            ImGui::PopStyleColor(3);
        }
    }
    
    } // End scrollable content area
    ImGui::EndChild();
    
    // =========================================================================
    // MAIN BUTTONS (Fixed at bottom, outside scroll area)
    // =========================================================================
    
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.72f, 0.96f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.48f, 0.64f, 1.0f));
    if (ImGui::Button("Close")) {  // Light purple button
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PROPERTIES);
    }
    ImGui::PopStyleColor(3);

    EndDialog();
}

void RenderAboutDialog() {
    using namespace ImGuiDialogs;
    
    bool open = true;
    if (!BeginDialog("About FotoSCIhop", &open)) {
        EndDialog();
        return;
    }
    
    // If user clicked the X button or pressed Escape, hide this dialog
    if (!open) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_ABOUT);
        EndDialog();
        return;
    }

    // Calculate responsive widths
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // APPLICATION INFO SECTION
    // =========================================================================
    
    // Center the main title
    const char* appTitle = "FotoSCIhop";
    float titleWidth = ImGui::CalcItemWidth() * 0.6f; // Estimate title width
    
    // Center alignment helper
    float windowWidth = ImGui::GetWindowSize().x;
    float center = (windowWidth - titleWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    
    // Large title with styling
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
    FotoSCIhopStyles::HeaderText(appTitle);
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // Subtitle - center alignment
    float subtitleWidth = availableWidth * 0.8f;
    center = (windowWidth - subtitleWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    ImGui::Text("Sierra SCI1.1/SCI32 Games Image Editor");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // =========================================================================
    // VERSION AND BUILD INFO
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Version Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Version: 2.0 (ImGui Edition)");
        ImGui::Text("Build Date: " __DATE__ " " __TIME__);
        ImGui::Text("Platform: Windows");
        
        #ifdef _WIN64
        ImGui::Text("Architecture: x64");
        #else
        ImGui::Text("Architecture: x86");
        #endif
        
        #ifdef _DEBUG
        FotoSCIhopStyles::WarningText("Build Type: Debug");
        #else
        FotoSCIhopStyles::SuccessText("Build Type: Release");
        #endif
    }
    
    // =========================================================================
    // COPYRIGHT AND AUTHORS
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Copyright & Credits", ImGuiTreeNodeFlags_DefaultOpen)) {
        FotoSCIhopStyles::HeaderText("Original Authors:");
        ImGui::Text("- Enrico Rolfi 'Endroz' (2004-2021)");
        ImGui::Text("- Daniel Arnold 'Dhel' (2022-2025)");
        
        ImGui::Spacing();
        FotoSCIhopStyles::HeaderText("ImGui Migration:");
        ImGui::Text("- Enhanced with modern ImGui interface");
        
        ImGui::Spacing();
        FotoSCIhopStyles::HeaderText("Copyright:");
        ImGui::Text("Copyright (C) Enrico Rolfi 'Endroz', 2004-2021");
        ImGui::Text("Copyright (C) Daniel Arnold 'Dhel', 2022-2025");
        
        ImGui::Spacing();
        FotoSCIhopStyles::InfoText("Part of the TraduSCI package");
    }
    
    // =========================================================================
    // DESCRIPTION
    // =========================================================================
    
    if (ImGui::CollapsingHeader("About This Tool")) {
        ImGui::TextWrapped("FotoSCIhop is a specialized tool for modifying .P56 and .V56 image files from Sierra SCI games. "
                          "It supports both SCI1.1 and SCI32 formats, allowing game modders and translators to edit "
                          "graphics, animations, and color palettes used in classic adventure games.");
        
        ImGui::Spacing();
        
        FotoSCIhopStyles::HeaderText("Supported File Types:");
        ImGui::BulletText(".P56 files - Picture resources (SCI1.1 and SCI32)");
        ImGui::BulletText(".V56 files - View/Animation resources");
        
        ImGui::Spacing();
        
        FotoSCIhopStyles::HeaderText("Key Features:");
        ImGui::BulletText("Import/Export BMP images");
        ImGui::BulletText("Edit color palettes");
        ImGui::BulletText("Modify animation loops and cells");
        ImGui::BulletText("Adjust link points and hot spots");
        ImGui::BulletText("Priority bar visualization");
        ImGui::BulletText("Reference image overlay support");
    }
    
    // =========================================================================
    // SYSTEM INFO (Optional)
    // =========================================================================
    
    if (ImGui::CollapsingHeader("System Information")) {
        char systemInfo[256];
        
        // Get Windows version info
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
        #pragma warning(push)
        #pragma warning(disable: 4996) // Disable deprecation warning for GetVersionEx
        if (GetVersionEx(&osvi)) {
            sprintf(systemInfo, "OS: Windows %d.%d (Build %d)", 
                   osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            ImGui::Text("%s", systemInfo);
        }
        #pragma warning(pop)
        
        // Memory info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            sprintf(systemInfo, "Total RAM: %.1f GB", (float)memInfo.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f));
            ImGui::Text("%s", systemInfo);
            
            sprintf(systemInfo, "Available RAM: %.1f GB", (float)memInfo.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f));
            ImGui::Text("%s", systemInfo);
        }
        
        // Current working directory
        char currentDir[MAX_PATH];
        if (GetCurrentDirectory(MAX_PATH, currentDir)) {
            ImGui::Text("Working Directory:");
            FotoSCIhopStyles::DisabledText(currentDir);
        }
    }
    
    // =========================================================================
    // THIRD PARTY ACKNOWLEDGMENTS
    // =========================================================================
    
    if (ImGui::CollapsingHeader("Third Party Libraries")) {
        FotoSCIhopStyles::HeaderText("This application uses:");
        
        ImGui::BulletText("Dear ImGui - Immediate Mode GUI");
        FotoSCIhopStyles::DisabledText("   https://github.com/ocornut/imgui");
        
        ImGui::BulletText("OpenGL - Graphics rendering");
        ImGui::BulletText("Windows GDI+ - Image processing");
        
        ImGui::Spacing();
        FotoSCIhopStyles::InfoText("Special thanks to the Sierra game preservation community!");
    }
    
    // =========================================================================
    // MAIN BUTTONS
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Center the close button
    float buttonWidth = 120.0f;
    center = (windowWidth - buttonWidth) * 0.5f;
    if (center > 0) {
        ImGui::SetCursorPosX(center);
    }
    
    if (FotoSCIhopStyles::CloseButton("Close")) {
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_ABOUT);
    }
    
    EndDialog();
}

bool SampleColorAtScreenPosition(int clientX, int clientY, int& colorIndex) {
    if (!g_clutGenerator || !g_clutGenerator->IsActive()) return false;
    
    // Use the same display origin calculation as the original display code
    int displayOriginX = UI_LEFT_MARGIN + picX + tableX;
    int displayOriginY = UI_TOP_MARGIN + picY;
    
    // Calculate relative position within the display area
    int relativeX = clientX - displayOriginX;
    int relativeY = clientY - displayOriginY;
    
    // Account for magnification factor (same as original display code)
    if (MagnifyFactor > 0) {
        relativeX = (relativeX * 100) / MagnifyFactor;
        relativeY = (relativeY * 100) / MagnifyFactor;
    }
    
    if (globalView && curCell && (*curCell)) {
        // For view files - sample from current view cell
        if (!(*curCell)->bmImage || !(*curCell)->bmInfo) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        if ((*curCell)->bmImage && (*curCell)->bmInfo) {
            CelHeaderView* header = (CelHeaderView*)&(*curCell)->Head;
            
            // Adjust for hot spot offset (same as DisplayCurrentView function)
            int imageX = relativeX - header->xHot;
            int imageY = relativeY - header->yHot;
            
            int width = (*curCell)->bmInfo->bmiHeader.biWidth;
            int height = abs((*curCell)->bmInfo->bmiHeader.biHeight);
            
            // Check bounds
            if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                // Calculate pixel index (accounting for row padding)
                int rowWidth = ((width + 3) & ~3); // Round up to multiple of 4
                int pixelIndex = imageY * rowWidth + imageX;
                colorIndex = (*curCell)->bmImage[pixelIndex];
                return true;
            }
        }
    }
    else if (globalPicture && curCellIndex >= 0 && curCellIndex < globalPicture->CellsCount()) {
        // For picture files - we need to handle both single cell and all cells display
        if (curCellIndex == 0) {
            // When displaying all cells, we need to check each cell
            for (int i = 0; i < globalPicture->CellsCount(); i++) {
                Cell* cell = globalPicture->cells[i];
                if (!cell) continue;
                
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    // Check if click is within this cell's bounds
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        } else {
            // When displaying specific cell
            Cell* cell = globalPicture->cells[curCellIndex];
            if (cell) {
                if (!cell->bmImage || !cell->bmInfo) {
                    cell->GetImage(&cell->bmInfo, &cell->bmImage);
                }
                
                if (cell->bmImage && cell->bmInfo) {
                    CelHeaderPic* header = (CelHeaderPic*)&cell->Head;
                    
                    int imageX = relativeX - header->xpos;
                    int imageY = relativeY - header->ypos;
                    
                    int width = cell->bmInfo->bmiHeader.biWidth;
                    int height = abs(cell->bmInfo->bmiHeader.biHeight);
                    
                    if (imageX >= 0 && imageX < width && imageY >= 0 && imageY < height) {
                        int rowWidth = ((width + 3) & ~3);
                        int pixelIndex = imageY * rowWidth + imageX;
                        colorIndex = cell->bmImage[pixelIndex];
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void RenderClutGeneratorDialog() {
    using namespace ImGuiDialogs;
    using namespace FotoSCIhopStyles;
    
    // Ensure theme is applied
    FotoSCIhopStyles::RefreshTheme();
    
    // Static variables for dialog state
    static std::string generatedCode = "";
    static bool showCode = false;
    static bool shouldClose = false;
    static bool magicWandWasEnabled = false;
    
    // Enhanced state for better editing - keeping original complexity but organizing better
    static int selectedRemapIndex = -1;  // Which remap is currently selected
    static int hoveredRemapIndex = -1;   // Which remap is being hovered
    static bool editingMode = false;     // Are we editing a selected remap?
    static std::string editModeStatus = "";
    static bool editingFromColor = true; // true = editing FROM, false = editing TO
    
    bool open = true;
    SetNextWindowSize(1200, 800); // Slightly larger for better spacing
    
    if (!BeginDialog("CLUT Generator - Live Preview", &open)) {
        EndDialog();
        return;
    }
    
    // Handle close button
    if (!open || shouldClose) {
        shouldClose = false;
        selectedRemapIndex = -1;
        editingMode = false;
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            g_clutGenerator->SetMagicWandEnabled(false);
            magicWandWasEnabled = false;
        }
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
        EndDialog();
        return;
    }
    
    // Initialize CLUT generator if needed
    if (!g_clutGenerator) {
        g_clutGenerator = new ClutGenerator();
    }
    
    if (!g_clutGenerator->IsActive()) {
        Palette* currentPalette = nullptr;
        if (globalView && globalView->palSCI) {
            currentPalette = globalView->palSCI;
        } else if (globalPicture && globalPicture->palSCI) {
            currentPalette = globalPicture->palSCI;
        }
        
        if (currentPalette) {
            g_clutGenerator->Initialize(currentPalette);
        } else {
            ErrorText("No palette loaded! Please open a .v56 or .p56 file first.");
            ImGui::Spacing();
            if (FotoSCIhopStyles::CloseButton("Close")) {
                ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
            }
            EndDialog();
            return;
        }
    }
    
    // Auto-enable magic wand
    if (!magicWandWasEnabled && g_clutGenerator) {
        g_clutGenerator->SetMagicWandEnabled(true);
        g_clutGenerator->AnalyzeImageColorUsage();
        magicWandWasEnabled = true;
    }
    
    float availableWidth = ImGui::GetContentRegionAvail().x;
    
    // =========================================================================
    // HEADER SECTION - Cleaner but still comprehensive
    // =========================================================================
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
    
    if (ImGui::BeginChild("HeaderSection", ImVec2(0, 90), true)) {
        
        // Row 1: Main controls
        ImGui::BeginGroup();
        {
            bool remapActive = g_clutGenerator->IsPreviewEnabled();
            if (ImGui::Checkbox("Live Preview", &remapActive)) {
                g_clutGenerator->SetPreviewEnabled(remapActive);
            }
            
            ImGui::SameLine();
            if (remapActive) {
                SuccessText("* ACTIVE");
            } else {
                DisabledText("- OFF");
            }
            
            ImGui::SameLine(); 
            ImGui::Dummy(ImVec2(30, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Analyze Image")) {
                g_clutGenerator->AnalyzeImageColorUsage();
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            char usageText[64];
            sprintf(usageText, "(%d colors found)", (int)usedColors.size());
            InfoText(usageText);
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
            if (ImGui::Button("Clear All")) {
                g_clutGenerator->ClearAllRemaps();
                g_clutGenerator->SetPreviewEnabled(false);
                selectedRemapIndex = -1;
                editingMode = false;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            
            ImGui::SameLine();
            if (FotoSCIhopStyles::CloseButton("Close")) {
                shouldClose = true;
            }
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        
        // Row 2: Status and mode indicators - more organized
        if (editingMode && selectedRemapIndex >= 0) {
            WarningText(">> EDIT MODE:");
            ImGui::SameLine();
            SuccessText(editModeStatus.c_str());
            ImGui::SameLine();
            
            // Toggle between editing FROM and TO
            const char* editModeText = editingFromColor ? "[Editing FROM]" : "[Editing TO]";
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(editingFromColor ? 1.0f : 0.3f, editingFromColor ? 0.3f : 1.0f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(editingFromColor ? 1.0f : 0.5f, editingFromColor ? 0.5f : 1.0f, 0.5f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(editingFromColor ? 0.8f : 0.1f, editingFromColor ? 0.1f : 0.8f, 0.1f, 0.8f));
            if (ImGui::Button(editModeText)) {
                editingFromColor = !editingFromColor;
                editModeStatus = editingFromColor ? "Now editing FROM color" : "Now editing TO color";
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.36f, 0.36f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.24f, 0.24f, 0.8f));
            if (ImGui::Button("Exit Edit Mode")) {
                editingMode = false;
                selectedRemapIndex = -1;
                editModeStatus = "";
            }
            ImGui::PopStyleColor(3);
        } else {
            WarningText("* Magic Wand Active: Left-click = FROM, Right-click = TO");
            ImGui::SameLine();
            InfoText("- Click remap entries to select and edit them");
        }
        
    }
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // =========================================================================
    // MAIN WORKSPACE - Keep 3-column layout but improve organization
    // =========================================================================
    
    if (ImGui::BeginChild("MainWorkspace", ImVec2(0, -240))) {
        
        // =====================================================================
        // LEFT COLUMN - Enhanced Color Selection (keep full functionality)
        // =====================================================================
        if (ImGui::BeginChild("LeftColumn", ImVec2(availableWidth * 0.32f, 0), true)) {
            
            HeaderText("Color Selection");
            ImGui::Separator();
            ImGui::Spacing();
            
            // Enhanced current selection display
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.2f, 0.9f));
            if (ImGui::BeginChild("CurrentSelection", ImVec2(0, 200), true)) {
                
                int fromColor = g_clutGenerator->GetSelectedFromColor();
                int toColor = g_clutGenerator->GetSelectedToColor();
                
                // FROM color display
                ImGui::Text("FROM Color:");
                PalEntry fromEntry;
                if (g_clutGenerator->GetOriginalPaletteEntry(fromColor, fromEntry)) {
                    char fromLabel[64];
                    sprintf(fromLabel, "  %d  ", fromColor);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                    ImGui::Button(fromLabel);
                    ImGui::PopStyleColor(3);
                    
                    ImGui::SameLine();
                    char rgbText[32];
                    sprintf(rgbText, "RGB(%d, %d, %d)", fromEntry.red, fromEntry.green, fromEntry.blue);
                    ImGui::Text("%s", rgbText);
                }
                
                ImGui::Spacing();
                
                // TO color display  
                ImGui::Text("TO Color:");
                PalEntry toEntry;
                if (g_clutGenerator->GetOriginalPaletteEntry(toColor, toEntry)) {
                    char toLabel[64];
                    sprintf(toLabel, "  %d  ", toColor);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                    ImGui::Button(toLabel);
                    ImGui::PopStyleColor(3);
                    
                    ImGui::SameLine();
                    char rgbText[32];
                    sprintf(rgbText, "RGB(%d, %d, %d)", toEntry.red, toEntry.green, toEntry.blue);
                    ImGui::Text("%s", rgbText);
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Edit mode status and navigation
                if (editingMode && selectedRemapIndex >= 0) {
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    if (selectedRemapIndex < remaps.size()) {
                        char editingText[64];
                        sprintf(editingText, "Editing Remap #%d", selectedRemapIndex + 1);
                        WarningText(editingText);
                        InfoText("Click palette colors to modify");
                        ImGui::Spacing();
                        
                        // Navigation buttons
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.72f, 0.96f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.48f, 0.64f, 0.8f));
                        if (ImGui::Button("<< Previous Color")) {
                            if (editingFromColor) {
                                int newFrom = (fromColor > 0) ? fromColor - 1 : 255;
                                g_clutGenerator->SetSelectedFromColor(newFrom);
                            } else {
                                int newTo = (toColor > 0) ? toColor - 1 : 255;
                                g_clutGenerator->SetSelectedToColor(newTo);
                            }
                        }
                        ImGui::PopStyleColor(3);
                        
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.72f, 0.96f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.32f, 0.48f, 0.64f, 0.8f));
                        if (ImGui::Button("Next Color >>")) {
                            if (editingFromColor) {
                                int newFrom = (fromColor < 255) ? fromColor + 1 : 0;
                                g_clutGenerator->SetSelectedFromColor(newFrom);
                            } else {
                                int newTo = (toColor < 255) ? toColor + 1 : 0;
                                g_clutGenerator->SetSelectedToColor(newTo);
                            }
                        }
                        ImGui::PopStyleColor(3);
                    }
                }
                
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            // Action buttons - keep original functionality
            if (!editingMode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 1.0f));
                if (ImGui::Button("Add New Remap", ImVec2(-1, 0))) {
                    int fromCol = g_clutGenerator->GetSelectedFromColor();
                    int toCol = g_clutGenerator->GetSelectedToColor();
                    if (fromCol != toCol) {
                        g_clutGenerator->AddRemap(fromCol, toCol);
                        // Auto-select the new remap
                        const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                        selectedRemapIndex = remaps.size() - 1;
                    }
                }
                ImGui::PopStyleColor(3);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.48f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.32f, 0.16f, 1.0f));
                if (ImGui::Button("Remove FROM Color", ImVec2(-1, 0))) {
                    int fromCol = g_clutGenerator->GetSelectedFromColor();
                    g_clutGenerator->RemoveRemap(fromCol);
                    selectedRemapIndex = -1;
                }
                ImGui::PopStyleColor(3);
            } else {
                // Edit mode buttons
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 1.0f));
                if (ImGui::Button("Apply Changes", ImVec2(-1, 0))) {
                    // Apply the current FROM/TO to the selected remap
                    if (selectedRemapIndex >= 0) {
                        const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                        if (selectedRemapIndex < remaps.size()) {
                            g_clutGenerator->RemoveRemap(remaps[selectedRemapIndex].fromColor);
                            g_clutGenerator->AddRemap(g_clutGenerator->GetSelectedFromColor(), 
                                                    g_clutGenerator->GetSelectedToColor());
                        }
                    }
                    editingMode = false;
                    selectedRemapIndex = -1;
                }
                ImGui::PopStyleColor(3);
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.36f, 0.36f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.24f, 0.24f, 0.8f));
                if (ImGui::Button("Cancel Edit", ImVec2(-1, 0))) {
                    editingMode = false;
                    selectedRemapIndex = -1;
                }
                ImGui::PopStyleColor(3);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Usage instructions
            HeaderText("Quick Guide:");
            if (editingMode) {
                InfoText(">> EDIT MODE ACTIVE");
                InfoText("- Click palette colors to modify");
                InfoText("- Use arrows to navigate colors");
                InfoText("- Toggle FROM/TO in header");
                InfoText("- Apply or Cancel when done");
            } else {
                InfoText("- Left-click image: FROM color");
                InfoText("- Right-click image: TO color");
                InfoText("- Click remap entries to edit");
                InfoText("- Magic wand always active");
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // =====================================================================
        // MIDDLE COLUMN - Full Palette Grid (keep all original features)
        // =====================================================================
        if (ImGui::BeginChild("MiddleColumn", ImVec2(availableWidth * 0.36f, 0), true)) {
            
            HeaderText("Palette Grid");
            ImGui::Separator();
            
            // Legend - updated for border system
            const std::set<int>& usedColors = g_clutGenerator->GetUsedColorIndices();
            if (usedColors.size() > 0) {
                ImGui::Text("Legend:");
                
                // Draw legend items with custom border examples
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                
                // Row 1
                ImVec2 startPos = ImGui::GetCursorScreenPos();
                
                // Used in image (thin blue border)
                ImVec2 usedMin = startPos;
                ImVec2 usedMax = ImVec2(startPos.x + 12, startPos.y + 12);
                drawList->AddRectFilled(usedMin, usedMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(usedMin, usedMax, IM_COL32(100, 150, 255, 255), 0.0f, 0, 1.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Used");
                
                ImGui::SameLine();
                ImVec2 currentPos = ImGui::GetCursorScreenPos();
                
                // FROM color (thick red border)
                ImVec2 fromMin = currentPos;
                ImVec2 fromMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(fromMin, fromMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(fromMin, fromMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("FROM");
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // TO color (thick green border)
                ImVec2 toMin = currentPos;
                ImVec2 toMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(toMin, toMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(toMin, toMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("TO");
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // Remapped (medium yellow border)
                ImVec2 remapMin = currentPos;
                ImVec2 remapMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(remapMin, remapMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(remapMin, remapMax, IM_COL32(255, 255, 80, 255), 0.0f, 0, 2.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Remap");
                
                ImGui::SameLine();
                currentPos = ImGui::GetCursorScreenPos();
                
                // Selected remap (thick magenta border)
                ImVec2 selMin = currentPos;
                ImVec2 selMax = ImVec2(currentPos.x + 12, currentPos.y + 12);
                drawList->AddRectFilled(selMin, selMax, IM_COL32(128, 128, 128, 255));
                drawList->AddRect(selMin, selMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 15, ImGui::GetCursorPos().y));
                ImGui::Text("Selected");
                
                // End the line and add separator
                ImGui::NewLine();
                ImGui::Separator();
            }
            
            ImGui::Spacing();
            
            if (g_clutGenerator && g_clutGenerator->IsActive()) {
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.1f, 0.8f));
                if (ImGui::BeginChild("PaletteGrid", ImVec2(0, 0), true)) {
                    
                    const int COLORS_PER_ROW = 16;
                    const float BUTTON_SIZE = 18.0f;
                    const float SPACING_VAL = 1.0f;
                    
                    // Get current selected remap for highlighting
                    const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
                    int highlightFromColor = -1;
                    int highlightToColor = -1;
                    
                    if (selectedRemapIndex >= 0 && selectedRemapIndex < remaps.size()) {
                        highlightFromColor = remaps[selectedRemapIndex].fromColor;
                        highlightToColor = remaps[selectedRemapIndex].toColor;
                    } else if (hoveredRemapIndex >= 0 && hoveredRemapIndex < remaps.size()) {
                        highlightFromColor = remaps[hoveredRemapIndex].fromColor;
                        highlightToColor = remaps[hoveredRemapIndex].toColor;
                    }
                    
                    for (int row = 0; row < 16; row++) {
                        for (int col = 0; col < 16; col++) {
                            int colorIndex = row * COLORS_PER_ROW + col;
                            
                            PalEntry originalEntry;
                            if (g_clutGenerator->GetOriginalPaletteEntry(colorIndex, originalEntry)) {
                                
                                char buttonId[16];
                                sprintf(buttonId, "##%d", colorIndex);
                                
                                // Enhanced state checking
                                int fromColor = g_clutGenerator->GetSelectedFromColor();
                                int toColor = g_clutGenerator->GetSelectedToColor();
                                bool isFromColor = (colorIndex == fromColor);
                                bool isToColor = (colorIndex == toColor);
                                bool hasRemap = g_clutGenerator->HasRemap(colorIndex);
                                bool isUsedInImage = g_clutGenerator->IsColorUsedInImage(colorIndex);
                                
                                // Check if this color is part of selected/hovered remap
                                bool isSelectedRemapFrom = (colorIndex == highlightFromColor);
                                bool isSelectedRemapTo = (colorIndex == highlightToColor);
                                
                                float r = originalEntry.red / 255.0f;
                                float g = originalEntry.green / 255.0f;
                                float b = originalEntry.blue / 255.0f;
                                
                                // Always use original color for button background
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, 1.0f));
                                
                                // Store button position for border drawing
                                ImVec2 buttonPos = ImGui::GetCursorScreenPos();
                                
                                if (ImGui::Button(buttonId, ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
                                    // Handle clicks
                                }
                                
                                ImGui::PopStyleColor(3);
                                
                                // Draw colored borders for different states (after button is drawn)
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                ImVec2 buttonMin = buttonPos;
                                ImVec2 buttonMax = ImVec2(buttonPos.x + BUTTON_SIZE, buttonPos.y + BUTTON_SIZE);
                                
                                // Priority system for border colors (highest priority wins)
                                if (isSelectedRemapFrom) {
                                    // Thick magenta border for selected remap FROM
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isSelectedRemapTo) {
                                    // Thick cyan border for selected remap TO
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 255, 255), 0.0f, 0, 3.0f);
                                } else if (isFromColor) {
                                    // Thick red border for current FROM
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 3.0f);
                                } else if (isToColor) {
                                    // Thick green border for current TO
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(80, 255, 80, 255), 0.0f, 0, 3.0f);
                                } else if (hasRemap) {
                                    // Medium yellow border for remapped colors
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 80, 200), 0.0f, 0, 2.0f);
                                } else if (isUsedInImage) {
                                    // Thin blue border for used in image
                                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(100, 150, 255, 150), 0.0f, 0, 1.0f);
                                }
                                
                                // Enhanced click handling for edit mode
                                if (ImGui::IsItemClicked(0)) { // Left click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM color updated";
                                        } else {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO color updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedFromColor(colorIndex);
                                    }
                                }
                                if (ImGui::IsItemClicked(1)) { // Right click
                                    if (editingMode) {
                                        if (editingFromColor) {
                                            g_clutGenerator->SetSelectedToColor(colorIndex);
                                            editModeStatus = "TO color updated";
                                        } else {
                                            g_clutGenerator->SetSelectedFromColor(colorIndex);
                                            editModeStatus = "FROM color updated";
                                        }
                                    } else {
                                        g_clutGenerator->SetSelectedToColor(colorIndex);
                                    }
                                }
                                
                                // Enhanced tooltip
                                if (ImGui::IsItemHovered()) {
                                    char tooltipText[512];
                                    std::string roleText = "";
                                    
                                    if (isSelectedRemapFrom) roleText += " [SELECTED FROM - Magenta Border]";
                                    if (isSelectedRemapTo) roleText += " [SELECTED TO - Cyan Border]";
                                    if (isFromColor) roleText += " [CURRENT FROM - Red Border]";
                                    if (isToColor) roleText += " [CURRENT TO - Green Border]";
                                    if (hasRemap) roleText += " [REMAPPED - Yellow Border]";
                                    if (isUsedInImage) roleText += " [USED IN IMAGE - Blue Border]";
                                    
                                    if (editingMode) {
                                        sprintf(tooltipText, 
                                            "Color %d: RGB(%d, %d, %d)%s\n"
                                            "Left = %s, Right = %s (EDIT MODE)",
                                            colorIndex, originalEntry.red, originalEntry.green, originalEntry.blue, 
                                            roleText.c_str(),
                                            editingFromColor ? "FROM" : "TO",
                                            editingFromColor ? "TO" : "FROM"
                                        );
                                    } else {
                                        sprintf(tooltipText, 
                                            "Color %d: RGB(%d, %d, %d)%s\n"
                                            "Left = FROM, Right = TO",
                                            colorIndex, originalEntry.red, originalEntry.green, originalEntry.blue, 
                                            roleText.c_str()
                                        );
                                    }
                                    ImGui::SetTooltip("%s", tooltipText);
                                }
                                
                                if (col < 15) {
                                    ImGui::SameLine(0, SPACING_VAL);
                                }
                            }
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // =====================================================================
        // RIGHT COLUMN - Full Remap Management (keep all features)
        // =====================================================================
        if (ImGui::BeginChild("RightColumn", ImVec2(0, 0), true)) {
            
            HeaderText("Active Remaps");
            ImGui::Separator();
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            
            // Enhanced status summary
            char statusText[128];
            int activeRemaps = 0;
            for (size_t i = 0; i < remaps.size(); i++) {
                if (remaps[i].active) activeRemaps++;
            }
            
            sprintf(statusText, "%d Active (max 12)", activeRemaps);
            if (activeRemaps > 12) {
                WarningText(statusText);
            } else if (activeRemaps > 0) {
                SuccessText(statusText);
            } else {
                DisabledText(statusText);
            }
            
            if (selectedRemapIndex >= 0) {
                ImGui::SameLine();
                char selectedText[64];
                sprintf(selectedText, "(#%d selected)", selectedRemapIndex + 1);
                WarningText(selectedText);
            }
            
            ImGui::Spacing();
            
            if (remaps.empty()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.1f, 0.3f));
                if (ImGui::BeginChild("EmptyState", ImVec2(0, 120), true)) {
                    ImGui::Spacing();
                    InfoText("No remaps yet");
                    ImGui::Spacing();
                    InfoText("Click colors in image");
                    InfoText("then 'Add New Remap'");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            } else {
                // Enhanced remap table with selection
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.08f, 0.8f));
                if (ImGui::BeginChild("RemapTable", ImVec2(0, 0), true)) {
                    
                    hoveredRemapIndex = -1; // Reset hover state
                    
                    for (int i = 0; i < static_cast<int>(remaps.size()); i++) {
                        const ColorRemapEntry& remap = remaps[i];
                        
                        // Enhanced selection highlighting
                        bool isSelected = (i == selectedRemapIndex);
                        if (isSelected) {
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.2f, 0.1f, 0.7f));
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                        }
                        
                        char remapChildId[32];
                        sprintf(remapChildId, "RemapEntry_%d", i);
                        
                        if (ImGui::BeginChild(remapChildId, ImVec2(0, 45), true)) {
                            
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
                            
                            ImGui::BeginGroup();
                            
                            // Color swatches and info
                            PalEntry fromEntry, toEntry;
                            if (g_clutGenerator->GetOriginalPaletteEntry(remap.fromColor, fromEntry) &&
                                g_clutGenerator->GetOriginalPaletteEntry(remap.toColor, toEntry)) {
                                
                                // FROM color
                                char fromId[32];
                                sprintf(fromId, "##from%d", i);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(fromEntry.red / 255.0f, fromEntry.green / 255.0f, fromEntry.blue / 255.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(fromEntry.red / 255.0f * 1.2f, fromEntry.green / 255.0f * 1.2f, fromEntry.blue / 255.0f * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(fromEntry.red / 255.0f * 0.8f, fromEntry.green / 255.0f * 0.8f, fromEntry.blue / 255.0f * 0.8f, 1.0f));
                                if (ImGui::Button(fromId, ImVec2(25, 25))) {
                                    // Enter edit mode for FROM color if already selected
                                    if (selectedRemapIndex == i && !editingMode) {
                                        editingMode = true;
                                        editingFromColor = true;
                                        g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                        g_clutGenerator->SetSelectedToColor(remap.toColor);
                                        editModeStatus = "Editing FROM color - click palette to modify";
                                    }
                                }
                                ImGui::PopStyleColor(3);
                                
                                ImGui::SameLine();
                                char fromText[16];
                                sprintf(fromText, "%d", remap.fromColor);
                                ImGui::Text("%s", fromText);
                                
                                ImGui::SameLine();
                                ImGui::Text("->");
                                
                                ImGui::SameLine();
                                // TO color
                                char toId[32];
                                sprintf(toId, "##to%d", i);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(toEntry.red / 255.0f, toEntry.green / 255.0f, toEntry.blue / 255.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(toEntry.red / 255.0f * 1.2f, toEntry.green / 255.0f * 1.2f, toEntry.blue / 255.0f * 1.2f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(toEntry.red / 255.0f * 0.8f, toEntry.green / 255.0f * 0.8f, toEntry.blue / 255.0f * 0.8f, 1.0f));
                                if (ImGui::Button(toId, ImVec2(25, 25))) {
                                    // Enter edit mode for TO color if already selected
                                    if (selectedRemapIndex == i && !editingMode) {
                                        editingMode = true;
                                        editingFromColor = false;
                                        g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                                        g_clutGenerator->SetSelectedToColor(remap.toColor);
                                        editModeStatus = "Editing TO color - click palette to modify";
                                    }
                                }
                                ImGui::PopStyleColor(3);
                                
                                ImGui::SameLine();
                                char toText[16];
                                sprintf(toText, "%d", remap.toColor);
                                ImGui::Text("%s", toText);
                            }
                            
                            ImGui::SameLine();
                            
                            // Toggle button
                            char toggleId[32];
                            sprintf(toggleId, "%s##T%d", remap.active ? "ON" : "OFF", i);
                            if (remap.active) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 0.7f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.24f, 0.7f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.16f, 0.7f));
                                if (ImGui::Button(toggleId, ImVec2(30, 25))) {
                                    g_clutGenerator->ToggleRemapActive(i);
                                }
                                ImGui::PopStyleColor(3);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
                                if (ImGui::Button(toggleId, ImVec2(30, 25))) {
                                    g_clutGenerator->ToggleRemapActive(i);
                                }
                                ImGui::PopStyleColor(3);
                            }
                            
                            ImGui::SameLine();
                            
                            // Delete button
                            char deleteId[32];
                            sprintf(deleteId, "X##%d", i);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.24f, 0.24f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.64f, 0.16f, 0.16f, 0.7f));
                            if (ImGui::Button(deleteId, ImVec2(25, 25))) {
                                g_clutGenerator->ClearRemap(i);
                                if (selectedRemapIndex == i) {
                                    selectedRemapIndex = -1;
                                    editingMode = false;
                                } else if (selectedRemapIndex > i) {
                                    selectedRemapIndex--;
                                }
                            }
                            ImGui::PopStyleColor(3);
                            
                            ImGui::EndGroup();
                            ImGui::PopStyleVar();
                            
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        
                        // ENTIRE ENTRY CLICK DETECTION
                        if (ImGui::IsItemClicked()) {
                            selectedRemapIndex = i;
                            g_clutGenerator->SetSelectedFromColor(remap.fromColor);
                            g_clutGenerator->SetSelectedToColor(remap.toColor);
                            editingMode = false; // Exit edit mode when selecting a different remap
                        }
                        
                        // Track hover for palette highlighting
                        if (ImGui::IsItemHovered()) {
                            hoveredRemapIndex = i;
                            ImGui::SetTooltip("Click to select - Click FROM/TO buttons to edit - Selected remap highlights in palette");
                        }
                        
                        if (i < static_cast<int>(remaps.size()) - 1) {
                            ImGui::Spacing();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            
        }
        ImGui::EndChild();
        
    }
    ImGui::EndChild();

    // =========================================================================
    // BOTTOM SECTION - Import and Export (keep full functionality)
    // =========================================================================
    
    ImGui::Separator();
    ImGui::Spacing();
    
    static int selectedTab = 0;
    
    if (ImGui::Button("Import from COLORTBL.SC")) selectedTab = 0;
    ImGui::SameLine();
    if (ImGui::Button("Generate SCI Code")) selectedTab = 1;
    
    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.9f));
    
    if (selectedTab == 0) {
        // IMPORT TAB
        if (ImGui::BeginChild("ImportTab", ImVec2(0, 0), true)) {
            
            HeaderText("Import Existing Remaps");
            InfoText("Paste a line from COLORTBL.SC to import existing color remaps");
            
            ImGui::Spacing();
            
            static char importBuffer[1024] = "";
            static std::string importStatus = "";
            static bool showImportStatus = false;
            
            ImGui::Text("COLORTBL.SC Line:");
            ImGui::PushItemWidth(availableWidth - 150);
            if (ImGui::InputText("##import_text", importBuffer, sizeof(importBuffer))) {
                showImportStatus = false;
                importStatus = "";
            }
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.24f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.16f, 0.64f, 1.0f));
            if (ImGui::Button("Import")) {
                if (strlen(importBuffer) > 0) {
                    g_clutGenerator->ClearAllRemaps();
                    selectedRemapIndex = -1;
                    editingMode = false;
                    std::string importLine(importBuffer);
                    bool success = g_clutGenerator->ImportFromSCITableEntry(importLine);
                    
                    if (success) {
                        const std::vector<ColorRemapEntry>& newRemaps = g_clutGenerator->GetCurrentRemaps();
                        char statusMsg[128];
                        sprintf(statusMsg, "Successfully imported %d remaps!", (int)newRemaps.size());
                        importStatus = statusMsg;
                        importBuffer[0] = '\0';
                    } else {
                        importStatus = "Failed to parse remap data. Check format.";
                    }
                    showImportStatus = true;
                } else {
                    importStatus = "Please paste a COLORTBL.SC line first";
                    showImportStatus = true;
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                importBuffer[0] = '\0';
                showImportStatus = false;
                importStatus = "";
            }
            
            ImGui::Spacing();
            
            if (showImportStatus && !importStatus.empty()) {
                if (importStatus.find("Success") != std::string::npos) {
                    SuccessText(importStatus.c_str());
                } else {
                    ErrorText(importStatus.c_str());
                }
            } else {
                DisabledText("Example: 99 0 100 38 101 0 -1 -1 -1 -1 ... ; black wolf");
            }
            
        }
        ImGui::EndChild();
        
    } else {
        // GENERATE CODE TAB
        if (ImGui::BeginChild("GenerateTab", ImVec2(0, 0), true)) {
            
            HeaderText("Generate COLORTBL.SC Code");
            InfoText("Export your remaps as Sierra SCI-compatible table entries");
            
            ImGui::Spacing();
            
            const std::vector<ColorRemapEntry>& remaps = g_clutGenerator->GetCurrentRemaps();
            int activeRemaps = 0;
            for (size_t i = 0; i < remaps.size(); i++) {
                if (remaps[i].active) activeRemaps++;
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.48f, 0.64f, 1.0f));
            if (ImGui::Button("Generate Code")) {
                if (activeRemaps > 0) {
                    generatedCode = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop CLUT Generator");
                    showCode = true;
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.96f, 0.48f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.64f, 0.32f, 1.0f));
            if (ImGui::Button("Copy to Clipboard")) {
                if (activeRemaps > 0) {
                    std::string sciTable = g_clutGenerator->GenerateSCITableEntry("Generated by FotoSCIhop CLUT Generator");
                    
                    if (OpenClipboard(hWnd)) {
                        EmptyClipboard();
                        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, (SIZE_T)(sciTable.length() + 1));
                        if (hClipboardData) {
                            char* pchData = (char*)GlobalLock(hClipboardData);
                            if (pchData) {
                                strcpy(pchData, sciTable.c_str());
                                GlobalUnlock(hClipboardData);
                                SetClipboardData(CF_TEXT, hClipboardData);
                                generatedCode = sciTable;
                                showCode = true;
                            }
                        }
                        CloseClipboard();
                    }
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            
            if (showCode && !generatedCode.empty()) {
                HeaderText("Generated Code:");
                
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
                
                static char codeBuffer[1024];
                size_t len = generatedCode.length();
                if (len >= sizeof(codeBuffer)) len = sizeof(codeBuffer) - 1;
                memcpy(codeBuffer, generatedCode.c_str(), len);
                codeBuffer[len] = '\0';
                
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##generated_code", codeBuffer, sizeof(codeBuffer));
                ImGui::PopItemWidth();
                
                ImGui::PopStyleColor(2);
                
                ImGui::Spacing();
                InfoText("Copy this line into your COLORTBL.SC file's lRemapTable array");
                
            } else {
                InfoText("Create some remaps first, then generate the code!");
            }
            
        }
        ImGui::EndChild();
    }
    
    ImGui::PopStyleColor();
    
    EndDialog();
}

#pragma warning(pop)  // Restore warning level