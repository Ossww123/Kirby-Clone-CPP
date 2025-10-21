#pragma once
#include <windows.h>
#include <algorithm>
#include "engine/Math.h"

namespace engine { namespace physics { struct CollisionReport; } }

namespace engine {

    struct PhysicsParams {
        float accelRun          = 3200.f;  // 가속도
        float decelRun          = 4000.f;  // 감속도
        float maxSpeedRun       = 220.f;   // 최대 달리기 속도
        float frictionGround    = 500.f;   // 마찰 감속도
        float frictionAir       = 80.f;    // 공중 마찰 감속도 (좌우)
        float gravity           = 1200.f;  // 중력 가속도
        float termVel           = 1050.f;  // +Y 하강 종단속도
    };

    class PhysicsBody {
    public:
        explicit PhysicsBody ( RECT worldBounds , const PhysicsParams& p = {} ) : m_bounds ( worldBounds ) , m_p ( p ) {}

        // ---- 설정/상태 쓰기
        void SetBounds ( RECT b ) { m_bounds = b; }
        void SetSize ( float w , float h ) { m_w = w; m_h = h; }
        void SetPosition ( float x , float y ) { m_x = x; m_y = y; }
        void SetVelocity ( const Vec2& v ) { m_vel = v; }
        void SetGrounded ( bool g ) { m_grounded = g; if ( g && m_vel.y > 0 ) m_vel.y = 0.f; }

        // ---- 입력(수평 이동 의도)
        void SetDesiredRunAxis ( float axisX ) { m_axisX = std::clamp ( axisX , -1.f , 1.f ); } // axisX: -1..+1 권장.
        void SetDesiredRunSpeedX ( float targetSpeed ) { m_axisX = 0.f; m_directTargetX = targetSpeed; m_useDirectTarget = true; }

        // ---- 물리 스텝(속도만 갱신; 위치는 안 옮김 → 충돌과 조합하기 좋게)
        void AdvanceKinematics ( double fixedDt );

        // ---- 충돌 없이도 바로 플레이 가능하게: 적분 + 경계 클램프
        void IntegrateAndClampNoCollision ( double fixedDt );

        // ---- 충돌 시스템 연동 경로
        // 1) 현재 상태에서 dt 후의 "제안 AABB" 계산 (충돌 검사에 사용)
        RECT ProposeAABB ( double fixedDt , int* outPrevBottom ,
                     float* outNX = nullptr , float* outNY = nullptr ) const;

        // 2) 충돌 시스템 결과를 바디에 반영
        void ApplyCollisionResult ( const RECT& aabbAfter , const Vec2& velAfter , bool grounded );
        // 축별 스냅용 오버로드 (충돌 없는 축은 float 유지)
        void ApplyCollisionResult ( const RECT& aabbAfter , const Vec2& velAfter ,
                              const physics::CollisionReport& rep ,
                              float proposedX , float proposedY );

        // ---- 쿼리
        void GetBounds ( int& x , int& y , int& w , int& h ) const {
            x = static_cast< int >( m_x ); y = static_cast< int >( m_y );
            w = static_cast< int >( m_w ); h = static_cast< int >( m_h );
        }
        RECT BoundsRect ( ) const {
            return RECT{ static_cast< LONG >( m_x ), static_cast< LONG >( m_y ),
                         static_cast< LONG >( m_x + m_w ), static_cast< LONG >( m_y + m_h ) };
        }
        Vec2  Velocity ( )  const { return m_vel; }
        bool  Grounded ( )  const { return m_grounded; }
        const PhysicsParams& Params ( ) const { return m_p; }
        PhysicsParams& Params ( ) { return m_p; }

        // ---- 행동
        void Jump ( float initialUpSpeed ) { m_vel.y = -std::abs ( initialUpSpeed ); m_grounded = false; }
        void AddImpulse ( const Vec2& dv ) { m_vel.x += dv.x; m_vel.y += dv.y; }

    private:
        // 내부 헬퍼
        void integratePosition ( float dt );

    private:
        PhysicsParams m_p;

        // 상태
        float m_x = 100.f , m_y = 100.f;
        float m_w = 56.f , m_h = 56.f;
        Vec2  m_vel{ 0.f, 0.f };
        bool  m_grounded = false;
        RECT  m_bounds{};

        // 입력/목표
        float m_axisX = 0.f;
        float m_directTargetX = 0.f;
        bool  m_useDirectTarget = false;
    };

} // namespace engine
