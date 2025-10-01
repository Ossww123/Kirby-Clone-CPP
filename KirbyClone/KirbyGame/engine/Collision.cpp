#include "engine/Collision.h"
#include "engine/D3D11DebugDraw.h"

namespace engine::physics {

    void CollisionSystem::Clear ( ) { m_static.clear ( ); }

    void CollisionSystem::AddStaticBox ( const RECT& r ) { m_static.push_back ( r ); }

    void CollisionSystem::AddStaticBox ( int x , int y , int w , int h ) {
        RECT r{ x, y, x + w, y + h };
        m_static.push_back ( r );
    }

    void CollisionSystem::MoveAndCollide ( RECT& aabb , engine::Vec2& vel , CollisionReport* out ) const
    {
        CollisionReport rep{};
        for ( const RECT& s : m_static ) {
            if ( !Overlap ( aabb , s ) ) continue;

            POINT mtv = ResolveMTV ( aabb , s );
            aabb.left += mtv.x; aabb.right += mtv.x;
            aabb.top += mtv.y; aabb.bottom += mtv.y;

            if ( mtv.x != 0 ) { vel.x = 0.f; rep.hitX = true; }
            if ( mtv.y != 0 ) {
                vel.y = 0.f; rep.hitY = true;
                if ( mtv.y < 0 ) rep.grounded = true; // 위로 밀리면 바닥 접촉
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
