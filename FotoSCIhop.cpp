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
#include "display.h"

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

bool g_pendingThemeChange = false;
FotoSCIhopStyles::ThemeMode g_pendingTheme = FotoSCIhopStyles::ThemeMode::PHOTOSHOP_DARK;

void ShowLoopCell(unsigned char newloop, unsigned char newcell) {
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
    if (curLoop) {
        // Additional safety check before accessing cells
        if (newcell < globalView->loops[newloop]->Head.numCels && globalView->loops[newloop]->cells[newcell]) {
            curCell = &globalView->loops[newloop]->cells[newcell];
        } else {
            curCell = nullptr; // Set to null if cell doesn't exist
        }
        
        if (curCell || (*curLoop)->Head.flags) {
            if (curCell && !(*curLoop)->Head.flags) {
                curCellIndex = newcell;
                
                // CRITICAL: Ensure image data is loaded before updating scroll bars
                if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
                    (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
                }
            }

            HMENU menu = GetMenu(hWnd); 
        
            EnableMenuItem(menu, ID_IMPORTABMP, ((*curLoop)->Head.flags ? MF_GRAYED : MF_ENABLED));
            EnableMenuItem(menu, ID_ESPORTABMP, ((*curLoop)->Head.flags ? MF_GRAYED : MF_ENABLED));
            EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_ENABLED);
            EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_ENABLED);
            if (newloop == globalView->Head.view32.loopCount - 1)
                EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_GRAYED);
        
            if (newloop == 0)
                EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_GRAYED);

            EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
            EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
            if ((newcell == globalView->loops[newloop]->Head.numCels - 1) || (globalView->loops[newloop]->Head.numCels == 0))
                EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
        
            if (newcell == 0)
                EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

            // CRITICAL: Update scroll bars after image data is ready
            UpdateScrollBars();

            InvalidateRgn(hWnd, NULL, true);
        }
    }
}

