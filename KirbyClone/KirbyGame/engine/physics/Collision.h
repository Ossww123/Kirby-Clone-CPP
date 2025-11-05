#pragma once
#include <windows.h>
#include <vector>
#include <algorithm>
#include "engine/Math.h"

namespace engine { class D3D11DebugDraw; }

namespace engine::physics {

    // AABB
    inline bool Overlap ( const IntRect& a , const IntRect& b ) {
        return !( a.r <= b.l || a.l >= b.r ||
                 a.b <= b.t || a.t >= b.b );
    }

    // MTV : 밀어내는 거리
    inline POINT ResolveMTV ( const IntRect& a , const IntRect& b ) {
        int leftPen = b.r - a.l;
        int rightPen = a.r - b.l;
        int topPen = b.b - a.t;
        int bottomPen = a.b - b.t;

        int penX = std::min ( leftPen , rightPen );
        int penY = std::min ( topPen , bottomPen );

        POINT mtv{ 0,0 };
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
        bool grounded = false; // 아래쪽 접촉
    };

    class CollisionSystem {
    public:
        void Clear ( );
        void AddStaticBox ( const IntRect& r );
        void AddStaticBox ( int x , int y , int w , int h );

        void AddOneWayBox ( const IntRect& r );
        void AddOneWayBox ( int x , int y , int w , int h );

        void MoveAndCollide ( IntRect& aabb , engine::Vec2& vel , CollisionReport* out = nullptr ,
                        bool ignoreOneWay = false , int prevBottom = INT32_MIN ) const;

        void DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy , COLORREF solidColor , COLORREF onewayColor ) const;

        const std::vector<IntRect>& Statics ( ) const { return m_static; }
        const std::vector<IntRect>& OneWays ( ) const { return m_oneway; }

    private:
        std::vector<IntRect> m_static;
        std::vector<IntRect> m_oneway;
    };

} // namespace engine::physics
