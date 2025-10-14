#pragma once
#include "game/Monster.h"

namespace game {

    class WaddleDoo : public Monster {
    public:
        struct CfgDoo {
            Monster::Cfg base{};
            float firePeriod = 1.2f;  // 발사 주기(초)
            float bulletSpeed = 420.f;
            float wakeRange = 360.f; // 타겟이 이 범위 안에 있으면 사격
        };

        WaddleDoo ( const RECT& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const CfgDoo& cfg = {} )
            : Monster ( worldBounds , col , cfg.base ) , m_cfg ( cfg ) {}

    protected:
        void TickAI ( double fixedDt , const engine::Input& ) override;

    private:
        CfgDoo m_cfg{};
        float  m_cd{ 0.f }; // 쿨다운
    };

} // namespace game
