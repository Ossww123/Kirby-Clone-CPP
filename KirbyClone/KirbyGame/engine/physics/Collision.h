#pragma once
//
// Responsibility: Axis-aligned box collisions (overlap/MTV) and resolution against
//                  static blocks + one-way platforms, with optional ground snap.
// Non-Goals:      Continuous collision for fast movers, slope/triangle meshes,
//                 dynamic-dynamic resolution, broadphase partitioning.
// Call-Context:   Main thread; integer world coords (pixels).
//

#include <cstdint>
#include <vector>
#include <algorithm>     // std::min
#include <windows.h>     // COLORREF (debug draw only)

#include "engine/util/Types.h"  // IntRect
#include "engine/util/Math.h"   // Vec2

namespace engine { class D3D11DebugDraw; }

namespace engine::physics {

    // Simple int2 (avoid Win32 POINT in headers)
    struct Int2 { int x{ 0 }; int y{ 0 }; };

    // AABB overlap test
    inline bool Overlap ( const IntRect& a , const IntRect& b ) noexcept {
        return !( a.r <= b.l || a.l >= b.r || a.b <= b.t || a.t >= b.b );
    }

    // Minimum Translation Vector (integer; push A out of B)
    inline Int2 ResolveMTV ( const IntRect& a , const IntRect& b ) noexcept {
        const int leftPen = b.r - a.l;
        const int rightPen = a.r - b.l;
        const int topPen = b.b - a.t;
        const int bottomPen = a.b - b.t;

        const int penX = std::min ( leftPen , rightPen );
        const int penY = std::min ( topPen , bottomPen );

        Int2 mtv{};
        if ( penX < penY ) {
            mtv.x = ( leftPen < rightPen ) ? leftPen : -rightPen;
        }
        else {
            mtv.y = ( topPen < bottomPen ) ? topPen : -bottomPen;
        }
        return mtv;
    }

    struct CollisionReport {
        bool hitX = false;
        bool hitY = false;
        bool grounded = false; // contacted below
    };

    class CollisionSystem {
    public:
        void Clear ( );

        void AddStaticBox ( const IntRect& r );
        void AddStaticBox ( int x , int y , int w , int h );

        void AddOneWayBox ( const IntRect& r );
        void AddOneWayBox ( int x , int y , int w , int h );

        // prevBottom: previous frame bottom (pixels). Use INT32_MIN if unknown.
        void MoveAndCollide ( IntRect& aabb ,
                            engine::Vec2& vel ,
                            CollisionReport* out = nullptr ,
                            bool ignoreOneWay = false ,
                            int prevBottom = INT32_MIN ) const;

        // Debug wireframe (kept COLORREF for consistency with D3D11DebugDraw)
        void DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ,
                       std::uint32_t solidRGBA , std::uint32_t onewayRGBA ) const;

        const std::vector<IntRect>& Statics ( ) const { return m_static; }
        const std::vector<IntRect>& OneWays ( ) const { return m_oneway; }

    private:
        std::vector<IntRect> m_static;
        std::vector<IntRect> m_oneway;
    };

} // namespace engine::physics
