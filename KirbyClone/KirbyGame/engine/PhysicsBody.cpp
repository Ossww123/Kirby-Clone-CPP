#include "engine/PhysicsBody.h"
#include <cmath>

namespace engine {

    static inline float sgn ( float x ) { return ( x > 0.f ) - ( x < 0.f ); }

    void PhysicsBody::AdvanceKinematics ( double fixedDt )
    {
        const float dt = static_cast< float >( fixedDt );

        // 1) x축 - 가속/감속
        float target = m_useDirectTarget ? m_directTargetX : ( m_axisX * m_p.maxSpeedRun );
        m_useDirectTarget = false; // 1프레임만 유효

        if ( target != 0.f ) {
            const float dv = target - m_vel.x;
            const float acc = ( std::fabs ( dv ) > 1.f ) ? m_p.accelRun : m_p.decelRun;
            const float step = acc * dt;
            if ( dv > 0 ) m_vel.x = std::min ( m_vel.x + step , target );
            else        m_vel.x = std::max ( m_vel.x - step , target );
        }
        else {
            // 미입력 → 마찰
            const float fr = m_grounded ? m_p.frictionGround : m_p.frictionAir;
            const float s = fr * dt;
            if ( m_vel.x > 0 ) m_vel.x = std::max ( 0.f , m_vel.x - s );
            else             m_vel.x = std::min ( 0.f , m_vel.x + s );
        }

        // 2) y축 - 중력 + 종단속도
        m_vel.y += m_p.gravity * dt;
        if ( m_vel.y > m_p.termVel ) m_vel.y = m_p.termVel;
    }

    void PhysicsBody::integratePosition ( float dt )
    {
        m_x += m_vel.x * dt;
        m_y += m_vel.y * dt;
    }

    void PhysicsBody::IntegrateAndClampNoCollision ( double fixedDt )
    {
        const float dt = static_cast< float >( fixedDt );
        integratePosition ( dt );

        // 월드 경계 클램프(충돌 시스템을 쓰지 않는 임시 플레이용)
        const int right = m_bounds.right - static_cast< int >( m_w );
        const int bottom = m_bounds.bottom - static_cast< int >( m_h );

        if ( m_x < m_bounds.left ) { m_x = static_cast< float >( m_bounds.left );  m_vel.x = 0.f; }
        if ( m_x > right ) { m_x = static_cast< float >( right );          m_vel.x = 0.f; }
        if ( m_y < m_bounds.top ) { m_y = static_cast< float >( m_bounds.top );   m_vel.y = 0.f; }
        if ( m_y > bottom ) { m_y = static_cast< float >( bottom );         m_vel.y = 0.f; m_grounded = true; }
    }

    RECT PhysicsBody::ProposeAABB ( double fixedDt , int* outPrevBottom ) const {
        const float dt = ( float ) fixedDt;
        if ( outPrevBottom ) *outPrevBottom = ( int ) std::floor ( m_y + m_h ); // ★
        const float nx = m_x + m_vel.x * dt;
        const float ny = m_y + m_vel.y * dt;
        const int l = ( int ) std::floor ( nx );
        const int t = ( int ) std::floor ( ny );
        const int w = ( int ) std::round ( m_w );
        const int h = ( int ) std::round ( m_h );
        return RECT{ l, t, l + w, t + h };
    }

    void PhysicsBody::ApplyCollisionResult ( const RECT& aabbAfter , const Vec2& velAfter , bool grounded )
    {
        m_x = static_cast< float >( aabbAfter.left );
        m_y = static_cast< float >( aabbAfter.top );
        m_vel = velAfter;
        m_grounded = grounded;
        if ( m_grounded && m_vel.y > 0.f ) m_vel.y = 0.f;
    }

} // namespace engine
