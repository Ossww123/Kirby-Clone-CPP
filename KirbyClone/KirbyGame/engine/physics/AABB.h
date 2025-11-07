#pragma once
//
// Responsibility: Minimal 2D overlap utilities (AABB plus circle/segment/capsule helpers).
// Non-Goals:      Rotations/OBB, swept tests, broadphase, platform deps.
// Call-Context:   Header-only; inline; no Windows types.
//
#include <cmath>
#include <algorithm>
#include "engine/util/Types.h" // IntRect
#include "engine/util/Math.h"  // Vec2

namespace engine::physics {

    // === Types ===
    struct Int2 { int x{ 0 }; int y{ 0 }; };

    // === Policy ===
    // Overlap is "strict": touching on the boundary is NOT considered overlap.
    // (Matches existing AABB Overlap() semantics.)

    // --- AABB x AABB ---
    [[nodiscard]] inline bool Overlap ( const IntRect& a , const IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }

    // MTV to push A out of B (axis-aligned, integer)
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

    // --- Helpers ---
    [[nodiscard]] inline float Clamp ( float v , float lo , float hi ) {
        return v < lo ? lo : ( v > hi ? hi : v );
    }

    [[nodiscard]] inline IntRect Expand ( const IntRect& r , float by ) {
        return IntRect{
            static_cast< int >( std::floor ( r.l - by ) ),
            static_cast< int >( std::floor ( r.t - by ) ),
            static_cast< int >( std::ceil ( r.r + by ) ),
            static_cast< int >( std::ceil ( r.b + by ) )
        };
    }

    // --- AABB x Circle (strict; boundary-touch = false) ---
    [[nodiscard]] inline bool OverlapRectCircle ( const IntRect& r , const engine::Vec2& c , float rad ) {
        const float cx = Clamp ( c.x , static_cast< float >( r.l ) , static_cast< float >( r.r ) );
        const float cy = Clamp ( c.y , static_cast< float >( r.t ) , static_cast< float >( r.b ) );
        const float dx = c.x - cx , dy = c.y - cy;
        return ( dx * dx + dy * dy ) < ( rad * rad ); // strict
    }

    // --- Segment x AABB (slab method; inclusive on boundary internally) ---
    [[nodiscard]] inline bool OverlapSegmentAABB ( const engine::Vec2& p0 ,
                                                 const engine::Vec2& p1 ,
                                                 const IntRect& b )
    {
        float tmin = 0.f , tmax = 1.f;
        const float dx = p1.x - p0.x , dy = p1.y - p0.y;
        auto upd = [ & ] ( float p , float q )->bool {
            const float eps = 1e-6f;
            if ( std::fabs ( p ) < eps ) return q >= 0.f; // parallel: inside slab?
            const float t = q / p;
            if ( p < 0.f ) { if ( t > tmax ) return false; if ( t > tmin ) tmin = t; }
            else { if ( t < tmin ) return false; if ( t < tmax ) tmax = t; }
            return true;
            };
        // x in [b.l, b.r]
        if ( !upd ( dx , static_cast< float >( b.r ) - p0.x ) ) return false;
        if ( !upd ( -dx , p0.x - static_cast< float >( b.l ) ) ) return false;
        // y in [b.t, b.b]
        if ( !upd ( dy , static_cast< float >( b.b ) - p0.y ) ) return false;
        if ( !upd ( -dy , p0.y - static_cast< float >( b.t ) ) ) return false;
        return tmax >= tmin;
    }

    // --- AABB x Capsule (segment + radius) ---
    // Implemented as segment vs expanded AABB for speed/stability.
    [[nodiscard]] inline bool OverlapRectCapsule ( const IntRect& r ,
                                                 const engine::Vec2& a ,
                                                 const engine::Vec2& b ,
                                                 float radius )
    {
        return OverlapSegmentAABB ( a , b , Expand ( r , radius ) );
    }

} // namespace engine::physics
