#pragma once
//
// Responsibility: WaddleDoo — patrol + beam attack with windup/cooldown.
// Non-Goals:      Rendering; data-driven VFX here.
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class WaddleDoo : public Monster {
    public:
        struct Cfg {
            Monster::Cfg base{};

            // instance flags
            int   dir = 1;
            bool  enableAttack = true;
            bool  enableMove = true;

            // shared tuning (type-level defaults)
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;
            float wakeRange = 360.f;
            float windupMs = 0.35f;
            float firePeriod = 1.20f;
            bool  stopDuringWindup = true;
        };

        WaddleDoo ( const engine::IntRect& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const Cfg& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
            , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX )
            , m_turnAtEdge ( cfg.turnAtEdge )
        {
            // if (auto* an = Animator()) an->Play("Walk", true);
        }

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Beam; }

    private:
        enum class AState { Idle , Windup , Cooldown };

        // movement (mirrors Dee names)
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;

        // attack state
        AState m_state{ AState::Idle };
        float m_cd = 0.f;
        float m_windupT = 0.f;
        int   m_face = +1; // attack-facing

        Cfg   m_cfg{};
    };

} // namespace game
