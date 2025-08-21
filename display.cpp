/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Display and rendering functions implementation
 *
 */

#include "stdafx.h"
#include "display.h"
#include "FotoSCIhop.h"
#include "fotoscihop_styles.h"
#include "palette.h"
#include "p56files.h"
#include "v56files.h"
#include "scicell.h"
#include "sciloop.h"
#include "language.h"
#include <set>

// ============================================================================
// GLOBAL STATE VARIABLES (DEFINITIONS)
// ============================================================================
int g_scrollX = 0;
int g_scrollY = 0;
int g_maxScrollX = 0;
int g_maxScrollY = 0;
int g_clientWidth = 0;
int g_clientHeight = 0;
bool g_isPanning = false;
POINT g_lastPanPoint = {0, 0};
int g_currentZoomIndex = 3; // Start at 100%

// Define skipColor here since it's used by display functions
RGBQUAD skipColor;

// ============================================================================
// STATIC HELPER FUNCTIONS (INTERNAL TO DISPLAY MODULE)
// ============================================================================

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

// Updated GetDisplayOrigin to account for scrolling
static POINT GetDisplayOriginWithScroll() {
    POINT origin = GetDisplayOrigin();
    origin.x -= g_scrollX;
    origin.y -= g_scrollY;
    return origin;
}

// ============================================================================
// CORE DISPLAY FUNCTIONS
// ============================================================================

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
        origin.x - UI_PADDING + bCell->xHot,
        origin.y - UI_PADDING + bCell->yHot,
        origin.x + imageWidth + UI_PADDING + bCell->xHot,
        origin.y + imageHeight + UI_PADDING + bCell->yHot
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

void DrawCellInfo(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    CelBase *bCell = (CelBase *)&(*curCell)->Head;
    
    // Themed info panel background - extend to accommodate zoom controls
    RECT infoPanel = {10, 2, g_clientWidth - 10, 23};
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
        xPos += 85;
    }

    // Add some spacing before zoom controls
    xPos += 20;

    // Draw zoom controls inline after cell info
    DrawZoomControls(hdc, xPos);
}

void DrawZoomControls(HDC hdc, int startX) {
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    
    // Calculate available space for zoom controls
    int availableSpace = g_clientWidth - startX - 20; // 20px right margin
    
    // Only draw if we have enough space (minimum 200px for all controls)
    if (availableSpace < 200) {
        return; // Not enough space, skip zoom controls
    }
    
    int xPos = startX;
    
    // Zoom percentage text
    char zoomText[64];
    sprintf(zoomText, "Zoom: %d%%", MagnifyFactor);
    FotoSCIhopStyles::DrawThemedText(hdc, zoomText, xPos, 5, 80, 15);
    xPos += 85;
    
    // Compact zoom buttons - smaller than the old floating panel
    int buttonWidth = 20;
    int buttonHeight = 15;
    int buttonY = 4;
    
    RECT zoomOutBtn = {xPos, buttonY, xPos + buttonWidth, buttonY + buttonHeight};
    xPos += buttonWidth + 2;
    
    RECT zoomInBtn = {xPos, buttonY, xPos + buttonWidth, buttonY + buttonHeight};
    xPos += buttonWidth + 5; // Extra space before next group
    
    RECT fitBtn = {xPos, buttonY, xPos + buttonWidth + 5, buttonY + buttonHeight}; // Slightly wider for "Fit"
    xPos += buttonWidth + 7;
    
    RECT resetBtn = {xPos, buttonY, xPos + buttonWidth + 10, buttonY + buttonHeight}; // Wider for "100%"
    
    // Draw buttons with compact styling
    FotoSCIhopStyles::DrawThemedButton(hdc, zoomOutBtn, "-", false, false, g_currentZoomIndex > 0);
    FotoSCIhopStyles::DrawThemedButton(hdc, zoomInBtn, "+", false, false, g_currentZoomIndex < ZOOM_LEVEL_COUNT - 1);
    FotoSCIhopStyles::DrawThemedButton(hdc, fitBtn, "Fit", false, false, true);
    FotoSCIhopStyles::DrawThemedButton(hdc, resetBtn, "100%", false, false, true);
}

// ============================================================================
// ZOOM CONTROL FUNCTIONS
// ============================================================================

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
        
    // This allows the click to complete immediately while the redraw happens asynchronously
    PostMessage(hWnd, WM_USER + 1, 0, 0); // Custom message for deferred update
    
    #ifdef _DEBUG
    char debugMsg[128];
    sprintf(debugMsg, "[DEBUG] SetZoomLevel: %d%% -> Index %d, Deferred update\n", 
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

// ============================================================================
// COORDINATE CONVERSION FUNCTIONS
// ============================================================================

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
// PANNING FUNCTIONS
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
// UI INTERACTION FUNCTIONS
// ============================================================================

bool HandleZoomControlClick(int x, int y) {
    // Early exit if not in top bar
    if (y < 2 || y > 23) {
        return false;
    }
    
    // OPTIMIZATION: Use static variables to cache button positions
    // Only recalculate when window size changes
    static int lastClientWidth = 0;
    static RECT cachedZoomOutBtn = {0};
    static RECT cachedZoomInBtn = {0};
    static RECT cachedFitBtn = {0};
    static RECT cachedResetBtn = {0};
    static bool validCache = false;
    
    // Invalidate cache if window width changed
    if (lastClientWidth != g_clientWidth) {
        validCache = false;
        lastClientWidth = g_clientWidth;
    }
    
    // Calculate button positions only if cache is invalid
    if (!validCache) {
        int xPos = 20;
        
        // Account for cell info width (simplified calculation)
        if (globalView) {
            xPos += 295; // Estimated total width for view info
        }
        if (globalPicture) {
            xPos += 255; // Estimated total width for picture info
        }
        if (curCell && (*curCell) && (*curCell)->changed) {
            xPos += 85; // Modified indicator
        }
        
        xPos += 20; // Spacing
        xPos += 85; // Zoom text
        
        // Check if we have space
        int availableSpace = g_clientWidth - xPos - 20;
        if (availableSpace < 200) {
            validCache = true; // Cache the "no buttons" state
            return false;
        }
        
        // Cache button positions
        int buttonWidth = 20;
        int buttonHeight = 15;
        int buttonY = 4;
        
        cachedZoomOutBtn = {xPos, buttonY, xPos + buttonWidth, buttonY + buttonHeight};
        xPos += buttonWidth + 2;
        
        cachedZoomInBtn = {xPos, buttonY, xPos + buttonWidth, buttonY + buttonHeight};
        xPos += buttonWidth + 5;
        
        cachedFitBtn = {xPos, buttonY, xPos + buttonWidth + 5, buttonY + buttonHeight};
        xPos += buttonWidth + 7;
        
        cachedResetBtn = {xPos, buttonY, xPos + buttonWidth + 10, buttonY + buttonHeight};
        
        validCache = true;
    }
    
    // Quick button hit testing using cached positions
    if (validCache) {
        if (PtInRect(&cachedZoomOutBtn, {x, y}) && g_currentZoomIndex > 0) {
            ZoomOut();
            return true;
        }
        if (PtInRect(&cachedZoomInBtn, {x, y}) && g_currentZoomIndex < ZOOM_LEVEL_COUNT - 1) {
            ZoomIn();
            return true;
        }
        if (PtInRect(&cachedFitBtn, {x, y})) {
            ZoomToFit();
            return true;
        }
        if (PtInRect(&cachedResetBtn, {x, y})) {
            ZoomTo100();
            return true;
        }
    }
    
    return false;
}