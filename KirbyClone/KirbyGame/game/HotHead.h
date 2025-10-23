#pragma once
#include "game/Monster.h"

namespace game {

    class HotHead : public Monster {
    public:
        struct Config {
            // 공통
            Monster::Cfg base{};

            // 이동
            int   dir = 1;
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;

            // 공격(화염 분사)
            float wakeRange = 260.f;      // 감지 거리
            float windupMs = 0.25f;      // 텔레그래프
            float breathMs = 0.55f;      // 분사 지속
            float fireIntervalMs = 0.06f; // 분사 중 탄 생성 간격
            float bulletSpeed = 360.f;    // 화염탄 속도
            bool  stopDuringWindup = true; // 윈드업/분사 중 정지
            float firePeriod = 1.1f;
        };

        HotHead ( const RECT& worldBounds ,
                const engine::physics::CollisionSystem* col ,
                const Config& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg ) , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX ) , m_turnAtEdge ( cfg.turnAtEdge ) {}

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Fire; }

    private:
        enum class AState { Idle , Windup , Breathing , Cooldown };

        // 이동
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;

        // 공격 상태
        AState m_state{ AState::Idle };
        float  m_cd = 0.f;        // 쿨다운
        float  m_windupT = 0.f;   // 텔레그래프 잔여
        float  m_breathT = 0.f;   // 분사 잔여
        float  m_emitT = 0.f;     // 다음 탄까지 간격

        Config m_cfg{};
    };

} // namespace game
