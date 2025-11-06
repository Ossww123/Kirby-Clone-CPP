#include "engine/physics/PhysicsBody.h"
#include "engine/physics/Collision.h"
#include <cmath>    // std::fabs, floor, round

namespace engine {

    void PhysicsBody::AdvanceKinematics ( double fixedDt )
    {
        const float dt = static_cast< float >( fixedDt );

        // X: accelerate/decelerate towards target
        const float target = m_useDirectTarget ? m_directTargetX : ( m_axisX * m_p.maxSpeedRun );
        m_useDirectTarget = false; // one-frame directive

        if ( target != 0.f ) {
            const float dv = target - m_vel.x;
            const float acc = ( std::fabs ( dv ) > 1.f ) ? m_p.accelRun : m_p.decelRun;
            const float step = acc * dt;
            if ( dv > 0 ) m_vel.x = std::min ( m_vel.x + step , target );
            else        m_vel.x = std::max ( m_vel.x - step , target );
        }
        else {
            // no input → friction
            const float fr = m_grounded ? m_p.frictionGround : m_p.frictionAir;
            const float s = fr * dt;
            if ( m_vel.x > 0 ) m_vel.x = std::max ( 0.f , m_vel.x - s );
            else             m_vel.x = std::min ( 0.f , m_vel.x + s );
        }

        // Y: gravity + terminal velocity
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

        // clamp to world bounds (no collision system)
        const int right = m_bounds.r - static_cast< int >( m_w );
        const int bottom = m_bounds.b - static_cast< int >( m_h );

        if ( m_x < m_bounds.l ) { m_x = static_cast< float >( m_bounds.l );  m_vel.x = 0.f; }
        if ( m_x > right ) { m_x = static_cast< float >( right );       m_vel.x = 0.f; }
        if ( m_y < m_bounds.t ) { m_y = static_cast< float >( m_bounds.t );  m_vel.y = 0.f; }
        if ( m_y > bottom ) { m_y = static_cast< float >( bottom );      m_vel.y = 0.f; m_grounded = true; }
    }

    IntRect PhysicsBody::ProposeAABB ( double fixedDt , int* outPrevBottom , float* outNX , float* outNY ) const
    {
        const float dt = static_cast< float >( fixedDt );
        if ( outPrevBottom ) *outPrevBottom = static_cast< int >( std::floor ( m_y + m_h ) );

        const float nx = m_x + m_vel.x * dt;
        const float ny = m_y + m_vel.y * dt;
        if ( outNX ) *outNX = nx;
        if ( outNY ) *outNY = ny;

        const int l = static_cast< int >( nx );
        const int t = static_cast< int >( ny );
        const int w = static_cast< int >( std::round ( m_w ) );
        const int h = static_cast< int >( std::round ( m_h ) );
        return IntRect{ l, t, l + w, t + h };
    }

    void PhysicsBody::ApplyCollisionResult ( const IntRect& aabbAfter , const Vec2& velAfter , bool grounded )
    {
        m_x = static_cast< float >( aabbAfter.l );
        m_y = static_cast< float >( aabbAfter.t );
        m_vel = velAfter;
        m_grounded = grounded;
        if ( m_grounded && m_vel.y > 0.f ) m_vel.y = 0.f;
    }

    void PhysicsBody::ApplyCollisionResult ( const IntRect& aabbAfter , const Vec2& velAfter ,
                                           const physics::CollisionReport& rep ,
                                           float proposedX , float proposedY )
    {
        // integer snap on collided axes; keep proposed float otherwise
        m_x = rep.hitX ? static_cast< float >( aabbAfter.l ) : proposedX;
        m_y = ( rep.grounded || rep.hitY ) ? static_cast< float >( aabbAfter.t ) : proposedY;

        m_vel = velAfter;
        m_grounded = rep.grounded;
        if ( m_grounded && m_vel.y > 0.f ) m_vel.y = 0.f;
    }

} // namespace engine
