/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  This file defines the entry point for the application, GUI events, etc.
 *
 *  FotoSCIhop is a tool to modify .P56 and .V56 image files from Sierra SCI games
 *
 *  This program is part of the TraduSCI package
 *
 */
 
#include "stdafx.h"
#include "FotoSCIhop.h"
#include "ClutGenerator.h"
#define MAX_LOADSTRING 100
#include "imgui_integration.h"
#include "imgui.h"
#include "fotoscihop_styles.h"
#include "librealmpal.h"
#include <set>
#include "display.h"
#include "fileio.h"

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_PATH+20];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
char szFileName[MAX_PATH] = "";
char szNextFileName[MAX_PATH] = "";

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// ============================================================================
// COMMAND LINE AND CONFIGURATION
// ============================================================================
char *argv[MAX_ARG];
char propstr[10240] = "";
char gAppPath[MAX_PATH];

// Configuration file and settings
char gConfigIni[_MAX_PATH];
int gAppResX = 700;
int gAppResY = 500;
int zScale = 100;
int gPosCells = 0;
int gCliMode = 0;
int gBaseMagnify = 100;
int gCliEnabled = 0;

// ============================================================================
// REFERENCE IMAGE SETTINGS
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
// GLOBAL APPLICATION STATE
// ============================================================================

// Main data objects
P56file32 *globalPicture = NULL;
V56file *globalView = NULL;
bool isPicture = true;

// Current selection state
Cell **curCell = 0;
Loop **curLoop = 0;
int curCellIndex = 0;
int curLoopIndex = 0;

// Application state flags
bool datasaved = true;
bool showpbars = false;

// ============================================================================
// MAGIC WAND AND CLUT GENERATOR STATE
// ============================================================================

// Global state for magic wand tool
bool g_magicWandEnabled = false;
std::set<int> g_usedColorIndices;

// ============================================================================
// DISPLAY AND UI STATE
// ============================================================================

// Display settings
int MagnifyFactor = gBaseMagnify;
int picX = 0;
int picY = 30;
int tableX = 0;

// UI elements and drawing
RECT rc;
HWND hWndTopBar;
HFONT hfDefault;

bool g_pendingThemeChange = false;
FotoSCIhopStyles::ThemeMode g_pendingTheme = FotoSCIhopStyles::ThemeMode::PHOTOSHOP_DARK;

void ShowLoopCell(unsigned char newloop, unsigned char newcell) {
    // Validate loop index first
    if (!globalView || newloop >= globalView->Head.view32.loopCount) {
        return; // Invalid loop index
    }
    
    // Validate that the loop exists
    if (!globalView->loops[newloop]) {
        return; // Loop is null
    }
    
    // Validate cell index for this specific loop
    if (newcell >= globalView->loops[newloop]->Head.numCels) {
        // If cell index is too high, use the last cell in this loop
        if (globalView->loops[newloop]->Head.numCels > 0) {
            newcell = globalView->loops[newloop]->Head.numCels - 1;
        } else {
            newcell = 0; // Loop has no cells, use 0
        }
    }
    
    curLoopIndex = newloop;
    
    curLoop = &globalView->loops[newloop];
    if (curLoop) {
        // Additional safety check before accessing cells
        if (newcell < globalView->loops[newloop]->Head.numCels && globalView->loops[newloop]->cells[newcell]) {
            curCell = &globalView->loops[newloop]->cells[newcell];
        } else {
            curCell = nullptr; // Set to null if cell doesn't exist
        }
        
        if (curCell || (*curLoop)->Head.flags) {
            if (curCell && !(*curLoop)->Head.flags) {
                curCellIndex = newcell;
                
                // CRITICAL: Ensure image data is loaded before updating scroll bars
                if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
                    (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
                }
            }

            HMENU menu = GetMenu(hWnd); 
        
            EnableMenuItem(menu, ID_IMPORTABMP, ((*curLoop)->Head.flags ? MF_GRAYED : MF_ENABLED));
            EnableMenuItem(menu, ID_ESPORTABMP, ((*curLoop)->Head.flags ? MF_GRAYED : MF_ENABLED));
            EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_ENABLED);
            EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_ENABLED);
            if (newloop == globalView->Head.view32.loopCount - 1)
                EnableMenuItem(menu, ID_CICLOSUCCESSIVO, MF_GRAYED);
        
            if (newloop == 0)
                EnableMenuItem(menu, ID_CICLOPRECEDENTE, MF_GRAYED);

            EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
            EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
            if ((newcell == globalView->loops[newloop]->Head.numCels - 1) || (globalView->loops[newloop]->Head.numCels == 0))
                EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
        
            if (newcell == 0)
                EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

            // CRITICAL: Update scroll bars after image data is ready
            UpdateScrollBars();

            InvalidateRgn(hWnd, NULL, true);
        }
    }
}

