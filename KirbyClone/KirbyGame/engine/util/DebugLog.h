// engine/util/DebugLog.h
#pragma once

// Simple debug log helper.
// On Win32, forwards to OutputDebugString; on other platforms you can swap implementation.

#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
inline void DBGLOG ( const wchar_t* msg ) {
    ::OutputDebugStringW ( msg );
    ::OutputDebugStringW ( L"\n" );
}
#else
inline void DBGLOG ( const wchar_t* msg ) {
    // TODO: implement for other platforms if needed
    ( void ) msg;
}
#endif
