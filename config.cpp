/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Configuration and settings management implementation
 *
 */

#include "stdafx.h"
#include "config.h"
#include "FotoSCIhop.h"
#include "fileio.h"
#include "display.h"
#include <cstdio>
#include <cstring>

// ============================================================================
// GLOBAL CONFIGURATION VARIABLES (DEFINITIONS)
// ============================================================================

// Command line arguments
char *argv[MAX_ARG];
char propstr[10240] = "";

// Application paths
char gAppPath[MAX_PATH];
char gConfigIni[_MAX_PATH];

// Application settings with defaults
int gAppResX = 700;
int gAppResY = 500;
int zScale = 100;
int gPosCells = 0;
int gCliMode = 0;
int gBaseMagnify = 100;
int gCliEnabled = 0;

// ============================================================================
// REFERENCE IMAGE SETTINGS (DEFINITIONS)
// ============================================================================
HWND hReferenceDialog;
float gReferenceScaleX = 100;
float gReferenceScaleY = 100;
char gReferenceBM[_MAX_PATH] = "reference.bmp";
int gReferenceXHot = 0;
int gReferenceYHot = 0;
int gReferenceLinkPoint = 0;
int gReferenceLinkPointX = 0;
int gReferenceLinkPointY = 0;
int gReferencePriority = 0;
int gReferenceTransparentIndex = 255;

// ============================================================================
// STATIC HELPER FUNCTIONS (INTERNAL TO CONFIG MODULE)
// ============================================================================

#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecation warnings for legacy functions

// ============================================================================
// CLI COMMAND HANDLERS
// ============================================================================

void HandleExport(int argc, char** argv) {
    if (!ExportCurrentCellBMP(argv[2]))
        fprintf(stderr, "[export] Failed to export to: %s\n", argv[2]);
}

void HandleImport(int argc, char** argv) {
    if (!ImportBMPToCurrentCell(argv[2], true)) {
        fprintf(stderr, "[import] Failed to import BMP: %s\n", argv[2]);
        return;
    }

    if (argc >= 5)
        cliScale(atoi(argv[3]), atoi(argv[4]));

    if (argc >= 7)
        cliSetHeader(atoi(argv[5]), atoi(argv[6]));

    DoFileSave(hWnd);
}

