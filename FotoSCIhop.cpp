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
#define MAX_LOADSTRING 100
#include "imgui_integration.h"
#include "fotoscihop_styles.h"




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
// REFERENCE IMAGE SETTINGS
// ============================================================================
//HWND hReferenceDialog;

// ============================================================================
// DIALOG WINDOWS
// ============================================================================
HWND hPropertiesDialog = NULL;
HWND hLinkPointDialog = NULL;

void RenderPropertiesDialog();

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

BOOL DoFileOpen(HWND hwnd, char *filename, char *ext)
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
      
	  if (!stricmp((ext==NULL?szFileName+ofn.nFileExtension:ext), "v56"))
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

				char *emsg;
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

				// Close any old dialog windows
				DestroyWindow(hPropertiesDialog);
				hPropertiesDialog = NULL;

				DestroyWindow(hLinkPointDialog);
				hLinkPointDialog = NULL;
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

				char *emsg=0;
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

				// Close any old dialog windows
				DestroyWindow(hPropertiesDialog);
				hPropertiesDialog = NULL;

				DestroyWindow(hLinkPointDialog);
				hLinkPointDialog = NULL;
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
		


		if (!stricmp(szNextFileName, fpath))
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
				if (!stricmp(FindFileData.cFileName, fname))
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

BOOL DoFileExport(HWND hwnd)
{
   OPENFILENAME ofn;
   char szBMPFileName[MAX_PATH] = "";

   ZeroMemory(&ofn, sizeof(ofn));

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = INTERFACE_BMPFILTER;
   ofn.lpstrFile = szBMPFileName;
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = "bmp";

   ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
               OFN_OVERWRITEPROMPT;
         
   if(GetSaveFileName(&ofn))
   {
		FILE *tempfile = fopen(szBMPFileName,"wb");
		if (tempfile)
		{
			BITMAPFILEHEADER tfileheader;
			tfileheader.bfType='MB';
			tfileheader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+256*(sizeof(RGBQUAD));
			tfileheader.bfSize=tfileheader.bfOffBits+(*curCell)->bmInfo->bmiHeader.biSizeImage;
			tfileheader.bfReserved1=0;
			tfileheader.bfReserved2=0;

			fwrite(&tfileheader, sizeof(BITMAPFILEHEADER),1,tempfile);
			BITMAPINFOHEADER tbmiheader;
            memcpy(&tbmiheader, &((*curCell)->bmInfo->bmiHeader),sizeof(BITMAPINFOHEADER));
            tbmiheader.biHeight = abs(tbmiheader.biHeight);
   
            fwrite(&tbmiheader,sizeof(BITMAPINFOHEADER),1,tempfile);
			fwrite((*curCell)->bmInfo->bmiColors,256*sizeof(RGBQUAD),1,tempfile);
			
            long mywidth = tbmiheader.biSizeImage / tbmiheader.biHeight; 
            
            for (long i=tbmiheader.biHeight-1; i>=0; i--)
                fwrite((void *)((unsigned long)(*curCell)->bmImage + i*mywidth),mywidth,1,tempfile);

			fclose(tempfile);
		} else
		{
           MessageBox(hWnd, ERR_CANTEXPORTBMP, ERR_TITLE,
                            MB_OK | MB_ICONSTOP);
           return FALSE;
        }

   }

   return TRUE;
}

// Dhel
BOOL CLIFileExport(char BMPFileName[MAX_PATH])
{
   if(BMPFileName)
   {
		FILE *tempfile = fopen(BMPFileName,"wb");
		if (tempfile)
		{
			BITMAPFILEHEADER tfileheader;
			tfileheader.bfType='MB';
			tfileheader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+256*(sizeof(RGBQUAD));
			tfileheader.bfSize=tfileheader.bfOffBits+(*curCell)->bmInfo->bmiHeader.biSizeImage;
			tfileheader.bfReserved1=0;
			tfileheader.bfReserved2=0;

			fwrite(&tfileheader, sizeof(BITMAPFILEHEADER),1,tempfile);
			BITMAPINFOHEADER tbmiheader;
            memcpy(&tbmiheader, &((*curCell)->bmInfo->bmiHeader),sizeof(BITMAPINFOHEADER));
            tbmiheader.biHeight = abs(tbmiheader.biHeight);
   
            fwrite(&tbmiheader,sizeof(BITMAPINFOHEADER),1,tempfile);
			fwrite((*curCell)->bmInfo->bmiColors,256*sizeof(RGBQUAD),1,tempfile);
			
            long mywidth = tbmiheader.biSizeImage / tbmiheader.biHeight; 
            
            for (long i=tbmiheader.biHeight-1; i>=0; i--)
                fwrite((void *)((unsigned long)(*curCell)->bmImage+ i*mywidth),mywidth,1,tempfile);

			fclose(tempfile);
		} 
   }

   return TRUE;
}

