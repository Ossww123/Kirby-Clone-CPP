#pragma once
//
// Responsibility: Minimal AABB utilities for integer-rect collisions.
// Non-Goals:      Rotations, swept tests, broadphase.
// Call-Context:   Header-only; no platform deps.
//
#include "engine/util/Types.h" // IntRect

namespace engine::physics {

    struct Int2 { int x{ 0 }; int y{ 0 }; };

    [[nodiscard]] inline bool Overlap ( const IntRect& a , const IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }

    // Minimum Translation Vector (push A out of B, axis-aligned, integer)
    [[nodiscard]] inline Int2 ResolveMTV ( const IntRect& a , const IntRect& b ) noexcept {
        const int leftPen = b.r - a.l;
        const int rightPen = a.r - b.l;
        const int topPen = b.b - a.t;
        const int bottomPen = a.b - b.t;

        const int penX = ( leftPen < rightPen ) ? leftPen : rightPen;
        const int penY = ( topPen < bottomPen ) ? topPen : bottomPen;

        Int2 mtv{};
        if ( penX < penY ) {
            mtv.x = ( leftPen < rightPen ) ? leftPen : -rightPen;
        }
        else {
            mtv.y = ( topPen < bottomPen ) ? topPen : -bottomPen;
        }
        return mtv;
    }

} // namespace engine::physics