void HandleScale(int argc, char** argv) {
    cliScale(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

void HandleHeader(int argc, char** argv) {
    cliSetHeader(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

void HandleAddCells(int argc, char** argv) {
    DoAddCells(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    DoFileSave(hWnd);
}

void HandleAddLoops(int argc, char** argv) {
    DoAddLoops(atoi(argv[2]), atoi(argv[3]));
    DoFileSave(hWnd);
}

// ============================================================================
// CLI COMMAND TABLE
// ============================================================================

CliCommand cliCommands[] = {
    { "export",   3, HandleExport,   "Export file to output path" },
    { "import",   3, HandleImport,   "Import BMP with optional scale/header" },
    { "scale",    4, HandleScale,    "Scale then save" },
    { "header",   4, HandleHeader,   "Set header then save" },
    { "addCells", 5, HandleAddCells, "Add animation cells" },
    { "addLoops", 4, HandleAddLoops, "Add animation loops" },
    { NULL, 0, NULL, NULL }
};

// ============================================================================
// CORE CONFIGURATION FUNCTIONS
// ============================================================================

void ParseAppPath(void)
{
    GetModuleFileName(NULL, gAppPath, MAX_PATH);
    char* lastBackslash = strrchr(gAppPath, '\\');
    if (lastBackslash)
        *lastBackslash = '\0';
}

void LoadConfig()
{
    // get ini settings
    sprintf(gConfigIni, "%s\\config.ini", gAppPath);

    gAppResX = GetPrivateProfileInt("main", "resX", gAppResX, gConfigIni);
    gAppResY = GetPrivateProfileInt("main", "resY", gAppResY, gConfigIni);
    zScale = GetPrivateProfileInt("main", "zScale", zScale, gConfigIni);
    gPosCells = GetPrivateProfileInt("main", "posCells", gPosCells, gConfigIni);
    gBaseMagnify = GetPrivateProfileInt("main", "magScale", gBaseMagnify, gConfigIni);
    gCliEnabled = GetPrivateProfileInt("main", "cliStartup", gCliEnabled, gConfigIni);

    // Load reference image settings
    LoadReferenceImageSettings();

    // Set the base magnification factor
    MagnifyFactor = gBaseMagnify;
}

void SaveConfig()
{
    char buffer[32];

    // Save main settings
    sprintf(buffer, "%d", gAppResX);
    WritePrivateProfileString("main", "resX", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gAppResY);
    WritePrivateProfileString("main", "resY", buffer, gConfigIni);
    
    sprintf(buffer, "%d", zScale);
    WritePrivateProfileString("main", "zScale", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gPosCells);
    WritePrivateProfileString("main", "posCells", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gBaseMagnify);
    WritePrivateProfileString("main", "magScale", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gCliEnabled);
    WritePrivateProfileString("main", "cliStartup", buffer, gConfigIni);

    // Save reference image settings
    SaveReferenceImageSettings();
}

void InitializeConfiguration()
{
    ParseAppPath();
    LoadConfig();
}

// ============================================================================
// COMMAND LINE PROCESSING
// ============================================================================

bool HandleCliCommands(char* cmdLine)
{
    const int MAX_ARGS = 16;
    char* argv[MAX_ARGS] = {0};
    int argc = 0;

    char* token = strtok(cmdLine, " ");
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc < 1) return false;

    // Parse startup file
    char startupfile[_MAX_PATH] = {0};
    if (argv[0][0] == '"' && argv[0][strlen(argv[0]) - 1] == '"') {
        strncpy(startupfile, argv[0] + 1, strlen(argv[0]) - 2);
        startupfile[strlen(argv[0]) - 2] = '\0';
    } else {
        strncpy(startupfile, argv[0], sizeof(startupfile) - 1);
    }

    if (argc == 1) {
        DoFileOpen(hWnd, startupfile, startupfile + strlen(startupfile) - 3);
        fprintf(stderr, "[CLI] No command given. Opened file only.\n");
        return true;
    }

    DoFileOpen(hWnd, startupfile, startupfile + strlen(startupfile) - 3);

    const char* command = argv[1];
    for (int i = 0; cliCommands[i].name; ++i) {
        if (strcmp(cliCommands[i].name, command) == 0) {
            if (argc < cliCommands[i].minArgs) {
                fprintf(stderr, "[%s] Not enough args (have %d, need %d)\n", command, argc, cliCommands[i].minArgs);
                return true;
            }
            cliCommands[i].handler(argc, argv);
            return true;
        }
    }

    fprintf(stderr, "[CLI Error] Unknown command: %s\n", command);
    return true;
}

// ============================================================================
// REFERENCE IMAGE SETTINGS
// ============================================================================

void LoadReferenceImageSettings()
{
    gReferenceScaleX = GetPrivateProfileInt("reference", "referenceScaleX", (int)gReferenceScaleX, gConfigIni);
    gReferenceScaleY = GetPrivateProfileInt("reference", "referenceScaleY", (int)gReferenceScaleY, gConfigIni);
    GetPrivateProfileString("reference", "referenceBM", gReferenceBM, gReferenceBM, _MAX_PATH, gConfigIni);
    gReferenceXHot = GetPrivateProfileInt("reference", "referenceXHot", gReferenceXHot, gConfigIni);
    gReferenceYHot = GetPrivateProfileInt("reference", "referenceYHot", gReferenceYHot, gConfigIni);
    gReferenceLinkPoint = GetPrivateProfileInt("reference", "referenceLinkPoint", gReferenceLinkPoint, gConfigIni);
    gReferenceLinkPointX = GetPrivateProfileInt("reference", "referenceLinkPointX", gReferenceLinkPointX, gConfigIni);
    gReferenceLinkPointY = GetPrivateProfileInt("reference", "referenceLinkPointY", gReferenceLinkPointY, gConfigIni);
    gReferencePriority = GetPrivateProfileInt("reference", "referencePriority", gReferencePriority, gConfigIni);
    gReferenceTransparentIndex = GetPrivateProfileInt("reference", "referenceTransparentIndex", gReferenceTransparentIndex, gConfigIni);
}

void SaveReferenceImageSettings()
{
    char buffer[32];

    sprintf(buffer, "%d", (int)gReferenceScaleX);
    WritePrivateProfileString("reference", "referenceScaleX", buffer, gConfigIni);
    
    sprintf(buffer, "%d", (int)gReferenceScaleY);
    WritePrivateProfileString("reference", "referenceScaleY", buffer, gConfigIni);
    
    WritePrivateProfileString("reference", "referenceBM", gReferenceBM, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceXHot);
    WritePrivateProfileString("reference", "referenceXHot", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceYHot);
    WritePrivateProfileString("reference", "referenceYHot", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceLinkPoint);
    WritePrivateProfileString("reference", "referenceLinkPoint", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceLinkPointX);
    WritePrivateProfileString("reference", "referenceLinkPointX", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceLinkPointY);
    WritePrivateProfileString("reference", "referenceLinkPointY", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferencePriority);
    WritePrivateProfileString("reference", "referencePriority", buffer, gConfigIni);
    
    sprintf(buffer, "%d", gReferenceTransparentIndex);
    WritePrivateProfileString("reference", "referenceTransparentIndex", buffer, gConfigIni);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void SetDefaultConfiguration()
{
    gAppResX = 700;
    gAppResY = 500;
    zScale = 100;
    gPosCells = 0;
    gCliMode = 0;
    gBaseMagnify = 100;
    gCliEnabled = 0;

    // Reference image defaults
    gReferenceScaleX = 100;
    gReferenceScaleY = 100;
    strcpy(gReferenceBM, "reference.bmp");
    gReferenceXHot = 0;
    gReferenceYHot = 0;
    gReferenceLinkPoint = 0;
    gReferenceLinkPointX = 0;
    gReferenceLinkPointY = 0;
    gReferencePriority = 0;
    gReferenceTransparentIndex = 255;
}

bool ValidateConfiguration()
{
    // Validate window dimensions
    if (gAppResX < 400 || gAppResX > 3840) {
        gAppResX = 700;
        return false;
    }
    
    if (gAppResY < 300 || gAppResY > 2160) {
        gAppResY = 500;
        return false;
    }

    // Validate zoom scale
    if (zScale < 25 || zScale > 1600) {
        zScale = 100;
        return false;
    }

    // Validate magnification
    if (gBaseMagnify < 25 || gBaseMagnify > 1600) {
        gBaseMagnify = 100;
        return false;
    }

    // Validate reference image settings
    if (gReferenceScaleX < 10 || gReferenceScaleX > 1000) {
        gReferenceScaleX = 100;
        return false;
    }

    if (gReferenceScaleY < 10 || gReferenceScaleY > 1000) {
        gReferenceScaleY = 100;
        return false;
    }

    if (gReferenceTransparentIndex < 0 || gReferenceTransparentIndex > 255) {
        gReferenceTransparentIndex = 255;
        return false;
    }

    return true;
}

#pragma warning(pop)  // Restore warning level