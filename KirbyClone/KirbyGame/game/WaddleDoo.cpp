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

        // 타겟이 범위 안이면 "공격시 페이싱"만 업데이트(이동 방향 m_dir 는 유지)
        if ( dist <= m_cfg.wakeRange && std::fabs ( to.x ) > 1.f ) {
            m_face = ( to.x >= 0.f ) ? +1 : -1;
        }

        switch ( m_state ) {
        case AttackState::Idle:
            if ( dist <= m_cfg.wakeRange && m_cd <= 0.f ) {
                // 공격 준비 진입
                m_state = AttackState::Windup;
                m_windupT = m_cfg.windupMs;
                // TODO : add anim
            }
            break;

        case AttackState::Windup:
            // 윈드업 동안 계속 목표 방향으로 "바라보기"만 갱신
            if ( std::fabs ( to.x ) > 1.f ) m_face = ( to.x >= 0.f ) ? +1 : -1;
            if ( m_windupT <= 0.f && m_spawnHV ) {
                // 빔 스윕(HitVolume) 생성
                Vec2 anchor = myCenter; anchor.x += ( float ) m_face * 8.f; // 손/눈 앞 오프셋
                m_spawnHV ( "BeamSweep" , Id ( ) , m_face , anchor );
                m_cd = m_cfg.firePeriod;
                m_state = AttackState::Cooldown;
                if ( auto* an = Animator ( ) ) an->Play ( "Attack" , false );
            }
            break;

        case AttackState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AttackState::Idle;
            break;
        }
    }

} // namespace game
