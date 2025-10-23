#pragma once
#include "game/Monster.h"

namespace game {

    class Sparky : public Monster {
    public:
        struct Config {
            // 공통
            Monster::Cfg base{};

            // 이동(통통 튀는 느낌)
            int   dir = 1;
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;
            float hopPeriodMs = 0.8f; // 점프 주기
            float hopVy = 360.f;      // 점프 초기상승(플러스 -> 내부에서 -적용)

            // 공격(자기장 링)
            float wakeRange = 220.f;
            float windupMs = 0.30f;
            float firePeriod = 1.40f;
            int   ringProjectiles = 10;
            float sparkSpeed = 260.f;
            bool  stopDuringWindup = true;
        };

        Sparky ( const RECT& worldBounds ,
               const engine::physics::CollisionSystem* col ,
               const Config& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg ) , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX ) , m_turnAtEdge ( cfg.turnAtEdge ) {}

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Spark; }

    private:
        enum class AState { Idle , Windup , Burst , Cooldown };

        // 이동/점프
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;
        float m_hopT = 0.f;

        // 공격 상태
        AState m_state{ AState::Idle };
        float  m_cd = 0.f;
        float  m_windupT = 0.f;

        Config m_cfg{};
    };

} // namespace game
