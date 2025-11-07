// game/combat/HitVolumeGeom.cpp
#include <algorithm>
#include <cmath>
#include "game/combat/HitVolumeGeom.h"

namespace {
    inline float Lerp ( float a , float b , float t ) { return a + ( b - a ) * t; }
    inline float Clamp01 ( float v ) { return v < 0.f ? 0.f : ( v > 1.f ? 1.f : v ); }
    inline float DegToRad ( float d ) { return d * 3.1415926535f / 180.f; }
}

namespace game {

    void HitVolumeGeom::BuildShape ( const HitVolume& hv ,
                                   engine::IntRect& outBox ,
                                   engine::Vec2& segA ,
                                   engine::Vec2& segB ,
                                   float& outRadius )
    {
        const auto& cfg = hv.GetCfg ( );
        const engine::Vec2 anchor = hv.Anchor ( );
        const int facing = hv.Facing ( );

        switch ( cfg.shape ) {
        case HitShape::Box: {
            const float w = cfg.w , h = cfg.h;
            const float x = anchor.x - w * 0.5f;
            const float y = anchor.y - h * 0.5f;
            outBox = { ( int ) std::floor ( x ), ( int ) std::floor ( y ),
                       ( int ) std::ceil ( x + w ), ( int ) std::ceil ( y + h ) };
            outRadius = 0.f;
            segA = segB = anchor;
            break;
        }
        case HitShape::Circle: {
            const float x = anchor.x - cfg.r;
            const float y = anchor.y - cfg.r;
            outBox = { ( int ) std::floor ( x ), ( int ) std::floor ( y ),
                       ( int ) std::ceil ( x + cfg.r * 2.f ), ( int ) std::ceil ( y + cfg.r * 2.f ) };
            outRadius = cfg.r;
            segA = segB = anchor;
            break;
        }
        case HitShape::Capsule: {
            float angDeg = 0.f;
            if ( cfg.behavior == HitBehavior::MeleeArc ) {
                float t = cfg.sweepDuration > 0.f ? hv.Age ( ) / cfg.sweepDuration : 1.f;
                angDeg = Lerp ( cfg.startDeg , cfg.endDeg , Clamp01 ( t ) );
            }
            if ( facing < 0 ) angDeg = 180.f - angDeg;

            const float ang = DegToRad ( angDeg );
            const engine::Vec2 dir{ std::cos ( ang ), std::sin ( ang ) };
            segA = anchor;
            segB = { anchor.x + dir.x * cfg.len, anchor.y + dir.y * cfg.len };
            outRadius = cfg.thick * 0.5f;

            const float minx = std::min ( segA.x , segB.x ) - outRadius;
            const float miny = std::min ( segA.y , segB.y ) - outRadius;
            const float maxx = std::max ( segA.x , segB.x ) + outRadius;
            const float maxy = std::max ( segA.y , segB.y ) + outRadius;
            outBox = { ( int ) std::floor ( minx ), ( int ) std::floor ( miny ),
                       ( int ) std::ceil ( maxx ),  ( int ) std::ceil ( maxy ) };
            break;
        }
        }
    }

} // namespace game
