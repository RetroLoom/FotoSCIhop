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
#include "config.h"

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_PATH+20];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
char szFileName[MAX_PATH] = "";
char szNextFileName[MAX_PATH] = "";

// Dialog activity flag
bool g_dialogActive = false;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

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
int picX = 0;
int picY = 30;
int tableX = 0;

// Zoom step levels (percentages)
static const int kZoomLevels[] = { 25, 50, 75, 100, 150, 200, 300, 400 };
static const int kZoomLevelCount = sizeof(kZoomLevels) / sizeof(kZoomLevels[0]);

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
                
                // Ensure image data is loaded
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

            // Simple invalidation for cell switching (don't clear cache)
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
        // Ensure image data is loaded
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

        // Simple invalidation for cell switching (don't clear cache)
        InvalidateRgn(hWnd, NULL, true);
    }
}

#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecation warnings for legacy functions

typedef BOOL (WINAPI*Func)(HWND, const char*, unsigned char, const char*, char*);
Func ExtractFromVolume;

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
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        
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
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        
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
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        
        if (GetOpenFileName(&ofn)) {
            g_realmpalExtraFile = fileName;
        }
    }
    
    if (g_requestPalMgrInputDialog) {
        g_requestPalMgrInputDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Palette files (*.bmp;*.png;*.pcx)\0*.bmp;*.png;*.pcx\0All files (*.*)\0*.*\0\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        ofn.lpstrTitle = "Import Palette From File";
        
        if (GetOpenFileName(&ofn)) {
            g_palMgrInputFile = fileName;
        }
    }
    
    if (g_requestPalMgrOutputDialog) {
        g_requestPalMgrOutputDialog = false;
        
        OPENFILENAME ofn;
        static char fileName[MAX_PATH] = "palette.bmp";
        
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "BMP Palette (*.bmp)\0*.bmp\0PCX Palette (*.pcx)\0*.pcx\0\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrTitle = "Export Palette To File";
        
        if (GetSaveFileName(&ofn)) {
            g_palMgrOutputFile = fileName;
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
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_IMMAGINA);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = (LPCTSTR)IDC_IMMAGINA;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon((HINSTANCE)wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

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
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                       x, y, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

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

    // Load theme setting
    int savedTheme = GetPrivateProfileInt("main", "theme", 0, gConfigIni);
    if (savedTheme >= 0 && savedTheme < 5) { // Validate theme index
        FotoSCIhopStyles::SetTheme((FotoSCIhopStyles::ThemeMode)savedTheme);
    }
    FotoSCIhopStyles::RefreshTheme();

    // Set up dialog callbacks with appropriate input policies
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PROPERTIES, "Properties", &RenderPropertiesDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_ABOUT, "About FotoSCIhop", &RenderAboutDialog);
    ImGuiDialogs::RegisterDialogWithInput(ImGuiDialogs::DIALOG_CLUT_GENERATOR, "CLUT Generator", &RenderClutGeneratorDialog, true); // Allow main window input for magic wand
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_REALMPAL, "Realmpal Converter", &RenderRealmpalDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PREFERENCES, "Preferences", &RenderPreferencesDialog);
    ImGuiDialogs::RegisterDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER, "Palette Manager", &RenderPaletteManagerDialog);

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
        globalPicture = NULL;
    }

    if (globalView) {
        delete globalView;
        globalView = NULL;
    }

    if (hfDefault) {
        DeleteObject(hfDefault);
        hfDefault = NULL;
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

// ============================================================================
// ZOOM AND SCROLL HELPERS
// ============================================================================

// Returns the total zoomed content size for the current cell/view
static SIZE GetZoomedContentSize() {
    SIZE sz = {0, 0};
    int scale = (zScale > 0) ? zScale : 100;
    if (curCell && *curCell && (*curCell)->bmInfo) {
        sz.cx = ((*curCell)->bmInfo->bmiHeader.biWidth  * scale) / 100;
        sz.cy = (-(*curCell)->bmInfo->bmiHeader.biHeight * scale) / 100;
    }
    return sz;
}

void UpdateScrollBars(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    int clientW = client.right  - UI_LEFT_MARGIN - tableX;
    int clientH = client.bottom - UI_TOP_MARGIN  - UI_INFO_HEIGHT;

    SIZE content = GetZoomedContentSize();

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;

    // Horizontal
    si.nMin  = 0;
    si.nMax  = (content.cx > clientW) ? content.cx : 0;
    si.nPage = (clientW > 0) ? clientW : 1;
    si.nPos  = -picX; // picX is negative offset
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

    // Vertical (picY starts at 30, so offset from that base)
    si.nMin  = 0;
    si.nMax  = (content.cy > clientH) ? content.cy : 0;
    si.nPage = (clientH > 0) ? clientH : 1;
    si.nPos  = -(picY - 30);
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

static void ClampScroll(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    int clientW = client.right  - UI_LEFT_MARGIN - tableX;
    int clientH = client.bottom - UI_TOP_MARGIN  - UI_INFO_HEIGHT;
    SIZE content = GetZoomedContentSize();

    int maxScrollX = (content.cx > clientW) ? (content.cx - clientW) : 0;
    int maxScrollY = (content.cy > clientH) ? (content.cy - clientH) : 0;

    if (-picX < 0)          picX = 0;
    if (-picX > maxScrollX) picX = -maxScrollX;
    if (-(picY - 30) < 0)          picY = 30;
    if (-(picY - 30) > maxScrollY) picY = 30 - maxScrollY;
}

static void ZoomStep(HWND hwnd, int delta) {
    // Find current level index
    int idx = kZoomLevelCount - 1;
    for (int i = 0; i < kZoomLevelCount; i++) {
        if (kZoomLevels[i] >= zScale) { idx = i; break; }
    }
    idx += delta;
    if (idx < 0) idx = 0;
    if (idx >= kZoomLevelCount) idx = kZoomLevelCount - 1;
    zScale = kZoomLevels[idx];
    ClampScroll(hwnd);
    UpdateScrollBars(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;

    // Menu checks
    HMENU menu = GetMenu(hWnd);
    if (menu)
        EnableMenuItem(menu, ID_SALVA, (datasaved == false) ? MF_ENABLED : MF_GRAYED);

    switch (message) 
    {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
            if (ImGuiDialogs::IsAnyDialogOpen() && !ImGuiDialogs::CurrentDialogAllowsMainWindowInput()) {
                // Dialog is open and doesn't allow main window input - redirect focus
                ImGuiDialogs::RestoreDialogFocus();
                return 0;
            }
        }
        break;

    case WM_SETFOCUS:
        if (ImGuiDialogs::IsAnyDialogOpen() && !ImGuiDialogs::CurrentDialogAllowsMainWindowInput()) {
            // Dialog is open and doesn't allow main window input - redirect focus
            ImGuiDialogs::RestoreDialogFocus();
            return 0;
        }
        break;

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
            InvalidateRect(hWnd, NULL, FALSE);
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
        
        case IDM_PALETTE_MANAGER:
            if ((globalView && globalView->palSCI) || (globalPicture && globalPicture->palSCI))
            {
                ImGuiDialogs::ShowDialog(ImGuiDialogs::DIALOG_PALETTE_MANAGER);
            }
            else
            {
                MessageBox(hWnd, "Please load a .v56 or .p56 file before using the Palette Manager.",
                        "No File Loaded", MB_OK | MB_ICONINFORMATION);
            }
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
                
                InvalidateRect(hWnd, NULL, FALSE);
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
                
                InvalidateRect(hWnd, NULL, FALSE);
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

        case ID_ZOOM_IN:
            ZoomStep(hWnd, +1);
            break;

        case ID_ZOOM_OUT:
            ZoomStep(hWnd, -1);
            break;

        case ID_ZOOM_RESET:
            zScale = 100;
            picX = (globalView) ? 220 : 0;
            picY = 30;
            UpdateScrollBars(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
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

        // Create double buffer
        HDC hdcBuffer = CreateCompatibleDC(hdcScreen);
        HBITMAP hbmBuffer = CreateCompatibleBitmap(hdcScreen, clientRect.right, clientRect.bottom);
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
        // (picX is only set on file load, not every paint - scroll offset is preserved)
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
        BitBlt(hdcScreen, 0, 0, clientRect.right, clientRect.bottom, hdcBuffer, 0, 0, SRCCOPY);

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
        ClampScroll(hWnd);
        UpdateScrollBars(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl+wheel = zoom
            ZoomStep(hWnd, (delta > 0) ? +1 : -1);
        } else {
            // Plain wheel = vertical scroll
            int step = 40;
            picY += (delta > 0) ? step : -step;
            ClampScroll(hWnd);
            UpdateScrollBars(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_HSCROLL:
    {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hWnd, SB_HORZ, &si);
        int pos = si.nPos;
        switch (LOWORD(wParam)) {
            case SB_LINELEFT:   pos -= 20; break;
            case SB_LINERIGHT:  pos += 20; break;
            case SB_PAGELEFT:   pos -= si.nPage; break;
            case SB_PAGERIGHT:  pos += si.nPage; break;
            case SB_THUMBTRACK: pos = HIWORD(wParam); break;
        }
        picX = -pos;
        ClampScroll(hWnd);
        UpdateScrollBars(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_VSCROLL:
    {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        int pos = si.nPos;
        switch (LOWORD(wParam)) {
            case SB_LINEUP:     pos -= 20; break;
            case SB_LINEDOWN:   pos += 20; break;
            case SB_PAGEUP:     pos -= si.nPage; break;
            case SB_PAGEDOWN:   pos += si.nPage; break;
            case SB_THUMBTRACK: pos = HIWORD(wParam); break;
        }
        picY = 30 - pos;
        ClampScroll(hWnd);
        UpdateScrollBars(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

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
            } else {
                // Click was outside image area
                SetWindowText(hWnd, "FotoSCIhop - Magic Wand: Click inside the image area");
                SetTimer(hWnd, 2, 2000, NULL);
            }
            return 0; // Consume the message
        }
        break;
    }

    case WM_TIMER:
        if (wParam == 1) { // ImGui timer
            if (g_pendingThemeChange) {
                FotoSCIhopStyles::SetTheme(g_pendingTheme);
                FotoSCIhopStyles::RefreshTheme();
                g_pendingThemeChange = false;
                
                InvalidateRect(hWnd, NULL, TRUE);
            }
            
            HandleRealmpalFileDialogs();
            
            if (ImGuiDialogs::IsAnyDialogOpen()) {
                ImGuiDialogs::Render();
                
                // Only do focus restoration for dialogs that block main window input
                if (!ImGuiDialogs::CurrentDialogAllowsMainWindowInput()) {
                    static DWORD lastFocusCheck = 0;
                    DWORD currentTime = GetTickCount();
                    
                    if (currentTime - lastFocusCheck > 1000) { // Check every second
                        HWND dialogWindow = ImGuiDialogs::GetDialogWindow();
                        if (dialogWindow && GetForegroundWindow() == hWnd) {
                            SetForegroundWindow(dialogWindow);
                        }
                        lastFocusCheck = currentTime;
                    }
                }
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
        break;

    case WM_SETCURSOR:
    {
        // Only change cursor when magic wand is enabled and mouse is over client area
        if (g_clutGenerator && g_clutGenerator->IsMagicWandEnabled()) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            
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

#pragma warning(pop)