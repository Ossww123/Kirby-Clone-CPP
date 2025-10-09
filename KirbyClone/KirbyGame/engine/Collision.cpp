#include "engine/Collision.h"
#include "engine/D3D11DebugDraw.h"

namespace engine::physics {

    void CollisionSystem::Clear ( ) { m_static.clear ( ); m_oneway.clear ( ); }

    void CollisionSystem::AddStaticBox ( const RECT& r ) { m_static.push_back ( r ); }
    void CollisionSystem::AddStaticBox ( int x , int y , int w , int h ) {
        RECT r{ x, y, x + w, y + h }; m_static.push_back ( r );
    }

    void CollisionSystem::AddOneWayBox ( const RECT& r ) { m_oneway.push_back ( r ); }
    void CollisionSystem::AddOneWayBox ( int x , int y , int w , int h ) {
        RECT r{ x,y,x + w,y + h }; m_oneway.push_back ( r );
    }

    void CollisionSystem::MoveAndCollide ( RECT& aabb , engine::Vec2& vel , CollisionReport* out ,
                                     bool ignoreOneWay , int prevBottom ) const
    {
        CollisionReport rep{};

        // 1) SOLID
        for ( const RECT& s : m_static ) {
            if ( !Overlap ( aabb , s ) ) continue;
            POINT mtv = ResolveMTV ( aabb , s );
            aabb.left += mtv.x; aabb.right += mtv.x;
            aabb.top += mtv.y; aabb.bottom += mtv.y;
            if ( mtv.x != 0 ) { vel.x = 0.f; rep.hitX = true; }
            if ( mtv.y != 0 ) { 
                vel.y = 0.f; rep.hitY = true;
                if ( mtv.y < 0 ) rep.grounded = true;
            }
        }

        // 2) ONEWAY
        if ( !ignoreOneWay ) {
            for ( const RECT& r : m_oneway ) {
                if ( vel.y <= 0.f )                                     continue;
                if ( prevBottom == INT32_MIN )                          continue; // prevBottom 필수
                if ( prevBottom > r.top )                               continue;
                if ( !( aabb.right > r.left && aabb.left < r.right ) )  continue; // 수평 오버랩 확인

                // 박스가 r.top을 가로질러 내려갔다면 클램프
                if ( aabb.bottom > r.top && aabb.top < r.top ) {
                    const int dy = r.top - aabb.bottom; // 음수 또는 0
                    aabb.top += dy; aabb.bottom += dy;
                    vel.y = 0.f; rep.hitY = true; rep.grounded = true;
                }
            }
        }

        if ( out ) *out = rep;
    }

    void CollisionSystem::DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy , COLORREF color ) const
    {
        for ( const RECT& r : m_static ) {
            dbg.WorldRect ( r.left , r.top , r.right - r.left , r.bottom - r.top , ox , oy , color );
        }
    }

} // namespace engine::physics
