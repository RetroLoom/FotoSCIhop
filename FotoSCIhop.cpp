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


void ShowLoopCell(unsigned char newloop, unsigned char newcell)
{
	curLoopIndex = newloop;
	

	curLoop = &globalView->loops[newloop];
	if (curLoop)
	{		
		curCell=&globalView->loops[newloop]->cells[newcell];
		
		if (curCell || (*curLoop)->Head.flags)
		{					
			if (curCell && !(*curLoop)->Head.flags)
				curCellIndex = newcell;

			HMENU menu = GetMenu(hWnd); 
		
			//EnableMenuItem(menu, ID_INGRANDIMENTO_NORMALE, ((*tloop)->Head.flags ?MF_GRAYED :MF_ENABLED));
			//EnableMenuItem(menu, ID_INGRANDIMENTO_X2, ((*tloop)->Head.flags ?MF_GRAYED :MF_ENABLED));
			//EnableMenuItem(menu, ID_INGRANDIMENTO_X3, ((*tloop)->Head.flags ?MF_GRAYED :MF_ENABLED));
			//EnableMenuItem(menu, ID_INGRANDIMENTO_X4, ((*tloop)->Head.flags ?MF_GRAYED :MF_ENABLED));

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

			if (hPropertiesDialog)
				DoUpdatePropertiesProc(hPropertiesDialog);

			if (hLinkPointDialog)
				DoUpdateLinkPointProc(hLinkPointDialog);
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
	curCellIndex = newcell;
	curCell=&globalPicture->cells[curCellIndex];
	if (curCell)
	{	
		//globalPicture->SelectedCell(newcell);

		HMENU menu = GetMenu(hWnd); 

		//EnableMenuItem(menu, ID_INGRANDIMENTO_NORMALE, MF_ENABLED);
		//EnableMenuItem(menu, ID_INGRANDIMENTO_X2, MF_ENABLED);
		//EnableMenuItem(menu, ID_INGRANDIMENTO_X3, MF_ENABLED);
		//EnableMenuItem(menu, ID_INGRANDIMENTO_X4, MF_ENABLED);
	
		EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
		EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
		if (curCellIndex == globalPicture->CellsCount() -1)
			EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
		
		if (curCellIndex == 0)
			EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

		InvalidateRgn(hWnd, NULL, true);

		if (hPropertiesDialog)
		{
			DoUpdatePropertiesProc(hPropertiesDialog);
		}
	}
}


BOOL DoFileOpen(HWND hwnd, char *filename, char *ext)
{
   OPENFILENAME ofn;
   bool proceed = false;

   if (filename == NULL)
   {
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		//szFileName[0] = 0;

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = INTERFACE_OPENFILEFILTER; 
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAX_PATH;

		ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOVALIDATE | OFN_FILEMUSTEXIST;
		proceed = (GetOpenFileName(&ofn)!=0);
   }
   else
   {
	    strcpy(szFileName, filename);
		proceed=true;
   }

   if(proceed)
   {
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
	  EnableMenuItem(menu, ID_INFO, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED));
	  EnableMenuItem(menu, IDM_PROPERTIES, (result==ID_NOERROR ?MF_ENABLED:MF_GRAYED)); 
	  EnableMenuItem(menu, IDM_LINKPOINTS, ((result==ID_NOERROR)&&(globalView) ?MF_ENABLED:MF_GRAYED));
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
BOOL CLIFileExportOG(char BMPFileName[MAX_PATH])
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

// Dhel - with CImage
BOOL CLIFileExport(char BMPFileName[MAX_PATH])
{
   if(BMPFileName)
   {
		CImage image;

        // Extract the width, height, and bit depth from the BITMAPINFOHEADER
        int width = (*curCell)->bmInfo->bmiHeader.biWidth;
        int height = abs((*curCell)->bmInfo->bmiHeader.biHeight); // Make sure the height is positive
        int bitCount = (*curCell)->bmInfo->bmiHeader.biBitCount;

        // Create the image with the specified dimensions and bit depth
        image.Create(width, height, bitCount);

        // Get a pointer to the image bits
        BYTE* pBits = (BYTE*)image.GetBits();

        // Calculate the width in bytes
        int rowBytes = image.GetPitch();

        // Copy the image data
        long mywidth = (*curCell)->bmInfo->bmiHeader.biSizeImage / height;
        for (int i = 0; i < height; i++)
        {
            memcpy(pBits + (height - 1 - i) * rowBytes, (BYTE*)(*curCell)->bmImage + i * mywidth, mywidth);
        }

        // Save the image
        HRESULT hr = image.Save(BMPFileName, Gdiplus::ImageFormatBMP);
        if (FAILED(hr))
        {
            // Handle the error, e.g., print an error message
            printf("Failed to save the image. HRESULT: 0x%lx\n", hr);
        }
   }

   return TRUE;
}

void ShiftPaletteIndices(CImage& image, int startIndex, int endIndex, int offset) {
    // Ensure the image is 8-bit indexed
    if (image.GetBPP() != 8) {
        //throw std::runtime_error("The image is not an 8-bit indexed BMP.");
    }

    // Get the number of colors in the palette
    int numColors = image.GetMaxColorTableEntries();

    // Load the palette
    RGBQUAD palette[256];
    image.GetColorTable(0, numColors, palette);

    // Create a copy of the palette to modify
    RGBQUAD newPalette[256];
    memcpy(newPalette, palette, sizeof(RGBQUAD) * numColors);

    // Shift the palette entries
    for (int i = startIndex; i <= endIndex; ++i) {
        int newIndex = (i + offset + numColors) % numColors;
        newPalette[newIndex] = palette[i];
    }

    // Set the modified palette back to the image
    image.SetColorTable(0, numColors, newPalette);

    // Update the pixel data to reflect the new palette indices
    int width = image.GetWidth();
    int height = image.GetHeight();
    for (int y = 0; y < height; ++y) {
        BYTE* scanline = reinterpret_cast<BYTE*>(image.GetPixelAddress(0, y));
        for (int x = 0; x < width; ++x) {
            BYTE oldIndex = scanline[x];
            if (oldIndex >= startIndex && oldIndex <= endIndex) {
                BYTE newIndex = (oldIndex + offset + numColors) % numColors;
                scanline[x] = newIndex;
            }
        }
    }
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

			// Temporarily Disable while testing palette shift
			//if (memcmp(tctab, (*curCell)->bmInfo->bmiColors, 256*sizeof(RGBQUAD)))			
			//	CLIPaletteImport (BMPFileName);

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
				(*curCell)->shiftPaletteRange (0, 108, 127, tctab);
				


				//HMENU menu = GetMenu(hwnd);

				//EnableMenuItem(menu, ID_SALVA, MF_ENABLED);
				//datasaved = false;
			}
			else
			{
				delete timage;
				delete tbinfo;
			}

			fclose(tempfile);

			return TRUE;
		}

        //MessageBox(hwnd, ERR_CANTLOADFILE, ERR_TITLE,
		//					MB_OK | MB_ICONSTOP);
   }

   return TRUE;
}

