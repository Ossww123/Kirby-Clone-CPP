#include "game/HitVolumeSystem.h"
#include "game/HitVolumeFactory.h"   // archetypes

#include <algorithm>
#include <cmath>

namespace game {

    static inline float Lerp ( float a , float b , float t ) { return a + ( b - a ) * t; }
    static inline float DegToRad ( float d ) { return d * 3.1415926535f / 180.f; }

    int HitVolumeSystem::Spawn ( const SpawnDesc& s ) {
        const HitVolume::Cfg* def = HitVolumeFactory::Find ( s.archetype );
        if ( !def ) return -1;

        auto hv = std::make_unique<HitVolume> ( s.ownerId , s.ownerFacing , s.worldAnchor , *def );

        Slot slot;
        slot.id = m_nextId++;
        slot.hv = std::move ( hv );
        m_vols.emplace_back ( std::move ( slot ) );
        return m_vols.back ( ).id;
    }

    void HitVolumeSystem::Step ( double fixedDt , const std::vector<Target>& targets ) {
        // 1) Update volumes / follow owners
        for ( auto& s : m_vols ) {
            auto& hv = *s.hv;

            // follow owner (anchor + facing)
            if ( m_locateOwner ) {
                engine::Vec2 pos; int fac = hv.Facing ( );
                if ( m_locateOwner ( hv.Owner ( ) , pos , fac ) ) {
                    // apply local offset & facing
                    const auto& cfg = hv.GetCfg ( );
                    engine::Vec2 anchor = pos;
                    if ( cfg.followFacing ) anchor.x += cfg.localOffset.x * ( fac >= 0 ? 1.f : -1.f );
                    else                  anchor.x += cfg.localOffset.x;
                    anchor.y += cfg.localOffset.y;
                    hv.SetAnchor ( anchor );
                    hv.SetFacing ( fac );
                }
            }

            hv.Update ( fixedDt , *( engine::Input* )nullptr );
        }

        // 2) Overlap tests vs targets
        for ( auto& s : m_vols ) {
            auto& hv = *s.hv;
            if ( !hv.Alive ( ) ) continue;

            RECT volBox{}; engine::Vec2 A{} , B{}; float R{};
            BuildShape ( hv , volBox , A , B , R );

            const auto& cfg = hv.GetCfg ( );
            for ( const auto& t : targets ) {
                if ( !t.alive ) continue;

                bool hit = false;
                switch ( cfg.shape ) {
                case HitShape::Box:
                    hit = !( t.aabb.right <= volBox.left || t.aabb.left >= volBox.right ||
                            t.aabb.bottom <= volBox.top || t.aabb.top >= volBox.bottom );
                    break;
                case HitShape::Circle: {
                    engine::Vec2 c = { ( volBox.left + volBox.right ) * 0.5f,
                                       ( volBox.top + volBox.bottom ) * 0.5f };
                    hit = Overlap_RectCircle ( t.aabb , c , cfg.r );
                    break;
                }
                case HitShape::Capsule: {
                    // current capsule
                    hit = Overlap_RectCapsule ( t.aabb , A , B , R );
                    // prev capsule
                    if ( !hit && s.hasPrev ) hit = Overlap_RectCapsule ( t.aabb , s.prevA , s.prevB , s.prevR );
                    // 
                    if ( !hit && s.hasPrev ) hit = Overlap_RectCapsule ( t.aabb , s.prevB , B , R );
                    break;
                }
                }

                if ( hit && t.id != hv.Owner ( ) && hv.CanHitTarget ( t.id ) ) {
                    hv.MarkHitTarget ( t.id );
                    m_hits.push_back ( HitEvent{ t.id, s.id, hv.Owner ( ), cfg.payload } );
                }
            }

            s.prevA = A; s.prevB = B; s.prevR = R; s.hasPrev = true;
        }

        // 3) Cleanup / despawn events
        for ( auto& s : m_vols ) {
            if ( s.hv && !s.hv->Alive ( ) ) {
                m_despawns.push_back ( DespawnEvent{ s.id, true } );
            }
        }
        m_vols.erase ( std::remove_if ( m_vols.begin ( ) , m_vols.end ( ) ,
            [ ] ( const Slot& s ) { return !s.hv || !s.hv->Alive ( ); } ) , m_vols.end ( ) );
    }