// Dhel
BOOL CLIFileImport(char BMPFileName[MAX_PATH])
{
   if(BMPFileName)
   {
		FILE *tempfile = fopen(BMPFileName,"rb");
		if (tempfile)
		{
			BITMAPFILEHEADER tfh;	
			fread(&tfh, sizeof(BITMAPFILEHEADER),1,tempfile);

			if (tfh.bfType!='MB')
			{
				//MessageBox(hWnd, ERR_INVALIDBMP, ERR_TITLE,
                 //           MB_OK | MB_ICONSTOP);
				fclose(tempfile);
				return FALSE;
			}

			BITMAPINFOHEADER tbih;
			fread(&tbih,sizeof(BITMAPINFOHEADER),1,tempfile);

			if (tbih.biBitCount!=8)
			{
				//MessageBox(hWnd, ERR_INVALIDCBITBMP, ERR_TITLE,
                //            MB_OK | MB_ICONSTOP);
				fclose(tempfile);
				return FALSE;
			}

			if (tbih.biCompression != BI_RGB)
			{
				//MessageBox(hWnd, ERR_INVALIDCOMPBMP, ERR_TITLE,
                //            MB_OK | MB_ICONSTOP);
				fclose(tempfile);
				return FALSE;
			}
			
			RGBQUAD tctab[256];
			fread(tctab,256*sizeof(RGBQUAD),1,tempfile);

			bool remap = false;

			/*
			if (memcmp(tctab, (*curCell)->bmInfo->bmiColors, 256 * sizeof(RGBQUAD)))
			{
				int btn;

				btn = MessageBox(hwnd, WARN_DIFFERENTPAL, WARN_ATTENTION,
								 MB_APPLMODAL | MB_ICONQUESTION | MB_YESNOCANCEL);

				if (btn == IDYES)
				{
					CLIPaletteImport(szBMPFileName);
					memcpy((*curCell)->bmInfo->bmiColors, tctab, 256 * sizeof(RGBQUAD));
				}

				if (btn == IDCANCEL)
					return FALSE;

				// remap = true;
			}
			*/

			/*
			{
				MessageBox(hWnd, ERR_DIFFERENTPALBIS, ERR_TITLE,
						   MB_OK | MB_ICONEXCLAMATION);
				// fclose(tempfile);
				// return FALSE;
			}
			*/

			// import palette automatically
			if (memcmp(tctab, (*curCell)->bmInfo->bmiColors, 256*sizeof(RGBQUAD)))			
				CLIPaletteImport (BMPFileName);

			bool isBottomTop=(tbih.biHeight>0);

			unsigned long newHeight = abs(tbih.biHeight);

			unsigned long expectedsize = tbih.biWidth*newHeight;
			int dwremainder = tbih.biWidth%4;
			if (dwremainder)
				expectedsize+= newHeight*(4-dwremainder); //bmp requires DWORD align for each scanline

			fseek(tempfile, 0, SEEK_END);
			unsigned long imsize = ftell(tempfile) -tfh.bfOffBits;

			fseek(tempfile, tfh.bfOffBits, SEEK_SET);

			if (expectedsize>imsize)//(expectedsize!=imsize)      changed to support Photoshop BMPs
			{
				//MessageBox(hWnd, ERR_INVALIDSIZEBMP, ERR_TITLE,
                //            MB_OK | MB_ICONSTOP);
				fclose(tempfile);
				return FALSE;
			}

			unsigned long newwidth = tbih.biWidth + (dwremainder ?4-dwremainder :0);
			unsigned char *timage = (unsigned char *) new char[imsize];
			if (!isBottomTop)
				fread(timage, imsize, 1, tempfile);
			else
				for (long i = newHeight-1; i>=0; i--)
					fread(&(timage[newwidth*i]),newwidth,1,tempfile);

			long tsizep = ((sizeof(BITMAPINFO)) + 256*(sizeof(RGBQUAD)));

			BITMAPINFO *tbinfo = (BITMAPINFO *) new char[tsizep];

			tbinfo->bmiHeader.biBitCount=8;
			tbinfo->bmiHeader.biClrImportant=256;
			tbinfo->bmiHeader.biClrUsed=256;
			tbinfo->bmiHeader.biCompression=BI_RGB;
			tbinfo->bmiHeader.biHeight=-newHeight; //so that it will be a top-down DIB
			tbinfo->bmiHeader.biPlanes=1;
			tbinfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
			tbinfo->bmiHeader.biSizeImage=imsize;
			tbinfo->bmiHeader.biWidth=tbih.biWidth;
			tbinfo->bmiHeader.biXPelsPerMeter=0; 
			tbinfo->bmiHeader.biYPelsPerMeter=0;

			for (int i = 0; i < 256; i++)
					tbinfo->bmiColors[i] = (*curCell)->bmInfo->bmiColors[i];

			if (curCell)
			{
				(*curCell)->SetImage(tbinfo, timage);
	
				//HMENU menu = GetMenu(hwnd);

				//EnableMenuItem(menu, ID_SALVA, MF_ENABLED);
				//datasaved = false;
			}
			else
			{
				delete[] timage;
				delete[] tbinfo;
			}

			fclose(tempfile);

			return TRUE;
		}

        //MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE,
		//					MB_OK | MB_ICONSTOP);
   }

   return TRUE;
}