void ShowCell(unsigned char newcell) {
    // Validate that we have a picture loaded
    if (!globalPicture) {
        return;
    }
    
    // Validate cell index
    int totalCells = globalPicture->CellsCount();
    if (newcell >= totalCells) {
        // If cell index is too high, use the last cell
        if (totalCells > 0) {
            newcell = totalCells - 1;
        } else {
            newcell = 0; // No cells, use 0
        }
    }
    
    // Additional bounds check
    if (newcell < 0) {
        newcell = 0;
    }
    
    curCellIndex = newcell;
    
    // Validate that the cell exists before accessing it
    if (newcell < globalPicture->CellsCount() && globalPicture->cells[newcell]) {
        curCell = &globalPicture->cells[curCellIndex];
    } else {
        curCell = nullptr; // Set to null if cell doesn't exist
        return; // Exit early if cell is invalid
    }
    
    if (curCell) {
        // CRITICAL: Ensure image data is loaded before updating scroll bars
        if (!(*curCell)->bmInfo || !(*curCell)->bmImage) {
            (*curCell)->GetImage(&(*curCell)->bmInfo, &(*curCell)->bmImage);
        }
        
        HMENU menu = GetMenu(hWnd); 

        EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_ENABLED);
        EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_ENABLED);
        if (curCellIndex == globalPicture->CellsCount() - 1)
            EnableMenuItem(menu, ID_CELLASUCCESSIVA, MF_GRAYED);
        
        if (curCellIndex == 0)
            EnableMenuItem(menu, ID_CELLAPRECEDENTE, MF_GRAYED);

        // CRITICAL: Update scroll bars after image data is ready
        UpdateScrollBars();
        
        InvalidateRgn(hWnd, NULL, true);
    }
}

void LoadConfig ()
{
	// get ini settings
	sprintf(gConfigIni, "%s\\config.ini", gAppPath);

	gAppResX = GetPrivateProfileInt("main", "resX", gAppResX, gConfigIni);
	gAppResY = GetPrivateProfileInt("main", "resY", gAppResY, gConfigIni);
	zScale = GetPrivateProfileInt("main", "zScale", zScale, gConfigIni);
	gPosCells = GetPrivateProfileInt("main", "posCells", gPosCells, gConfigIni);
	gBaseMagnify = GetPrivateProfileInt("main", "magScale", gBaseMagnify, gConfigIni);
	gCliEnabled = GetPrivateProfileInt("main", "cliStartup", gCliEnabled, gConfigIni);

	// image references
	gReferenceScaleX = GetPrivateProfileInt("reference", "referenceScaleX", gReferenceScaleX, gConfigIni);
	gReferenceScaleY = GetPrivateProfileInt("reference", "referenceScaleY", gReferenceScaleY, gConfigIni);
	GetPrivateProfileString("reference", "referenceBM", gReferenceBM, gReferenceBM, _MAX_PATH, gConfigIni);
	gReferenceXHot = GetPrivateProfileInt("reference", "referenceXHot", gReferenceXHot, gConfigIni);
	gReferenceYHot = GetPrivateProfileInt("reference", "referenceYHot", gReferenceYHot, gConfigIni);
	gReferenceLinkPoint = GetPrivateProfileInt("reference", "referenceLinkPoint", gReferenceLinkPoint, gConfigIni);
	gReferenceLinkPointX = GetPrivateProfileInt("reference", "referenceLinkPointX", gReferenceLinkPointX, gConfigIni);
	gReferenceLinkPointY = GetPrivateProfileInt("reference", "referenceLinkPointY", gReferenceLinkPointY, gConfigIni);
	gReferencePriority = GetPrivateProfileInt("reference", "referencePriority", gReferencePriority, gConfigIni);

	MagnifyFactor = gBaseMagnify;
}