void Cell::shiftPaletteRange (int startIndex, int endIndex, int offset, RGBQUAD *tctab)
{
	GetImage (&bmInfo, &bmImage);
	
	unsigned short width = static_cast<unsigned short>(bmInfo->bmiHeader.biWidth);
	unsigned short height = static_cast<unsigned short>(-bmInfo->bmiHeader.biHeight);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Copy pixel data
			bmImage[y * width + x] = bmImage[y * width + x];

			// Update palette indices
			BYTE oldIndex = bmImage[y * width + x];
			if (oldIndex >= startIndex && oldIndex <= endIndex)
			{
				BYTE newIndex = (oldIndex + offset + 256) % 256;
				bmImage[y * width + x] = newIndex;
			}
		}
	}

	// Load the palette
	Palette *tnewpal;
	
	//memcmp((*curCell)->bmInfo->bmiColors, tctab, 256*sizeof(RGBQUAD));

	if (isPicture)
		tnewpal = globalPicture->palSCI;
	else
		tnewpal = globalView->palSCI;

	for (int i = 0; i < 256; i++)
	{
		PalEntry *pe = tnewpal->GetPalEntry(i);
		PalEntry npe;
		npe.remap = (pe == NULL ? 0 : pe->remap);

		// Apply the palette index shift here
		// Corrected the newIndex calculation to handle positive and negative offsets correctly
		int newIndex = (i - offset) % 256;
		if (newIndex < 0)
		{
			newIndex += 256;
		}

		npe.blue = tctab[newIndex].rgbBlue;
		npe.green = tctab[newIndex].rgbGreen;
		npe.red = tctab[newIndex].rgbRed;

		tnewpal->SetPalEntry(npe, i);
	}

	SetImage (bmInfo, bmImage);

}

BOOL DoImageConversion(char fileName[MAX_PATH])
{
	// char tempFile[32];
	// sprintf(tempFile, "%s/temp", gAppPath);

	char cmd[1024];

	sprintf(cmd, "%s-pal-temp.bmp", fileName);
	CLIFileExport(cmd);

	sprintf(cmd, "%s/tools/SCIC/SCIC --for %d -t %d -c %s-pal-temp.bmp -f %s.png -o %s.bmp", gAppPath, colorLimit, tolerance, fileName, fileName, fileName);
	system(cmd);

	//sprintf(cmd, "%s/tools/nconvert -quiet -overwrite -out bmp %s.bmp", gAppPath, fileName);
	//system(cmd);

	sprintf(cmd, "del %s-pal-temp.bmp", fileName);
	system(cmd);
	
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

		if (stricmp(szBMPFileName + ofn.nFileExtension, "bmp"))
		{

			char fileName[sizeof(szBMPFileName)];
			sprintf(fileName, szBMPFileName);

			fileName[strlen(fileName) - 4] = '\0';

			if (DialogBox(NULL,
						  MAKEINTRESOURCE(IDD_IMPORT_IMAGE),
						  hwnd,
						  (DLGPROC)DoImportImageDlg) == IDOK)
			{
				DoImageConversion(fileName);
				RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
			}

			

			// return TRUE;
			sprintf(szBMPFileName, "%s.bmp", fileName);
		}

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

BOOL CALLBACK ChangeFrameSizeProc(HWND hwndDlg, 
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
              // SetDlgItemInt(hwndDlg, IDDI_LEFT, (*curCell)->Left(), TRUE);
               //SetDlgItemInt(hwndDlg, IDDI_TOP, (*curCell)->Top(), TRUE);
               //SetDlgItemInt(hwndDlg, IDDI_WIDTH, (*curCell)->Width(), TRUE);
              // SetDlgItemInt(hwndDlg, IDDI_HEIGHT, (*curCell)->Height(), TRUE);
            }
        
            return TRUE;
        }   
    
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                
       		       if (curCell)
                   {
                       // (*curCell)->Left(GetDlgItemInt(hwndDlg, IDDI_LEFT, NULL, TRUE));
                       // (*curCell)->Top(GetDlgItemInt(hwndDlg, IDDI_TOP, NULL, TRUE));
                       // (*curCell)->Width(GetDlgItemInt(hwndDlg, IDDI_WIDTH, NULL, TRUE));
                       // (*curCell)->Height(GetDlgItemInt(hwndDlg, IDDI_HEIGHT, NULL, TRUE));

                   }
 
                case IDCANCEL: 
                    EndDialog(hwndDlg, wParam); 
                    return TRUE; 
            } 
    } 
    return FALSE; 
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

