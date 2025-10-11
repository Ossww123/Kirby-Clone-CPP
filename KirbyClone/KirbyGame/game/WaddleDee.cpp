#include "game/WaddleDee.h"
#include <cmath>

using namespace engine;
using namespace engine::physics;

namespace game {

    void WaddleDee::TickAI ( double , const engine::Input& )
    {
        // 1) 에지 판정으로 선회
        if ( m_turnAtEdge && Grounded ( ) ) {
            if ( !HasGroundAhead ( m_dir ) ) {
                m_dir *= -1;
            }
        }

        // 2) 진행 방향 설정
        m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );

        // 3) 벽 충돌 시 턴(이전 물리 스텝의 결과 사용)
        if ( m_turnOnHitX && m_rep.hitX ) {
            m_dir *= -1;
            // 다음 프레임에는 반대 방향을 향해 가속
            m_body.SetDesiredRunAxis ( static_cast< float >( m_dir ) );
        }
    }

} // namespace game
