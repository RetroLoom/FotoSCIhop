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
    static int lastScrollX = -1, lastScrollY = -1;
    static int lastClientWidth = -1, lastClientHeight = -1;
    static POINT cachedOrigin = {0, 0};
    
    // Only recalculate if values have changed (including client size for resize handling)
    if (picX != lastPicX || picY != lastPicY || tableX != lastTableX || 
        g_scrollX != lastScrollX || g_scrollY != lastScrollY ||
        g_clientWidth != lastClientWidth || g_clientHeight != lastClientHeight) {
        
        cachedOrigin.x = UI_LEFT_MARGIN + picX + tableX - g_scrollX;
        cachedOrigin.y = UI_TOP_MARGIN + picY - g_scrollY;
        
        lastPicX = picX;
        lastPicY = picY;
        lastTableX = tableX;
        lastScrollX = g_scrollX;
        lastScrollY = g_scrollY;
        lastClientWidth = g_clientWidth;
        lastClientHeight = g_clientHeight;
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
void DrawPaletteStatusIndicators(HDC hdc, Palette* tpalette) {
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    
    if (!tpalette->palData) {
        FotoSCIhopStyles::DrawThemedText(hdc, INTERFACE_MISSINGPALETTE, 30, 300, 190, 20, true);
        return;
    }

    // Missing colors indicator with themed colors
    RECT missingRect = {20, 300, 32, 312};
    FotoSCIhopStyles::DrawRoundedRect(hdc, missingRect, colors.warning, colors.warning);
    
    // Draw warning icon (triangle)
    HPEN iconPen = CreatePen(PS_SOLID, 2, colors.textPrimary);
    HPEN oldPen = (HPEN)SelectObject(hdc, iconPen);
    
    POINT triangle[4] = {
        {26, 304}, {23, 309}, {29, 309}, {26, 304}
    };
    Polyline(hdc, triangle, 4);
    
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);

    FotoSCIhopStyles::DrawThemedText(hdc, INTERFACE_MISSINGCOLORSSTR, 40, 295, 150, 20, true);

    // Locked colors indicator
    if (!tpalette->Head.type) {
        RECT lockRect = {20, 320, 32, 332};
        FotoSCIhopStyles::DrawRoundedRect(hdc, lockRect, colors.error, colors.error);
        
        // Draw lock icon
        HPEN lockPen = CreatePen(PS_SOLID, 1, colors.textPrimary);
        oldPen = (HPEN)SelectObject(hdc, lockPen);
        
        Rectangle(hdc, 23, 327, 29, 331);
        Arc(hdc, 24, 322, 28, 328, 24, 325, 28, 325);
        
        SelectObject(hdc, oldPen);
        DeleteObject(lockPen);

        FotoSCIhopStyles::DrawThemedText(hdc, INTERFACE_LOCKEDCOLORSSTR, 40, 316, 150, 20, true);
    }
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

void DisplayCellWithFrame(HDC hdc, int index) {
    if (!globalPicture || index < 0 || index >= globalPicture->CellsCount()) return;

    void* cellPtr = globalPicture->cells[index];
    if (!cellPtr) return;

    // Refresh bitmap data if needed (same as original DisplayCell)
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

    // Draw frame AFTER bitmap data is confirmed valid
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    POINT origin = GetDisplayOrigin();
    
    int imageWidth = ScaleCoordinate(globalPicture->cells[index]->bmInfo->bmiHeader.biWidth, MagnifyFactor);
    int imageHeight = ScaleCoordinate(abs(globalPicture->cells[index]->bmInfo->bmiHeader.biHeight), MagnifyFactor);
    
    RECT frameRect = {
        origin.x + ScaleCoordinate(bCell->xpos, MagnifyFactor) - UI_PADDING,
        origin.y + ScaleCoordinate(bCell->ypos, MagnifyFactor) - UI_PADDING,
        origin.x + ScaleCoordinate(bCell->xpos, MagnifyFactor) + imageWidth + UI_PADDING,
        origin.y + ScaleCoordinate(bCell->ypos, MagnifyFactor) + imageHeight + UI_PADDING
    };
    
    // Draw shadow
    RECT shadowRect = frameRect;
    OffsetRect(&shadowRect, 2, 2);
    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    // Draw frame
    FotoSCIhopStyles::DrawThemedFrame(hdc, frameRect);

    // Display the actual image
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

void DisplayCurrentViewWithFrame(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    // Original display logic first
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

    // NOW draw frame - after we know bitmap data is valid
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    POINT origin = GetDisplayOrigin();
    
    int imageWidth = ScaleCoordinate((*curCell)->bmInfo->bmiHeader.biWidth, MagnifyFactor);
    int imageHeight = ScaleCoordinate(abs((*curCell)->bmInfo->bmiHeader.biHeight), MagnifyFactor);
    
    RECT frameRect = {
        origin.x - UI_PADDING,
        origin.y - UI_PADDING,
        origin.x + imageWidth + UI_PADDING,
        origin.y + imageHeight + UI_PADDING
    };
    
    // Draw shadow
    RECT shadowRect = frameRect;
    OffsetRect(&shadowRect, 2, 2);
    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    // Draw frame
    FotoSCIhopStyles::DrawThemedFrame(hdc, frameRect);

    // Then display the actual image
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

void DisplayCurrentPicWithFrame(HDC hdc) {
    if (!globalPicture) return;

    if (curCellIndex == 0) {
        // Composite mode - display all cells WITHOUT individual cell frames
        // The composite view should show all cells naturally without extra framing
        for (int i = 0; i < globalPicture->CellsCount(); i++) {
            DisplayCell(hdc, i);
        }
    } else {
        // Individual cell mode - display specific cell WITH frame
        DisplayCellWithFrame(hdc, curCellIndex);
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
    Palette *tpalette = (isPicture ? globalPicture->palSCI : globalView->palSCI);
    
    if (!tpalette) {
        FotoSCIhopStyles::DrawThemedText(hdc, INTERFACE_MISSINGPALETTE, 30, 300, 160, 20, true);
        return;
    }

    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();

    // Themed background for palette area
    RECT paletteBackground = {
        UI_LEFT_MARGIN - 4, 
        UI_TOP_MARGIN - 4, 
        UI_LEFT_MARGIN + (PALETTE_COLORS_PER_ROW * PALETTE_CELL_WIDTH) + 4, 
        UI_TOP_MARGIN + (PALETTE_COLORS_PER_ROW * PALETTE_CELL_HEIGHT) + 4
    };
    FotoSCIhopStyles::DrawRoundedRect(hdc, paletteBackground, colors.surface, colors.border);

    // Draw palette grid with theme colors
    for (int i = 0; i < PALETTE_COLORS_PER_ROW; i++) {
        for (int j = 0; j < PALETTE_COLORS_PER_ROW; j++) {
            int colorIndex = i * PALETTE_COLORS_PER_ROW + j;
            PalEntry *tentry = tpalette->GetPalEntry(colorIndex);
            
            if (!tentry) continue;

            RECT cellRect = {
                UI_LEFT_MARGIN + (j * PALETTE_CELL_WIDTH), 
                UI_TOP_MARGIN + (i * PALETTE_CELL_HEIGHT), 
                UI_LEFT_MARGIN + (j * PALETTE_CELL_WIDTH) + PALETTE_CELL_DISPLAY_SIZE, 
                UI_TOP_MARGIN + (i * PALETTE_CELL_HEIGHT) + PALETTE_CELL_DISPLAY_SIZE
            };

            bool isOutOfRange = (colorIndex < tpalette->Head.startOffset) ||
                               (colorIndex >= tpalette->Head.startOffset + tpalette->Head.nColors);

            COLORREF cellColor = RGB(tentry->red, tentry->green, tentry->blue);
            
            // Draw the color cell
            COLORREF borderColor = isOutOfRange ? colors.error : colors.border;
            FotoSCIhopStyles::DrawRoundedRect(hdc, cellRect, cellColor, borderColor, 3);

            // Remap indicator
            if (tentry->remap == 1) {
                RECT remapRect = {cellRect.left, cellRect.bottom + 1, cellRect.right, cellRect.bottom + 4};
                FotoSCIhopStyles::DrawRoundedRect(hdc, remapRect, colors.warning, colors.warning, 1);
            }

            // Invalid color indicator
            if (isOutOfRange) {
                HPEN errorPen = CreatePen(PS_SOLID, 2, colors.error);
                HPEN oldPen = (HPEN)SelectObject(hdc, errorPen);
                
                MoveToEx(hdc, cellRect.left + 2, cellRect.top + 2, NULL);
                LineTo(hdc, cellRect.right - 2, cellRect.bottom - 2);
                MoveToEx(hdc, cellRect.right - 2, cellRect.top + 2, NULL);
                LineTo(hdc, cellRect.left + 2, cellRect.bottom - 2);
                
                SelectObject(hdc, oldPen);
                DeleteObject(errorPen);
            }
        }
    }

    DrawPaletteStatusIndicators(hdc, tpalette);
}

void DrawCellInfo(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    CelBase *bCell = (CelBase *)&(*curCell)->Head;
    
    // Themed info panel background
    RECT infoPanel = {10, 2, 500, 23};
    FotoSCIhopStyles::DrawThemedFrame(hdc, infoPanel);
    
    char textBuffer[128];
    int xPos = 20;

    // View-specific information (V56 files only)
    if (globalView) {
        sprintf(textBuffer, "Loop %d/%d", curLoopIndex + 1, globalView->Head.view32.loopCount);
        FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 80, 15);
        xPos += 85;

        // Safe loop access - only for V56 files
        if (curLoop && (*curLoop)) {
            if ((*curLoop)->Head.flags) {
                sprintf(textBuffer, "Mirror -> %d", (*curLoop)->Head.altLoop + 1);
                FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 120, 15, true);
            } else {
                sprintf(textBuffer, "Cell %d/%d", curCellIndex + 1, (*curLoop)->Head.numCels);
                FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 80, 15);
            }
        }
        xPos += 125;
        
        // Skip color info for V56 files - only show if not mirrored loop
        if (curLoop && (*curLoop) && !(*curLoop)->Head.flags) {
            sprintf(textBuffer, "Skip: %d", bCell->skip);
            FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 60, 15);
            
            if ((*curCell)->bmInfo && bCell->skip < PALETTE_TOTAL_COLORS) {
                RGBQUAD skipColorQuad = (*curCell)->bmInfo->bmiColors[bCell->skip];
                RECT swatchRect = {xPos + 65, 7, xPos + 80, 17};
                COLORREF swatchColor = RGB(skipColorQuad.rgbRed, skipColorQuad.rgbGreen, skipColorQuad.rgbBlue);
                FotoSCIhopStyles::DrawRoundedRect(hdc, swatchRect, swatchColor, colors.border, 2);
            }
            xPos += 85;
        }
    }

    // Picture-specific information (P56 files only)
    if (globalPicture) {
        const char* versionStr = (globalPicture->format == _PIC_11) ? "SCI1.1" : "SCI32";
        FotoSCIhopStyles::DrawThemedText(hdc, versionStr, xPos, 5, 80, 15);
        xPos += 85;

        sprintf(textBuffer, "Cell %d/%d", curCellIndex + 1, globalPicture->CellsCount());
        FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 80, 15);
        xPos += 85;
        
        // Skip color info for P56 files - always show since pictures don't have loops
        sprintf(textBuffer, "Skip: %d", bCell->skip);
        FotoSCIhopStyles::DrawThemedText(hdc, textBuffer, xPos, 5, 60, 15);
        
        if ((*curCell)->bmInfo && bCell->skip < PALETTE_TOTAL_COLORS) {
            RGBQUAD skipColorQuad = (*curCell)->bmInfo->bmiColors[bCell->skip];
            RECT swatchRect = {xPos + 65, 7, xPos + 80, 17};
            COLORREF swatchColor = RGB(skipColorQuad.rgbRed, skipColorQuad.rgbGreen, skipColorQuad.rgbBlue);
            FotoSCIhopStyles::DrawRoundedRect(hdc, swatchRect, swatchColor, colors.border, 2);
        }
        xPos += 85;
    }

    // Changed indicator with theme color - safe for both file types
    if ((*curCell)->changed) {
        FotoSCIhopStyles::DrawStatusText(hdc, "* Modified", xPos, 5, 80, 15, FotoSCIhopStyles::STATUS_WARNING);
    }
}

