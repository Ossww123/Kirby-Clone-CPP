#pragma once
//
// Responsibility: Win32 COLORREF <-> RGBA8(0xAARRGGBB) helpers.
// Non-Goals: Color profiles, gamma/HDR, blending.
// Call-Context: Header-only; constexpr/inlined.
//

#include <cstdint>
#include <windows.h> // COLORREF (0x00BBGGRR)

namespace engine::win32 {

    // Build RGBA8
    [[nodiscard]] constexpr uint32_t RGBA8 ( uint8_t r , uint8_t g , uint8_t b , uint8_t a = 255 ) noexcept {
        return ( uint32_t ( a ) << 24 ) | ( uint32_t ( r ) << 16 ) | ( uint32_t ( g ) << 8 ) | uint32_t ( b );
    }

    // COLORREF(0x00BBGGRR) -> RGBA8(0xAARRGGBB)
    [[nodiscard]] constexpr uint32_t RGBA8_FromCOLORREF ( COLORREF c ) noexcept {
        const uint32_t v = static_cast< uint32_t >( c );
        const uint32_t r = v & 0xFFu;
        const uint32_t g = ( v >> 8 ) & 0xFFu;
        const uint32_t b = ( v >> 16 ) & 0xFFu;
        return 0xFF000000u | ( r << 16 ) | ( g << 8 ) | b;
    }

    // RGBA8(0xAARRGGBB) -> COLORREF(0x00BBGGRR)
    [[nodiscard]] constexpr COLORREF COLORREF_FromRGBA8 ( uint32_t rgba ) noexcept {
        const uint32_t r = ( rgba >> 16 ) & 0xFFu;
        const uint32_t g = ( rgba >> 8 ) & 0xFFu;
        const uint32_t b = rgba & 0xFFu;
        return static_cast< COLORREF >( ( b << 16 ) | ( g << 8 ) | r );
    }

} // namespace engine::win32