BOOL DoFileImport(HWND hwnd)
{
	OPENFILENAME ofn;
	char szBMPFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	// szFileName[0] = 0;

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = INTERFACE_BMPFILTER;
	ofn.lpstrFile = szBMPFileName;
	ofn.nMaxFile = MAX_PATH;

	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileName(&ofn))
	{
		FILE *tempfile = fopen(szBMPFileName, "rb");
		if (tempfile)
		{
			if (!stricmp(szBMPFileName + ofn.nFileExtension, "bmp"))
			{
				CLIFileImport (szBMPFileName);

				InvalidateRect(hwnd, NULL, true);

				datasaved = false;

				//sprintf(cmd, "del %s", szBMPFileName);
				//system(cmd);

				return TRUE;
			}
		}

		MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE,
							MB_OK | MB_ICONSTOP);
	}

   return TRUE;
}

BOOL CLIPaletteImport(char *palette)
{
  FILE *tempfile = fopen(palette, "rb");
  if (tempfile)
  {
	  Palette *tnewpal;

	  // load a palette from a bitmap
	  BITMAPFILEHEADER tfh;
	  fread(&tfh, sizeof(BITMAPFILEHEADER), 1, tempfile);

	  if (tfh.bfType != 'MB')
	  {
		  // MessageBox(hWnd, ERR_INVALIDBMP, ERR_TITLE,
		  //		  MB_OK | MB_ICONSTOP);
		  fclose(tempfile);
		  return FALSE;
	  }

	  BITMAPINFOHEADER tbih;
	  fread(&tbih, sizeof(BITMAPINFOHEADER), 1, tempfile);

	  if (tbih.biBitCount != 8)
	  {
		 // MessageBox(hWnd, ERR_INVALIDCBITBMP, ERR_TITLE,
		//			 MB_OK | MB_ICONSTOP);
		  fclose(tempfile);
		  return FALSE;
	  }

	  if (tbih.biCompression != BI_RGB)
	  {
		//  MessageBox(hWnd, ERR_INVALIDCOMPBMP, ERR_TITLE,
		//			 MB_OK | MB_ICONSTOP);
		  fclose(tempfile);
		  return FALSE;
	  }

	  RGBQUAD tctab[256];
	  fread(tctab, 256 * sizeof(RGBQUAD), 1, tempfile);

	  if (isPicture)
		  tnewpal = globalPicture->palSCI;
	  else
		  tnewpal = globalView->palSCI;

	  for (int i = 0; i < 256; i++)
	  {
		  PalEntry *pe = tnewpal->GetPalEntry(i);
		  PalEntry npe;
		  npe.remap = (pe == NULL ? 0 : pe->remap);
		  npe.blue = tctab[i].rgbBlue;
		  npe.green = tctab[i].rgbGreen;
		  npe.red = tctab[i].rgbRed;
		  tnewpal->SetPalEntry(npe, i);
	  }

	  //(*curCell)->bmImage = 0;
	  //(*curCell)->bmInfo = 0;

	  if (isPicture)
	  {
		  // cycle for clearing images cache
		  for (int i = 0; i < globalPicture->CellsCount(); i++)
		  {
			  globalPicture->cells[i]->setPalette(&tnewpal);
		  }

		  ShowCell(curCellIndex);
	  }
	  else
	  {
		  // cycle for clearing images cache
		  for (int j = 0; j < globalView->Head.view32.loopCount; j++)
		  {
			Loop *tloop = globalView->loops[j];
			  for (int i = 0; i < tloop->Head.numCels; i++)
			  {
				  globalView->loops[j]->cells[i]->setPalette(&tnewpal);
			  }
		  }

		  Loop *tloop = globalView->loops[curLoopIndex];

		  ShowLoopCell(curLoopIndex, curCellIndex);
	  }

	  datasaved = false;

	  fclose(tempfile);
	  return TRUE;
  }

   //MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE,
	//		  MB_OK | MB_ICONSTOP);
}

