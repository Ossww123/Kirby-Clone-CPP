#pragma once
#include <algorithm>
#include "engine/Math.h" // engine::Vec2

namespace game {

    enum class Team { Player , Enemy , Neutral };
    enum class HitKind { Contact , Projectile , Hazard , Pit };

    struct Damage {
        int amount = 1;
        engine::Vec2 knockback{ 0.f, 0.f };

        // --- optional metadata ---
        Team    from = Team::Enemy;         // 아군/적군/중립 필터
        HitKind kind = HitKind::Contact;    // 접촉/투사체/함정 등
        bool    ignoreIFrames = false;      // 컷신 강제 타격 등 특수 상황
        bool    additiveImpulse = false;    // true면 속도에 더하기, false면 설정(SetVelocity)
    };

    struct Health {
        int   maxHp{ 3 };
        int   hp{ 3 };
        float iFrameMs{ 0.8f };
        float iFrameT{ 0.f };

        void Reset ( int maxHp_ , float iFrameMs_ ) {
            maxHp = maxHp_; hp = maxHp; iFrameMs = iFrameMs_; iFrameT = 0.f;
        }
        void Tick ( float dt ) { iFrameT = std::max ( 0.f , iFrameT - dt ); }
        bool Invuln ( ) const { return iFrameT > 0.f; }
        bool Alive ( ) const { return hp > 0; }
        bool Apply ( int dmg ) {
            if ( Invuln ( ) || !Alive ( ) ) return false;
            hp = std::max ( 0 , hp - std::max ( 0 , dmg ) );
            iFrameT = iFrameMs;
            return true;
        }
    };

} // namespace game
