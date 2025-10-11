#pragma once
#include <algorithm>
#include "engine/Math.h" // engine::Vec2

namespace game {

    struct Damage {
        int amount = 1;
        engine::Vec2 knockback{ 0.f, 0.f };
    };

    struct Health {
        int   maxHp{ 3 };
        int   hp{ 3 };
        float iFrameMs{ 0.8f };
        float iFrameT{ 0.f }; // 남은 무적

        void Reset ( int maxHp_ , float iFrameMs_ ) {
            maxHp = maxHp_; hp = maxHp; iFrameMs = iFrameMs_; iFrameT = 0.f;
        }
        void Tick ( float dt ) { iFrameT = std::max ( 0.f , iFrameT - dt ); }
        bool Invuln ( ) const { return iFrameT > 0.f; }
        bool Alive ( ) const { return hp > 0; }
        // 실제로 HP를 깎고 무적을 건다. 이미 무적이면 false.
        bool Apply ( int dmg ) {
            if ( Invuln ( ) || !Alive ( ) ) return false;
            hp = std::max ( 0 , hp - std::max ( 0 , dmg ) );
            iFrameT = iFrameMs;
            return true;
        }
    };

} // namespace game
