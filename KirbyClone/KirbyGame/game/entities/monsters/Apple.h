#pragma once
//
// Responsibility: Apple — telegraph, fall, single bounce, then roll.
// Non-Goals:      Rendering/VFX lifetime.
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class Apple : public Monster {
    public:
        struct Cfg {
            Monster::Cfg base{};
            float telegraphMs = 0.6f;   // blink & wait at spawn
            float bounceVx = 140.f;  // first bounce horizontal speed
            float bounceVy = 360.f;  // first bounce upward speed
            float rollSpeed = 90.f;   // ground roll target speed
        };

        Apple ( const engine::IntRect& worldBounds ,
              const engine::physics::CollisionSystem* col ,
              const Cfg& cfg ,
              float spawnX , float spawnY )
            : Monster ( worldBounds , col , cfg.base ) , m_cfg ( cfg )
        {
            SetPosition ( spawnX , spawnY );
            m_phase = Phase::Telegraph;
            m_phaseT = m_cfg.telegraphMs;
        }

        bool    Inhalable ( ) const override;
        Ability AbilityGift ( ) const override;

        void TickAI ( double dt , const engine::Input& ) override;

    private:
        enum class Phase { Telegraph , Fall , Bounce , Roll };

        Phase m_phase{};
        float m_phaseT = 0.f;
        int   m_dir = +1;

        Cfg   m_cfg{};
    };

} // namespace game
