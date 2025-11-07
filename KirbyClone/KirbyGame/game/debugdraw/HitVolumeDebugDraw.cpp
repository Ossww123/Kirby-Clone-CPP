#include "game/debugdraw/HitVolumeDebugDraw.h"
#include "game/combat/HitVolume.h"
#include "engine/render/IDebugDraw.h"
#include <cmath>
#include <algorithm>

namespace {
    inline float Lerp ( float a , float b , float t ) { return a + ( b - a ) * t; }
    inline float DegToRad ( float d ) { return d * 3.1415926535f / 180.f; }
    constexpr engine::Rgba32 RGBA8 ( unsigned r , unsigned g , unsigned b , unsigned a = 255 ) {
        return ( engine::Rgba32 ( a ) << 24 ) | ( engine::Rgba32 ( r ) << 16 ) | ( engine::Rgba32 ( g ) << 8 ) | engine::Rgba32 ( b );
    }
    void DrawCircleApprox ( engine::IDebugDraw* dbg , const engine::Vec2& c , float r , int segs ,
                          int ox , int oy , engine::Rgba32 col ) {
        if ( !dbg ) return; if ( segs < 6 ) segs = 6;
        float px = c.x + r , py = c.y;
        for ( int i = 1; i <= segs; ++i ) {
            float ang = ( float ) i / segs * 6.2831853f;
            float x = c.x + std::cos ( ang ) * r;
            float y = c.y + std::sin ( ang ) * r;
            dbg->WorldLine ( ( int ) px , ( int ) py , ( int ) x , ( int ) y , ox , oy , col );
            px = x; py = y;
        }
    }
}

namespace game {

    void HitVolumeDebugDraw::Draw ( const HitVolume& hv , engine::IDebugDraw* dbg , int ox , int oy ) {
        if ( !dbg || !hv.Alive ( ) ) return;
        const auto& cfg = hv.GetCfg ( );
        const engine::Vec2 anchor = hv.Anchor ( );
        int facing = hv.Facing ( );
        const auto col = RGBA8 ( 120 , 240 , 255 );

        switch ( cfg.shape ) {
        case HitShape::Box: {
            const int x = ( int ) std::floor ( anchor.x - cfg.w * 0.5f );
            const int y = ( int ) std::floor ( anchor.y - cfg.h * 0.5f );
            dbg->WorldRect ( x , y , ( int ) std::ceil ( cfg.w ) , ( int ) std::ceil ( cfg.h ) , ox , oy , col );
            break;
        }
        case HitShape::Circle:
            DrawCircleApprox ( dbg , anchor , cfg.r , 20 , ox , oy , col );
            break;

        case HitShape::Capsule: {
            float angDeg = 0.f;
            if ( cfg.behavior == HitBehavior::MeleeArc ) {
                float t = ( cfg.sweepDuration > 0.f ) ? ( hv.Age ( ) / cfg.sweepDuration ) : 1.f;
                t = std::clamp ( t , 0.f , 1.f );
                angDeg = Lerp ( cfg.startDeg , cfg.endDeg , t );
            }
            if ( facing < 0 ) angDeg = 180.f - angDeg;

            const float ang = DegToRad ( angDeg );
            const engine::Vec2 dir{ std::cos ( ang ), std::sin ( ang ) };
            const engine::Vec2 A = anchor;
            const engine::Vec2 B = { anchor.x + dir.x * cfg.len, anchor.y + dir.y * cfg.len };
            const float R = cfg.thick * 0.5f;

            dbg->WorldLine ( ( int ) A.x , ( int ) A.y , ( int ) B.x , ( int ) B.y , ox , oy , col );
            engine::Vec2 d{ B.x - A.x, B.y - A.y };
            float len = std::sqrt ( d.x * d.x + d.y * d.y );
            if ( len < 1e-5f ) { DrawCircleApprox ( dbg , A , R , 16 , ox , oy , col ); return; }
            engine::Vec2 n{ -d.y / len, d.x / len };
            engine::Vec2 A1{ A.x + n.x * R, A.y + n.y * R } , A2{ A.x - n.x * R, A.y - n.y * R };
            engine::Vec2 B1{ B.x + n.x * R, B.y + n.y * R } , B2{ B.x - n.x * R, B.y - n.y * R };
            dbg->WorldLine ( ( int ) A1.x , ( int ) A1.y , ( int ) B1.x , ( int ) B1.y , ox , oy , col );
            dbg->WorldLine ( ( int ) A2.x , ( int ) A2.y , ( int ) B2.x , ( int ) B2.y , ox , oy , col );
            DrawCircleApprox ( dbg , A , R , 14 , ox , oy , col );
            DrawCircleApprox ( dbg , B , R , 14 , ox , oy , col );
            break;
        }
        }
    }

} // namespace game