void EnsureScrollBarsAfterLoad() {
    // Force image data to be loaded if not already
    if (curCell && (*curCell)) {
        if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        // ADDITIONAL: Force a small delay to ensure image data is fully processed
        Sleep(50);  // 50ms delay to ensure bitmap data is ready
    }
    
    // Update scroll bars with current image data
    UpdateScrollBars();
    
    #ifdef _DEBUG
    char debugMsg[256];
    sprintf(debugMsg, "[DEBUG] EnsureScrollBarsAfterLoad: maxScrollX=%d, maxScrollY=%d, clientW/H=%d/%d\n", 
            g_maxScrollX, g_maxScrollY, g_clientWidth, g_clientHeight);
    OutputDebugStringA(debugMsg);
    #endif
}

int FindZoomIndex(int percentage) {
    // Find the first zoom level that's >= the requested percentage
    for (int i = 0; i < ZOOM_LEVEL_COUNT; i++) {
        if (ZOOM_LEVELS[i] >= percentage) {
            return i;
        }
    }
    // If percentage is higher than max zoom level, return the highest index
    return ZOOM_LEVEL_COUNT - 1;
}

void SetZoomLevel(int zoomPercentage) {
    if (MagnifyFactor == zoomPercentage) {
        return; // No change needed
    }
    
    // Clamp to valid range
    zoomPercentage = max(ZOOM_LEVELS[0], min(ZOOM_LEVELS[ZOOM_LEVEL_COUNT - 1], zoomPercentage));
    
    MagnifyFactor = zoomPercentage;
    
    // CRITICAL: Synchronize the zoom index
    g_currentZoomIndex = FindZoomIndex(zoomPercentage);
    
    // Update menu checkmarks
    HMENU menu = GetMenu(hWnd);
    if (menu) {
        CheckMenuItem(menu, ID_INGRANDIMENTO_NORMALE, MF_UNCHECKED);
        CheckMenuItem(menu, ID_INGRANDIMENTO_X2, MF_UNCHECKED);
        CheckMenuItem(menu, ID_INGRANDIMENTO_X3, MF_UNCHECKED);
        CheckMenuItem(menu, ID_INGRANDIMENTO_X4, MF_UNCHECKED);
        
        // Check appropriate menu item based on zoom level
        if (zoomPercentage == gBaseMagnify) {
            CheckMenuItem(menu, ID_INGRANDIMENTO_NORMALE, MF_CHECKED);
        } else if (zoomPercentage == gBaseMagnify * 2) {
            CheckMenuItem(menu, ID_INGRANDIMENTO_X2, MF_CHECKED);
        } else if (zoomPercentage == gBaseMagnify * 3) {
            CheckMenuItem(menu, ID_INGRANDIMENTO_X3, MF_CHECKED);
        } else if (zoomPercentage == gBaseMagnify * 4) {
            CheckMenuItem(menu, ID_INGRANDIMENTO_X4, MF_CHECKED);
        }
    }
    
    // Update scroll bars first
    UpdateScrollBars();
    
    // CRITICAL: Always force a complete window redraw after zoom changes
    // This fixes the issue where zoom changes don't render until window resize
    InvalidateRect(hWnd, NULL, FALSE);
    
    #ifdef _DEBUG
    char debugMsg[128];
    sprintf(debugMsg, "[DEBUG] SetZoomLevel: %d%% -> Index %d, Forced invalidation\n", 
            zoomPercentage, g_currentZoomIndex);
    OutputDebugStringA(debugMsg);
    #endif
}

