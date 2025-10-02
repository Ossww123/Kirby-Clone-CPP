#pragma once
#include <windows.h>
#include "engine/Object.h"
#include "engine/Input.h"
#include "engine/Math.h"
#include "engine/PhysicsBody.h"

namespace game {

    class Player final : public engine::Object {
    public:
        explicit Player ( RECT playBounds )
            : m_body ( playBounds )
        {
            m_body.SetSize ( m_w , m_h );
            m_body.SetPosition ( m_x , m_y );
        }

        // 지금 단계: 충돌 시스템 없이도 동작하도록 내부 적분+클램프 호출
        // (나중에 CollisionSystem 연동 시, IntegrateAndClampNoCollision 대신
        //  AdvanceKinematics → ProposeAABB → MoveAndCollide → ApplyCollisionResult 로 전환)
        void Update ( double fixedDt , const engine::Input& input ) override
        {
            // 입력을 물리로 전달
            const float axisX = input.GetAxis ( "MoveX" );  // -1..+1
            m_body.SetDesiredRunAxis ( axisX );

            // 속도만 갱신
            m_body.AdvanceKinematics ( fixedDt );

            // 임시: 충돌 없이 적분 + 경계 클램프
            m_body.IntegrateAndClampNoCollision ( fixedDt );

            // 로컬 캐시(렌더용) 갱신
            int bx , by , bw , bh; m_body.GetBounds ( bx , by , bw , bh );
            m_x = ( float ) bx; m_y = ( float ) by; m_w = ( float ) bw; m_h = ( float ) bh;
        }

        void Render ( HDC dc , int ox , int oy ) override
        {
            HBRUSH br = CreateSolidBrush ( RGB ( 255 , 180 , 64 ) );
            HGDIOBJ old = SelectObject ( dc , br );
            const int sx = static_cast< int >( m_x ) - ox;
            const int sy = static_cast< int >( m_y ) - oy;
            RoundRect ( dc , sx , sy , sx + static_cast< int >( m_w ) , sy + static_cast< int >( m_h ) , 12 , 12 );
            SelectObject ( dc , old ); DeleteObject ( br );
        }

        // 외부에서 월드/크기 조정 시 PhysicsBody와 동기화
        void SetBounds ( RECT b ) { m_body.SetBounds ( b ); }
        void SetSize ( float w , float h ) { m_w = w; m_h = h; m_body.SetSize ( w , h ); }
        void SetPosition ( float x , float y ) { m_x = x; m_y = y; m_body.SetPosition ( x , y ); }

        engine::Vec2 Center ( ) const { return { m_x + m_w * 0.5f, m_y + m_h * 0.5f }; }
        void GetBounds ( int& x , int& y , int& w , int& h ) const { m_body.GetBounds ( x , y , w , h ); }

        // 충돌 연동용(필요 시 노출)
        engine::PhysicsBody& Body ( ) { return m_body; }
        const engine::PhysicsBody& Body ( ) const { return m_body; }

    private:
        // 렌더를 위해 약간의 캐시만 유지(소스 호환)
        float m_x = 100.f , m_y = 100.f;
        float m_w = 32.f , m_h = 24.f;

        engine::PhysicsBody m_body;
    };

} // namespace game
