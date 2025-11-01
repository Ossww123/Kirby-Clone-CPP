#pragma once
#include "game/Monster.h"

namespace game {

    class Apple : public Monster {
    public:
        struct Config {
            Monster::Cfg base{};
            float telegraphMs = 0.6f;  // 깜빡이며 제자리 대기
            float bounceVx = 140.f;  // 1회 바운스 가로 속도
            float bounceVy = 360.f;  // 1회 바운스 상승 초기 속도
            float rollSpeed = 90.f;   // 구르기 목표 속도(DesiredRunAxis 로 가속)
        };

        Apple ( const RECT& worldBounds ,
              const engine::physics::CollisionSystem* col ,
              const Config& cfg ,
              float spawnX , float spawnY )
            : Monster ( worldBounds , col , cfg.base )
            , m_cfg ( cfg )
        {
            SetPosition ( spawnX , spawnY );
            // 시각 크기/스프라이트는 GameApp 쪽에서 적절히 세팅해줌
            m_phase = Phase::Telegraph;
            m_phaseT = m_cfg.telegraphMs;
        }

        bool Inhalable ( ) const override { return true; }
        Ability AbilityGift ( ) const override { return Ability::None; }

        void TickAI ( double dt , const engine::Input& ) override {
            // 이전 프레임 충돌 결과는 m_rep에 누적됨 (StepPhysics 이후에 갱신)
            switch ( m_phase ) {
            case Phase::Telegraph:
                m_body.SetDesiredRunAxis ( 0.f );
                // 제자리 유지(낙하 금지) — 아주 짧게 y속도 0으로 유지
                m_body.SetVelocity ( { m_body.Velocity ( ).x * 0.0f, 0.f } );
                m_phaseT -= static_cast< float >( dt );
                if ( m_phaseT <= 0.f ) {
                    m_phase = Phase::Fall;
                }
                break;

            case Phase::Fall:
                m_body.SetDesiredRunAxis ( 0.f ); // 수직 낙하
                if ( m_rep.grounded ) {
                    // 최초 접지 → 플레이어 쪽으로 1회 바운스
                    engine::Vec2 me = Center ( );
                    engine::Vec2 tp = m_queryTarget ? m_queryTarget ( ) : me;
                    m_dir = ( tp.x >= me.x ) ? +1 : -1;

                    engine::Vec2 v{ m_dir * m_cfg.bounceVx, -m_cfg.bounceVy };
                    m_body.SetVelocity ( v );
                    m_phase = Phase::Bounce;
                }
                break;

            case Phase::Bounce:
                m_body.SetDesiredRunAxis ( 0.f ); // 포물선 비행 중
                if ( m_rep.grounded ) {
                    m_phase = Phase::Roll;
                }
                break;

            case Phase::Roll:
                // 바닥에서 굴러가며 충돌 시 방향전환(혹은 유지, 취향)
                m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
                // 목표 속도 근처로 붙이기
                auto v = m_body.Velocity ( );
                const float target = m_dir * m_cfg.rollSpeed;
                if ( ( m_dir > 0 && v.x < target ) || ( m_dir < 0 && v.x > target ) ) {
                    // accelRun 으로 자연 가속; 별도 조치 불필요
                }
                if ( m_rep.hitX ) { m_dir *= -1; }
                break;
            }
        }

    private:
        enum class Phase { Telegraph , Fall , Bounce , Roll };
        Phase m_phase{};
        float m_phaseT = 0.f;
        int   m_dir = +1;

        Config m_cfg;

        engine::Vec2 Center ( ) const {
            int x , y , w , h; GetBounds ( x , y , w , h );
            return { x + w * 0.5f, y + h * 0.5f };
        }
    };

} // namespace game