void ZoomIn() {
    if (g_currentZoomIndex < ZOOM_LEVEL_COUNT - 1) {
        g_currentZoomIndex++;
        SetZoomLevel(ZOOM_LEVELS[g_currentZoomIndex]);
    }
}

void ZoomOut() {
    if (g_currentZoomIndex > 0) {
        g_currentZoomIndex--;
        SetZoomLevel(ZOOM_LEVELS[g_currentZoomIndex]);
    }
}

void ZoomToFit() {
    if (!curCell || !(*curCell) || !(*curCell)->bmInfo) return;
    
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int imageWidth = (*curCell)->bmInfo->bmiHeader.biWidth;
    int imageHeight = abs((*curCell)->bmInfo->bmiHeader.biHeight);
    
    // Calculate zoom to fit both dimensions with some padding
    int availableWidth = clientRect.right - 300; // Account for palette space
    int availableHeight = clientRect.bottom - 100; // Account for top bar and info
    
    int zoomX = (availableWidth * 100) / imageWidth;
    int zoomY = (availableHeight * 100) / imageHeight;
    
    int fitZoom = min(zoomX, zoomY);
    fitZoom = max(25, min(1600, fitZoom)); // Clamp to reasonable range
    
    // Find closest zoom level
    for (int i = 0; i < ZOOM_LEVEL_COUNT; i++) {
        if (ZOOM_LEVELS[i] >= fitZoom || i == ZOOM_LEVEL_COUNT - 1) {
            g_currentZoomIndex = i;
            break;
        }
    }
    
    SetZoomLevel(ZOOM_LEVELS[g_currentZoomIndex]);
}

