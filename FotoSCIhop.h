/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  global stuff
 *
 */
 
#ifndef FOTOSCIHOP_H
#define FOTOSCIHOP_H

#include "resource.h"
#include "language.h"
#include "list.h"
#include "palette.h"
#include "scicell.h"
#include "sciloop.h"
#include "p56files.h"
#include "v56files.h"

#include "english.h"
#define MAX_ARG 512
char *argv[MAX_ARG];
char propstr[10240]="";
char gAppPath[MAX_PATH];


// Dialog
HWND hPropertiesDialog;
HWND hLinkPointDialog;
void DoUpdatePropertiesProc(HWND hwndDlg);
void DoUpdateLinkPointProc(HWND hwndDlg);

// ini
char gConfigIni[_MAX_PATH];
int gAppResX = 700;
int gAppResY = 500;
int zScale = 100;
int gPosCells = 0;
int gCliMode = 0;
int gBaseMagnify = 100;
int gCliEnabled = 0;

// Reference Image
HWND hReferenceDialog;
float gReferenceScaleX = 100;
float gReferenceScaleY = 100;
char gReferenceBM[_MAX_PATH] = "reference.bmp";
int gReferenceXHot = 0;
int gReferenceYHot = 0;

//BOOL DoImageConversion(char fileName[MAX_PATH], int cLimit, int tolerance);
BOOL CLIPaletteImport(char *palette);
int cliExport(char *name);

P56file32 *globalPicture = NULL;
V56file *globalView = NULL;
bool isPicture=true;

RGBQUAD skipColor;

Cell **curCell =0;
Loop **curLoop =0;
int curCellIndex = 0;
int curLoopIndex = 0;

int MagnifyFactor = gBaseMagnify;
bool showpbars = false;

bool datasaved =true;

RECT rc;

HWND hWndTopBar;
HFONT hfDefault;

int picX = 0;
int picY = 30;
int tableX = 0;

// image import
int colorLimit = 255;
int tolerance = 50;

BOOL CALLBACK DoImportImageDlg(HWND hwndDlg,
                               UINT message,
                               WPARAM wParam,
                               LPARAM lParam
                               );


#endif