BOOL DoPaletteImport(HWND hwnd)
{
   OPENFILENAME ofn;
   char szPALFileName[MAX_PATH] = "";

   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   //szFileName[0] = 0;

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = INTERFACE_PALINFILTER; 
   ofn.lpstrFile = szPALFileName;
   ofn.nMaxFile = MAX_PATH;

   ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
   if(GetOpenFileName(&ofn))
   {
		FILE *tempfile = fopen(szPALFileName,"rb");
		if (tempfile)
		{
            Palette *tnewpal;
            if (!stricmp(szPALFileName+ofn.nFileExtension, "bmp"))
			{
                //load a palette from a bitmap
                BITMAPFILEHEADER tfh;	
                fread(&tfh, sizeof(BITMAPFILEHEADER),1,tempfile);

                if (tfh.bfType!='MB')
                {
				  MessageBox(hWnd, ERR_INVALIDBMP, ERR_TITLE,
                            MB_OK | MB_ICONSTOP);
				  fclose(tempfile);
				  return FALSE;
			    }

                BITMAPINFOHEADER tbih;
			    fread(&tbih,sizeof(BITMAPINFOHEADER),1,tempfile);

			    if (tbih.biBitCount!=8)
			    {
				  MessageBox(hWnd, ERR_INVALIDCBITBMP, ERR_TITLE,
                            MB_OK | MB_ICONSTOP);
				  fclose(tempfile);
				  return FALSE;
			    }

			    if (tbih.biCompression != BI_RGB)
			    {
				  MessageBox(hWnd, ERR_INVALIDCOMPBMP, ERR_TITLE,
                            MB_OK | MB_ICONSTOP);
				  fclose(tempfile);
				  return FALSE;
			    }
			
			    RGBQUAD tctab[256];
			    fread(tctab,256*sizeof(RGBQUAD),1,tempfile);
                
                if (isPicture)
					tnewpal = globalPicture->palSCI;
                else
                    tnewpal = globalView->palSCI; 
                                                      
			    for (int i=0; i<256; i++)
				{
                   PalEntry *pe = tnewpal->GetPalEntry(i);
                   PalEntry npe;
                   npe.remap = (pe == NULL ? 0: pe->remap);
                   npe.blue = tctab[i].rgbBlue;
			       npe.green = tctab[i].rgbGreen;
                   npe.red = tctab[i].rgbRed;
                   tnewpal->SetPalEntry(npe, i);
                }
                
                
            } else {
              tnewpal= new Palette;
   
              fseek(tempfile, 0, SEEK_END);
              unsigned long tpsize = ftell(tempfile);
              fseek(tempfile, 0, SEEK_SET);
			  if (tnewpal->loadPalette(tempfile, tpsize))
			  {
				if (isPicture)
				{
					delete globalPicture->palSCI;
					globalPicture->palSCI = tnewpal;
                }
				else
				{
					delete globalView->palSCI;
					globalView->palSCI = tnewpal;	
				}
              }
			  else
			  {
				delete tnewpal;
				MessageBox(hwnd, ERR_CANTLOADPALETTE, ERR_TITLE,
							MB_OK | MB_ICONSTOP);
       
                fclose(tempfile);
			    return TRUE;
				
			  }

            }

            (*curCell)->bmImage = 0;
			(*curCell)->bmInfo = 0;
   
            if (isPicture)
            {   
                //cycle for clearing images cache
                for (int i=0; i<globalPicture->CellsCount(); i++)
				{
						globalPicture->cells[i]->setPalette(&tnewpal);
				}
     
                ShowCell(curCellIndex);                                                      
            } else
            {            
                 //cycle for clearing images cache
                for (int j=0; j<globalView->Head.view32.loopCount; j++)
				{
						Loop *tloop=globalView->loops[j];
						for (int i=0; i < tloop->Head.numCels; i++)
						{
							tloop->cells[i]->setPalette(&tnewpal);
							
						}
						
				}
				Loop *tloop = globalView->loops[curLoopIndex];

				ShowLoopCell(curLoopIndex, curCellIndex);
            }

			datasaved = false;
    
			InvalidateRect(hwnd, NULL, true); 

			fclose(tempfile);
			return TRUE;
		}

		MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE,
							MB_OK | MB_ICONSTOP);
   }
   return TRUE;
}

BOOL DoPaletteExport(HWND hwnd)
{
   OPENFILENAME ofn;
   char szPALFileName[MAX_PATH] = "";

   ZeroMemory(&ofn, sizeof(ofn));

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = INTERFACE_PALFILTER;
   ofn.lpstrFile = szPALFileName;
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = "pal";

   ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
               OFN_OVERWRITEPROMPT;
         
   if(GetSaveFileName(&ofn))
   {
		FILE *tempfile = fopen(szPALFileName,"wb");
		if (tempfile)
		{
			if (isPicture)
				globalPicture->palSCI->WritePalette(tempfile, true);
			else
				globalView->palSCI->WritePalette(tempfile, true);

			fclose(tempfile);
		} else
		{
           MessageBox(hWnd, ERR_CANTEXPORTPALETTE, ERR_TITLE,
                            MB_OK | MB_ICONSTOP);
           return FALSE;
        }

   }

   return TRUE; 
}

