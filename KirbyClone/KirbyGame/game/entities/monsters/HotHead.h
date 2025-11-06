#pragma once
//
// Responsibility: HotHead — patrol + fire breath (projectile burst) FSM.
// Non-Goals:      Rendering; data-driven VFX ownership.
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class HotHead : public Monster {
    public:
        struct Cfg {
            // Base
            Monster::Cfg base{};

            // Movement
            int   dir = 1;
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;

            // Attack (Fire)
            float wakeRange = 260.f;   // detection range
            float windupMs = 0.25f;   // telegraph
            float breathMs = 0.55f;   // breath duration
            float fireIntervalMs = 0.06f;   // pellet interval during breath
            float bulletSpeed = 360.f;   // projectile speed
            bool  stopDuringWindup = true; // stop during windup/breath
            float firePeriod = 1.1f;    // cooldown

            // Instance flags
            bool  enableMove = true;
            bool  enableAttack = true;
        };

        HotHead ( const engine::IntRect& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Cfg& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
            , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX )
            , m_turnAtEdge ( cfg.turnAtEdge ) {}

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Fire; }

    private:
        enum class AState { Idle , Windup , Breathing , Cooldown };

        // movement
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;

        // attack state
        AState m_state{ AState::Idle };
        float  m_cd = 0.f;
        float  m_windupT = 0.f;
        float  m_breathT = 0.f;
        float  m_emitT = 0.f;
        int    m_face = +1;

        Cfg    m_cfg{};
    };

} // namespace game
