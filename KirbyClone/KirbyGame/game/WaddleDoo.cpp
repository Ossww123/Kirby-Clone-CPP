#include "game/WaddleDoo.h"
#include "engine/Math.h"
#include "game/Projectile.h"
#include <cmath>

using namespace engine;

namespace game {

    static inline float Len ( const Vec2& v ) { return std::sqrt ( v.x * v.x + v.y * v.y ); }
    static inline Vec2  Norm ( const Vec2& v ) {
        const float l = Len ( v ); if ( l <= 1e-6f ) return { 0,0 }; return { v.x / l, v.y / l };
    }

    void WaddleDoo::TickAI ( double fixedDt , const engine::Input& )
    {
        const float dt = static_cast< float >( fixedDt );

        // --- 0) 타이머 업데이트 ---
        if ( m_cd > 0.f ) m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_state == AttackState::Windup )
            m_windupT = std::max ( 0.f , m_windupT - dt );

        // --- 1) 에지/벽 턴(Dee와 동일 패턴) ---
        if ( m_turnAtEdge && Grounded ( ) ) {
            if ( !HasGroundAhead ( m_dir ) ) {
                m_dir *= -1;
            }
        }
        if ( m_turnOnHitX && m_rep.hitX ) {    // m_rep은 Monster 공통 충돌 결과
            m_dir *= -1;
        }

        // --- 2) 기본 이동 축(윈드업 중엔 선택적으로 정지) ---
        const bool stop = ( m_state == AttackState::Windup ) && m_cfg.stopDuringWindup;
        m_body.SetDesiredRunAxis ( stop ? 0.f : static_cast< float >( m_dir ) );

        // --- 3) 타겟 체크 & 공격 상태 머신 ---
        if ( !m_queryTarget ) return;

        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const Vec2 myCenter{ x + w * 0.5f, y + h * 0.5f };
        const Vec2 target = m_queryTarget ( );
        const Vec2 to = { target.x - myCenter.x, target.y - myCenter.y };
        const float dist = Len ( to );

        // 타겟이 범위 안이면 바라보는 방향 업데이트
        if ( dist <= m_cfg.wakeRange ) {
            if ( std::fabs ( to.x ) > 1.f ) m_dir = ( to.x >= 0.f ) ? +1 : -1;
        }

        switch ( m_state ) {
        case AttackState::Idle:
            if ( dist <= m_cfg.wakeRange && m_cd <= 0.f ) {
                // 공격 준비 진입
                m_state = AttackState::Windup;
                m_windupT = m_cfg.windupMs;
            }
            break;

        case AttackState::Windup:
            if ( m_windupT <= 0.f && m_spawnProj ) {
                // 발사
                Vec2 dir = Norm ( to );
                if ( dir.x == 0.f && dir.y == 0.f )
                    dir = { static_cast< float >( m_dir ), 0.f };

                const Vec2 vel = { dir.x * m_cfg.bulletSpeed, dir.y * m_cfg.bulletSpeed };
                const Vec2 muzzle{ myCenter.x, myCenter.y - 6.f }; // 눈높이 근처

                m_spawnProj ( muzzle , vel , ProjOwner::Enemy );
                m_cd = m_cfg.firePeriod;
                m_state = AttackState::Cooldown;
            }
            break;

        case AttackState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AttackState::Idle;
            break;
        }
    }

} // namespace game