#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecation warnings for legacy functions

typedef BOOL (WINAPI*Func)(HWND, const char*, unsigned char, const char*, char*);
Func ExtractFromVolume;

void ParseAppPath(void)
{
    GetModuleFileName(NULL, gAppPath, MAX_PATH);
    char* lastBackslash = strrchr(gAppPath, '\\');
    if (lastBackslash)
        *lastBackslash = '\0';
}

typedef void (*CliHandler)(int argc, char** argv);

typedef struct {
    const char* name;
    int minArgs;
    CliHandler handler;
    const char* description;
} CliCommand;

// === Command Handlers ===

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

// === Command Table ===

CliCommand cliCommands[] = {
    { "export",   3, HandleExport,   "Export file to output path" },
    { "import",   3, HandleImport,   "Import BMP with optional scale/header" },
    { "scale",    4, HandleScale,    "Scale then save" },
    { "header",   4, HandleHeader,   "Set header then save" },
    { "addCells", 5, HandleAddCells, "Add animation cells" },
    { "addLoops", 4, HandleAddLoops, "Add animation loops" },
    { NULL, 0, NULL, NULL }
};

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

void HandleRealmpalFileDialogs() {
    if (g_requestInputDialog) {
        g_requestInputDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Image files (*.png, *.bmp *.jpg)\0*.png;*.bmp;*.jpg\0All files (*.*)\0*.*\0\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalInputFile = fileName;
        }
    }
    
    if (g_requestPaletteDialog) {
        g_requestPaletteDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = INTERFACE_PALINFILTER;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalPaletteFile = fileName;
        }
    }
    
    if (g_requestExtraDialog) {
        g_requestExtraDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = INTERFACE_PALINFILTER;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalExtraFile = fileName;
        }
    }
}

#ifdef __DEVC
int STDCALL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else 
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    MSG msg;
    HACCEL hAccelTable;

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_IMMAGINA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    ParseAppPath();
    LoadConfig();

    if (gCliEnabled && lpCmdLine[0] != '\0')
    {
        if (HandleCliCommands(lpCmdLine))
        {
            return 0;
        }
    }

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_IMMAGINA);

#if defined _M_IX86
    HINSTANCE DLL = LoadLibrary("SCIdump.dll");
    if (!DLL) {
        MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
    } else {
        ExtractFromVolume = (Func)GetProcAddress(DLL, "?ExtractFromVolumeSkel@@YAHPAUHWND__@@PADE11@Z");
        if (!ExtractFromVolume) {
            FreeLibrary(DLL);
            DLL = NULL;
            MessageBox(NULL, ERR_CANTLOADDLL, ERR_TITLE, MB_OK | MB_ICONERROR);
        }
    }