void ShowCell(unsigned char newcell) {
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
    
    if (curCell) {
        // CRITICAL: Ensure image data is loaded before updating scroll bars
        if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        HMENU menu = GetMenu(hWnd); 

        EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
        EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
        if (curCellIndex == globalPicture->CellsCount() - 1)
            EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
        
        if (curCellIndex == 0)
            EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

        // CRITICAL: Update scroll bars after image data is ready
        UpdateScrollBars();
        
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

      // Reset scroll position when loading new file
      g_scrollX = 0;
      g_scrollY = 0;

      RECT clientRect;
      GetClientRect(hWnd, &clientRect);
      g_clientWidth = clientRect.right;
      g_clientHeight = clientRect.bottom;

      // IMPORTANT: Don't call UpdateScrollBars here - let ShowCell/ShowLoopCell handle it
      // after image data is properly loaded
      
      // Use a timer to ensure scroll bars are updated after image loading is complete
      SetTimer(hWnd, 3, 100, NULL); // 100ms delay
      
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
    fread(&palette, sizeof(RGBQUAD), 256, file);

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
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // Removed CS_OWNDC if present
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_IMMAGINA);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL; // IMPORTANT: Set to NULL to prevent auto-erase
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

    // Enable scroll bars
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style |= WS_HSCROLL | WS_VSCROLL;
    SetWindowLong(hWnd, GWL_STYLE, style);

    // Initialize scroll system
    UpdateScrollBars();

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

    // Load theme setting (add this at the end)
    int savedTheme = GetPrivateProfileInt("main", "theme", 0, gConfigIni);
    if (savedTheme >= 0 && savedTheme < 5) { // Validate theme index
        FotoSCIhopStyles::SetTheme((FotoSCIhopStyles::ThemeMode)savedTheme);
    }
    FotoSCIhopStyles::RefreshTheme();

    // Set up dialog callbacks
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PROPERTIES, "Properties", &RenderPropertiesDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_ABOUT, "About FotoSCIhop", &RenderAboutDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR, "CLUT Generator", &RenderClutGeneratorDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_REALMPAL, "Realmpal Converter", &RenderRealmpalDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PREFERENCES, "Preferences", &RenderPreferencesDialog);

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

        case IDM_PREFERENCES:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_PREFERENCES);
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
        PAINTSTRUCT ps;
        HDC hdcScreen = BeginPaint(hWnd, &ps);

        // Get client rect
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        g_clientWidth = clientRect.right;
        g_clientHeight = clientRect.bottom;

        // Create double buffer
        HDC hdcBuffer = CreateCompatibleDC(hdcScreen);
        HBITMAP hbmBuffer = CreateCompatibleBitmap(hdcScreen, g_clientWidth, g_clientHeight);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBuffer, hbmBuffer);

        // Set up theme font
        HFONT themeFont = FotoSCIhopStyles::CreateThemeFont(16);
        HFONT oldFont = (HFONT)SelectObject(hdcBuffer, themeFont);

        // Theme background
        const FotoSCIhopStyles::UnifiedColors &colors = FotoSCIhopStyles::GetCurrentColors();
        HBRUSH bgBrush = CreateSolidBrush(colors.background);
        FillRect(hdcBuffer, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        // Themed top bar
        RECT topBar = {0, 0, clientRect.right, 25};
        FotoSCIhopStyles::DrawRoundedRect(hdcBuffer, topBar, colors.surface, colors.border, 0);

        // Initialize layout variables properly based on file type
        if (globalView)
        {
            picX = 220; // Views need space for loop information
        }
        if (globalPicture)
        {
            picX = 0; // Pictures start at left edge
        }

        // Enhanced palette with unified styling
        if (tableX > 0)
        {
            DrawPaletteTable(hdcBuffer);
        }

        // Image display
        if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags)
        {
            if (gReferenceBM && !gReferencePriority)
                DisplayReferenceImage(hdcBuffer);

            DisplayCurrentViewWithFrame(hdcBuffer);

            if (gReferenceBM && gReferencePriority)
                DisplayReferenceImage(hdcBuffer);

            if ((*curCell) && (*curCell)->Head.view.linkTableCount >= 1)
                DisplayLinkPoints(hdcBuffer);
        }

        if (globalPicture)
        {
            DisplayCurrentPicWithFrame(hdcBuffer);

            if (showpbars)
                DisplayPriorityBars(hdcBuffer);
        }

        // Themed cell info display
        if (curCell && (*curCell))
        {
            DrawCellInfo(hdcBuffer);
        }

        // Copy buffer to screen in one operation
        BitBlt(hdcScreen, 0, 0, g_clientWidth, g_clientHeight, hdcBuffer, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcBuffer, oldFont);
        SelectObject(hdcBuffer, hbmOld);
        DeleteObject(hbmBuffer);
        DeleteDC(hdcBuffer);
        FotoSCIhopStyles::SafeDeleteFont(themeFont);

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1; // Non-zero means "we handled it"

    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            // Clear any cached coordinate calculations
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            g_clientWidth = clientRect.right;
            g_clientHeight = clientRect.bottom;

            // Update scroll system first
            UpdateScrollBars();

            // Force complete redraw after resize to prevent artifacts
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    case WM_HSCROLL:
    {
        int scrollCode = LOWORD(wParam);
        int scrollPos = HIWORD(wParam);

        switch (scrollCode)
        {
        case SB_LINEUP:
            ScrollBy(-20, 0);
            break;
        case SB_LINEDOWN:
            ScrollBy(20, 0);
            break;
        case SB_PAGEUP:
            ScrollBy(-g_clientWidth / 4, 0);
            break;
        case SB_PAGEDOWN:
            ScrollBy(g_clientWidth / 4, 0);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollTo(scrollPos, g_scrollY);
            break;
        }
        break;
    }

    case WM_VSCROLL:
    {
        int scrollCode = LOWORD(wParam);
        int scrollPos = HIWORD(wParam);

        switch (scrollCode)
        {
        case SB_LINEUP:
            ScrollBy(0, -20);
            break;
        case SB_LINEDOWN:
            ScrollBy(0, 20);
            break;
        case SB_PAGEUP:
            ScrollBy(0, -g_clientHeight / 4);
            break;
        case SB_PAGEDOWN:
            ScrollBy(0, g_clientHeight / 4);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollTo(g_scrollX, scrollPos);
            break;
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        WORD keys = GET_KEYSTATE_WPARAM(wParam);

        if (keys & MK_CONTROL)
        {
            // Ctrl + wheel = zoom
            if (delta > 0)
            {
                ZoomIn();
            }
            else
            {
                ZoomOut();
            }
        }
        else
        {
            // Plain wheel = vertical scroll
            ScrollBy(0, -delta / 4);
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Handle panning
        if (g_isPanning)
        {
            UpdatePanning(x, y);
        }

        break;
    }

    case WM_LBUTTONUP:
        StopPanning();
        break;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Check zoom control click first
        if (HandleZoomControlClick(x, y))
        {
            break;
        }

        // Check if magic wand is enabled
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled())
        {
            int colorIndex;
            if (SampleColorAtScreenPosition(x, y, colorIndex))
            {
                g_clutGenerator->SetSelectedFromColor(colorIndex);
                char message[256];
                sprintf(message, "FotoSCIhop - Magic Wand: Selected color %d as FROM color", colorIndex);
                SetWindowText(hWnd, message);
                SetTimer(hWnd, 2, 3000, NULL);
            }
            break;
        }

        // Start panning with middle mouse button or space key + drag
        WORD keys = wParam;
        if (keys & MK_MBUTTON || GetKeyState(VK_SPACE) & 0x8000)
        {
            StartPanning(x, y);
        }

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

    case WM_USER + 1:
    {
        UpdateScrollBars();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_TIMER:
    if (wParam == 1) { // ImGui timer
        if (g_pendingThemeChange) {
            FotoSCIhopStyles::SetTheme(g_pendingTheme);
            FotoSCIhopStyles::RefreshTheme();
            g_pendingThemeChange = false;
            
            // Lightweight redraw - let Windows handle timing
            InvalidateRect(hWnd, NULL, TRUE);
            // Don't force immediate update - let it happen naturally
        }
        
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
    else if (wParam == 3) { // Scroll bar initialization timer
        KillTimer(hWnd, 3);
        EnsureScrollBarsAfterLoad();
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