// Dhel - modify box
BOOL CALLBACK DoModifyPropertiesProc(HWND hwndDlg,
									 UINT message,
									 WPARAM wParam,
									 LPARAM lParam)
{
	int selLoop = 0;
	
	if (globalView)
		selLoop = curLoopIndex;
	if (globalPicture)
		selLoop = 0;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		
		DoUpdatePropertiesProc(hwndDlg);
		
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:


			if (globalView)
			{

				if (curCell)
				{

					//globalView->cells[i]->Head.pic[(*curCell)->headerIndex].xShift = GetDlgItemInt(hwndDlg, IDDI_LEFT, NULL, TRUE);
					//globalView->cells[i]->Head.pic[(*curCell)->headerIndex].yShift = GetDlgItemInt(hwndDlg, IDDI_TOP, NULL, TRUE);

						globalView->loops[selLoop]->Head.flags = GetDlgItemInt(hwndDlg, IDC_LOOP_MIRROR, NULL, TRUE);
						globalView->loops[selLoop]->Head.altLoop = GetDlgItemInt(hwndDlg, IDC_LOOP_BASE, NULL, TRUE);

						if (!(*curLoop)->Head.flags)
						{
							CelHeaderView *bCell = new CelHeaderView;
							bCell = (CelHeaderView *)&(*curCell)->Head;

							bCell->xHot = GetDlgItemInt(hwndDlg, IDDI_LEFT, NULL, TRUE);
							bCell->yHot = GetDlgItemInt(hwndDlg, IDDI_TOP, NULL, TRUE);

							globalView->loops[selLoop]->Head.contLoop = GetDlgItemInt(hwndDlg, IDC_LOOP_CONTINUE, NULL, TRUE);
							globalView->loops[selLoop]->Head.startCel = GetDlgItemInt(hwndDlg, IDC_LOOP_STARTCELL, NULL, TRUE);
							globalView->loops[selLoop]->Head.endCel = GetDlgItemInt(hwndDlg, IDC_LOOP_ENDCELL, NULL, TRUE);
							globalView->loops[selLoop]->Head.repeatCount = GetDlgItemInt(hwndDlg, IDC_LOOP_REPEAT, NULL, TRUE);
							globalView->loops[selLoop]->Head.stepSize = GetDlgItemInt(hwndDlg, IDC_LOOP_STEPSIZE, NULL, TRUE);
						}
						else
						{
							globalView->loops[selLoop]->Head.contLoop = -1;
							globalView->loops[selLoop]->Head.startCel = -1;
							globalView->loops[selLoop]->Head.endCel = -1;
							globalView->loops[selLoop]->Head.repeatCount = 255;
							globalView->loops[selLoop]->Head.stepSize = 3;
						}
				}

				globalView->Head.view32.resX = GetDlgItemInt(hwndDlg, IDDI_RESX, NULL, TRUE);
				globalView->Head.view32.resY = GetDlgItemInt(hwndDlg, IDDI_RESY, NULL, TRUE);

	
				int addLoop = 0;

				int loopDelta = GetDlgItemInt(hwndDlg, IDC_LOOPS_DELTA, NULL, TRUE);

				if (loopDelta != 0)
					DoAddLoops (curLoopIndex, loopDelta);

				int addCell = 0;

				int cellDelta = GetDlgItemInt(hwndDlg, IDC_CELLS_DELTA, NULL, TRUE);
				
				if (cellDelta != 0)
					DoAddCells(curLoopIndex, curCellIndex, cellDelta);
			
				ShowLoopCell(curLoopIndex, curCellIndex); // refresh screen
			}

			if (globalPicture)
			{
				PicHeader11 *bPic11 = new PicHeader11;
				PicHeader32 *bPic32 = new PicHeader32;

				switch (globalPicture->format)
				{
				case _PIC_11:
					bPic11 = (PicHeader11 *)&globalPicture->Head;
					bPic11->vanishX = GetDlgItemInt(hwndDlg, IDDI_RESX, NULL, TRUE);
					bPic11->viewAngle = GetDlgItemInt(hwndDlg, IDDI_RESY, NULL, TRUE);
					break;

				case _PIC_32:
					bPic32 = (PicHeader32 *)&globalPicture->Head;
					bPic32->resX = GetDlgItemInt(hwndDlg, IDDI_RESX, NULL, TRUE);
					bPic32->resY = GetDlgItemInt(hwndDlg, IDDI_RESY, NULL, TRUE);
					break;
				}		

				if (curCell)
				{
					CelHeaderPic *bCell = new CelHeaderPic;
					bCell = (CelHeaderPic*)&(*curCell)->Head;

					bCell->xpos = GetDlgItemInt(hwndDlg, IDDI_LEFT, NULL, TRUE);
					bCell->ypos = GetDlgItemInt(hwndDlg, IDDI_TOP, NULL, TRUE);
					bCell->priority = GetDlgItemInt(hwndDlg, IDC_POS_PRI, NULL, TRUE);
				}

				int addCell = 0;
				int cellDelta = GetDlgItemInt(hwndDlg, IDC_CELLS_DELTA, NULL, TRUE);
				
				if (cellDelta != 0)
					DoAddCells(curLoopIndex, curCellIndex, cellDelta);

				ShowCell(curCellIndex); // refresh screen
			};

			datasaved = false;
						
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

// Dhel - modify box
void DoUpdatePropertiesProc(HWND hwndDlg)
{
	int selLoop = 0;
	
	if (globalView)
		selLoop = curLoopIndex;
	if (globalPicture)
		selLoop = 0;

	

			
		if (globalView)
		{
			SetDlgItemInt(hwndDlg, IDDI_RESX, globalView->Head.view32.resX, TRUE);
			SetDlgItemInt(hwndDlg, IDDI_RESY, globalView->Head.view32.resY, TRUE);

			if (curCell)
			{

			if (!(*curLoop)->Head.flags)
			{

				CelHeaderView *bCell = new CelHeaderView;
				bCell = (CelHeaderView *)&(*curCell)->Head;

				SetDlgItemInt(hwndDlg, IDDI_LEFT, bCell->xHot, TRUE);
				SetDlgItemInt(hwndDlg, IDDI_TOP, bCell->yHot, TRUE);

				SetDlgItemInt(hwndDlg, IDC_LOOP_CONTINUE, globalView->loops[selLoop]->Head.contLoop, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_STARTCELL, globalView->loops[selLoop]->Head.startCel, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_ENDCELL, globalView->loops[selLoop]->Head.endCel, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_REPEAT, globalView->loops[selLoop]->Head.repeatCount, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_STEPSIZE, globalView->loops[selLoop]->Head.stepSize, TRUE);
			}
			else
			{
				SetDlgItemInt(hwndDlg, IDDI_LEFT, 0, TRUE);
				SetDlgItemInt(hwndDlg, IDDI_TOP, 0, TRUE);

				SetDlgItemInt(hwndDlg, IDC_LOOP_CONTINUE, -1, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_STARTCELL, -1, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_ENDCELL, -1, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_REPEAT, 255, TRUE);
				SetDlgItemInt(hwndDlg, IDC_LOOP_STEPSIZE, 3, TRUE);
			}

			SetDlgItemInt(hwndDlg, IDC_LOOP_MIRROR, globalView->loops[selLoop]->Head.flags, TRUE);
			SetDlgItemInt(hwndDlg, IDC_LOOP_BASE, globalView->loops[selLoop]->Head.altLoop, TRUE);
			}

			SetDlgItemInt(hwndDlg, IDC_LOOPS_DELTA, 0, TRUE);
			SetDlgItemInt(hwndDlg, IDC_CELLS_DELTA, 0, TRUE);
		}

		if (globalPicture)
		{
			PicHeader11 *bPic11 = new PicHeader11;
			PicHeader32 *bPic32 = new PicHeader32;

			switch (globalPicture->format)
			{
			case _PIC_11:
			bPic11 = (PicHeader11 *)&globalPicture->Head;
			SetDlgItemInt(hwndDlg, IDDI_RESX, bPic11->vanishX, TRUE);
			SetDlgItemInt(hwndDlg, IDDI_RESY, bPic11->viewAngle, TRUE);
			break;

			case _PIC_32:
			bPic32 = (PicHeader32 *)&globalPicture->Head;
			SetDlgItemInt(hwndDlg, IDDI_RESX, bPic32->resX, TRUE);
			SetDlgItemInt(hwndDlg, IDDI_RESY, bPic32->resY, TRUE);
			break;
			}

			if (curCell)
			{
				CelHeaderPic *bCell = new CelHeaderPic;
				bCell = (CelHeaderPic*)&(*curCell)->Head;

				SetDlgItemInt(hwndDlg, IDDI_LEFT, bCell->xpos, TRUE);
				SetDlgItemInt(hwndDlg, IDDI_TOP, bCell->ypos, TRUE);
				SetDlgItemInt(hwndDlg, IDC_POS_PRI, bCell->priority, TRUE);
			}

			SetDlgItemInt(hwndDlg, IDC_CELLS_DELTA, 0, TRUE);
		}

		InvalidateRgn(hPropertiesDialog, NULL, true);

}

void DoModifyProperties(HWND hwnd)
{
		if (globalView)
			hPropertiesDialog = CreateDialog(NULL,
											 MAKEINTRESOURCE(IDD_PROPERTIES_V),
											 hwnd,
											 (DLGPROC)DoModifyPropertiesProc);

		if (globalPicture)
			hPropertiesDialog = CreateDialog(NULL,
											 MAKEINTRESOURCE(IDD_PROPERTIES_P),
											 hwnd,
											 (DLGPROC)DoModifyPropertiesProc);

		if (hPropertiesDialog != NULL)
		{
			ShowWindow(hPropertiesDialog, SW_SHOW);
		}
		else
		{
			// Failed to create dialog
		}
}

BOOL CALLBACK DoLinkPointProc(HWND hwndDlg,
									 UINT message,
									 WPARAM wParam,
									 LPARAM lParam)
{
	int selLoop = 0;
	
	if (globalView)
		selLoop = curLoopIndex;
	if (globalPicture)
		selLoop = 0;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		
		DoUpdateLinkPointProc(hwndDlg);
		
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:


			if (globalView)
			{
				if (curCell)
				{
					CelHeaderView *bCell = new CelHeaderView;
					bCell = (CelHeaderView*)&(*curCell)->Head;

					bCell->linkTableCount = GetDlgItemInt(hwndDlg, IDC_LINK_COUNT, NULL, TRUE);

					for (int i = 0; i < bCell->linkTableCount; i++)
					{
							(*curCell)->linkPoints[i].x = GetDlgItemInt(hwndDlg, IDC_LINK_X_1 + i, NULL, TRUE);
							(*curCell)->linkPoints[i].y = GetDlgItemInt(hwndDlg, IDC_LINK_Y_1 + i, NULL, TRUE);
							(*curCell)->linkPoints[i].priority = GetDlgItemInt(hwndDlg, IDC_LINK_PRI_1 + i, NULL, TRUE);
							(*curCell)->linkPoints[i].positionType = GetDlgItemInt(hwndDlg, IDC_LINK_TYPE_1 + i, NULL, TRUE);
					}
				}

				ShowLoopCell(curLoopIndex, curCellIndex); // refresh screen
			}

			datasaved = false;
				
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

void DoUpdateLinkPointProc(HWND hwndDlg)
{
	int selLoop = 0;
	
	if (globalView)
		selLoop = curLoopIndex;
	if (globalPicture)
		selLoop = 0;

			
		if (globalView)
		{			
			if (!(*curLoop)->Head.flags)
			{
				CelHeaderView *bCell = new CelHeaderView;
				bCell = (CelHeaderView*)&(*curCell)->Head;
				
				SetDlgItemInt(hwndDlg, IDC_LINK_COUNT, bCell->linkTableCount, TRUE);

				// link table
				for (int i = 0; i < bCell->linkTableCount; i++)
				{
					SetDlgItemInt(hwndDlg, IDC_LINK_X_1 + i, (*curCell)->linkPoints[i].x, TRUE);
					SetDlgItemInt(hwndDlg, IDC_LINK_Y_1 + i, (*curCell)->linkPoints[i].y, TRUE);
					SetDlgItemInt(hwndDlg, IDC_LINK_PRI_1 + i, (*curCell)->linkPoints[i].priority, TRUE);
					SetDlgItemInt(hwndDlg, IDC_LINK_TYPE_1 + i, (*curCell)->linkPoints[i].positionType, TRUE);
				}
			}
			
			InvalidateRgn(hLinkPointDialog, NULL, true);

		}
}

void DoLinkPointDialog(HWND hwnd)
{
    hLinkPointDialog = CreateDialog(NULL, 
                                                MAKEINTRESOURCE(IDD_LINKPOINTS), 
                                                hwnd, 
                                                (DLGPROC)DoLinkPointProc);
    if (hLinkPointDialog != NULL)
    {
        ShowWindow(hLinkPointDialog, SW_SHOW);
    }
    else
    {
        // Failed to create dialog
    }
}


void DoPropertyBox(HWND hwnd)
{
	/*
	char propstr[1024]="";

	if (isPicture)
	{
		char tscreenres[64];
		sprintf(tscreenres, "%d x %d", globalPicture->MaxWidth(), globalPicture->MaxHeight());
		if (globalPicture->MaxHeight()==0)
			switch (globalPicture->MaxWidth())
			{
			case 0:
				strcat(tscreenres, " (320 x 200)");
				break;
			case 1:
				strcat(tscreenres, " (640 x 480)");
				break;
			case 2:
				strcat(tscreenres, " (640 x 400)");
				break;
			}
		sprintf(propstr, INTERFACE_P56PROPSTR,
						' ',' ',
						globalPicture->CellsCount(),
						(globalPicture->IsCompressed()?"":INTERFACE_NOTSTR),
						tscreenres,
						globalPicture->CellRecSize());
						
		if (globalPicture->format == _PIC_11)
		{
			sprintf(tscreenres, "\n%s%ld\n", INTERFACE_VECTORSIZESTR, globalPicture->Head.oldhead.VectorDataLenght);
			strcat(propstr, tscreenres);
			
			sprintf(tscreenres, INTERFACE_PRIORITYSTR, globalPicture->Head.oldhead.nPriorities, globalPicture->Head.oldhead.priLineCount);
			strcat(propstr, tscreenres);
		}
		
		if (curCell)
		{
			CelHeaderPic *bCell = new CelHeaderPic;
			bCell = (CelHeaderPic*)&(*curCell)->Head;

            char tcell[256];
            sprintf(tcell, INTERFACE_P56CELLPROPSTR,
						' ',
						curCellIndex+1,
						' ',
						//(*curCell)->Width(),(*curCell)->Height(),
						bCell->xDim,bCell->yDim,
						bCell->xHot, bCell->yHot,
						bCell->skip,
						bCell->compressType,
						bCell->dataFlags,
						bCell->priority,
						bCell->xpos,
						bCell->ypos
						);

			strcat(propstr, tcell);
        }
	}
	else //isView
	{
		char tviewres[64], tscreenres[64], tcell[256];
		sprintf(tviewres, "%d", globalView->Head.ViewSize);
		switch (globalView->Head.ViewSize)
		{
		case 0:
			strcat(tviewres, " (320 x 200)");
			break;
		case 1:
			strcat(tviewres, " (640 x 480)");
			break;
		case 2:
			strcat(tviewres, " (640 x 400)");
			break;
		}

		sprintf(tscreenres, "%d x %d", globalView->Head.ResolutionX, globalView->Head.ResolutionY);
		if ((globalView->Head.ResolutionX == 0) && (globalView->Head.ResolutionY == 0))
		{
			strcat(tscreenres, INTERFACE_SEEVIEWRESSTR);
		}

		Loop *tloop=globalView->loops[curLoopIndex];

		sprintf(propstr, INTERFACE_PROPVIEWSTR,
						' ', ' ',
						globalView->Head.LoopCount,
						globalView->Head.stripView,
						(globalView->Head.Compressed?"":INTERFACE_NOTSTR),
						tviewres,
						globalView->Head.CellsCount,
						globalView->Head.LoopRecSize,
						globalView->Head.CellRecSize,
						tscreenres,

						' ',
						curLoopIndex+1,
						' ',
						tloop->Head.numCels,
						tloop->Head.altLoop,
						(tloop->Head.flags?"":INTERFACE_NOTSTR),
						tloop->Head.contLoop,
                        tloop->Head.startCel,
                        tloop->Head.endCel,
                        tloop->Head.repeatCount,
                        tloop->Head.stepSize
						);
		
		if (curCell)
		{
			CelHeaderView *bCell = new CelHeaderView;
			bCell = (CelHeaderView*)&(*curCell)->Head;

			sprintf(tcell, INTERFACE_VIEWCELLPROPSTR,
							' ',
							curCellIndex+1,
							' ',
							bCell->xDim, bCell->yDim,
							bCell->xHot, bCell->yHot,
							bCell->skip,
							bCell->compressType,
							bCell->dataFlags
							);
			strcat(propstr, tcell);
			
		}
	}
	
	MessageBox(hwnd, propstr, INTERFACE_PROPERTIES, MB_OK);

	*/
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

typedef BOOL (WINAPI*Func)(HWND, char *, unsigned char, char *, char *);
 
/* this is what is going to hold our function, I like to name it like the function we are importing,
    but you can make it whatever you like. */
Func ExtractFromVolume;


#ifdef __DEVC

int STDCALL WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
{

#else 
int APIENTRY _tWinMain (HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR    lpCmdLine,
                     int       nCmdShow)
{      
#endif
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_IMMAGINA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// get app path
	GetModuleFileName(NULL, gAppPath, MAX_PATH);
    gAppPath[strrchr(gAppPath, '\\') - gAppPath] = '\0'; 

	// get ini settings
	char ini[_MAX_PATH];
	sprintf(ini, "%s\\config.ini", gAppPath);

	gAppResX = GetPrivateProfileInt("preferences", "resX", gAppResX, ini);
	gAppResY = GetPrivateProfileInt("preferences", "resY", gAppResY, ini);
	zScale = GetPrivateProfileInt("preferences", "zScale", zScale, ini);
	gPosCells = GetPrivateProfileInt("preferences", "posCells", gPosCells, ini);
	gBaseMagnify = GetPrivateProfileInt("preferences", "magScale", gBaseMagnify, ini);

	GetPrivateProfileString("preferences", "referenceBM", gReferenceBM, gReferenceBM, _MAX_PATH, ini);
	gReferenceX = GetPrivateProfileInt("preferences", "referenceX", gReferenceX, ini);
	gReferenceY = GetPrivateProfileInt("preferences", "referenceY", gReferenceY, ini);

	gCliEnabled = GetPrivateProfileInt("preferences", "cliStartup", gCliEnabled, ini);

	MagnifyFactor = gBaseMagnify;

	if (gCliEnabled)
	{
		// Dhel - cli
		if (lpCmdLine[0] != 0)
		{
			char startupfile[MAX_PATH];
			
			// tokenize arguments to array
			int i = 0;
			char *p = strtok(lpCmdLine, " ");

			while (p != NULL)
			{
				argv[i++] = p;
				p = strtok(NULL, " ");
			}

			startupfile[MAX_PATH];
			if (lpCmdLine[0] == '\"')
			{
				strncpy(startupfile, argv[0] + 1, strlen(argv[0]) - 2);
				startupfile[strlen(argv[0]) - 2] = 0;
			}
			else
				strcpy(startupfile, argv[0]);

			// do cli processes
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
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_IMMAGINA);

	#if defined _M_IX86
    HINSTANCE DLL = LoadLibrary("SCIdump.dll");
    /* check for error on loading the DLL */
    if(DLL==NULL) 
              MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);

    ExtractFromVolume=(Func)GetProcAddress((HMODULE)DLL, "?ExtractFromVolumeSkel@@YAHPAUHWND__@@PADE11@Z");
    /* check for error on getting the function */
    if(ExtractFromVolume==NULL) 
    {
       FreeLibrary((HMODULE)DLL);
       MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
    }
	#endif

	// Original
	if (lpCmdLine[0] != 0)
	{
		char startupfile[MAX_PATH];
		if (lpCmdLine[0] == '\"')
		{
			strncpy(startupfile, lpCmdLine + 1, strlen(lpCmdLine) - 2);
			startupfile[strlen(lpCmdLine) - 2] = 0;
		}
		else
			strcpy(startupfile, lpCmdLine);

		DoFileOpen(hWnd, startupfile, startupfile + (strlen(startupfile) - 3));
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

	#if defined _M_IX86
    FreeLibrary((HMODULE)DLL);
	#endif

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_IMMAGINA);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= (LPCTSTR)IDC_IMMAGINA;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    // Get the vanishX and viewAngle of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Get the vanishX and viewAngle of the window
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

    // Show the window
    ShowWindow(hWnd, nCmdShow);

    // Create a font to use for the window
    hfDefault = CreateFont(16, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_SWISS, "Arial");


    // Update the window
    UpdateWindow(hWnd);

    return TRUE;
}

void exit_proc(HWND hwnd)
{
	if (globalPicture)
		  delete globalPicture;


	 if (globalView)
		  delete globalView;

	DeleteObject(hfDefault);
	DestroyWindow(hwnd);
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
// 
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	//HDC hdc;

	// Menu checks
	HMENU menu = GetMenu(hWnd);
	EnableMenuItem(menu, ID_SALVA, (datasaved == false) ? MF_ENABLED:MF_GRAYED);

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
            szAppPath[strrchr(szAppPath, '\\') - szAppPath] = '\0'; 
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
            if (ExtractFromVolume(hWnd, NULL, 0x80, "v56", exFile))
                DoFileOpen(hWnd, exFile, "v56");         
            break;
		}
        case ID_CARICAP56VOL:
        {
            char exFile[MAX_PATH]="";
            if (ExtractFromVolume(hWnd, NULL, 0x81, "p56", exFile))
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
			DoModifyProperties(hWnd);
			break;

		case ID_INFO:
			DoPropertyBox(hWnd);
			break;

		case IDM_LINKPOINTS:
			DoLinkPointDialog(hWnd);
			break;
                
		case ID_PALETTE:
			{
				HMENU menu = GetMenu(hWnd);
				switch (CheckMenuItem(menu, ID_PALETTE, MF_BYCOMMAND))
				{
				case MF_CHECKED:
					CheckMenuItem(menu, ID_PALETTE, MF_UNCHECKED);
					tableX=0;
					break;

				case MF_UNCHECKED:
					CheckMenuItem(menu, ID_PALETTE, MF_CHECKED);
					tableX=190;
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
					showpbars=false;
					break;

				case MF_UNCHECKED:
					CheckMenuItem(menu, ID_PRIORITYBARS, MF_CHECKED);
					showpbars=true;
					break;
				}
				
				InvalidateRgn(hWnd, NULL, true);
				
				break;
			}
		case ID_CICLOPRECEDENTE:
			if (!isPicture)
				ShowLoopCell(curLoopIndex-1,0);
			break;
		case ID_CICLOSUCCESSIVO:
			if (!isPicture)
				ShowLoopCell(curLoopIndex+1,0);
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
		GetClientRect (hWnd, &rc);
		SetBkMode(hdc, TRANSPARENT);
		GetWindowRect(hWnd, &rc);
		long int twidth = rc.right-rc.left;
		SetRect(&rc, 0, 0, twidth, 20);
		FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

		if (globalView)
			picX = 220;

		if (globalPicture)
			picX = 0;

		// palette will be drawn only if the image exists
		if (tableX > 0)
		{

			HPEN redpen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

			HBRUSH tbrush;
			Palette *tpalette = (isPicture ? globalPicture->palSCI : globalView->palSCI);
			for (int i = 0; i < 16; i++)
				for (int j = 0; j < 16; j++)
				{
					PalEntry *tentry = tpalette->GetPalEntry(i * 16 + j);

					tbrush = CreateSolidBrush(RGB(tentry->red, tentry->green, tentry->blue));

					SetRect(&rc, 10 + (j * 11), 30 + (i * 16), 20 + (j * 11), 40 + (i * 16));
					FillRect(hdc, &rc, tbrush);
					DeleteObject(tbrush);

					if (tentry->remap == 1)
					{
						HPEN tpen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
						SelectObject(hdc, tpen);
						MoveToEx(hdc, 10 + (j * 11), 41 + (i * 16), NULL);
						LineTo(hdc, 20 + (j * 11), 41 + (i * 16));

						MoveToEx(hdc, 10 + (j * 11), 40 + (i * 16), NULL);
						LineTo(hdc, 20 + (j * 11), 40 + (i * 16));
						SelectObject(hdc, GetStockObject(WHITE_PEN));
						DeleteObject(tpen);
						MoveToEx(hdc, 10 + (j * 11), 39 + (i * 16), NULL);
						LineTo(hdc, 20 + (j * 11), 39 + (i * 16));
						SelectObject(hdc, GetStockObject(BLACK_PEN));
					}

					if (((i * 16 + j) < tpalette->Head.startOffset) ||
						((i * 16 + j) >= tpalette->Head.startOffset + tpalette->Head.nColors))
					{
						SelectObject(hdc, redpen);
						MoveToEx(hdc, 9 + (j * 11), 29 + (i * 16), NULL);
						LineTo(hdc, 21 + (j * 11), 41 + (i * 16));
						MoveToEx(hdc, 9 + (j * 11), 40 + (i * 16), NULL);
						LineTo(hdc, 21 + (j * 11), 28 + (i * 16));
						SelectObject(hdc, GetStockObject(BLACK_PEN));
					}
				}

			if (tpalette->palData)
			{
				tbrush = CreateSolidBrush(RGB(0, 255, 255));

				SetRect(&rc, 20, 300, 30, 310);
				FillRect(hdc, &rc, tbrush);
				DeleteObject(tbrush);

				SelectObject(hdc, redpen);
				MoveToEx(hdc, 19, 299, NULL);
				LineTo(hdc, 31, 311);
				MoveToEx(hdc, 19, 310, NULL);
				LineTo(hdc, 31, 298);
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				SetRect(&rc, 40, 295, 190, 315);
				DrawText(hdc, INTERFACE_MISSINGCOLORSSTR, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

				if (!tpalette->Head.type)
				{
					SetRect(&rc, 20, 320, 30, 330);
					FillRect(hdc, &rc, tbrush);
					DeleteObject(tbrush);

					SelectObject(hdc, redpen);
					MoveToEx(hdc, 20, 332, NULL);
					LineTo(hdc, 30, 332);
					MoveToEx(hdc, 20, 331, NULL);
					LineTo(hdc, 30, 331);

					SelectObject(hdc, GetStockObject(WHITE_PEN));

					MoveToEx(hdc, 20, 330, NULL);
					LineTo(hdc, 30, 330);
					SelectObject(hdc, GetStockObject(BLACK_PEN));

					SetRect(&rc, 40, 316, 190, 336);
					DrawText(hdc, INTERFACE_LOCKEDCOLORSSTR, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
				}
			}
			else
			{
				SetRect(&rc, 30, 300, 190, 320);
				DrawText(hdc, INTERFACE_MISSINGPALETTE, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
			}

			// SetRect(&rc, 20, 340, 440, 900);
			// DoPropertyTable(hWnd);
			// DrawText (hdc, INTERFACE_PROPTABLE, -1, &rc, DT_LEFT | DT_VCENTER);

			DeleteObject(redpen);
		}

		if (gReferenceBM)
		{
			HBITMAP hbm = (HBITMAP)LoadImage(NULL, gReferenceBM, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

			BITMAP bm;

			GetObject(hbm, sizeof(BITMAP), &bm);

			if (hbm)
			{
				HDC memdc = CreateCompatibleDC(hdc);
				SelectObject(memdc, hbm);

				TransparentBlt(hdc, 10 + picX + tableX + gReferenceX, 30 + picY + gReferenceY, bm.bmWidth, bm.bmHeight, memdc, 0, 0, bm.bmWidth, bm.bmHeight, RGB(skipColor.rgbRed, skipColor.rgbGreen, skipColor.rgbBlue));
				// BitBlt(hdc, 10 + picX + tableX, 30 + picY, 640, 480, memdc, 0, 0, SRCCOPY);
				//  SetDIBitsToDevice(hdc, 10 + picX + tableX + xPos, 30 + picY + yPos, bmWidth, -bmHeight, 0, 0, 0, -bmHeight, bmImage, bmImageHeader, DIB_RGB_COLORS);
			}
		}

		if (globalView && !(*curLoop)->Head.flags)
		{
			if ((*curCell)->cellImage->image != (*curCell)->bmImage)
			{
				delete (*curCell)->bmImage;

				if ((*curCell)->bmInfo)
					delete (*curCell)->bmInfo;

				(*curCell)->bmInfo = 0;
				(*curCell)->bmImage = 0;
			}

			if (!(*curCell)->bmInfo || !(*curCell)->bmImage)
				(*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);

			CelHeaderView *bCell = new CelHeaderView;
			bCell = (CelHeaderView*)&(*curCell)->Head;

			skipColor = (*curCell)->bmInfo->bmiColors[bCell->skip];

			int xPos = bCell->xHot;
			int yPos = bCell->yHot;

			int bmWidth = (*curCell)->bmInfo->bmiHeader.biWidth;
			int bmHeight = (*curCell)->bmInfo->bmiHeader.biHeight;

			HBITMAP hbm = CreateCompatibleBitmap(hdc, bmWidth, -bmHeight);

			HDC memdc = CreateCompatibleDC(hdc);
			SelectObject(memdc, hbm);
			SetDIBitsToDevice(memdc, 0, 0, bmWidth, -bmHeight, 0, 0, 0, -bmHeight,
							  (*curCell)->bmImage, (*curCell)->bmInfo, DIB_RGB_COLORS);

			TransparentBlt(hdc, 10 + picX + tableX, 30 + picY, (bmWidth * MagnifyFactor) / 100, (-bmHeight * MagnifyFactor) / 100, memdc, 0, 0, bmWidth, -bmHeight, RGB(skipColor.rgbRed, skipColor.rgbGreen, skipColor.rgbBlue));

			DeleteDC(memdc);

			// draw link points
			if (bCell->linkTableCount >= 1)
			{
				// Create a pen with a thickness of 6 * MagnifyFactor / 100 if there is only one link
				// or a thickness of 2 * MagnifyFactor / 100 if there are multiple links
				int pointSize = (4 * MagnifyFactor) / 100;
				int dottedSize = 1;

				// Calculate the x and y coordinates for the last link point
				int linkX = ((*curCell)->linkPoints[bCell->linkTableCount - 1].x * MagnifyFactor) / 100;
				int linkY = ((*curCell)->linkPoints[bCell->linkTableCount - 1].y * MagnifyFactor) / 100;

				HPEN dottedPen = CreatePen(PS_DOT, dottedSize, RGB(255, 0, 0));
				;
				HPEN solidPointPen = CreatePen(PS_SOLID, pointSize, RGB(255, 0, 0));
				HPEN accentPen = CreatePen(PS_SOLID, pointSize + 2, RGB(255, 255, 255));

				// Select the accent pen for drawing and draw a point at the position of the last link point
				SelectObject(hdc, accentPen);
				MoveToEx(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY, NULL);
				LineTo(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY);

				// Select the solid pen for drawing and draw a point at the position of the last link point
				SelectObject(hdc, solidPointPen);
				MoveToEx(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY, NULL);
				LineTo(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY);

				// Select the dotted pen for drawing
				SelectObject(hdc, dottedPen);

				// Iterate through all the link points and draw lines between them
				for (int i = 0; i < bCell->linkTableCount; i++)
				{
					CelHeaderView *tCell = new CelHeaderView;
					bCell = (CelHeaderView*)&globalView->loops[curLoopIndex]->cells[curCellIndex]->Head;

					// Calculate the x and y shift values for the current cell
					xPos = tCell->xHot;
					yPos = tCell->yHot;

					// Calculate the x and y coordinates for the current link point
					linkX = ((*curCell)->linkPoints[i].x * MagnifyFactor) / 100;
					linkY = ((*curCell)->linkPoints[i].y * MagnifyFactor) / 100;

					// Calculate the position of the current link point
					int linkPosX = 10 + picX + tableX + linkX;
					int linkPosY = 30 + picY + linkY;

					// Calculate the color of the pen for the current link point
					int colorStep = 255 / bCell->linkTableCount;
					int colorRed = 255 - (colorStep * i);
					int colorBlue = (colorStep * i);

					// Create a dotted pen with the calculated color and size
					HPEN dottedPen = CreatePen(PS_DOT, dottedSize, RGB(colorRed, 0, colorBlue));

					// Select the dotted pen for drawing
					SelectObject(hdc, dottedPen);

					// Draw a dotted line from the previous link point to the current one
					LineTo(hdc, linkPosX, linkPosY);

					// Select the accent pen for drawing and draw a point at the position of the last link point
					SelectObject(hdc, accentPen);
					MoveToEx(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY, NULL);
					LineTo(hdc, 10 + picX + tableX + linkX, 30 + picY + linkY);

					// Create a solid point pen with the calculated color and size
					HPEN solidPointPen = CreatePen(PS_SOLID, pointSize, RGB(colorRed, 0, colorBlue));

					// Select the solid point pen for drawing
					SelectObject(hdc, solidPointPen);

					// Draw a solid point at the current link point
					MoveToEx(hdc, linkPosX, linkPosY, NULL);
					LineTo(hdc, linkPosX, linkPosY);
				}
			}
		}

		if (globalPicture)
		{
			if ((*curCell)->cellImage->image != (*curCell)->bmImage)
			{
				delete (*curCell)->bmImage;

				if ((*curCell)->bmInfo)
					delete (*curCell)->bmInfo;

				(*curCell)->bmInfo = 0;
				(*curCell)->bmImage = 0;
			}

			if (!(*curCell)->bmInfo || !(*curCell)->bmImage)
				(*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);

			int xPos = 0;
			int yPos = 0;

			CelHeaderPic *bCell = new CelHeaderPic;
			bCell = (CelHeaderPic*)&(*curCell)->Head;

			skipColor = (*curCell)->bmInfo->bmiColors[bCell->skip];

			if (gPosCells == 1)
			{
				xPos = bCell->xpos * MagnifyFactor / 100;
				yPos = bCell->ypos * MagnifyFactor / 100;
			}

			int zDepth = bCell->priority;

			int bmWidth = (*curCell)->bmInfo->bmiHeader.biWidth;
			int bmHeight = (*curCell)->bmInfo->bmiHeader.biHeight;

			HBITMAP hbm = CreateCompatibleBitmap(hdc, bmWidth, -bmHeight);
			HDC memdc = CreateCompatibleDC(hdc);
			SelectObject(memdc, hbm);
			SetDIBitsToDevice(memdc, 0, 0, bmWidth, -bmHeight, 0, 0, 0, -bmHeight, (*curCell)->bmImage, (*curCell)->bmInfo, DIB_RGB_COLORS);

			TransparentBlt(hdc, 10 + picX + tableX + xPos, 30 + picY + yPos, (bmWidth * MagnifyFactor) / 100, (-bmHeight * MagnifyFactor) / 100, memdc, 0, 0, bmWidth, -bmHeight, RGB(skipColor.rgbRed, skipColor.rgbGreen, skipColor.rgbBlue));
			DeleteDC(memdc);

			// draw all cells
			if (curCellIndex == 0)
				for (int i = 1; i < globalPicture->CellsCount(); i++)
				{

					if (globalPicture->cells[i]->cellImage->image != globalPicture->cells[i]->bmImage)
					{
						delete globalPicture->cells[i]->bmImage;

						if (globalPicture->cells[i]->bmInfo)
							delete globalPicture->cells[i]->bmInfo;

						globalPicture->cells[i]->bmInfo = 0;
						globalPicture->cells[i]->bmImage = 0;
					}

					if (!globalPicture->cells[i]->bmInfo || !globalPicture->cells[i]->bmImage)
						globalPicture->cells[i]->GetImage(&globalPicture->cells[i]->bmInfo, &globalPicture->cells[i]->bmImage);

					if (globalPicture->cells[i]->bmInfo)
					{
						CelHeaderPic *bCell = new CelHeaderPic;
						bCell = (CelHeaderPic*)&globalPicture->cells[i]->Head;

						xPos = (bCell->xpos * MagnifyFactor) / 100;
						yPos = (bCell->ypos * MagnifyFactor) / 100;
						zDepth = (bCell->priority * MagnifyFactor) / 100;

						bmWidth = globalPicture->cells[i]->bmInfo->bmiHeader.biWidth;
						bmHeight = globalPicture->cells[i]->bmInfo->bmiHeader.biHeight;

						hbm = CreateCompatibleBitmap(hdc, bmWidth, -bmHeight);
						memdc = CreateCompatibleDC(hdc);
						SelectObject(memdc, hbm);

						SetDIBitsToDevice(memdc, 0, 0, bmWidth, -bmHeight, 0, 0, 0, -bmHeight,
										  globalPicture->cells[i]->bmImage, globalPicture->cells[i]->bmInfo, DIB_RGB_COLORS);

						TransparentBlt(hdc, 10 + picX + tableX + xPos, 30 + picY + yPos, (bmWidth * MagnifyFactor) / 100, (-bmHeight * MagnifyFactor) / 100, memdc, 0, 0, bmWidth, -bmHeight, RGB(skipColor.rgbRed, skipColor.rgbGreen, skipColor.rgbBlue));

						DeleteDC(memdc);
					}
				}
			//

			if (showpbars)
			{
				if (globalPicture->format == _PIC_11)
				{
					for (int i = 0; i < 14; i++)
					{
						CelBase *bCell = new CelBase;
						bCell = (CelBase *)&globalPicture->cells[curCellIndex]->Head;

						MoveToEx(hdc, 5 + picX + tableX, 30 + picY + (globalPicture->Head.pic11.priLines[i] * MagnifyFactor) / 100, NULL);
						LineTo(hdc, 15 + picX + tableX + (bCell->xDim * MagnifyFactor) / 100, 30 + picY + (globalPicture->Head.pic11.priLines[i] * MagnifyFactor) / 100);
					}
				}

				if (!globalPicture->format == _PIC_11)
				{
					// Create a red pen with a thickness of 2 * MagnifyFactor / 100
					HPEN redpen = CreatePen(PS_SOLID, (2 * MagnifyFactor) / 100, RGB(255, 0, 0));

					// Select the red pen for drawing
					SelectObject(hdc, redpen);

					for (int i = 1; i < globalPicture->CellsCount(); i++)
					{
						CelHeaderPic *bCell = new CelHeaderPic;
						bCell = (CelHeaderPic *)&globalPicture->cells[i]->Head;

						xPos = bCell->xpos;
						yPos = bCell->ypos;
						bmWidth = globalPicture->cells[i]->bmInfo->bmiHeader.biWidth;
						bmHeight = globalPicture->cells[i]->bmInfo->bmiHeader.biHeight;
						int zOffset = bCell->ypos + bmHeight - (bCell->priority * zScale) / 100;
						zDepth = yPos + bmHeight - zOffset;
						if (curCellIndex == i || curCellIndex == 0)
						{
							MoveToEx(hdc, 5 + picX + tableX + (xPos * MagnifyFactor) / 100, 30 + picY + (zDepth * MagnifyFactor) / 100, NULL);
							LineTo(hdc, 15 + picX + tableX + (xPos * MagnifyFactor) / 100 + (bCell->xDim * MagnifyFactor) / 100, 30 + picY + (zDepth * MagnifyFactor) / 100);
						}
					}
				}
			}
		}

		// draw cell info
		if (curCell)
		{

			CelBase *bCell = new CelBase;
			bCell = (CelBase*)&(*curCell)->Head;

			if (globalView)
			{
				char finalstr[64];
				sprintf(finalstr, INTERFACE_LOOPSSTR, curLoopIndex + 1, globalView->Head.view32.loopCount);
				SetRect(&rc, 25, 0, 125, 20);
				DrawText(hdc, finalstr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

				if (curLoop)
				{
					if ((*curLoop)->Head.flags)
					{
						char mirr[64];
						sprintf(mirr, INTERFACE_MIRROREDSTR, (*curLoop)->Head.altLoop + 1);
						SetRect(&rc, 125, 0, 325, 20);
						DrawText(hdc, mirr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					}
					else
					{
						sprintf(finalstr, INTERFACE_CELLSSTR, curCellIndex + 1, (*curLoop)->Head.numCels);
						SetRect(&rc, 125, 0, 225, 20);
						DrawText(hdc, finalstr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					}
				}

				if (!(*curLoop)->Head.flags)
				{

					char finalstr[64];
					sprintf(finalstr, INTERFACE_SKIPCOLORSTR, bCell->skip);
					SetRect(&rc, 250, 0, 390, 20);
					DrawText(hdc, finalstr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					// or TextOut(hdc, 230,0, finalstr,strlen(finalstr));

					if ((*curCell)->bmInfo)
					{
						HPEN outline = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
						SelectObject(hdc, outline);

						RGBQUAD tquad = (*curCell)->bmInfo->bmiColors[bCell->skip];
						HBRUSH tbrush = CreateSolidBrush(RGB(tquad.rgbRed, tquad.rgbGreen, tquad.rgbBlue));
						SelectBrush(hdc, tbrush);
						Rectangle(hdc, 225, 2, 245, 18);

						DeleteObject(tbrush);
					}

					if ((*curCell)->changed)
					{
						SetTextColor(hdc, RGB(255, 0, 0));
						SetRect(&rc, 480, 0, 530, 20);
						DrawText(hdc, INTERFACE_CHANGEDSTR, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
						SetTextColor(hdc, RGB(0, 0, 0));
					}
				}
			}

			if (globalPicture)
			{
				// Draw transparency info
				char finalstr[64];
				sprintf(finalstr, INTERFACE_SKIPCOLORSTR, bCell->skip);
				SetRect(&rc, 250, 0, 390, 20);
				DrawText(hdc, finalstr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
				// or TextOut(hdc, 230,0, finalstr,strlen(finalstr));
				if ((*curCell)->bmInfo)
				{
					RGBQUAD tquad = (*curCell)->bmInfo->bmiColors[bCell->skip];
					HBRUSH tbrush = CreateSolidBrush(RGB(tquad.rgbRed, tquad.rgbGreen, tquad.rgbBlue));
					SelectBrush(hdc, tbrush);
					Rectangle(hdc, 225, 2, 245, 18);

					DeleteObject(tbrush);
				}

				// Draw version info
				char vers[10];
				strcpy(vers, (globalPicture->format == _PIC_11 ? "SCI1.1" : "SCI32"));
				SetRect(&rc, 25, 0, 100, 20);
				DrawText(hdc, vers, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

				// char finalstr[64];
				sprintf(finalstr, INTERFACE_CELLSSTR, curCellIndex + 1, globalPicture->CellsCount());
				SetRect(&rc, 125, 0, 250, 20);
				DrawText(hdc, finalstr, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

				if ((*curCell)->changed)
				{
					SetTextColor(hdc, RGB(255, 0, 0));
					SetRect(&rc, 480, 0, 530, 20);
					DrawText(hdc, INTERFACE_CHANGEDSTR, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					SetTextColor(hdc, RGB(0, 0, 0));
				}
			}
		}

		EndPaint(hWnd, &ps);
		break;
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

// Message handler for about box.
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
