#pragma once
#include <string>
#include <string_view>
#include <Windows.h>

// UTF-8 std::string -> UTF-16 std::wstring
inline std::wstring ToWide ( std::string_view s ) {
    if ( s.empty ( ) ) return {};
    const int inLen = static_cast< int >( s.size ( ) );
    const int outLen = MultiByteToWideChar ( CP_UTF8 , 0 , s.data ( ) , inLen , nullptr , 0 );
    std::wstring out ( outLen , L'\0' );
    MultiByteToWideChar ( CP_UTF8 , 0 , s.data ( ) , inLen , out.data ( ) , outLen );
    return out;
}

// UTF-16 std::wstring -> UTF-8 std::string (필요하면)
inline std::string ToUtf8 ( std::wstring_view ws ) {
    if ( ws.empty ( ) ) return {};
    const int inLen = static_cast< int >( ws.size ( ) );
    const int outLen = WideCharToMultiByte ( CP_UTF8 , 0 , ws.data ( ) , inLen , nullptr , 0 , nullptr , nullptr );
    std::string out ( outLen , '\0' );
    WideCharToMultiByte ( CP_UTF8 , 0 , ws.data ( ) , inLen , out.data ( ) , outLen , nullptr , nullptr );
    return out;
}
