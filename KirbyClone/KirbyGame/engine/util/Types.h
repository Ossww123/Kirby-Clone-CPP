#pragma once
//
// Responsibility: Small engine-wide POD rect type and helpers.
// Non-Goals:      Platform types (e.g., RECT), rendering/IO.
// Call-Context:   Header-only; constexpr/inline only.
//

namespace engine {

    struct IntRect { int l , t , r , b; };

    inline constexpr IntRect MakeIRectLTWH ( int x , int y , int w , int h )
    {
        return { x , y , x + w , y + h };
    }

    inline constexpr int Width ( const IntRect& rc ) { return rc.r - rc.l; }
    inline constexpr int Height ( const IntRect& rc ) { return rc.b - rc.t; }

} // namespace engine