BOOL CALLBACK DoImportImageDlg(HWND hwndDlg,
							   UINT message,
							   WPARAM wParam,
							   LPARAM lParam)
{ 
	

    switch (message) 
    { 
        case WM_INITDIALOG:
        {
       		if (curCell)
            {
               SetDlgItemInt(hwndDlg, IDC_IMPORT_CLIMIT, colorLimit, TRUE);
               SetDlgItemInt(hwndDlg, IDC_IMPORT_TOLERANCE, tolerance, TRUE);
              
            }
        
            return TRUE;
        }

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:

				colorLimit = GetDlgItemInt(hwndDlg, IDC_IMPORT_CLIMIT, NULL, TRUE);
				tolerance = GetDlgItemInt(hwndDlg, IDC_IMPORT_TOLERANCE, NULL, TRUE);

			case IDCANCEL:
				EndDialog(hwndDlg, wParam);
				return TRUE;
			}
		}
	return FALSE; 
} 

int cliExport(char *name)
{
	int retVal = 0;

	if (globalView)
	{
		for (int l = 0; l < globalView->Head.view32.loopCount; l++)
		{
			Loop *tloop = globalView->loops[l];
			for (int c = 0; c < tloop->Head.numCels; c++)
			{
				ShowLoopCell(l, c);

				if (!tloop->Head.flags)
				{
					char cellName[MAX_PATH];

					sprintf(cellName, "%s-%d-%d.bmp", name, l + 1, c + 1);

					CLIFileExport(cellName);
				}
			}
		}
	}

	if (globalPicture)
	{
		for (int c = 0; c < globalPicture->CellsCount(); c++)
		{
			ShowCell(c);

			char cellName[MAX_PATH];

			sprintf(cellName, "%s-%d.bmp", name, c + 1);

			CLIFileExport(cellName);


		}
	}

	retVal = 1;

	return retVal;
}

int cliImport( char *name)
{
	int retVal = 0;

	if (globalView)
	{
		for (int l = 0; l < globalView->Head.view32.loopCount; l++)
		{
			Loop *tloop = globalView->loops[l];

			for (int c = 0; c < tloop->Head.numCels; c++)
			{
				ShowLoopCell(l, c);

				if (!tloop->Head.flags)
				{
					char cellName[MAX_PATH];

					sprintf(cellName, "%s-%d-%d.bmp", name, l + 1, c + 1);

					CLIPaletteImport(cellName);
					CLIFileImport(cellName);
				}
			}
		}
	}
	if (globalPicture)
	{

		for (int c = 0; c < globalPicture->CellsCount(); c++)
		{
			ShowCell(c);

			char cellName[MAX_PATH];

			sprintf(cellName, "%s-%d.bmp", name, c + 1);

			CLIPaletteImport(cellName);
			CLIFileImport(cellName);
		}
	}

	retVal = 1;

	return retVal;
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

typedef BOOL (WINAPI*Func)(HWND, char*, unsigned char, char*, char*);
Func ExtractFromVolume;

#ifdef __DEVC
int STDCALL WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
#else 
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPSTR     lpCmdLine,
                       int       nCmdShow)
