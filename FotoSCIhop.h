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
#include <string>

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

void ForceImageRefresh();

// Function declarations
void ShowLoopCell(unsigned char newloop, unsigned char newcell);
void ShowCell(unsigned char newcell);
bool ImportPaletteFromBMP(const char* filename, Palette* targetPal);
bool ImportBMPToCurrentCell(const char* filename, bool applyPalette);

// Display constants
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

// Dialog functions
void RenderPropertiesDialog();
void RenderAboutDialog(); 
void RenderClutGeneratorDialog();
void RenderRealmpalDialog();

// Realmpal dialog state variables
extern std::string g_realmpalInputFile;
extern std::string g_realmpalPaletteFile;
extern std::string g_realmpalExtraFile;
extern bool g_requestInputDialog;
extern bool g_requestPaletteDialog;
extern bool g_requestExtraDialog;

#endif // FOTOSCIHOP_H