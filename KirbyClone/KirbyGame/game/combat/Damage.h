#pragma once
//
// Responsibility: Team/hit kinds and lightweight damage & health helpers.
// Non-Goals:      Hit resolution, VFX, invuln visuals.
// Call-Context:   Header-only; used by combat/physics.
// Notes:          Uses engine::Vec2 (requires Math.h).
//
#include <algorithm>
#include "engine/util/Math.h" // engine::Vec2

namespace game {

    enum class Team : int { Player , Enemy , Neutral };
    enum class HitKind : int { Contact , Projectile , Hazard , Pit };

    struct Damage {
        int amount = 1;
        engine::Vec2 knockback{ 0.f, 0.f };

        // optional metadata
        Team    from = Team::Enemy;          // source team filter
        HitKind kind = HitKind::Contact;     // contact/projectile/etc.
        bool    ignoreIFrames = false;       // cutscene-forced hits
        bool    additiveImpulse = false;     // true:add, false:set velocity
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
        bool Alive ( )  const { return hp > 0; }
        bool Apply ( int dmg ) {
            if ( Invuln ( ) || !Alive ( ) ) return false;
            hp = std::max ( 0 , hp - std::max ( 0 , dmg ) );
            iFrameT = iFrameMs;
            return true;
        }
    };

} // namespace game
