#pragma once
#include <windows.h>
#include "engine/Object.h"
#include "engine/Input.h"

namespace game {

    class Player final : public engine::Object {
    public:
        explicit Player ( RECT playBounds ) : m_bounds ( playBounds ) {}

        void Update ( double fixedDt , const engine::Input& input ) override {
            const float mx = input.GetAxis ( "MoveX" );
            const float my = input.GetAxis ( "MoveY" );
            m_x += mx * SPEED * static_cast< float >( fixedDt );
            m_y -= my * SPEED * static_cast< float >( fixedDt ); // 화면 Y는 아래로 +이므로 반전

            // 화면 경계 클램프
            const int right = m_bounds.right - static_cast< int >( m_w );
            const int bottom = m_bounds.bottom - static_cast< int >( m_h );
            if ( m_x < m_bounds.left ) m_x = static_cast< float >( m_bounds.left );
            if ( m_y < m_bounds.top )  m_y = static_cast< float >( m_bounds.top );
            if ( m_x > right )  m_x = static_cast< float >( right );
            if ( m_y > bottom ) m_y = static_cast< float >( bottom );
        }

        void Render ( HDC dc ) override {
            HBRUSH br = CreateSolidBrush ( RGB ( 255 , 180 , 64 ) );
            HGDIOBJ old = SelectObject ( dc , br );
            RoundRect ( dc , ( int ) m_x , ( int ) m_y , ( int ) ( m_x + m_w ) , ( int ) ( m_y + m_h ) , 12 , 12 );
            SelectObject ( dc , old );
            DeleteObject ( br );
        }

        void SetBounds ( RECT b ) { m_bounds = b; }

    private:
        static constexpr float SPEED = 180.f; // px/s
        float m_x = 100.f , m_y = 100.f;
        float m_w = 32.f , m_h = 24.f;
        RECT  m_bounds{};
    };

} // namespace game
