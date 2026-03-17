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

// Define skipColor here since it's used by display functions
RGBQUAD skipColor;

// ============================================================================
// STATIC HELPER FUNCTIONS (INTERNAL TO DISPLAY MODULE)
// ============================================================================

// Simple display origin calculation
static POINT GetDisplayOrigin() {
    POINT origin;
    // picX/picY are scroll offsets (negative when scrolled right/down)
    origin.x = UI_LEFT_MARGIN + picX + tableX;
    origin.y = UI_TOP_MARGIN + picY;
    return origin;
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
    if (!text || !*text) return;
    
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

// ============================================================================
// CORE DISPLAY FUNCTIONS
// ============================================================================

void ForceImageRefresh() {
    if (isPicture && globalPicture && curCell && (*curCell)) {
        (*curCell)->bmInfo = nullptr;
        (*curCell)->bmImage = nullptr;
        ShowCell(curCellIndex);
    } else if (globalView) {
        ShowLoopCell(curLoopIndex, curCellIndex);
    }
    
    InvalidateRect(hWnd, NULL, TRUE);
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

    // Calculate scaling without magnification
    int scaleX = (bm.bmWidth * gReferenceScaleX) / 10000;
    int scaleY = (bm.bmHeight * gReferenceScaleY) / 10000;
    
    // Prevent zero or negative scaling
    if (scaleX < 1) scaleX = 1;
    if (scaleY < 1) scaleY = 1;

    int xOrigin = (scaleX >> 1) - gReferenceXHot;
    int yOrigin = scaleY - gReferenceYHot;

    int xHot = bCell->xHot;
    int yHot = bCell->yHot;

    // Calculate position
    int xPos = gReferenceLinkPointX - xOrigin + xHot;
    int yPos = gReferenceLinkPointY - yOrigin + yHot;

    // Safe link point access
    if (IsValidIndex(gReferenceLinkPoint - 1, MAX_LINK_POINTS) && gReferenceLinkPoint > 0) {
        int linkIndex = gReferenceLinkPoint - 1;
        xPos = (*curCell)->linkPoints[linkIndex].x - xOrigin + xHot;
        yPos = (*curCell)->linkPoints[linkIndex].y - yOrigin + yHot;
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

    int bmWidth  = bmInfo->bmiHeader.biWidth;
    int bmHeight = -bmInfo->bmiHeader.biHeight; // biHeight is negative (top-down)

    // Apply zoom: zScale is a percentage (100 = 1x, 200 = 2x, 50 = 0.5x)
    int scale = (zScale > 0) ? zScale : 100;
    int dstW = (bmWidth  * scale) / 100;
    int dstH = (bmHeight * scale) / 100;
    if (dstW < 1) dstW = 1;
    if (dstH < 1) dstH = 1;

    int dstX = origin.x + (xPos * scale) / 100;
    int dstY = origin.y + (yPos * scale) / 100;

    // Use a memory DC so we can TransparentBlt with zoom
    HBITMAP hbm = CreateCompatibleBitmap(hdc, dstW, dstH);
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memdc, hbm);

    // Stretch the DIB into the memory DC at the zoomed size
    StretchDIBits(memdc, 0, 0, dstW, dstH,
                  0, 0, bmWidth, bmHeight,
                  bmImage, bmInfo, DIB_RGB_COLORS, SRCCOPY);

    // Blit to screen with transparency
    TransparentBlt(hdc, dstX, dstY, dstW, dstH,
                   memdc, 0, 0, dstW, dstH,
                   RGBQUADToColorRef(skipColor));

    SelectObject(memdc, oldBitmap);
    DeleteObject(hbm);
    DeleteDC(memdc);
}

void DisplayCell(HDC hdc, int index) {
    if (!globalPicture || index < 0 || index >= globalPicture->CellsCount()) return;

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

    // Draw frame
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    POINT origin = GetDisplayOrigin();
    
    int imageWidth = globalPicture->cells[index]->bmInfo->bmiHeader.biWidth;
    int imageHeight = abs(globalPicture->cells[index]->bmInfo->bmiHeader.biHeight);
    
    int scale = (zScale > 0) ? zScale : 100;
    RECT frameRect = {
        origin.x + (bCell->xpos * scale) / 100 - UI_PADDING,
        origin.y + (bCell->ypos * scale) / 100 - UI_PADDING,
        origin.x + (bCell->xpos * scale) / 100 + (imageWidth  * scale) / 100 + UI_PADDING,
        origin.y + (bCell->ypos * scale) / 100 + (imageHeight * scale) / 100 + UI_PADDING
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

    // Draw frame
    const FotoSCIhopStyles::UnifiedColors& colors = FotoSCIhopStyles::GetCurrentColors();
    POINT origin = GetDisplayOrigin();
    
    int imageWidth = (*curCell)->bmInfo->bmiHeader.biWidth;
    int imageHeight = abs((*curCell)->bmInfo->bmiHeader.biHeight);
    
    int scale2 = (zScale > 0) ? zScale : 100;
    RECT frameRect = {
        origin.x + (bCell->xHot * scale2) / 100 - UI_PADDING,
        origin.y + (bCell->yHot * scale2) / 100 - UI_PADDING,
        origin.x + (bCell->xHot * scale2) / 100 + (imageWidth  * scale2) / 100 + UI_PADDING,
        origin.y + (bCell->yHot * scale2) / 100 + (imageHeight * scale2) / 100 + UI_PADDING
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
    DisplayImage(hdc, (*curCell)->bmImage, (*curCell)->bmInfo, bCell->xHot, bCell->yHot);
}

void DisplayLinkPoints(HDC hdc) {
    if (!curCell || !(*curCell)) return;

    CelHeaderView *bCell = (CelHeaderView *)&(*curCell)->Head;
    if (bCell->linkTableCount <= 0) return;

    POINT origin = GetDisplayOrigin();
    int scale = (zScale > 0) ? zScale : 100;

    int xHotScaled = (bCell->xHot * scale) / 100;
    int yHotScaled = (bCell->yHot * scale) / 100;
    int pointSize = LINK_POINT_BASE_SIZE;

    auto scaledX = [&](int lx) { return origin.x + xHotScaled + (lx * scale) / 100; };
    auto scaledY = [&](int ly) { return origin.y + yHotScaled + (ly * scale) / 100; };

    HPEN accentPen = CreatePen(PS_SOLID, pointSize + LINK_POINT_ACCENT_THICKNESS, COLOR_WHITE);

    int lastIndex = (bCell->linkTableCount - 1 < MAX_LINK_POINTS - 1) ? bCell->linkTableCount - 1 : MAX_LINK_POINTS - 1;
    int xPos = scaledX((*curCell)->linkPoints[lastIndex].x);
    int yPos = scaledY((*curCell)->linkPoints[lastIndex].y);

    HPEN lastPointPen = CreatePen(PS_SOLID, pointSize, COLOR_RED);
    DrawPoint(hdc, xPos, yPos, accentPen);
    DrawPoint(hdc, xPos, yPos, lastPointPen);

    if (bCell->linkTableCount <= 1) {
        SafeDeleteGDIObject(accentPen);
        SafeDeleteGDIObject(lastPointPen);
        return;
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, accentPen);

    int maxPoints = (bCell->linkTableCount < MAX_LINK_POINTS) ? bCell->linkTableCount : MAX_LINK_POINTS;
    for (int i = 0; i < maxPoints; i++) {
        xPos = scaledX((*curCell)->linkPoints[i].x);
        yPos = scaledY((*curCell)->linkPoints[i].y);

        COLORREF pointColor = InterpolateColor(i, bCell->linkTableCount - 1, COLOR_RED, RGB(0, 0, 255));

        HPEN coloredDottedPen = CreatePen(PS_DOT, DOTTED_LINE_THICKNESS, pointColor);
        HPEN coloredSolidPen  = CreatePen(PS_SOLID, pointSize, pointColor);

        SelectObject(hdc, coloredDottedPen);
        LineTo(hdc, xPos, yPos);

        DrawPoint(hdc, xPos, yPos, accentPen);
        DrawPoint(hdc, xPos, yPos, coloredSolidPen);

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

    HPEN redpen = CreatePen(PS_SOLID, 1, COLOR_RED);
    HPEN oldPen = (HPEN)SelectObject(hdc, redpen);

    if (globalPicture->format == _PIC_11) {
        // SCI 1.1 priority lines
        if (!IsValidIndex(curCellIndex, globalPicture->CellsCount())) {
            SelectObject(hdc, oldPen);
            SafeDeleteGDIObject(redpen);
            return;
        }
        
        CelBase *bCell = (CelBase *)&globalPicture->cells[curCellIndex]->Head;
        int xSpan = xOrigin + bCell->xDim;
        
        for (int i = 0; i < MAX_PRIORITY_LINES; i++) {
            int yPos = yOrigin + globalPicture->Head.pic11.priLines[i];
            
            MoveToEx(hdc, xOrigin, yPos, NULL);
            LineTo(hdc, xSpan, yPos);
        }
    } else {
        // SCI32 priority lines
        int cellCount = globalPicture->CellsCount();
        
        for (int i = 1; i < cellCount; i++) {
            // Skip if not displaying this cell
            if (curCellIndex != i && curCellIndex != 0) continue;
            
            CelHeaderPic *bCell = (CelHeaderPic *)&globalPicture->cells[i]->Head;

            int xPos = xOrigin + bCell->xpos;
            int xSpan = xPos + bCell->xDim;
            
            // Prevent division by zero
            int priorityScale = (zScale > 0) ? zScale : 100;
            int zOffset = bCell->ypos + bCell->yDim - (bCell->priority * priorityScale / 100);
            int zDepth = bCell->ypos + bCell->yDim - zOffset;
            int yPos = yOrigin + zDepth;
            
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

    // Draw palette grid
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

    // Missing colors indicator
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
    
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    // Themed info panel background
    RECT infoPanel = {10, 2, clientRect.right - 10, 23};
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
        
        // Skip color info for V56 files
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
        
        // Skip color info for P56 files
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

    // Changed indicator
    if ((*curCell)->changed) {
        FotoSCIhopStyles::DrawStatusText(hdc, "* Modified", xPos, 5, 80, 15, FotoSCIhopStyles::STATUS_WARNING);
        xPos += 85;
    }

    // Zoom level indicator (right-aligned)
    {
        char zoomBuf[16];
        sprintf(zoomBuf, "%d%%", zScale);
        FotoSCIhopStyles::DrawThemedText(hdc, zoomBuf, clientRect.right - 55, 5, 45, 15);
    }
}