    void HitVolumeSystem::DebugDraw ( engine::D3D11DebugDraw& dbg , int ox , int oy ) const {
        auto drawCircleApprox = [ & ] ( const engine::Vec2& c , float r , int segs , COLORREF col ) {
            if ( segs < 6 ) segs = 6;
            float prevx = c.x + r , prevy = c.y;
            for ( int i = 1; i <= segs; ++i ) {
                float ang = ( float ) i / segs * 6.2831853f;
                float x = c.x + std::cos ( ang ) * r;
                float y = c.y + std::sin ( ang ) * r;
                dbg.WorldLine ( ( int ) prevx , ( int ) prevy , ( int ) x , ( int ) y , ox , oy , col );
                prevx = x; prevy = y;
            }
        };

        for ( const auto& s : m_vols ) {
            const auto& hv = *s.hv;
            RECT box{}; engine::Vec2 A{} , B{}; float R{};
            BuildShape ( hv , box , A , B , R );

            COLORREF col = RGB ( 120 , 240 , 255 );
            switch ( hv.GetCfg ( ).shape ) {
            case HitShape::Box:
                dbg.WorldRect ( box.left , box.top , box.right - box.left , box.bottom - box.top , ox , oy , col );
                break;
            case HitShape::Circle:
                drawCircleApprox ( { ( box.left + box.right ) * 0.5f, ( box.top + box.bottom ) * 0.5f } ,
                                 hv.GetCfg ( ).r , 20 , col );
                break;
            case HitShape::Capsule: {
                // 중심선
                dbg.WorldLine ( ( int ) A.x , ( int ) A.y , ( int ) B.x , ( int ) B.y , ox , oy , col );

                // 굵기 표현: 법선 벡터로 ±R 오프셋
                engine::Vec2 d{ B.x - A.x, B.y - A.y };
                float len = std::sqrt ( d.x * d.x + d.y * d.y );
                if ( len < 1e-5f ) { drawCircleApprox ( A , R , 16 , col ); break; }
                engine::Vec2 n{ -d.y / len, d.x / len }; // 좌측 법선

                engine::Vec2 A1{ A.x + n.x * R, A.y + n.y * R };
                engine::Vec2 A2{ A.x - n.x * R, A.y - n.y * R };
                engine::Vec2 B1{ B.x + n.x * R, B.y + n.y * R };
                engine::Vec2 B2{ B.x - n.x * R, B.y - n.y * R };

                // 옆면 두 줄
                dbg.WorldLine ( ( int ) A1.x , ( int ) A1.y , ( int ) B1.x , ( int ) B1.y , ox , oy , col );
                dbg.WorldLine ( ( int ) A2.x , ( int ) A2.y , ( int ) B2.x , ( int ) B2.y , ox , oy , col );

                // 끝단 반원 근사
                drawCircleApprox ( A , R , 14 , col );
                drawCircleApprox ( B , R , 14 , col );
                break;
            }
            }
        }
    }


    // === Helpers ===

    static inline float clampf ( float v , float lo , float hi ) { return v < lo ? lo : ( v > hi ? hi : v ); }

    bool HitVolumeSystem::Overlap_RectCircle ( const RECT& r , const engine::Vec2& c , float rad ) {
        // Closest point on AABB to circle center
        float cx = clampf ( c.x , ( float ) r.left , ( float ) r.right );
        float cy = clampf ( c.y , ( float ) r.top , ( float ) r.bottom );
        float dx = c.x - cx;
        float dy = c.y - cy;
        return ( dx * dx + dy * dy ) <= ( rad * rad );
    }

