#pragma once
//
// Responsibility: Kinematic character body (run accel/decel, friction, gravity)
//                 and AABB proposal for collision systems.
// Non-Goals:      Rotations, slopes, CCD for fast movers, multi-shape bodies.
// Call-Context:   Main thread; integer world coords (pixels) for AABB.
//
#include <algorithm>            // std::clamp
#include "engine/util/Math.h"   // Vec2
#include "engine/util/Types.h"  // IntRect

namespace engine { namespace physics { struct CollisionReport; } }

namespace engine {

    struct PhysicsParams {
        float accelRun = 3200.f;
        float decelRun = 4000.f;
        float maxSpeedRun = 220.f;
        float frictionGround = 500.f;
        float frictionAir = 80.f;
        float gravity = 1200.f;
        float termVel = 1050.f;   // +Y downward
    };

    class PhysicsBody {
    public:
        explicit PhysicsBody ( IntRect worldBounds , const PhysicsParams& p = {} )
            : m_bounds ( worldBounds ) , m_p ( p ) {}

        // ---- write state
        void SetBounds ( IntRect b ) { m_bounds = b; }
        void SetSize ( float w , float h ) { m_w = w; m_h = h; }
        void SetPosition ( float x , float y ) { m_x = x; m_y = y; }
        void SetVelocity ( const Vec2& v ) { m_vel = v; }
        void SetGrounded ( bool g ) { m_grounded = g; if ( g && m_vel.y > 0 ) m_vel.y = 0.f; }

        // ---- input (horizontal intent)
        void SetDesiredRunAxis ( float axisX ) { m_axisX = std::clamp ( axisX , -1.f , 1.f ); }
        void SetDesiredRunSpeedX ( float target ) { m_axisX = 0.f; m_directTargetX = target; m_useDirectTarget = true; }

        // ---- physics step (velocity only)
        void AdvanceKinematics ( double fixedDt );

        // ---- quick path: integrate + clamp to world bounds (no collision system)
        void IntegrateAndClampNoCollision ( double fixedDt );

        // ---- collision system integration
        // 1) propose AABB after dt (returns integer AABB)
        IntRect ProposeAABB ( double fixedDt , int* outPrevBottom ,
                            float* outNX = nullptr , float* outNY = nullptr ) const;

        // 2) apply result from collision system
        void ApplyCollisionResult ( const IntRect& aabbAfter , const Vec2& velAfter , bool grounded );
        void ApplyCollisionResult ( const IntRect& aabbAfter , const Vec2& velAfter ,
                                  const physics::CollisionReport& rep ,
                                  float proposedX , float proposedY );

        // ---- queries
        void  GetBounds ( int& x , int& y , int& w , int& h ) const {
            x = static_cast< int >( m_x ); y = static_cast< int >( m_y );
            w = static_cast< int >( m_w ); h = static_cast< int >( m_h );
        }
        IntRect BoundsRect ( ) const {
            return IntRect{
                static_cast< int >( m_x ), static_cast< int >( m_y ),
                static_cast< int >( m_x + m_w ), static_cast< int >( m_y + m_h )
            };
        }
        Vec2  Velocity ( )  const { return m_vel; }
        bool  Grounded ( )  const { return m_grounded; }
        const PhysicsParams& Params ( ) const { return m_p; }
        PhysicsParams& Params ( ) { return m_p; }

        // ---- actions
        void Jump ( float initialUpSpeed ) { m_vel.y = -std::abs ( initialUpSpeed ); m_grounded = false; }
        void AddImpulse ( const Vec2& dv ) { m_vel.x += dv.x; m_vel.y += dv.y; }

    private:
        void integratePosition ( float dt );

    private:
        PhysicsParams m_p;

        // state
        float m_x = 100.f , m_y = 100.f;
        float m_w = 56.f , m_h = 56.f;
        Vec2  m_vel{ 0.f, 0.f };
        bool  m_grounded = false;
        IntRect m_bounds{};   // world clamp rect

        // input/targets
        float m_axisX = 0.f;
        float m_directTargetX = 0.f;
        bool  m_useDirectTarget = false;
    };

} // namespace engine
