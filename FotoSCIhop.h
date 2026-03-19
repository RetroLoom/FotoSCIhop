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
#include "display.h"
#include <set>
#include <string>
#include <vector>
#include "librealmpal.h"
#include "imgui_integration.h"
#include "imgui.h"
#include "fotoscihop_styles.h"

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
// UI STATE
// ============================================================================

// Display settings
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

// CLI functions
int cliExport(char *name);

// Magic wand and color sampling functions
bool SampleColorAtScreenPosition(int screenX, int screenY, int& colorIndex);

// Dialog procedure
BOOL CALLBACK DoImportImageDlg(HWND hwndDlg,
                               UINT message,
                               WPARAM wParam,
                               LPARAM lParam);

// Core application functions
void ShowLoopCell(unsigned char newloop, unsigned char newcell);
void ShowCell(unsigned char newcell);
bool ImportPaletteFromBMP(const char* filename, Palette* targetPal);
bool ImportPaletteFromBMP(const char* filename, Palette* targetPal,
                           const uint8_t* pixels, int w, int h, int rowStride);
bool ImportBMPToCurrentCell(const char* filename, bool applyPalette);
void UpdateScrollBars(HWND hwnd);

// Dialog functions
extern bool g_dialogActive;
void ForceDisplayRefresh();
void RenderPropertiesDialog();
void RenderAboutDialog(); 
void RenderClutGeneratorDialog();
void RenderRealmpalDialog();
void RenderPreferencesDialog();
void RenderPaletteManagerDialog();

// Realmpal dialog state variables
extern std::string g_realmpalInputFile;
extern std::string g_realmpalPaletteFile;
extern std::string g_realmpalExtraFile;
extern bool g_requestInputDialog;
extern bool g_requestPaletteDialog;
extern bool g_requestExtraDialog;

// Palette Manager dialog state variables
extern std::string g_palMgrInputFile;
extern std::string g_palMgrOutputFile;  
extern bool g_requestPalMgrInputDialog;
extern bool g_requestPalMgrOutputDialog;

#endif // FOTOSCIHOP_H