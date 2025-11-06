#pragma once
//
// Responsibility: Simple walker enemy — patrol, turn at edges/walls.
// Non-Goals:      Rendering; complex attacks.
// Call-Context:   Main thread; fixed update via Monster.
//
#include "game/entities/monsters/Monster.h"

namespace game {

    class WaddleDee : public Monster {
    public:
        struct Cfg {
            Monster::Cfg base{};
            int   dir = 1;          // +1: right, -1: left
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;

            // instance flags
            bool  enableMove = true;
            bool  enableAttack = false;
        };

        WaddleDee ( const engine::IntRect& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const Cfg& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX )
            , m_turnAtEdge ( cfg.turnAtEdge )
            , m_enableMove ( cfg.enableMove )
            , m_enableAttack ( cfg.enableAttack )
        {
            // optional: simple walk loop
            // if (auto* an = Animator()) an->Play("Walk", true);
        }

        void TickAI ( double fixedDt , const engine::Input& input ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::None; }

    private:
        int  m_dir = 1;
        bool m_turnOnHitX = true;
        bool m_turnAtEdge = true;
        bool m_enableMove = true;
        bool m_enableAttack = false; // unused
    };

} // namespace game
