// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//


#ifndef STDAFX_H
#define STDAFX_H


//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here
#include <stdio.h>
#include <windowsx.h>
#include <wingdi.h>
#include <Commdlg.h>

static HWND hWnd;

//	data types for external data (files or OS data structures)
//	each contain the number of bits indicated
typedef short			Int16;
typedef unsigned short	UInt16;
typedef int				Int32;
typedef unsigned int	UInt32;
typedef unsigned char	uchar;

#include <atlimage.h> // CImage
#include <gdiplus.h> // GDI+
#pragma comment(lib, "gdiplus.lib")

#endif
