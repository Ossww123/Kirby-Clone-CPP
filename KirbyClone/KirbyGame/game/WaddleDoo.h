#pragma once
#include "game/Monster.h"

namespace game {

    class WaddleDoo : public Monster {
    public:
        struct Config {
            // 공통(몬스터 베이스)
            Monster::Cfg base{};

            // === 이동(Dee와 동일 키) ===
            int   dir = 1;             // +1: 오른쪽, -1: 왼쪽
            bool  turnOnHitX = true;   // 벽 충돌 시 방향 전환
            bool  turnAtEdge = true;   // 낭떠러지 앞에서 방향 전환

            // === 사격 ===
            float wakeRange = 360.f; // 감지 범위
            float windupMs = 0.35f; // 공격 준비(텔레그래프) 시간
            float firePeriod = 1.2f;  // 발사 주기(쿨다운)
            float bulletSpeed = 420.f; // 탄속

            bool  stopDuringWindup = true; // 윈드업 중 멈춤
        };

        WaddleDoo ( const RECT& worldBounds ,
                  const engine::physics::CollisionSystem* col ,
                  const Config& cfg = {} )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
            , m_dir ( cfg.dir )
            , m_turnOnHitX ( cfg.turnOnHitX )
            , m_turnAtEdge ( cfg.turnAtEdge )
        {
            // 필요 시 애니 시작
            // m_anim.Play("Walk", true);
        }

        void TickAI ( double fixedDt , const engine::Input& ) override;

        bool    Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::Beam; }

    private:
        enum class AttackState { Idle , Windup , Cooldown };

        // 이동(Dee와 동일 네이밍)
        int   m_dir = 1;
        bool  m_turnOnHitX = true;
        bool  m_turnAtEdge = true;

        // 공격 상태
        AttackState m_state{ AttackState::Idle };
        float m_cd = 0.f;       // 쿨다운 타이머
        float m_windupT = 0.f;  // 윈드업 잔여 시간

        Config m_cfg{};
    };

} // namespace game
