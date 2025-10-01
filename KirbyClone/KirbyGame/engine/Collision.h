#pragma once
#include <algorithm>
#include <windows.h>

namespace engine::coll {

    // AABB 교차 테스트
    inline bool Overlap ( const RECT& a , const RECT& b ) {
        return !( a.right <= b.left || a.left >= b.right ||
                 a.bottom <= b.top || a.top >= b.bottom );
    }

    // 최소 이동 벡터(MTV) 계산: a를 b 바깥으로 밀어냄
    // 반환: (dx,dy) 보정값. 축 우선 분리(침투가 더 작은 축으로)
    inline POINT ResolveMTV ( const RECT& a , const RECT& b ) {
        int leftPen = b.right - a.left;   // b가 a 왼쪽에서 밀어냄(+) → a를 +x로
        int rightPen = a.right - b.left;   // a가 b 왼쪽 침투(+)  → a를 -x로
        int topPen = b.bottom - a.top;    // b가 a 위에서 밀어냄(+) → a를 +y로
        int bottomPen = a.bottom - b.top;    // a가 b 위로 침투(+)   → a를 -y로

        // 실제 침투량(양수만)
        int penX = std::min ( leftPen , rightPen );
        int penY = std::min ( topPen , bottomPen );

        POINT mtv{ 0,0 };
        if ( penX < penY ) {
            // X축으로 분리
            if ( leftPen < rightPen )  mtv.x = leftPen;   // a를 +x로
            else                     mtv.x = -rightPen;  // a를 -x로
        }
        else {
            // Y축으로 분리
            if ( topPen < bottomPen )  mtv.y = topPen;    // a를 +y로(위로 밀림)
            else                     mtv.y = -bottomPen; // a를 -y로(아래로 밀림)
        }
        return mtv;
    }

} // namespace engine::coll
