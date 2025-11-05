#pragma once
#include "game/Monster.h"

namespace game {

    class WaddleDee : public Monster {
    public:
        struct Config {
            Monster::Cfg base{};
            int   dir = 1;       // +1: right, -1: left
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;

            // Instance flag
            bool  enableMove = true;
            bool  enableAttack = false;
        };

        WaddleDee ( const RECT& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const Config& cfg = {} )
            : Monster ( worldBounds , col , cfg.base ) , m_dir ( cfg.dir ) ,
            m_turnOnHitX ( cfg.turnOnHitX ) , m_turnAtEdge ( cfg.turnAtEdge ),
            m_enableMove ( cfg.enableMove ) , m_enableAttack ( cfg.enableAttack )
        {
            // 애니(선택): "Walk" 클립만 간단히 만들어 둠(없어도 동작)
            // m_anim.AddClip("Walk", engine::Animator::MakeRowClip(...));
            m_anim.Play ( "Walk" , true );
        }

        void TickAI ( double fixedDt , const engine::Input& input ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::None; }

    private:
        int  m_dir = 1;
        bool m_turnOnHitX = true;
        bool m_turnAtEdge = true;
        bool m_enableMove = true;
        bool m_enableAttack = false; // 미사용
    };

} // namespace game
