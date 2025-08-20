/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Global application definitions and declarations
 *
 */

#ifndef FOTOSCIHOP_H
#define FOTOSCIHOP_H

// ============================================================================
// SYSTEM INCLUDES
// ============================================================================
#include "resource.h"
#include "language.h"
#include "list.h"
#include "palette.h"
#include "scicell.h"
#include "sciloop.h"
#include "p56files.h"
#include "v56files.h"
#include "english.h"
#include <set>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class ClutGenerator;

// ============================================================================
// CONSTANTS
// ============================================================================
#define MAX_ARG 512

// ============================================================================
// COMMAND LINE AND CONFIGURATION
// ============================================================================
extern char *argv[MAX_ARG];
extern char propstr[10240];
extern char gAppPath[MAX_PATH];

// Configuration file and settings
extern char gConfigIni[_MAX_PATH];
extern int gAppResX;
extern int gAppResY;
extern int zScale;
extern int gPosCells;
extern int gCliMode;
extern int gBaseMagnify;
extern int gCliEnabled;

// ============================================================================
// REFERENCE IMAGE SETTINGS
// ============================================================================
extern HWND hReferenceDialog;
extern float gReferenceScaleX;
extern float gReferenceScaleY;
extern char gReferenceBM[_MAX_PATH];
extern int gReferenceXHot;
extern int gReferenceYHot;
extern int gReferenceLinkPoint;
extern int gReferenceLinkPointX;
extern int gReferenceLinkPointY;
extern int gReferencePriority;
extern int gReferenceTransparentIndex;

// ============================================================================
// GLOBAL APPLICATION STATE
// ============================================================================

// Main data objects
extern P56file32 *globalPicture;
extern V56file *globalView;
extern bool isPicture;

// Current selection state
extern Cell **curCell;
extern Loop **curLoop;
extern int curCellIndex;
extern int curLoopIndex;

// Application state flags
extern bool datasaved;
extern bool showpbars;

// ============================================================================
// MAGIC WAND AND CLUT GENERATOR STATE
// ============================================================================

// Global state for magic wand tool
extern bool g_magicWandEnabled;
extern std::set<int> g_usedColorIndices;

// Global CLUT generator instance
extern ClutGenerator* g_clutGenerator;

// ============================================================================
// DISPLAY AND UI STATE
// ============================================================================

// Display settings
extern int MagnifyFactor;
extern int picX;
extern int picY;
extern int tableX;

// UI elements and drawing
extern RECT rc;
extern HWND hWndTopBar;
extern HFONT hfDefault;
extern RGBQUAD skipColor;
extern HWND hWnd;

// Image import settings
extern int colorLimit;
extern int tolerance;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
bool HandleCliCommands(char* cmdLine);
void ParseAppPath(void);

// CLI and palette functions
BOOL CLIPaletteImport(char *palette);
int cliExport(char *name);

// Magic wand and color sampling functions
bool SampleColorAtScreenPosition(int screenX, int screenY, int& colorIndex);

// Dialog procedure
BOOL CALLBACK DoImportImageDlg(HWND hwndDlg,
                               UINT message,
                               WPARAM wParam,
                               LPARAM lParam);

void RenderPropertiesDialog();
void RenderAboutDialog();
void RenderClutGeneratorDialog();
void RenderRealmpalDialog();
void ForceImageRefresh();

#endif // FOTOSCIHOP_H