#endif

    if (lpCmdLine[0] != '\0') {
        char startupfile[_MAX_PATH] = {0};

        if (lpCmdLine[0] == '"') {
            size_t len = strlen(lpCmdLine);
            if (len > 2) {
                strncpy(startupfile, lpCmdLine + 1, len - 2);
                startupfile[len - 2] = '\0';
            }
        } else {
            strncpy(startupfile, lpCmdLine, sizeof(startupfile) - 1);
        }

        size_t len = strlen(startupfile);
        if (len >= 3) {
            DoFileOpen(hWnd, startupfile, startupfile + (len - 3));
        }
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

#if defined _M_IX86
    if (DLL) {
        FreeLibrary(DLL);
    }
#endif

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // Removed CS_OWNDC if present
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_IMMAGINA);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL; // IMPORTANT: Set to NULL to prevent auto-erase
    wcex.lpszMenuName   = (LPCTSTR)IDC_IMMAGINA;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    // Get the width and height of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Get the width and height of the window
    int windowWidth = gAppResX;
    int windowHeight = gAppResY;

    // Calculate the x and y coordinates to center the window on the screen
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // Create the window
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                       x, y, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

    // Enable scroll bars
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style |= WS_HSCROLL | WS_VSCROLL;
    SetWindowLong(hWnd, GWL_STYLE, style);

    // Initialize scroll system
    UpdateScrollBars();

    // If the window couldn't be created, return FALSE
    if (!hWnd)
    {
        return FALSE;
    }

    // Initialize ImGui AFTER window creation
    if (!ImGuiDialogs::Initialize(hWnd))
    {
        MessageBox(hWnd, "Failed to initialize ImGui", "Error", MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    FotoSCIhopStyles::Initialize();

    // Load theme setting (add this at the end)
    int savedTheme = GetPrivateProfileInt("main", "theme", 0, gConfigIni);
    if (savedTheme >= 0 && savedTheme < 5) { // Validate theme index
        FotoSCIhopStyles::SetTheme((FotoSCIhopStyles::ThemeMode)savedTheme);
    }
    FotoSCIhopStyles::RefreshTheme();

    // Set up dialog callbacks
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PROPERTIES, "Properties", &RenderPropertiesDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_ABOUT, "About FotoSCIhop", &RenderAboutDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR, "CLUT Generator", &RenderClutGeneratorDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_REALMPAL, "Realmpal Converter", &RenderRealmpalDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PREFERENCES, "Preferences", &RenderPreferencesDialog);

    SetTimer(hWnd, 1, 16, NULL);

    // Show the window
    ShowWindow(hWnd, nCmdShow);

    // Create a font to use for the window
    hfDefault = CreateFont(16, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE, ANSI_CHARSET, 
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                          VARIABLE_PITCH | FF_SWISS, "Arial");

    // Update the window
    UpdateWindow(hWnd);

    return TRUE;
}

