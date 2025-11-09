//
// Responsibility: WaddleDoo AI — patrol and beam sweep attack.
// Non-Goals:      Rendering; VFX ownership.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/WaddleDoo.h"

#include <algorithm>
#include <cmath>
#include "engine/util/Math.h"
#include "game/combat/Ability.h" // Ability

namespace game {

    void WaddleDoo::TickAI ( double fixedDt , const engine::Input& ) {
        const float dt = static_cast< float >( fixedDt );

        // --- 0) timers ---
        if ( m_cd > 0.f )      m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_state == AState::Windup )
            m_windupT = std::max ( 0.f , m_windupT - dt );

        // --- 1) edge/wall turning ---
        if ( m_cfg.enableMove && m_turnAtEdge && Grounded ( ) ) {
            if ( !HasGroundAhead ( m_dir ) ) {
                m_dir *= -1;
            }
        }
        if ( m_cfg.enableMove && m_turnOnHitX && HitX ( ) ) {
            m_dir *= -1;
        }

        // --- 2) base movement (optional stop during windup) ---
        const bool  stop = ( m_state == AState::Windup ) && m_cfg.stopDuringWindup;
        const float axis = ( m_cfg.enableMove ? static_cast< float >( m_dir ) : 0.f );
        m_body.SetDesiredRunAxis ( stop ? 0.f : axis );

        // --- 3) target/attack FSM ---
        if ( !m_queryTarget ) return;
        if ( !m_cfg.enableAttack ) { m_state = AState::Idle; return; }

        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const engine::Vec2 myCenter{ x + w * 0.5f, y + h * 0.5f };
        const engine::Vec2 target = m_queryTarget ( );
        const engine::Vec2 to{ target.x - myCenter.x, target.y - myCenter.y };
        const float dist = to.Length ( );

        auto patrolFaceUpdate = [ & ] ( ) { m_face = ( m_dir >= 0 ) ? +1 : -1; };

        switch ( m_state ) {
        case AState::Idle:
            patrolFaceUpdate ( );
            if ( dist <= m_cfg.wakeRange && m_cd <= 0.f ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
                if ( std::fabs ( to.x ) > 1.f ) m_face = ( to.x >= 0.f ) ? +1 : -1;

                // TODO: animator windup if needed
            }
            break;

        case AState::Windup:
            if ( m_windupT <= 0.f && m_spawnHV ) {
                engine::Vec2 anchor = myCenter;
                anchor.x += static_cast< float >( m_face ) * 8.f; // slight forward offset
                m_spawnHV ( "BeamSweep" , Id ( ) , m_face , anchor );
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

    bool WaddleDoo::Inhalable ( ) const
    {
        return true;
    }

    Ability WaddleDoo::AbilityGift ( ) const
    {
        return Ability::Beam;
    }

} // namespace game
