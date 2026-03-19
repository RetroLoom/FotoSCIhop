/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  File I/O functions header
 *
 */

#ifndef FILEIO_H
#define FILEIO_H

#include <windows.h>

// Forward declarations
class Palette;
struct P56file32;
struct V56file;

// ============================================================================
// FILE I/O CONSTANTS
// ============================================================================

// File operation results
#define ID_NOERROR 0
#define ID_CANTOPENFILE 1
#define ID_WRONGHEADER 2
#define ID_WRONGCELLRECSIZE 3
#define ID_WRONGPALETTELOC 4
#define ID_WRONGLOOPRECSIZE 5

// ============================================================================
// GLOBAL VARIABLES (EXTERNAL DECLARATIONS)
// ============================================================================

// File name variables (defined in main application)
extern char szFileName[MAX_PATH];
extern char szNextFileName[MAX_PATH];

// ============================================================================
// CORE FILE OPERATIONS
// ============================================================================

// Main file operations
BOOL DoFileOpen(HWND hwnd, const char *filename, const char *ext);
BOOL DoFileSave(HWND hwnd);
BOOL DoFileSaveAs(HWND hwnd);
BOOL DoNextFile(HWND hwnd);
int DoSaveChangesDialog(HWND hwnd);

// ============================================================================
// BITMAP IMPORT/EXPORT FUNCTIONS
// ============================================================================

// Core BMP functions
bool ExportCurrentCellBMP(const char* filename);
bool ImportBMPToCurrentCell(const char* filename, bool applyPalette);

// Unified BMP functions (GUI or CLI)
BOOL ExportBitmapUnified(HWND hwnd, const char* path);
BOOL ImportBitmapUnified(HWND hwnd, const char* path, BOOL applyPalette);

// ============================================================================
// PALETTE IMPORT/EXPORT FUNCTIONS
// ============================================================================

// Palette functions
bool ImportPaletteFromBMP(const char* filename, Palette* targetPal);
// Pixel-aware overload: scans image pixels to assign remap flags correctly.
// pixels/w/h/rowStride describe the DWORD-padded 8-bit indexed image buffer.
bool ImportPaletteFromBMP(const char* filename, Palette* targetPal,
                           const uint8_t* pixels, int w, int h, int rowStride);
BOOL ImportPaletteUnified(HWND hwnd, const char* path);
BOOL ExportPaletteUnified(HWND hwnd, const char* path);

// ============================================================================
// CLI FUNCTIONS
// ============================================================================

// Command-line interface functions
int cliExport(char* baseName);
int cliImport(char* baseName);
int cliScale(int scaleX, int scaleY);
int cliSetHeader(int vanishX, int viewAngle);

// Cell/Loop manipulation
BOOL DoAddCells(int loop, int base, int amount);
BOOL DoAddLoops(int base, int amount);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// File validation helpers
bool ValidateBitmapHeader(FILE* file, BITMAPFILEHEADER& fileHeader, BITMAPINFOHEADER& infoHeader);

#endif // FILEIO_H