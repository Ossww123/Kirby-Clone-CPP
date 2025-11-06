//
// Responsibility: WaddleDee AI — patrol + turn at edge/wall.
// Non-Goals:      Rendering, advanced combat.
// Call-Context:   Main thread.
//
#include "game/entities/monsters/WaddleDee.h"

namespace game {

    void WaddleDee::TickAI ( double , const engine::Input& ) {
        if ( !m_enableMove ) {
            m_body.SetDesiredRunAxis ( 0.f );
            return;
        }

        if ( m_turnAtEdge && Grounded ( ) ) {
            if ( !HasGroundAhead ( m_dir ) ) m_dir *= -1;
        }

        m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );

        if ( m_turnOnHitX && HitX ( ) ) {
            m_dir *= -1;
            m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
        }
    }

} // namespace game
