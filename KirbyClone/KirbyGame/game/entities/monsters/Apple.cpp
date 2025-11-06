//
// Responsibility: Apple AI — telegraph → fall → bounce → roll.
// Non-Goals:      Rendering/VFX lifetime.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/Apple.h"

#include <algorithm>
#include "engine/util/Math.h" // Vec2

namespace game {

    void Apple::TickAI ( double dt , const engine::Input& ) {
        switch ( m_phase ) {
        case Phase::Telegraph:
            m_body.SetDesiredRunAxis ( 0.f );
            // brief hold: prevent immediate falling
            m_body.SetVelocity ( { m_body.Velocity ( ).x * 0.0f, 0.f } );
            m_phaseT -= static_cast< float >( dt );
            if ( m_phaseT <= 0.f ) {
                m_phase = Phase::Fall;
            }
            break;

        case Phase::Fall:
            m_body.SetDesiredRunAxis ( 0.f ); // pure vertical fall
            if ( Grounded ( ) ) {
                // first contact → bounce once toward player
                const engine::Vec2 me = Center ( );
                const engine::Vec2 tp = m_queryTarget ? m_queryTarget ( ) : me;
                m_dir = ( tp.x >= me.x ) ? +1 : -1;

                engine::Vec2 v{ static_cast< float >( m_dir ) * m_cfg.bounceVx, -m_cfg.bounceVy };
                m_body.SetVelocity ( v );
                m_phase = Phase::Bounce;
            }
            break;

        case Phase::Bounce:
            m_body.SetDesiredRunAxis ( 0.f ); // parabolic flight
            if ( Grounded ( ) ) {
                m_phase = Phase::Roll;
            }
            break;

        case Phase::Roll:
            // ground roll with turn-on-wall
            m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
            if ( HitX ( ) ) { m_dir *= -1; }
            break;
        }
    }

} // namespace game
