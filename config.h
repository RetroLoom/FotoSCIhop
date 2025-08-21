/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Configuration and settings management header
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================
#define MAX_ARG 512

// ============================================================================
// COMMAND LINE AND CONFIGURATION VARIABLES
// ============================================================================

// Command line arguments
extern char *argv[MAX_ARG];
extern char propstr[10240];

// Application paths
extern char gAppPath[MAX_PATH];
extern char gConfigIni[_MAX_PATH];

// Application settings
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
// CONFIGURATION FUNCTIONS
// ============================================================================

// Configuration file management
void LoadConfig();
void SaveConfig();

// Application initialization
void ParseAppPath();
void InitializeConfiguration();

// Command line processing
bool HandleCliCommands(char* cmdLine);

// ============================================================================
// CLI COMMAND STRUCTURES AND TYPES
// ============================================================================

// Command handler function pointer type
typedef void (*CliHandler)(int argc, char** argv);

// CLI command structure
typedef struct {
    const char* name;
    int minArgs;
    CliHandler handler;
    const char* description;
} CliCommand;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration helpers
void SetDefaultConfiguration();
bool ValidateConfiguration();

// Reference image helpers
void LoadReferenceImageSettings();
void SaveReferenceImageSettings();

#endif // CONFIG_H