#include "engine/physics/Collision.h"
#include "engine/render/D3D11DebugDraw.h"

namespace engine::physics {

    void CollisionSystem::Clear ( ) {
        m_static.clear ( );
        m_oneway.clear ( );
    }

    void CollisionSystem::AddStaticBox ( const IntRect& r ) { m_static.push_back ( r ); }
    void CollisionSystem::AddStaticBox ( int x , int y , int w , int h ) {
        m_static.push_back ( IntRect{ x, y, x + w, y + h } );
    }

    void CollisionSystem::AddOneWayBox ( const IntRect& r ) { m_oneway.push_back ( r ); }
    void CollisionSystem::AddOneWayBox ( int x , int y , int w , int h ) {
        m_oneway.push_back ( IntRect{ x, y, x + w, y + h } );
    }

    void CollisionSystem::MoveAndCollide ( IntRect& aabb ,
                                         engine::Vec2& vel ,
                                         CollisionReport* out ,
                                         bool ignoreOneWay ,
                                         int prevBottom ) const
    {
        CollisionReport rep{};

        // 1) SOLID blocks
        for ( const IntRect& s : m_static ) {
            if ( !Overlap ( aabb , s ) ) continue;

            const Int2 mtv = ResolveMTV ( aabb , s );
            aabb.l += mtv.x; aabb.r += mtv.x;
            aabb.t += mtv.y; aabb.b += mtv.y;

            if ( mtv.x != 0 ) { vel.x = 0.f; rep.hitX = true; }
            if ( mtv.y != 0 ) {
                vel.y = 0.f; rep.hitY = true;
                if ( mtv.y < 0 ) rep.grounded = true;
            }
        }

        // 2) ONEWAY platforms
        if ( !ignoreOneWay ) {
            for ( const IntRect& r : m_oneway ) {
                if ( vel.y <= 0.f )                continue;            // only when moving downward
                if ( prevBottom == INT32_MIN )     continue;            // need previous bottom
                if ( prevBottom > r.t )            continue;            // already below top
                if ( !( aabb.r > r.l && aabb.l < r.r ) ) continue;        // horizontal overlap

                // (A) normal clamp while crossing top plane
                if ( aabb.b > r.t && aabb.t < r.t ) {
                    const int dy = r.t - aabb.b;
                    aabb.t += dy; aabb.b += dy;
                    vel.y = 0.f; rep.hitY = true; rep.grounded = true;
                }
                // (B) snap-to-top if within 1px tolerance
                else {
                    constexpr int SNAP_EPS = 1;
                    const bool nearTop = ( aabb.b <= r.t ) && ( r.t - aabb.b <= SNAP_EPS );
                    if ( nearTop ) {
                        const int dy = r.t - aabb.b;
                        aabb.t += dy; aabb.b += dy;
                        vel.y = 0.f; rep.hitY = true; rep.grounded = true;
                    }
                }
            }
        }

        // 3) Optional ground snap on solid tops (1px tolerance)
        if ( !rep.grounded && vel.y >= 0.f ) {
            constexpr int SNAP_EPS = 1;
            for ( const IntRect& s : m_static ) {
                const bool overlapX = ( aabb.r > s.l && aabb.l < s.r );
                const bool nearTop = ( aabb.b <= s.t ) && ( s.t - aabb.b <= SNAP_EPS );
                if ( overlapX && nearTop ) {
                    const int dy = s.t - aabb.b;
                    aabb.t += dy; aabb.b += dy;
                    vel.y = 0.f; rep.hitY = true; rep.grounded = true;
                    break;
                }
            }
        }

        if ( out ) *out = rep;
    }

    void CollisionSystem::DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ,
                                    std::uint32_t solid , std::uint32_t oneway ) const {
    {
        for ( const IntRect& r : m_static )
            dbg.WorldRect ( r.l , r.t , r.r - r.l , r.b - r.t , ox , oy , solid );

        for ( const IntRect& r : m_oneway )
            dbg.WorldRect ( r.l , r.t , r.r - r.l , r.b - r.t , ox , oy , oneway );
    }

} // namespace engine::physics
