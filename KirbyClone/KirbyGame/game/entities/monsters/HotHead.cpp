//
// Responsibility: HotHead AI — patrol and fire-breath projectile bursts.
// Non-Goals:      Rendering; VFX ownership.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/HotHead.h"

#include <algorithm>
#include <cmath>
#include "engine/util/Math.h"
#include "game/combat/CombatTypes.h" // ProjOwner
#include "game/combat/Ability.h" // Ability

namespace game {

    void HotHead::TickAI ( double fixedDt , const engine::Input& ) {
        const float dt = static_cast< float >( fixedDt );

        // --- timers ---
        if ( m_cd > 0.f ) m_cd = std::max ( 0.f , m_cd - dt );
        if ( m_windupT > 0.f ) m_windupT = std::max ( 0.f , m_windupT - dt );
        if ( m_breathT > 0.f ) m_breathT = std::max ( 0.f , m_breathT - dt );
        if ( m_emitT > 0.f ) m_emitT = std::max ( 0.f , m_emitT - dt );

        // --- edge/wall turning (only when movement enabled) ---
        if ( m_cfg.enableMove ) {
            if ( m_turnAtEdge && Grounded ( ) && !HasGroundAhead ( m_dir ) ) m_dir *= -1;
            if ( m_turnOnHitX && HitX ( ) ) m_dir *= -1;
        }

        // --- movement axis (stop during windup/breath if requested) ---
        const bool  stopAtk = ( m_state == AState::Windup ) || ( m_state == AState::Breathing );
        const float axis = ( m_cfg.enableMove ? static_cast< float >( m_dir ) : 0.f );
        m_body.SetDesiredRunAxis ( ( m_cfg.stopDuringWindup && stopAtk ) ? 0.f : axis );

        // --- target ---
        if ( !m_queryTarget ) return;
        int x , y , w , h; m_body.GetBounds ( x , y , w , h );
        const engine::Vec2 myC{ x + w * 0.5f, y + h * 0.5f };
        const engine::Vec2 target = m_queryTarget ( );
        const engine::Vec2 to{ target.x - myC.x, target.y - myC.y };
        const float dist = to.Length ( );

        // update patrol dir toward target only in Idle
        if ( m_state == AState::Idle && std::fabs ( to.x ) > 1.f )
            m_dir = ( to.x >= 0.f ) ? +1 : -1;

        // --- attack FSM ---
        switch ( m_state ) {
        case AState::Idle:
            if ( m_cfg.enableAttack && dist <= m_cfg.wakeRange && m_cd <= 0.f ) {
                m_state = AState::Windup;
                m_windupT = m_cfg.windupMs;
                if ( std::fabs ( to.x ) > 1.f ) m_face = ( to.x >= 0.f ) ? +1 : -1; // lock facing
            }
            break;

        case AState::Windup:
            if ( m_windupT <= 0.f ) {
                m_state = AState::Breathing;
                m_breathT = m_cfg.breathMs;
                m_emitT = 0.f; // emit immediately
            }
            break;

        case AState::Breathing:
            if ( m_emitT <= 0.f && m_spawnProjId ) {
                // emit pellets slightly ahead of mouth
                const float muzzleX = ( m_face > 0 ) ? ( myC.x + w * 0.5f + 4.f )
                    : ( myC.x - w * 0.5f - 4.f );
                const float muzzleY = myC.y - 4.f;
                const engine::Vec2 pos{ muzzleX, muzzleY };
                const engine::Vec2 vel{ static_cast< float >( m_face ) * m_cfg.bulletSpeed, 0.f };
                m_spawnProjId ( "FirePellet" , pos , vel , ProjOwner::Enemy );
                m_emitT = m_cfg.fireIntervalMs;
            }
            if ( m_breathT <= 0.f ) {
                m_cd = std::max ( 0.f , m_cfg.firePeriod );
                m_state = AState::Cooldown;
                m_dir = m_face;
            }
            break;

        case AState::Cooldown:
            if ( m_cd <= 0.f ) m_state = AState::Idle;
            break;
        }
    }

    bool HotHead::Inhalable() const
    {
        return true;
    }

    Ability HotHead::AbilityGift() const
    {
        return Ability::Fire;
    }

} // namespace game