void exit_proc(HWND hwnd)
{
    if (globalPicture) {
        delete globalPicture;
        globalPicture = NULL;  // Prevent double deletion
    }

    if (globalView) {
        delete globalView;
        globalView = NULL;  // Prevent double deletion
    }

    if (hfDefault) {
        DeleteObject(hfDefault);
        hfDefault = NULL;  // Prevent double deletion
    }

    FotoSCIhopStyles::Shutdown();
    ImGuiDialogs::Shutdown();

    // Clean up CLUT generator
    if (g_clutGenerator) {
        g_clutGenerator->Shutdown();
        delete g_clutGenerator;
        g_clutGenerator = nullptr;
    }
    
    DestroyWindow(hwnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;

    // Menu checks
    HMENU menu = GetMenu(hWnd);
    if (menu)  // Safety check
        EnableMenuItem(menu, ID_SALVA, (datasaved == false) ? MF_ENABLED : MF_GRAYED);

    switch (message) 
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_ABOUT);
            break;
            
        case IDM_MANUAL:
            {
                char szAppPath[MAX_PATH];
                GetModuleFileName(NULL, szAppPath, MAX_PATH);
                char* lastBackslash = strrchr(szAppPath, '\\');
                if (lastBackslash) *lastBackslash = '\0';
                ShellExecute(hWnd, "open", MANUAL_PATH, NULL, szAppPath, SW_SHOW);
                break; 
            }
            
        case ID_CARICA:
            if (DoSaveChangesDialog(hWnd) != IDCANCEL) 
                DoFileOpen(hWnd, NULL, NULL);
            break;
            
        case ID_CARICAV56VOL:
            {
                char exFile[MAX_PATH]="";
                if (ExtractFromVolume && ExtractFromVolume(hWnd, NULL, 0x80, "v56", exFile))
                    DoFileOpen(hWnd, exFile, "v56");         
                break;
            }
            
        case ID_CARICAP56VOL:
            {
                char exFile[MAX_PATH]="";
                if (ExtractFromVolume && ExtractFromVolume(hWnd, NULL, 0x81, "p56", exFile))
                    DoFileOpen(hWnd, exFile, "p56");         
                break;
            }
            
        case ID_FILE_NEXTFILE:
            DoNextFile(hWnd);
            RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
            Sleep(200);
            break;
            
        case ID_SALVA:
            DoFileSave(hWnd);
            break;
            
        case ID_SALVACOME:
            DoFileSaveAs(hWnd);
            break;
            
        case ID_IMPORTABMP:
            ImportBitmapUnified(hWnd, NULL, TRUE);
            break;
            
        case ID_ESPORTABMP:
            ExportBitmapUnified(hWnd, NULL);
            break;

        case IDM_CLUTGEN:
            if ((globalView && globalView->palSCI) || (globalPicture && globalPicture->palSCI))
            {
                ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_CLUT_GENERATOR);
            }
            else
            {
                MessageBox(hWnd, "Please load a .v56 or .p56 file before using the CLUT Generator.",
                           "No File Loaded", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case IDM_REALMPAL_IMPORT:
            if ((globalView && globalView->palSCI) || (globalPicture && globalPicture->palSCI))
            {
                ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_REALMPAL);
            }
            else
            {
                MessageBox(hWnd, "Please load a .v56 or .p56 file before using the PNG import.",
                           "No File Loaded", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case IDM_PROPERTIES:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_PROPERTIES);
            break;

        case IDM_PREFERENCES:
            ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_PREFERENCES);
            break;
                
        case ID_PALETTE:
            {
                HMENU menu = GetMenu(hWnd);
                switch (CheckMenuItem(menu, ID_PALETTE, MF_BYCOMMAND))
                {
                case MF_CHECKED:
                    CheckMenuItem(menu, ID_PALETTE, MF_UNCHECKED);
                    tableX = 0;
                    break;

                case MF_UNCHECKED:
                    CheckMenuItem(menu, ID_PALETTE, MF_CHECKED);
                    tableX = 190;
                    break;
                }
                
                InvalidateRgn(hWnd, NULL, true);
                break;
            }
            
        case ID_COLORI_IMPORTACOLORI:
            ImportPaletteUnified(hWnd, NULL);
            break;
            
        case ID_COLORI_ESPORTACOLORI:
            ExportPaletteUnified(hWnd, NULL);
            break;

        case ID_PRIORITYBARS:
            {
                HMENU menu = GetMenu(hWnd);
                switch (CheckMenuItem(menu, ID_PRIORITYBARS, MF_BYCOMMAND))
                {
                case MF_CHECKED:
                    CheckMenuItem(menu, ID_PRIORITYBARS, MF_UNCHECKED);
                    showpbars = false;
                    break;

                case MF_UNCHECKED:
                    CheckMenuItem(menu, ID_PRIORITYBARS, MF_CHECKED);
                    showpbars = true;
                    break;
                }
                
                InvalidateRgn(hWnd, NULL, true);
                break;
            }
            
        case ID_CICLOPRECEDENTE:
            if (!isPicture)
                ShowLoopCell(curLoopIndex-1, 0);
            break;
            
        case ID_CICLOSUCCESSIVO:
            if (!isPicture)
                ShowLoopCell(curLoopIndex+1, 0);
            break;

        case ID_CELLAPRECEDENTE:
            if (isPicture)
                ShowCell(curCellIndex-1);
            else
            {
                Loop *tloop = globalView->loops[curLoopIndex];
                if (tloop)
                {
                    ShowLoopCell(curLoopIndex, curCellIndex-1);
                }
            }
            break;
            
        case ID_CELLASUCCESSIVA:
            if (isPicture)
                ShowCell(curCellIndex+1);
            else
            {
                Loop *tloop = globalView->loops[curLoopIndex];
                if (tloop)
                {
                    ShowLoopCell(curLoopIndex, curCellIndex+1);
                }
            }
            break;
            
        case IDM_EXIT:
            if (DoSaveChangesDialog(hWnd) != IDCANCEL)   
                exit_proc(hWnd);
            break;
            
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdcScreen = BeginPaint(hWnd, &ps);

        // Get client rect
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        g_clientWidth = clientRect.right;
        g_clientHeight = clientRect.bottom;

        // Create double buffer
        HDC hdcBuffer = CreateCompatibleDC(hdcScreen);
        HBITMAP hbmBuffer = CreateCompatibleBitmap(hdcScreen, g_clientWidth, g_clientHeight);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBuffer, hbmBuffer);

        // Set up theme font
        HFONT themeFont = FotoSCIhopStyles::CreateThemeFont(16);
        HFONT oldFont = (HFONT)SelectObject(hdcBuffer, themeFont);

        // Theme background
        const FotoSCIhopStyles::UnifiedColors &colors = FotoSCIhopStyles::GetCurrentColors();
        HBRUSH bgBrush = CreateSolidBrush(colors.background);
        FillRect(hdcBuffer, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        // Themed top bar
        RECT topBar = {0, 0, clientRect.right, 25};
        FotoSCIhopStyles::DrawRoundedRect(hdcBuffer, topBar, colors.surface, colors.border, 0);

        // Initialize layout variables properly based on file type
        if (globalView)
        {
            picX = 220; // Views need space for loop information
        }
        if (globalPicture)
        {
            picX = 0; // Pictures start at left edge
        }

        // Enhanced palette with unified styling
        if (tableX > 0)
        {
            DrawPaletteTable(hdcBuffer);
        }

        // Image display
        if (globalView && curLoop && (*curLoop) && !(*curLoop)->Head.flags)
        {
            if (gReferenceBM && !gReferencePriority)
                DisplayReferenceImage(hdcBuffer);

            DisplayCurrentViewWithFrame(hdcBuffer);

            if (gReferenceBM && gReferencePriority)
                DisplayReferenceImage(hdcBuffer);

            if ((*curCell) && (*curCell)->Head.view.linkTableCount >= 1)
                DisplayLinkPoints(hdcBuffer);
        }

        if (globalPicture)
        {
            DisplayCurrentPicWithFrame(hdcBuffer);

            if (showpbars)
                DisplayPriorityBars(hdcBuffer);
        }

        // Themed cell info display
        if (curCell && (*curCell))
        {
            DrawCellInfo(hdcBuffer);
        }

        // Copy buffer to screen in one operation
        BitBlt(hdcScreen, 0, 0, g_clientWidth, g_clientHeight, hdcBuffer, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcBuffer, oldFont);
        SelectObject(hdcBuffer, hbmOld);
        DeleteObject(hbmBuffer);
        DeleteDC(hdcBuffer);
        FotoSCIhopStyles::SafeDeleteFont(themeFont);

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1; // Non-zero means "we handled it"

    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            // Clear any cached coordinate calculations
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            g_clientWidth = clientRect.right;
            g_clientHeight = clientRect.bottom;

            // Update scroll system first
            UpdateScrollBars();

            // Force complete redraw after resize to prevent artifacts
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    case WM_HSCROLL:
    {
        int scrollCode = LOWORD(wParam);
        int scrollPos = HIWORD(wParam);

        switch (scrollCode)
        {
        case SB_LINEUP:
            ScrollBy(-20, 0);
            break;
        case SB_LINEDOWN:
            ScrollBy(20, 0);
            break;
        case SB_PAGEUP:
            ScrollBy(-g_clientWidth / 4, 0);
            break;
        case SB_PAGEDOWN:
            ScrollBy(g_clientWidth / 4, 0);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollTo(scrollPos, g_scrollY);
            break;
        }
        break;
    }

    case WM_VSCROLL:
    {
        int scrollCode = LOWORD(wParam);
        int scrollPos = HIWORD(wParam);

        switch (scrollCode)
        {
        case SB_LINEUP:
            ScrollBy(0, -20);
            break;
        case SB_LINEDOWN:
            ScrollBy(0, 20);
            break;
        case SB_PAGEUP:
            ScrollBy(0, -g_clientHeight / 4);
            break;
        case SB_PAGEDOWN:
            ScrollBy(0, g_clientHeight / 4);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollTo(g_scrollX, scrollPos);
            break;
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        WORD keys = GET_KEYSTATE_WPARAM(wParam);

        if (keys & MK_CONTROL)
        {
            // Ctrl + wheel = zoom
            if (delta > 0)
            {
                ZoomIn();
            }
            else
            {
                ZoomOut();
            }
        }
        else
        {
            // Plain wheel = vertical scroll
            ScrollBy(0, -delta / 4);
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Handle panning
        if (g_isPanning)
        {
            UpdatePanning(x, y);
        }

        break;
    }

    case WM_LBUTTONUP:
        StopPanning();
        break;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Check zoom control click first
        if (HandleZoomControlClick(x, y))
        {
            break;
        }

        // Check if magic wand is enabled
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled())
        {
            int colorIndex;
            if (SampleColorAtScreenPosition(x, y, colorIndex))
            {
                g_clutGenerator->SetSelectedFromColor(colorIndex);
                char message[256];
                sprintf(message, "FotoSCIhop - Magic Wand: Selected color %d as FROM color", colorIndex);
                SetWindowText(hWnd, message);
                SetTimer(hWnd, 2, 3000, NULL);
            }
            break;
        }

        // Start panning with middle mouse button or space key + drag
        WORD keys = wParam;
        if (keys & MK_MBUTTON || GetKeyState(VK_SPACE) & 0x8000)
        {
            StartPanning(x, y);
        }

        break;
    }

    case WM_RBUTTONDOWN:
    {
        // Check if magic wand is enabled first
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            int colorIndex;
            int clientX = LOWORD(lParam);
            int clientY = HIWORD(lParam);
            
            if (SampleColorAtScreenPosition(clientX, clientY, colorIndex)) {
                g_clutGenerator->SetSelectedToColor(colorIndex);
                
                // Show feedback to user
                char message[256];
                sprintf(message, "FotoSCIhop - Magic Wand: Selected color %d as TO color", colorIndex);
                SetWindowText(hWnd, message);
                
                // Restore normal title after 3 seconds
                SetTimer(hWnd, 2, 3000, NULL);
                
                // Optional: Also show in console for debugging
                #ifdef _DEBUG
                char debugMsg[128];
                sprintf(debugMsg, "[DEBUG] Magic Wand TO: Color %d at (%d,%d)\n", colorIndex, clientX, clientY);
                OutputDebugStringA(debugMsg);
                #endif
            } else {
                // Click was outside image area
                SetWindowText(hWnd, "FotoSCIhop - Magic Wand: Click inside the image area");
                SetTimer(hWnd, 2, 2000, NULL);
            }
            return 0; // Consume the message
        }
        // If magic wand not enabled, let default processing handle it
        break;
    }

    case WM_USER + 1:
    {
        UpdateScrollBars();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_TIMER:
    if (wParam == 1) { // ImGui timer
        if (g_pendingThemeChange) {
            FotoSCIhopStyles::SetTheme(g_pendingTheme);
            FotoSCIhopStyles::RefreshTheme();
            g_pendingThemeChange = false;
            
            // Lightweight redraw - let Windows handle timing
            InvalidateRect(hWnd, NULL, TRUE);
            // Don't force immediate update - let it happen naturally
        }
        
        HandleRealmpalFileDialogs();
        
        if (ImGuiDialogs::IsAnyDialogOpen()) {
            ImGuiDialogs::Render();
        }
    }
    else if (wParam == 2) { // Title restore timer
        KillTimer(hWnd, 2);
        
        // Restore normal window title
        char wname[MAX_PATH + 15] = "FotoSCIhop";
        if (strlen(szFileName) > 0) {
            strcat(wname, " - ");
            
            // Extract just the filename from the full path
            char* filename = strrchr(szFileName, '\\');
            if (filename) {
                strcat(wname, filename + 1); // Skip the backslash
            } else {
                strcat(wname, szFileName);
            }
        }
        SetWindowText(hWnd, wname);
    }
    else if (wParam == 3) { // Scroll bar initialization timer
        KillTimer(hWnd, 3);
        EnsureScrollBarsAfterLoad();
    }
    break;

    case WM_SETCURSOR:
    {
        // Only change cursor when magic wand is enabled and mouse is over client area
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            
            // Check if cursor is over the image display area
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            
            if (pt.x >= 0 && pt.x < clientRect.right && pt.y >= 0 && pt.y < clientRect.bottom) {
                SetCursor(LoadCursor(NULL, IDC_CROSS));
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_CLOSE:
        if (DoSaveChangesDialog(hWnd) != IDCANCEL)   
            exit_proc(hWnd);
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma warning(pop)  // Restore warning level