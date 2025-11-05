#include "game/WaddleDee.h"
#include <cmath>

using namespace engine;
using namespace engine::physics;

namespace game {

    void WaddleDee::TickAI ( double , const engine::Input& )
    {
        if ( !m_enableMove ) { m_body.SetDesiredRunAxis ( 0.f ); return; }
        if ( m_turnAtEdge && Grounded ( ) ) { if ( !HasGroundAhead ( m_dir ) ) m_dir *= -1; }

        m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );

        if ( m_turnOnHitX && m_rep.hitX ) { 
            m_dir *= -1;
            m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
        }
    }
} // namespace game
