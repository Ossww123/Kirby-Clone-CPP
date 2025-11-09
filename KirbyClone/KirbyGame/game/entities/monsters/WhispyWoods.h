#pragma once
//
// Responsibility: WhispyWoods boss — alternating puff volley and apple drop patterns.
// Non-Goals:      Rendering/VFX lifetime; movement (stationary boss).
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class WhispyWoods : public Monster {
    public:
        struct Cfg {
            Monster::Cfg base{ .knockbackMul = 0.f }; // boss doesn't move from knockback

            // air puff (projectile) pattern
            int   puffVolleyCount = 3;
            float puffIntervalMs = 0.33f;
            float puffSpeed = 220.f;
            float puffRestMs = 1.4f;

            // apple drop pattern
            int   applesPerWave = 3;
            float appleSpanPx = 240.f;   // spread left/right from boss center
            float appleTelegraphMs = 0.65f;
            float appleRestMs = 2.8f;
        };

        WhispyWoods ( const engine::IntRect& worldBounds ,
                    const engine::physics::CollisionSystem* col ,
                    const Cfg& cfg ,
                    float x , float y )
            : Monster ( worldBounds , col , cfg.base ) , m_cfg ( cfg )
        {
            SetPosition ( x , y );
            m_state = State::Rest;
            m_timer = 0.5f; // short delay before first action
        }

        bool    Inhalable ( ) const override;
        Ability AbilityGift ( ) const override;

        void TickAI ( double dt , const engine::Input& ) override;

    private:
        enum class State { Rest , Puffing , AppleDrop };

        void beginPuffVolley ( );
        void firePuffOnce ( );
        void beginAppleDrop ( );
        void dropApplesOnce ( );

    private:
        State m_state{};
        bool  m_flip = false;
        float m_timer = 0.f;
        float m_innerT = 0.f;
        int   m_shotsLeft = 0;
        Cfg   m_cfg{};
    };

} // namespace game