#endif
{
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_IMMAGINA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Get app path - safer version of original logic
    GetModuleFileName(NULL, gAppPath, MAX_PATH);
    char* lastBackslash = strrchr(gAppPath, '\\');
    if (lastBackslash != NULL) {
        *lastBackslash = '\0';  // Safer than pointer arithmetic
    }

    LoadConfig();

    char startupfile[_MAX_PATH];
    memset(startupfile, 0, _MAX_PATH);

    if (gCliEnabled)
    {
        // Dhel - cli - Original logic preserved exactly
        if (lpCmdLine[0] != 0)
        {
            // tokenize arguments to array
            int i = 0;
            char *p = strtok(lpCmdLine, " ");

            while (p != NULL)
            {
                argv[i++] = p;
                p = strtok(NULL, " ");
            }

            if (lpCmdLine[0] == '\"')
            {
                size_t len = strlen(argv[0]);
                if (len > 2) {  // Safety check
                    strncpy(startupfile, argv[0] + 1, len - 2);
                    startupfile[len - 2] = 0;
                }
            }
            else
                strcpy(startupfile, argv[0]);

            // do cli processes - Original logic preserved
            if (argv[1])
            {
                DoFileOpen(hWnd, startupfile, startupfile + (strlen(startupfile) - 3));

                if (!strcmp(argv[1], "export"))
                {
                    cliExport(argv[2]);
                    return 0;
                }

                if (!strcmp(argv[1], "import") && argv[2])
                {
                    cliImport(argv[2]);

                    if (argv[3] && argv[4])
                        cliScale(atoi(argv[3]), atoi(argv[4]));

                    if (argv[5] && argv[6])
                        cliSetHeader(atoi(argv[5]), atoi(argv[6]));

                    DoFileSave(hWnd);
                    return 0;
                }

                if (!strcmp(argv[1], "scale"))
                {
                    if (argv[2] && argv[3])
                        cliScale(atoi(argv[2]), atoi(argv[3]));

                    DoFileSave(hWnd);
                    return 0;
                }

                if (!strcmp(argv[1], "header"))
                {
                    if (argv[2] && argv[3])
                    {
                        cliSetHeader(atoi(argv[2]), atoi(argv[3]));
                    }

                    DoFileSave(hWnd);
                    return 0;
                }

                if (!strcmp(argv[1], "addCells"))
                {
                    if (argv[2] && argv[3] && argv[4])
                        DoAddCells(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

                    DoFileSave(hWnd);
                    return 0;
                }

                if (!strcmp(argv[1], "addLoops"))
                {
                    if (argv[2] && argv[3])
                        DoAddLoops(atoi(argv[2]), atoi(argv[3]));

                    DoFileSave(hWnd);
                    return 0;
                }
            }
        }
    }
    
    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_IMMAGINA);

    // DLL Loading - Original logic with safety improvements
    #if defined _M_IX86
    HINSTANCE DLL = LoadLibrary("SCIdump.dll");
    /* check for error on loading the DLL */
    if (DLL == NULL) 
        MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
    else  // Only try to get function if DLL loaded successfully
    {
        ExtractFromVolume = (Func)GetProcAddress((HMODULE)DLL, "?ExtractFromVolumeSkel@@YAHPAUHWND__@@PADE11@Z");
        /* check for error on getting the function */
        if (ExtractFromVolume == NULL) 
        {
            FreeLibrary((HMODULE)DLL);
            DLL = NULL;  // Mark as invalid
            MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
        }
    }
    #endif

    // Original startup file handling
    if (lpCmdLine[0] != 0)
    {
        if (lpCmdLine[0] == '\"')
        {
            size_t len = strlen(lpCmdLine);
            if (len > 2) {  // Safety check
                strncpy(startupfile, lpCmdLine + 1, len - 2);
                startupfile[len - 2] = 0;
            }
        }
        else
            strcpy(startupfile, lpCmdLine);

        size_t pathLen = strlen(startupfile);
        if (pathLen >= 3) {  // Safety check
            DoFileOpen(hWnd, startupfile, startupfile + (pathLen - 3));
        }
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup - Original logic preserved
    #if defined _M_IX86
    if (DLL != NULL)  // Only free if we have a valid handle
        FreeLibrary((HMODULE)DLL);
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

    // Set up a timer for ImGui rendering (30 FPS - sufficient for dialogs)
    SetTimer(hWnd, 1, 33, NULL);

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
            DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
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
            DoFileImport(hWnd);
            break;
            
        case ID_ESPORTABMP:
            DoFileExport(hWnd);
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
            DoPaletteImport(hWnd);
            break;
            
        case ID_COLORI_ESPORTACOLORI:
            DoPaletteExport(hWnd);
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

    case WM_TIMER:
    if (wParam == 1) { // Our ImGui timer
        ImGuiDialogs::Render();
    }
    break;
        
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

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
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
    float availableWidth = GetContentRegionAvailWidth();
    float buttonWidth = availableWidth * 0.22f; // 22% for each button

    // =========================================================================
    // FILE INFO SECTION (just adding colors)
    // =========================================================================
    if (CollapsingHeader("File Information", true)) {
        char textBuffer[256];
        
        if (globalView) {
            TextColored(0.8f, 0.9f, 1.0f, 1.0f, "File Type: View File (.v56)");  // Light blue
            sprintf(textBuffer, "Current Loop: %d / %d", curLoopIndex + 1, globalView->Head.view32.loopCount);
            Text(textBuffer);
            
            if (curLoop && (*curLoop)) {
                if ((*curLoop)->Head.flags) {
                    sprintf(textBuffer, "Loop Type: Mirror of Loop %d", (*curLoop)->Head.altLoop + 1);
                    TextColored(1.0f, 0.8f, 0.3f, 1.0f, textBuffer);  // Orange
                } else {
                    sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, (*curLoop)->Head.numCels);
                    Text(textBuffer);
                    TextColored(0.3f, 1.0f, 0.3f, 1.0f, "Loop Type: Normal");  // Green
                }
            }
        } else if (globalPicture) {
            const char* version = (globalPicture->format == _PIC_11) ? "SCI1.1 Picture" : "SCI32 Picture";
            sprintf(textBuffer, "File Type: %s (.p56)", version);
            TextColored(0.8f, 0.9f, 1.0f, 1.0f, textBuffer);  // Light blue
            sprintf(textBuffer, "Current Cell: %d / %d", curCellIndex + 1, globalPicture->CellsCount());
            Text(textBuffer);
        } else {
            TextColored(1.0f, 0.5f, 0.5f, 1.0f, "No file loaded");  // Red
        }
    }
    
    // =========================================================================
    // RESOLUTION SECTION (keeping original logic, adding color to apply button)
    // =========================================================================
    if (CollapsingHeader("Resolution Settings", true)) {
        
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
        PushItemWidth(120);
        InputInt("Width", &resX);
        InputInt("Height", &resY);
        
        // Apply resolution button (now with green color)
        if (ButtonColored("Apply Resolution", 0.2f, 0.7f, 0.2f, 1.0f)) {
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
    }
    
    // =========================================================================
    // LOOP PROPERTIES SECTION (View files only) - just adding colors
    // =========================================================================
    if (globalView && curLoop && (*curLoop)) {
        if (CollapsingHeader("Loop Properties")) {
            
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
            Checkbox("Mirror Loop", &mirror);
            loopMirror = mirror ? 1 : 0;
            
            InputInt("Base Loop", &loopBase);
            
            if (!loopMirror) {
                Separator();
                TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Animation Settings:");  // Light blue header
                InputInt("Continue Loop", &loopContinue);
                InputInt("Start Cell", &loopStartCell);
                InputInt("End Cell", &loopEndCell);
                InputInt("Repeat Count", &loopRepeat);
                InputInt("Step Size", &loopStepSize);
            }
            
            // Apply loop properties button (now with green color)
            if (ButtonColored("Apply Loop Properties", 0.2f, 0.7f, 0.2f, 1.0f)) {
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
        }
    }
    
    // =========================================================================
    // CELL PROPERTIES SECTION WITH AUTO-APPLY (keeping original logic)
    // =========================================================================
    if (curCell && (*curCell)) {
        if (CollapsingHeader("Cell Properties")) {
            
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
                    TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Hot Spot:");  // Light blue header
                    
                    if (InputInt("X Hot", &cellX)) cellEditingStarted = true;
                    if (InputInt("Y Hot", &cellY)) cellEditingStarted = true;
                    
                } else {
                    TextDisabled("Cell properties not available for mirror loops");
                }
            } else if (globalPicture) {
                TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Position:");  // Light blue header
                
                if (InputInt("X Position", &cellX)) cellEditingStarted = true;
                if (InputInt("Y Position", &cellY)) cellEditingStarted = true;
                if (InputInt("Priority", &cellPriority)) cellEditingStarted = true;
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
                Separator();
                
                // Show changed indicator with colors
                if (cellHasChanges) {
                    TextColored(1.0f, 0.8f, 0.3f, 1.0f, "* Values have been modified *");  // Orange
                } else {
                    TextColored(0.7f, 0.7f, 0.7f, 1.0f, "No changes");  // Gray
                }
                
                if (cellHasChanges && ButtonColored("Reset to Original", 0.8f, 0.3f, 0.3f, 1.0f)) {  // Red button
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
                
                SameLine();
                if (ButtonColored("Done Editing", 0.2f, 0.7f, 0.2f, 1.0f)) {  // Green button
                    cellEditingStarted = false;
                    cellHasChanges = false;
                    // Keep current values as new originals
                    originalCellX = cellX;
                    originalCellY = cellY;
                    originalCellPriority = cellPriority;
                }
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
        Separator();

        // =====================================================================
        // QUICK OPERATIONS
        // =====================================================================
        if (CollapsingHeader("Quick Operations", true))
        {
            // Loop operations (V56 only)
            if (hasLoops)
            {
                FotoSCIhopStyles::HeaderText("Loop Operations:");

                BeginGroup();
                if (Button("Add Loop", buttonWidth, 0))
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
                if (IsItemHovered())
                {
                    SetTooltip("Add a new loop after the current loop");
                }

                SameLine();
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
                    if (IsItemHovered())
                    {
                        SetTooltip("Remove the current loop");
                    }
                }
                else
                {
                    PushStyleVar(IMGUI_STYLE_VAR_ALPHA, 0.5f);
                    Button("Remove Loop", buttonWidth, 0);
                    PopStyleVar();
                    if (IsItemHovered())
                    {
                        SetTooltip("Cannot remove the last loop");
                    }
                }
                EndGroup();

                Separator();
            }

            // Cell operations (both P56 and V56)
            FotoSCIhopStyles::HeaderText("Cell Operations:");

            BeginGroup();
            if (Button("Add Cell", buttonWidth, 0))
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
            if (IsItemHovered())
            {
                SetTooltip("Add a new empty cell after the current cell");
            }

            SameLine();
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
                if (IsItemHovered())
                {
                    SetTooltip("Remove the current cell");
                }
            }
            else
            {
                PushStyleVar(IMGUI_STYLE_VAR_ALPHA, 0.5f);
                Button("Remove Cell", buttonWidth, 0);
                PopStyleVar();
                if (IsItemHovered())
                {
                    SetTooltip("Cannot remove the last cell");
                }
            }
            EndGroup();
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
        if (CollapsingHeader("Link Points")) {
            
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
            if (InputInt("Number of Link Points", &linkCount)) {
                linkEditingStarted = true;
            }
            if (linkCount < 0) linkCount = 0;
            if (linkCount > 10) linkCount = 10;
            
            if (linkCount > 0) {
                Separator();
                Text("Link Point Coordinates:");
                
                // Show link points in single column layout
                for (int i = 0; i < linkCount; i++) {
                    char headerLabel[32];
                    sprintf(headerLabel, "Link Point %d", i + 1);
                    
                    if (CollapsingHeader(headerLabel)) {
                        char label[32];
                        
                        sprintf(label, "X##%d", i);
                        if (InputInt(label, &linkX[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Y##%d", i);
                        if (InputInt(label, &linkY[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Priority##%d", i);
                        if (InputInt(label, &linkPri[i])) linkEditingStarted = true;
                        
                        sprintf(label, "Type##%d", i);
                        if (InputInt(label, &linkType[i])) linkEditingStarted = true;
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
                Separator();
                
                // Show changed indicator here to prevent shifting
                if (linkHasChanges) {
                    PushStyleVar(IMGUI_STYLE_VAR_ALPHA, 0.8f);
                    Text("* Link points have been modified *");
                    PopStyleVar();
                }
                
                if (linkHasChanges && Button("Reset Link Points")) {
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
                
                SameLine();
                if (Button("Done with Link Points")) {
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
        PushStyleVar(IMGUI_STYLE_VAR_ALPHA, 0.6f);
        if (CollapsingHeader("Link Points (Not Available)")) {
            Text("Link points are only available for:");
            Text("- View files (.v56)");
            Text("- Non-mirrored loops");
            Text("- When a loop and cell are selected");
        }
        PopStyleVar();
    }

    // =========================================================================
    // REFERENCE IMAGE SECTION WITH AUTO-APPLY
    // =========================================================================
    if (CollapsingHeader("Reference Image")) {
        
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
        TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Scale Settings:");  // Light blue header
        
        // Limit scale input to 3 digits (like original dialog)
        PushItemWidth(120);
        if (InputFloat("Scale X (%)", &refScaleX, 0.0f, 0.0f, 1)) refEditingStarted = true;
        if (refScaleX < 0) refScaleX = 0;
        if (refScaleX > 999) refScaleX = 999;
        
        if (InputFloat("Scale Y (%)", &refScaleY, 0.0f, 0.0f, 1)) refEditingStarted = true;
        if (refScaleY < 0) refScaleY = 0;
        if (refScaleY > 999) refScaleY = 999;
        PopItemWidth();
        
        Separator();
        
        // Bitmap File
        TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Reference Bitmap:");  // Light blue header
        if (InputText("Bitmap File", refBitmapName, _MAX_PATH)) refEditingStarted = true;
        
        Separator();
        
        // Hot Spot Settings
        TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Hot Spot:");  // Light blue header
        
        PushItemWidth(120);
        // Limit hot spot values to 4 digits (like original dialog)
        if (InputInt("X Hot", &refXHot)) refEditingStarted = true;
        if (refXHot < -9999) refXHot = -9999;
        if (refXHot > 9999) refXHot = 9999;
        
        if (InputInt("Y Hot", &refYHot)) refEditingStarted = true;
        if (refYHot < -9999) refYHot = -9999;
        if (refYHot > 9999) refYHot = 9999;
        PopItemWidth();
        
        Separator();
        
        // Link Point Settings
        TextColored(0.7f, 0.9f, 1.0f, 1.0f, "Link Point:");  // Light blue header
        
        PushItemWidth(120);
        // Limit link point to 2 digits (like original dialog)
        if (InputInt("Link Point Index", &refLinkPoint)) refEditingStarted = true;
        if (refLinkPoint < 0) refLinkPoint = 0;
        if (refLinkPoint > 99) refLinkPoint = 99;
        
        // Limit link point coordinates to 4 digits (like original dialog)
        if (InputInt("Link Point X", &refLinkPointX)) refEditingStarted = true;
        if (refLinkPointX < -9999) refLinkPointX = -9999;
        if (refLinkPointX > 9999) refLinkPointX = 9999;
        
        if (InputInt("Link Point Y", &refLinkPointY)) refEditingStarted = true;
        if (refLinkPointY < -9999) refLinkPointY = -9999;
        if (refLinkPointY > 9999) refLinkPointY = 9999;
        PopItemWidth();
        
        Separator();
        
        // Priority Setting
        if (Checkbox("Priority", &refPriority)) refEditingStarted = true;
        
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
            Separator();
            
            // Show changed indicator with colors
            if (refHasChanges) {
                TextColored(1.0f, 0.8f, 0.3f, 1.0f, "* Reference image settings have been modified *");  // Orange
            } else {
                TextColored(0.7f, 0.7f, 0.7f, 1.0f, "No changes");  // Gray
            }
            
            if (refHasChanges && ButtonColored("Reset to Original", 0.8f, 0.3f, 0.3f, 1.0f)) {  // Red button
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
            
            SameLine();
            if (ButtonColored("Done Editing", 0.2f, 0.7f, 0.2f, 1.0f)) {  // Green button
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
        }
    }
    
    // =========================================================================
    // MAIN BUTTONS
    // =========================================================================
    
    Separator();
    
    if (ButtonColored("Close", 0.6f, 0.6f, 0.8f, 1.0f)) {  // Light purple button
        ImGuiDialogs::HideDialog(ImGuiDialogs::DIALOG_PROPERTIES);
    }

    EndDialog();
}

#pragma warning(pop)  // Restore warning level