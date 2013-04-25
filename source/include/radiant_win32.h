#ifndef _RADIANT_WIN32_H
#define _RADIANT_WIN32_H

#if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif

#if !defined(NOMINMAX)
#  define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>

#pragma warning(disable: 4355)   // warning C4355: 'this' : used in base member initializer list

// The Google Logging library wants to define ERROR to something else.
// Windows defines it in WinGDI.h as follows:
//
//     /* Region Flags */
//     #define ERROR               0
//     #define NULLREGION          1
//     #define SIMPLEREGION        2
//     #define COMPLEXREGION       3
//     #define RGN_ERROR ERROR
//
// Google wins!

#if defined(ERROR)
#  undef ERROR
#endif

#endif // _RADIANT_WIN32_H
