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
            float hopRestMs = 0.5f;
            float hopVy = 360.f;  // 점프 초기상승(플러스 -> 내부에서 -적용)
            // 점프 종류별 가로 목표 이동거리(px)
            float hopSmallDist = 32.f;  // 0.5 tile (타일 64px 기준)
            float hopMediumDist = 96.f;  // 1.5 tile

            // 공격(자기장 오라: HitVolume "SparkAura")
            float wakeRange = 220.f;
            float windupMs = 0.30f;
            float firePeriod = 1.40f;
            bool  stopDuringWindup = true;
            
            // 인스턴스 플래그(새 스키마)
            bool  enableMove = true;
            bool  enableAttack = true;
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
        enum class AState { Idle , Windup , Cooldown };

        // 이동/점프
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;
        float m_restT = 0.f;        // 지상 휴식 타이머(착지 후에만 카운트)
        bool  m_airborne = false;   // 공중 여부(점프 시작~착지까지)
        float m_lockedVx = 0.f;     // 점프 동안 고정할 수평 속도

        // 공격 상태
        AState m_state{ AState::Idle };
        float  m_cd = 0.f;
        float  m_windupT = 0.f;

        Config m_cfg{};
    };

} // namespace game
