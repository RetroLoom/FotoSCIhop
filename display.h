/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Display and rendering functions header
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <windows.h>
#include <set>

// Forward declarations
class Palette;
struct P56file32;
struct V56file;
struct Cell;
struct Loop;

// ============================================================================
// DISPLAY CONSTANTS
// ============================================================================
static const int UI_LEFT_MARGIN = 10;
static const int UI_TOP_MARGIN = 30;
static const int UI_PRIORITY_MARGIN = 5;
static const int UI_INFO_HEIGHT = 20;
static const int UI_PADDING = 8;

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

// ============================================================================
// CORE DISPLAY FUNCTIONS
// ============================================================================
void ForceImageRefresh();

// Image display functions
void DisplayReferenceImage(HDC hdc);
void DisplayImage(HDC hdc, unsigned char *bmImage, BITMAPINFO *bmInfo, int xPos, int yPos);
void DisplayCell(HDC hdc, int index);
void DisplayCellWithFrame(HDC hdc, int index);
void DisplayCurrentView(HDC hdc);
void DisplayCurrentViewWithFrame(HDC hdc);
void DisplayCurrentPic(HDC hdc);
void DisplayCurrentPicWithFrame(HDC hdc);
void DisplayLinkPoints(HDC hdc);
void DisplayPriorityBars(HDC hdc);

// UI drawing functions
void DrawPaletteTable(HDC hdc);
void DrawCellInfo(HDC hdc);
void DrawPaletteStatusIndicators(HDC hdc, Palette* tpalette);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
RGBQUAD ExtractPaletteIndexFromBM(char *image, int index);

#endif // DISPLAY_H