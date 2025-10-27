#include "game/Sparky.h"
#include "engine/Math.h"
#include <cmath>
#include <cstdlib>

using namespace engine;

namespace game {

    void Sparky::TickAI ( double fixedDt , const engine::Input& )
    {
        const float dt = static_cast< float >( fixedDt );

        // --- 타이머 ---
        if ( m_cd > 0.f )      m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_windupT > 0.f ) m_windupT = std::max ( 0.f , m_windupT - dt );
        if ( m_restT > 0.f )   m_restT = std::max ( 0.f , m_restT - dt );

        const bool grounded = Grounded ( );
        
        // --- 착지 감지: 공중→지상 전환 시 수평속도 0, 휴식 시작 ---
        if ( m_airborne && grounded ) {
            auto v = m_body.Velocity ( );
            v.x = 0.f;                 // 지상 미끄럼 방지(걷기 없음)
            m_body.SetVelocity ( v );
            m_airborne = false;
            m_lockedVx = 0.f;
            // 점프 사이 여유(최소 0.5s)
            m_restT = std::max ( m_restT , m_cfg.hopRestMs );
        }

        // --- 걷기 제거: 항상 수평 가속 입력 0 (이동은 점프 수평속도로만) ---
        m_body.SetDesiredRunAxis ( 0.f );

        // --- 타겟 ---
        if ( !m_queryTarget ) return;
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const Vec2 myC{ x + w * 0.5f, y + h * 0.5f };
        const Vec2 target = m_queryTarget ( );
        const Vec2 to{ target.x - myC.x, target.y - myC.y };
        // 점프 방향은 "점프 순간"에만 결정한다(공중에서 경로 변경 금지)
        if ( m_cfg.enableMove && grounded && std::fabs ( to.x ) > 1.f ) {
            m_dir = ( to.x >= 0.f ) ? +1 : -1;
        }
            // --- 에지/벽 턴: 다음 점프 준비 시에만 사용 ---
        if ( m_cfg.enableMove && grounded ) {
            if ( m_turnAtEdge && !HasGroundAhead ( m_dir ) ) m_dir *= -1;
            if ( m_turnOnHitX && m_rep.hitX ) m_dir *= -1;
        }
            
        // --- 공중 수평속도 고정: 공기 마찰 등으로 감속되지 않도록 보정 ---
        if ( m_airborne ) {
            auto v = m_body.Velocity ( );
            v.x = m_lockedVx;
            m_body.SetVelocity ( v );
        }

        // --- 점프 트리거: 지상 & 이동허용 & Idle & 휴식 끝 ---
        if ( m_cfg.enableMove && grounded && m_state == AState::Idle && m_restT <= 0.f ) {
            const int choice = std::rand ( ) % 3; // 0: 제자리 작은, 1: 방향 작은, 2: 방향 중간
            auto v = m_body.Velocity ( );
            // 수직속도
            v.y = -m_cfg.hopVy;
            // 비행시간 근사: T ~= 2*vy/g
            const float g = std::max ( 1.f , m_cfg.base.phys.gravity );
            const float T = ( 2.f * m_cfg.hopVy ) / g;
            float vx = 0.f;
            if ( choice == 1 ) {
                vx = ( m_cfg.hopSmallDist / T );
            }
            else if ( choice == 2 ) {
                vx = ( m_cfg.hopMediumDist / T );
            }
            v.x = ( choice == 0 ) ? 0.f : ( ( float ) m_dir * vx );
            m_lockedVx = v.x;    // 공중 경로 고정
            m_airborne = true;
            m_body.SetVelocity ( v );
            // 휴식 타이머는 착지 시점에 세팅(위에서 처리)
        }

        // --- 공격 FSM ---
        switch ( m_state ) {
        case AState::Idle:
            // 공격은 지상에서만 시작(공중 중 시작 금지)
            if ( m_cfg.enableAttack && grounded && m_cd <= 0.f &&
                ( to.x * to.x + to.y * to.y ) <= ( m_cfg.wakeRange * m_cfg.wakeRange ) ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
                // TODO : add anim
            }
            break;

        case AState::Windup:
            // 윈드업 중에도 수평 가속은 0(걷기 없음). 착지 여부와 무관하게 오라만 켠다.
            if ( m_windupT <= 0.f ) {
            if ( m_spawnHV ) {
                    Vec2 anchor = myC; // 몸 중심
                    m_spawnHV ( "SparkAura" , Id ( ) , /*facing*/ +1 , anchor );
                }
                m_cd = m_cfg.firePeriod;
                m_state = AState::Cooldown;
                if ( auto* an = Animator ( ) ) an->Play ( "Attack" , false );
            }
            break;

        case AState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AState::Idle;
            break;
        }
    }

} // namespace game