    // Segment vs AABB (axis-aligned) using slab method
    bool HitVolumeSystem::Overlap_SegmentAABB ( const engine::Vec2& p0 , const engine::Vec2& p1 , const RECT& B ) {
        float tmin = 0.f , tmax = 1.f;
        const float dx = p1.x - p0.x;
        const float dy = p1.y - p0.y;

        auto update = [ & ] ( float p , float q )->bool {
            if ( std::fabs ( p ) < 1e-6f ) return q >= 0.f; // parallel
            const float t = q / p;
            if ( p < 0.f ) { // t >= t
                if ( t > tmax ) return false;
                if ( t > tmin ) tmin = t;
            }
            else {       // t <= t
                if ( t < tmin ) return false;
                if ( t < tmax ) tmax = t;
            }
            return true;
            };

        // x in [Bx0, Bx1]  →  p0.x + dx*t >= B.left  and  -(p0.x + dx*t) >= -B.right
        if ( !update ( dx , ( float ) B.right - p0.x ) ) return false; //  dx*t <= right-p0.x
        if ( !update ( -dx , p0.x - ( float ) B.left ) ) return false; // -dx*t <= p0.x-left

        // y in [By0, By1]
        if ( !update ( dy , ( float ) B.bottom - p0.y ) ) return false;
        if ( !update ( -dy , p0.y - ( float ) B.top ) ) return false;

        return tmax >= tmin;
    }

    bool HitVolumeSystem::Overlap_RectCapsule ( const RECT& r , const engine::Vec2& p0 , const engine::Vec2& p1 , float radius ) {
        // AABB inflated by radius: capsule intersects rect iff segment intersects inflated rect
        RECT R{
            ( int ) std::floor ( r.left - radius ),
            ( int ) std::floor ( r.top - radius ),
            ( int ) std::ceil ( r.right + radius ),
            ( int ) std::ceil ( r.bottom + radius )
        };
        return Overlap_SegmentAABB ( p0 , p1 , R );
    }

    void HitVolumeSystem::BuildShape ( const HitVolume& hv , RECT& outBox , engine::Vec2& segA , engine::Vec2& segB , float& outRadius ) const {
        const auto& cfg = hv.GetCfg ( );
        const engine::Vec2 anchor = hv.Anchor ( );
        const int   facing = hv.Facing ( );

        switch ( cfg.shape ) {
        case HitShape::Box: {
            const float w = cfg.w , h = cfg.h;
            const float x = anchor.x - w * 0.5f;
            const float y = anchor.y - h * 0.5f;
            outBox = RECT{ ( int ) std::floor ( x ), ( int ) std::floor ( y ),
                           ( int ) std::ceil ( x + w ), ( int ) std::ceil ( y + h ) };
            outRadius = 0.f;
            segA = segB = anchor;
            break;
        }
        case HitShape::Circle: {
            const float x = anchor.x - cfg.r;
            const float y = anchor.y - cfg.r;
            outBox = RECT{ ( int ) std::floor ( x ), ( int ) std::floor ( y ),
                           ( int ) std::ceil ( x + cfg.r * 2.f ), ( int ) std::ceil ( y + cfg.r * 2.f ) };
            outRadius = cfg.r;
            segA = segB = anchor;
            break;
        }
        case HitShape::Capsule: {
            // Two main uses:
            //  - Attached thrust (straight capsule forward)
            //  - MeleeArc: sweep angle over time
            float angleDeg = 0.f;
            if ( cfg.behavior == HitBehavior::MeleeArc ) {
                float t = ( cfg.sweepDuration > 0.f ) ? hv.Age ( ) / cfg.sweepDuration : 1.f;
                t = clampf ( t , 0.f , 1.f );
                angleDeg = Lerp ( cfg.startDeg , cfg.endDeg , t );
            }
            // Facing: flip angles horizontally
            if ( facing < 0 ) angleDeg = 180.f - angleDeg;

            const float ang = DegToRad ( angleDeg );
            const engine::Vec2 dir = { std::cos ( ang ), std::sin ( ang ) };
            const engine::Vec2 A = anchor;
            const engine::Vec2 B = { anchor.x + dir.x * cfg.len, anchor.y + dir.y * cfg.len };

            segA = A; segB = B; outRadius = cfg.thick * 0.5f;

            // Bounding for debug (approx, not used for hit)
            const float minx = std::min ( A.x , B.x ) - outRadius;
            const float miny = std::min ( A.y , B.y ) - outRadius;
            const float maxx = std::max ( A.x , B.x ) + outRadius;
            const float maxy = std::max ( A.y , B.y ) + outRadius;
            outBox = RECT{ ( int ) std::floor ( minx ), ( int ) std::floor ( miny ),
                           ( int ) std::ceil ( maxx ),  ( int ) std::ceil ( maxy ) };
            break;
        }
        }
    }

} // namespace game