void ZoomTo100() {
    for (int i = 0; i < ZOOM_LEVEL_COUNT; i++) {
        if (ZOOM_LEVELS[i] == 100) {
            g_currentZoomIndex = i;
            break;
        }
    }
    SetZoomLevel(100);
}

// ============================================================================
// SCROLL MANAGEMENT FUNCTIONS
// ============================================================================

void UpdateScrollBars() {
    if (!hWnd) return;
    
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    g_clientWidth = clientRect.right;
    g_clientHeight = clientRect.bottom;
    
    // Calculate content size based on actual image positioning and zoom
    int contentWidth = 600;   // Default minimum
    int contentHeight = 400;
    
    if (curCell && (*curCell) && (*curCell)->bmInfo) {
        int imageWidth = ScaleCoordinate((*curCell)->bmInfo->bmiHeader.biWidth, MagnifyFactor);
        int imageHeight = ScaleCoordinate(abs((*curCell)->bmInfo->bmiHeader.biHeight), MagnifyFactor);
        
        // Account for actual image positioning
        int imageStartX = UI_LEFT_MARGIN + picX + tableX;
        int imageStartY = UI_TOP_MARGIN + picY;
        
        // VIEW FILES: Special handling for view layout
        if (globalView) {
            // For view files, image is positioned at xHot/yHot offset
            CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
            
            // View images are centered around their hot spot
            int xHot = ScaleCoordinate(bCell->xHot, MagnifyFactor);
            int yHot = ScaleCoordinate(bCell->yHot, MagnifyFactor);
            
            // FIXED: Simpler, more reliable calculation for views
            // The image extends from its origin to its full dimensions
            int imageRight = imageStartX + imageWidth;
            int imageBottom = imageStartY + imageHeight;
            
            // Add reasonable margins
            contentWidth = imageRight + 100;
            contentHeight = imageBottom + 100;
        }
        // PICTURE FILES: Use existing logic
        else if (globalPicture && curCellIndex > 0) {
            // Individual cell - use its actual position
            CelHeaderPic *bCell = (CelHeaderPic *)&(*curCell)->Head;
            imageStartX += ScaleCoordinate(bCell->xpos, MagnifyFactor);
            imageStartY += ScaleCoordinate(bCell->ypos, MagnifyFactor);
            
            contentWidth = imageStartX + imageWidth + 100;
            contentHeight = imageStartY + imageHeight + 100;
        } else if (globalPicture && curCellIndex == 0) {
            // Composite view - calculate bounds of all cells
            int minX = 0, maxX = imageWidth;
            int minY = 0, maxY = imageHeight;
            
            for (int i = 0; i < globalPicture->CellsCount(); i++) {
                if (globalPicture->cells[i] && globalPicture->cells[i]->bmInfo) {
                    CelHeaderPic *cellHeader = (CelHeaderPic *)&globalPicture->cells[i]->Head;
                    int cellWidth = ScaleCoordinate(globalPicture->cells[i]->bmInfo->bmiHeader.biWidth, MagnifyFactor);
                    int cellHeight = ScaleCoordinate(abs(globalPicture->cells[i]->bmInfo->bmiHeader.biHeight), MagnifyFactor);
                    int cellX = ScaleCoordinate(cellHeader->xpos, MagnifyFactor);
                    int cellY = ScaleCoordinate(cellHeader->ypos, MagnifyFactor);
                    
                    minX = min(minX, cellX);
                    minY = min(minY, cellY);
                    maxX = max(maxX, cellX + cellWidth);
                    maxY = max(maxY, cellY + cellHeight);
                }
            }
            
            imageWidth = maxX - minX;
            imageHeight = maxY - minY;
            imageStartX += minX;
            imageStartY += minY;
            
            contentWidth = imageStartX + imageWidth + 100;
            contentHeight = imageStartY + imageHeight + 100;
        }
        
        // Ensure minimum content size but don't go crazy
        contentWidth = max(contentWidth, g_clientWidth);
        contentHeight = max(contentHeight, g_clientHeight);
        
        // SAFETY: Cap content size to prevent infinite scrolling
        contentWidth = min(contentWidth, g_clientWidth * 10);  // Max 10x window size
        contentHeight = min(contentHeight, g_clientHeight * 10); // Max 10x window size
    }
    
    // Store old values to check if update is needed
    int oldMaxScrollX = g_maxScrollX;
    int oldMaxScrollY = g_maxScrollY;
    
    // Calculate max scroll values
    g_maxScrollX = max(0, contentWidth - g_clientWidth);
    g_maxScrollY = max(0, contentHeight - g_clientHeight);
    
    // Clamp current scroll position to valid range
    g_scrollX = max(0, min(g_scrollX, g_maxScrollX));
    g_scrollY = max(0, min(g_scrollY, g_maxScrollY));
    
    // CRITICAL: Add validation for scroll info values
    #ifdef _DEBUG
    char debugMsg[512];
    sprintf(debugMsg, "[DEBUG] ScrollInfo - ClientW/H: %d/%d, ContentW/H: %d/%d, MaxScrollX/Y: %d/%d, ScrollX/Y: %d/%d\n", 
            g_clientWidth, g_clientHeight, contentWidth, contentHeight, g_maxScrollX, g_maxScrollY, g_scrollX, g_scrollY);
    OutputDebugStringA(debugMsg);
    #endif
    
    // Set up horizontal scroll bar
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = contentWidth - 1;  // IMPORTANT: Windows expects nMax to be contentWidth - 1
    si.nPage = g_clientWidth;
    si.nPos = g_scrollX;
    
    // VALIDATION: Ensure values make sense for horizontal scroll
    if (si.nPage >= si.nMax) {
        si.nMax = si.nPage + 1;  // Ensure nMax > nPage for thumb to appear
    }
    
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
    
    // Set up vertical scroll bar with same validation
    si.nMax = contentHeight - 1;  // IMPORTANT: Windows expects nMax to be contentHeight - 1
    si.nPage = g_clientHeight;
    si.nPos = g_scrollY;
    
    // CRITICAL: Ensure values make sense for vertical scroll
    if (si.nPage >= si.nMax) {
        si.nMax = si.nPage + 1;  // Ensure nMax > nPage for thumb to appear
    }
    
    #ifdef _DEBUG
    sprintf(debugMsg, "[DEBUG] VerticalScrollInfo - nMin: %d, nMax: %d, nPage: %d, nPos: %d\n", 
            si.nMin, si.nMax, si.nPage, si.nPos);
    OutputDebugStringA(debugMsg);
    #endif
    
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    
    // Show/hide scroll bars and force immediate update
    BOOL needsHorzScroll = (g_maxScrollX > 0);
    BOOL needsVertScroll = (g_maxScrollY > 0);
    
    ShowScrollBar(hWnd, SB_HORZ, needsHorzScroll);
    ShowScrollBar(hWnd, SB_VERT, needsVertScroll);
    
    // CRITICAL: Force immediate window frame update to show/hide scroll bars
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    
    // Only invalidate for SCROLL RANGE changes here
    // Zoom-related invalidation is now handled in SetZoomLevel()
    if (oldMaxScrollX != g_maxScrollX || oldMaxScrollY != g_maxScrollY) {
        #ifdef _DEBUG
        OutputDebugStringA("[DEBUG] UpdateScrollBars: Scroll ranges changed, invalidating\n");
        #endif
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void ScrollBy(int deltaX, int deltaY) {
    int newScrollX = g_scrollX + deltaX;
    int newScrollY = g_scrollY + deltaY;
    
    newScrollX = max(0, min(newScrollX, g_maxScrollX));
    newScrollY = max(0, min(newScrollY, g_maxScrollY));
    
    if (newScrollX != g_scrollX || newScrollY != g_scrollY) {
        g_scrollX = newScrollX;
        g_scrollY = newScrollY;
        
        // Update scroll bar positions with immediate redraw
        SetScrollPos(hWnd, SB_HORZ, g_scrollX, TRUE);  // TRUE = immediate redraw
        SetScrollPos(hWnd, SB_VERT, g_scrollY, TRUE);  // TRUE = immediate redraw
        
        // Invalidate the full content area INCLUDING under zoom controls
        // The zoom controls will be redrawn on top
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        
        // Invalidate everything except the top bar
        RECT contentArea = {0, 25, clientRect.right, clientRect.bottom};
        InvalidateRect(hWnd, &contentArea, FALSE);
    }
}

void ScrollTo(int x, int y) {
    ScrollBy(x - g_scrollX, y - g_scrollY);
}

// ============================================================================
// ENHANCED COORDINATE FUNCTIONS
// ============================================================================

// Updated GetDisplayOrigin to account for scrolling
static POINT GetDisplayOriginWithScroll() {
    POINT origin = GetDisplayOrigin();
    origin.x -= g_scrollX;
    origin.y -= g_scrollY;
    return origin;
}

// Convert screen coordinates to image coordinates
POINT ScreenToImageCoords(int screenX, int screenY) {
    POINT origin = GetDisplayOriginWithScroll();
    POINT imagePoint;
    
    imagePoint.x = ((screenX - origin.x) * 100) / MagnifyFactor;
    imagePoint.y = ((screenY - origin.y) * 100) / MagnifyFactor;
    
    return imagePoint;
}

// Convert image coordinates to screen coordinates  
POINT ImageToScreenCoords(int imageX, int imageY) {
    POINT origin = GetDisplayOriginWithScroll();
    POINT screenPoint;
    
    screenPoint.x = origin.x + ScaleCoordinate(imageX, MagnifyFactor);
    screenPoint.y = origin.y + ScaleCoordinate(imageY, MagnifyFactor);
    
    return screenPoint;
}

// ============================================================================
// PANNING SUPPORT
// ============================================================================

void StartPanning(int x, int y) {
    g_isPanning = true;
    g_lastPanPoint.x = x;
    g_lastPanPoint.y = y;
    SetCapture(hWnd);
    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
}

void UpdatePanning(int x, int y) {
    if (!g_isPanning) return;
    
    int deltaX = g_lastPanPoint.x - x;
    int deltaY = g_lastPanPoint.y - y;
    
    // Use the smooth scrolling version
    ScrollBy(deltaX, deltaY);
    
    g_lastPanPoint.x = x;
    g_lastPanPoint.y = y;
}

void StopPanning() {
    if (g_isPanning) {
        g_isPanning = false;
        ReleaseCapture();
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

// ============================================================================
// ZOOM UI CONTROLS
// ============================================================================

void DrawZoomControls(HDC hdc) {
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    
    // Zoom control panel
    RECT zoomPanel = {g_clientWidth - 180, 30, g_clientWidth - 10, 80};
    FotoSCIhopStyles::DrawThemedFrame(hdc, zoomPanel);
    
    // Zoom percentage text
    char zoomText[64];
    sprintf(zoomText, "Zoom: %d%%", MagnifyFactor);
    FotoSCIhopStyles::DrawThemedText(hdc, zoomText, zoomPanel.left + 10, zoomPanel.top + 5, 100, 20);
    
    // Zoom buttons
    RECT zoomOutBtn = {zoomPanel.left + 10, zoomPanel.top + 25, zoomPanel.left + 35, zoomPanel.top + 45};
    RECT zoomInBtn = {zoomPanel.left + 40, zoomPanel.top + 25, zoomPanel.left + 65, zoomPanel.top + 45};
    RECT fitBtn = {zoomPanel.left + 70, zoomPanel.top + 25, zoomPanel.left + 100, zoomPanel.top + 45};
    RECT resetBtn = {zoomPanel.left + 105, zoomPanel.top + 25, zoomPanel.left + 135, zoomPanel.top + 45};
    
    FotoSCIhopStyles::DrawThemedButton(hdc, zoomOutBtn, "-", false, false, g_currentZoomIndex > 0);
    FotoSCIhopStyles::DrawThemedButton(hdc, zoomInBtn, "+", false, false, g_currentZoomIndex < ZOOM_LEVEL_COUNT - 1);
    FotoSCIhopStyles::DrawThemedButton(hdc, fitBtn, "Fit", false, false, true);
    FotoSCIhopStyles::DrawThemedButton(hdc, resetBtn, "100%", false, false, true);
}

bool HandleZoomControlClick(int x, int y) {
    RECT zoomPanel = {g_clientWidth - 180, 30, g_clientWidth - 10, 80};
    
    if (x < zoomPanel.left || x > zoomPanel.right || y < zoomPanel.top || y > zoomPanel.bottom) {
        return false; // Click not in zoom control area
    }
    
    RECT zoomOutBtn = {zoomPanel.left + 10, zoomPanel.top + 25, zoomPanel.left + 35, zoomPanel.top + 45};
    RECT zoomInBtn = {zoomPanel.left + 40, zoomPanel.top + 25, zoomPanel.left + 65, zoomPanel.top + 45};
    RECT fitBtn = {zoomPanel.left + 70, zoomPanel.top + 25, zoomPanel.left + 100, zoomPanel.top + 45};
    RECT resetBtn = {zoomPanel.left + 105, zoomPanel.top + 25, zoomPanel.left + 135, zoomPanel.top + 45};
    
    if (PtInRect(&zoomOutBtn, {x, y})) {
        ZoomOut();
        return true;
    }
    if (PtInRect(&zoomInBtn, {x, y})) {
        ZoomIn();
        return true;
    }
    if (PtInRect(&fitBtn, {x, y})) {
        ZoomToFit();
        return true;
    }
    if (PtInRect(&resetBtn, {x, y})) {
        ZoomTo100();
        return true;
    }
    
    return false;
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
            SetZoomLevel(gBaseMagnify);
            break;
            
        case ID_INGRANDIMENTO_X2:
            SetZoomLevel(gBaseMagnify * 2);
            break;
            
        case ID_INGRANDIMENTO_X3:
            SetZoomLevel(gBaseMagnify * 3);
            break;
            
        case ID_INGRANDIMENTO_X4:
            SetZoomLevel(gBaseMagnify * 4);
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

        // Draw zoom controls
        DrawZoomControls(hdcBuffer);

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