#pragma once
#include "game/Monster.h"

namespace game {

    class WaddleDoo : public Monster {
    public:
        struct Config {
            // 공통(몬스터 베이스)
            Monster::Cfg base{};
            // 타입 고정 튜닝(수치는 코드/데이터에서 고정) + 인스턴스 플래그
            int   dir = 1;
            bool  enableAttack = true;
            bool  enableMove = true;
            // 아래는 타입 공통값(인스턴스별 변경 X)
            bool  turnOnHitX = true;
            bool  turnAtEdge = true;
            float wakeRange = 360.f;
            float windupMs = 0.35f;
            float firePeriod = 1.20f;
            bool  stopDuringWindup = true;
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
        int   m_face = +1;      // 공격 시 바라보는 방향(이동 방향과 분리)

        Config m_cfg{};
    };

} // namespace game
