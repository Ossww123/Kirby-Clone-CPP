#pragma once
#include <windows.h>
#include <vector>
#include <algorithm>
#include "engine/Math.h"

namespace engine { class D3D11DebugDraw; }

namespace engine::physics {

    // AABB
    inline bool Overlap ( const RECT& a , const RECT& b ) {
        return !( a.right <= b.left || a.left >= b.right ||
                 a.bottom <= b.top || a.top >= b.bottom );
    }

    // MTV : 밀어내는 거리
    inline POINT ResolveMTV ( const RECT& a , const RECT& b ) {
        int leftPen = b.right - a.left;
        int rightPen = a.right - b.left;
        int topPen = b.bottom - a.top;
        int bottomPen = a.bottom - b.top;

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
        void AddStaticBox ( const RECT& r );
        void AddStaticBox ( int x , int y , int w , int h );

        void AddOneWayBox ( const RECT& r );
        void AddOneWayBox ( int x , int y , int w , int h );

        void MoveAndCollide ( RECT& aabb , engine::Vec2& vel , CollisionReport* out = nullptr ,
                        bool ignoreOneWay = false , int prevBottom = INT32_MIN ) const;

        void DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy , COLORREF color ) const;

        const std::vector<RECT>& Statics ( ) const { return m_static; }
        const std::vector<RECT>& OneWays ( ) const { return m_oneway; }

    private:
        std::vector<RECT> m_static;
        std::vector<RECT> m_oneway;
    };

} // namespace engine::physics
