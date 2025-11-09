//
// Responsibility: Sparky AI — hop movement and spark aura.
// Non-Goals:      Rendering; VFX ownership.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/Sparky.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "engine/util/Math.h"
#include "game/combat/Ability.h" // Ability

namespace game {

    void Sparky::TickAI ( double fixedDt , const engine::Input& ) {
        const float dt = static_cast< float >( fixedDt );

        // --- timers ---
        if ( m_cd > 0.f ) m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_windupT > 0.f ) m_windupT = std::max ( 0.f , m_windupT - dt );
        if ( m_restT > 0.f ) m_restT = std::max ( 0.f , m_restT - dt );

        const bool grounded = Grounded ( );

        // --- landing detect: airborne→grounded ---
        if ( m_airborne && grounded ) {
            auto v = m_body.Velocity ( );
            v.x = 0.f;                   // no ground slide (no walking)
            m_body.SetVelocity ( v );
            m_airborne = false;
            m_lockedVx = 0.f;
            // ensure minimum rest between hops
            m_restT = std::max ( m_restT , m_cfg.hopRestMs );
        }

        // --- walking disabled: always zero run axis (movement via jump only) ---
        m_body.SetDesiredRunAxis ( 0.f );

        // --- target ---
        if ( !m_queryTarget ) return;
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const engine::Vec2 myC{ x + w * 0.5f, y + h * 0.5f };
        const engine::Vec2 target = m_queryTarget ( );
        const engine::Vec2 to{ target.x - myC.x, target.y - myC.y };

        // choose next hop direction only when grounded
        if ( m_cfg.enableMove && grounded && std::fabs ( to.x ) > 1.f ) {
            m_dir = ( to.x >= 0.f ) ? +1 : -1;
        }

        // edge/wall turn only when preparing next hop
        if ( m_cfg.enableMove && grounded ) {
            if ( m_turnAtEdge && !HasGroundAhead ( m_dir ) ) m_dir *= -1;
            if ( m_turnOnHitX && HitX ( ) )                 m_dir *= -1;
        }

        // maintain horizontal speed mid-air
        if ( m_airborne ) {
            auto v = m_body.Velocity ( );
            v.x = m_lockedVx;
            m_body.SetVelocity ( v );
        }

        // --- hop trigger ---
        if ( m_cfg.enableMove && grounded && m_state == AState::Idle && m_restT <= 0.f ) {
            const int choice = std::rand ( ) % 3; // 0: in-place small, 1: small forward, 2: medium forward
            auto v = m_body.Velocity ( );

            // vertical speed
            v.y = -m_cfg.hopVy;

            // flight time approx: T ≈ 2*vy/g
            const float g = std::max ( 1.f , m_cfg.base.phys.gravity );
            const float T = ( 2.f * m_cfg.hopVy ) / g;

            float vx = 0.f;
            if ( choice == 1 )      vx = ( m_cfg.hopSmallDist / T );
            else if ( choice == 2 ) vx = ( m_cfg.hopMediumDist / T );

            v.x = ( choice == 0 ) ? 0.f : ( static_cast< float >( m_dir ) * vx );
            m_lockedVx = v.x;
            m_airborne = true;
            m_body.SetVelocity ( v );
            // rest timer starts on landing (handled above)
        }

        // --- attack FSM ---
        switch ( m_state ) {
        case AState::Idle:
            // start only on ground
            if ( m_cfg.enableAttack && grounded && m_cd <= 0.f
                && ( to.x * to.x + to.y * to.y ) <= ( m_cfg.wakeRange * m_cfg.wakeRange ) ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
                // TODO: windup animation
            }
            break;

        case AState::Windup:
            if ( m_windupT <= 0.f ) {
                if ( m_spawnHV ) {
                    engine::Vec2 anchor = myC; // center
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

    bool Sparky::Inhalable ( ) const
    {
        return true;
    }

    Ability Sparky::AbilityGift ( ) const
    {
        return Ability::Spark;
    }

} // namespace game
