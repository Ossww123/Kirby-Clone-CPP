#pragma once
//
// Responsibility: Sparky — hop-based movement + spark aura attack FSM.
// Non-Goals:      Rendering; VFX lifetime.
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class Sparky : public Monster {
    public:
        struct Cfg {
            // Base
            Monster::Cfg base{};

            // Hop movement
            int   dir = 1;
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;
            float hopRestMs = 0.5f;
            float hopVy = 360.f;        // upward speed at takeoff (positive; applied as -vy)
            float hopSmallDist = 32.f; // target horizontal distance (px)
            float hopMediumDist = 96.f; // target horizontal distance (px)

            // Aura attack (HitVolume "SparkAura")
            float wakeRange = 220.f;
            float windupMs = 0.30f;
            float firePeriod = 1.40f;
            bool  stopDuringWindup = true;

            // Instance flags
            bool  enableMove = true;
            bool  enableAttack = true;
        };

        Sparky ( const engine::IntRect& worldBounds ,
               const engine::physics::CollisionSystem* col ,
               const Cfg& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
            , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX )
            , m_turnAtEdge ( cfg.turnAtEdge ) {}

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Spark; }

    private:
        enum class AState { Idle , Windup , Cooldown };

        // movement/jump
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;
        float m_restT = 0.f;      // ground rest timer (starts on land)
        bool  m_airborne = false; // airborne between takeoff and landing
        float m_lockedVx = 0.f;   // fixed horizontal velocity while airborne

        // attack state
        AState m_state{ AState::Idle };
        float  m_cd = 0.f;
        float  m_windupT = 0.f;

        Cfg    m_cfg{};
    };

} // namespace game
