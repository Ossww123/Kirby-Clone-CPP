#include "game/HotHead.h"
#include "engine/Math.h"
#include "game/CombatTypes.h"
#include <cmath>

using namespace engine;

namespace game {

    void HotHead::TickAI ( double fixedDt , const engine::Input& )
    {
        const float dt = static_cast< float >( fixedDt );

        // --- 타이머 ---
        if ( m_cd > 0.f )      m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_windupT > 0.f ) m_windupT = std::max ( 0.f , m_windupT - dt );
        if ( m_breathT > 0.f ) m_breathT = std::max ( 0.f , m_breathT - dt );
        if ( m_emitT > 0.f )   m_emitT = std::max ( 0.f , m_emitT - dt );

        // --- 에지/벽 턴(이동 비활성 시 비활성화) ---
        if ( m_cfg.enableMove ) {
            if ( m_turnAtEdge && Grounded ( ) && !HasGroundAhead ( m_dir ) ) m_dir *= -1;
            if ( m_turnOnHitX && m_rep.hitX ) m_dir *= -1;
        }

        // --- 이동 축: 웨이들두와 동일, 윈드업/분사 중 정지 옵션 ---
        const bool stopAtk = ( m_state == AState::Windup ) || ( m_state == AState::Breathing );
        const float axis = ( m_cfg.enableMove ? static_cast< float >( m_dir ) : 0.f );
        m_body.SetDesiredRunAxis ( ( m_cfg.stopDuringWindup && stopAtk ) ? 0.f : axis );

        // --- 타겟 ---
        if ( !m_queryTarget ) return;
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const Vec2 myC{ x + w * 0.5f, y + h * 0.5f };
        const Vec2 target = m_queryTarget ( );
        const Vec2 to{ target.x - myC.x, target.y - myC.y };
        const float dist = to.Length ( );

        // Idle에서만 이동 방향을 플레이어 쪽으로 업데이트
        if ( m_state == AState::Idle && std::fabs ( to.x ) > 1.f )
            m_dir = ( to.x >= 0.f ) ? +1 : -1;

        // --- 공격 FSM ---
        switch ( m_state ) {
        case AState::Idle:
            if ( m_cfg.enableAttack && dist <= m_cfg.wakeRange && m_cd <= 0.f ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
                // 윈드업 진입 시 페이싱 고정
                if ( std::fabs ( to.x ) > 1.f ) m_face = ( to.x >= 0.f ) ? +1 : -1;
            }
            break;

        case AState::Windup:
            if ( m_windupT <= 0.f ) {
                m_state = AState::Breathing;
                m_breathT = m_cfg.breathMs;
                m_emitT = 0.f; // 바로 첫 탄 발사
            }
            break;

        case AState::Breathing:
            if ( m_emitT <= 0.f && m_spawnProjId ) {
                // 입 앞에서 전방으로 연속 탄 생성 (프로젝타일 ID 지원 시 FirePellet 사용)
                const float muzzleX = ( m_face > 0 ) ? ( myC.x + w * 0.5f + 4.f ) : ( myC.x - w * 0.5f - 4.f );
                const float muzzleY = myC.y - 4.f;
                const Vec2  pos{ muzzleX, muzzleY };
                const Vec2  vel{ float ( m_face ) * m_cfg.bulletSpeed, 0.f };
                m_spawnProjId ( "FirePellet" , pos , vel , ProjOwner::Enemy );
                m_emitT = m_cfg.fireIntervalMs;
            }
            if ( m_breathT <= 0.f ) {
                m_cd = std::max ( 0.f , m_cfg.firePeriod ); // 재사용(아래에서 설정)
                m_state = AState::Cooldown;
                m_dir = m_face;
            }
            break;

        case AState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AState::Idle;
            break;
        }
    }

} // namespace game
