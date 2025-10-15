#pragma once
#include "game/Monster.h"

namespace game {

    class WaddleDee : public Monster {
    public:
        struct Config {
            Monster::Cfg base{};
            int   dir = 1;       // +1: 오른쪽, -1: 왼쪽
            bool  turnOnHitX = true; // 벽 충돌 시 방향 전환
            bool  turnAtEdge = true; // 낭떠러지 앞에서 방향 전환
        };

        WaddleDee ( const RECT& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const Config& cfg = {} )
            : Monster ( worldBounds , col , cfg.base ) , m_dir ( cfg.dir ) ,
            m_turnOnHitX ( cfg.turnOnHitX ) , m_turnAtEdge ( cfg.turnAtEdge )
        {
            // 애니(선택): "Walk" 클립만 간단히 만들어 둠(없어도 동작)
            // m_anim.AddClip("Walk", engine::Animator::MakeRowClip(...));
            m_anim.Play ( "Walk" , true );
        }

        void TickAI ( double fixedDt , const engine::Input& input ) override;

    private:
        int  m_dir = 1;
        bool m_turnOnHitX = true;
        bool m_turnAtEdge = true;
    };

} // namespace game
