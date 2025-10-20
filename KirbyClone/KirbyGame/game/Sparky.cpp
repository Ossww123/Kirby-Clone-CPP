#include "game/Sparky.h"
#include "engine/Math.h"
#include "game/Projectile.h"
#include <cmath>

using namespace engine;

namespace game {

    void Sparky::TickAI ( double fixedDt , const engine::Input& )
    {
        const float dt = static_cast< float >( fixedDt );

        // --- 타이머 ---
        if ( m_cd > 0.f )      m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_windupT > 0.f ) m_windupT = std::max ( 0.f , m_windupT - dt );
        if ( m_hopT > 0.f )    m_hopT = std::max ( 0.f , m_hopT - dt );

        // --- 에지/벽 턴 ---
        if ( m_turnAtEdge && Grounded ( ) && !HasGroundAhead ( m_dir ) ) m_dir *= -1;
        if ( m_turnOnHitX && m_rep.hitX ) m_dir *= -1;

        // --- 통통 점프 이동 ---
        m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
        if ( Grounded ( ) && m_hopT <= 0.f ) {
            auto v = m_body.Velocity ( );
            v.y = -m_cfg.hopVy; // 위로 튀어오름
            m_body.SetVelocity ( v );
            m_hopT = m_cfg.hopPeriodMs;
        }

        // --- 타겟 ---
        if ( !m_queryTarget ) return;
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const Vec2 myC{ x + w * 0.5f, y + h * 0.5f };
        const Vec2 target = m_queryTarget ( );
        const Vec2 to{ target.x - myC.x, target.y - myC.y };
        if ( std::fabs ( to.x ) > 1.f ) m_dir = ( to.x >= 0.f ) ? +1 : -1; // 바라보는 방향

        // --- 공격 FSM ---
        switch ( m_state ) {
        case AState::Idle:
            if ( m_cd <= 0.f && ( to.x * to.x + to.y * to.y ) <= ( m_cfg.wakeRange * m_cfg.wakeRange ) ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
            }
            break;

        case AState::Windup:
            // 윈드업 중엔(옵션) 정지
            if ( m_cfg.stopDuringWindup ) m_body.SetDesiredRunAxis ( 0.f );
            if ( m_windupT <= 0.f ) {
                // 링 발사
                if ( m_spawnProj ) {
                    const int N = std::max ( 4 , m_cfg.ringProjectiles );
                    for ( int i = 0; i < N; ++i ) {
                        const float t = ( 2.f * 3.14159265f ) * ( i / float ( N ) );
                        const Vec2 vel{ std::cos ( t ) * m_cfg.sparkSpeed, std::sin ( t ) * m_cfg.sparkSpeed };
                        const Vec2 pos{ myC.x, myC.y - 2.f }; // 몸 중심 약간 위
                        m_spawnProj ( pos , vel , ProjOwner::Enemy );
                    }
                }
                m_cd = m_cfg.firePeriod;
                m_state = AState::Cooldown;
            }
            break;

        case AState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AState::Idle;
            break;
        }
    }

} // namespace game
